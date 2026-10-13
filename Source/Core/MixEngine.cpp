#include "MixEngine.h"
#include <cstring>

namespace mps
{
MixEngine::MixEngine (MixerParams& p, MixerMeters& m) : params (p), meters (m) {}

void MixEngine::prepare (double sr, int maxBlockSize)
{
    sampleRate = sr > 0.0 ? sr : 48000.0;
    maxBlock = std::max (1, maxBlockSize);

    states.assign ((size_t) kMaxInputChannels, ChannelState{});
    chL.assign ((size_t) maxBlock, 0.0f);
    chR.assign ((size_t) maxBlock, 0.0f);
    sumL.assign ((size_t) maxBlock, 0.0f);
    sumR.assign ((size_t) maxBlock, 0.0f);

    for (auto& s : states)
    {
        s.inGain.reset (sampleRate, kGainRampSeconds);
        s.srcFade.reset (sampleRate, kSourceFadeSeconds);
        s.postL.reset (sampleRate, kGainRampSeconds);
        s.postR.reset (sampleRate, kGainRampSeconds);
        s.route.reset (sampleRate, kGainRampSeconds);
        s.inGain.setImmediate (1.0f);
        s.srcFade.setImmediate (0.0f);
        s.dsp.prepare (sampleRate);
        s.activeSource = -1;
        s.activeWidth = 0;
    }

    mainGain.reset (sampleRate, kGainRampSeconds);
    mainGain.setImmediate (faderDbToGain (params.main.faderDb.get()) * (params.main.mute.getBool() ? 0.0f : 1.0f));

    fxRack.prepare (sampleRate, maxBlockSize);
    for (int i = 0; i < 12; ++i)
    {
        busBufL[i].assign ((size_t) maxBlockSize, 0.0f);
        busBufR[i].assign ((size_t) maxBlockSize, 0.0f);
    }
    for (int i = 0; i < 4; ++i)
    {
        fxSendBufL[i].assign ((size_t) maxBlockSize, 0.0f);
        fxSendBufR[i].assign ((size_t) maxBlockSize, 0.0f);
        fxRetBufL[i].assign ((size_t) maxBlockSize, 0.0f);
        fxRetBufR[i].assign ((size_t) maxBlockSize, 0.0f);
    }
    insBufL.assign ((size_t) maxBlockSize, 0.0f);
    insBufR.assign ((size_t) maxBlockSize, 0.0f);

    for (auto& g : fxReturnGains)
    {
        g.reset (sampleRate, kGainRampSeconds);
        g.setImmediate (1.0f);
    }
    for (auto& g : fxSendGains)
    {
        g.reset (sampleRate, kGainRampSeconds);
        g.setImmediate (1.0f);
    }

    initRtaFilters (sampleRate);
}

static inline BiquadCoeffs calcRtaBPF (float freq, float Q, float sr) noexcept
{
    freq = clampf (freq, 10.0f, sr * 0.48f);
    const float w0 = 2.0f * kPi * freq / sr;
    const float alpha = std::sin (w0) / (2.0f * std::max (0.1f, Q));
    const float a0 = 1.0f + alpha;
    BiquadCoeffs c;
    c.b0 = alpha / a0;
    c.b1 = 0.0f;
    c.b2 = -alpha / a0;
    c.a1 = (-2.0f * std::cos (w0)) / a0;
    c.a2 = (1.0f - alpha) / a0;
    return c;
}

void MixEngine::initRtaFilters (double sr) noexcept
{
    for (size_t b = 0; b < 31; ++b)
    {
        rtaCoeffs[b] = calcRtaBPF (kGeqFrequencies[b], 4.318f, (float) sr);
        rtaFilters[b].reset();
    }
}

void MixEngine::processRta (const float* l, const float* r, int width, int n, int ch) noexcept
{
    if (ch < 0 || ch >= kMaxInputChannels || n <= 0) return;
    auto& cm = meters.channels[(size_t) ch];

    for (size_t b = 0; b < 31; ++b)
    {
        float peak = 0.0f;
        const auto& coeffs = rtaCoeffs[b];
        auto& filter = rtaFilters[b];

        for (int i = 0; i < n; ++i)
        {
            const float in = (width == 2) ? 0.5f * (l[i] + r[i]) : l[i];
            const float out = filter.process (in, coeffs);
            peak = std::max (peak, std::abs (out));
        }

        cm.rtaBands[b].update (peak);
    }
}

static inline float blockPeak (const float* d, int n) noexcept
{
    float p = 0.0f;
    for (int i = 0; i < n; ++i) p = std::max (p, std::abs (d[i]));
    return p;
}

void MixEngine::process (const float* const* slotData, const int* slotChannels,
                         int n, float* outL, float* outR) noexcept
{
    if (n <= 0) return;
    if (n > maxBlock) n = maxBlock; // caller is responsible for splitting; never overrun

    std::memset (sumL.data(), 0, sizeof (float) * (size_t) n);
    std::memset (sumR.data(), 0, sizeof (float) * (size_t) n);
    for (int b = 0; b < 12; ++b)
    {
        std::memset (busBufL[(size_t) b].data(), 0, sizeof (float) * (size_t) n);
        std::memset (busBufR[(size_t) b].data(), 0, sizeof (float) * (size_t) n);
    }
    for (int s = 0; s < 4; ++s)
    {
        std::memset (fxSendBufL[(size_t) s].data(), 0, sizeof (float) * (size_t) n);
        std::memset (fxSendBufR[(size_t) s].data(), 0, sizeof (float) * (size_t) n);
    }

    const bool anySolo = params.anySolo();

    for (int c = 0; c < kMaxInputChannels; ++c)
    {
        auto& p  = params.channels[(size_t) c];
        auto& st = states[(size_t) c];
        auto& m  = meters.channels[(size_t) c];

        auto slotValid = [&] (int s) { return s >= 0 && s < kMaxTracks && slotChannels[s] > 0; };

        int wanted = p.source.getInt();
        if (! slotValid (wanted)) wanted = -1;

        // The previously active slot may have been removed: it is silent now, switch at once.
        if (st.activeSource >= 0 && ! slotValid (st.activeSource))
        {
            st.activeSource = -1;
            st.srcFade.setImmediate (0.0f);
        }

        // ---- source switching: fade out old source, then fade in the new one
        if (wanted != st.activeSource)
        {
            if (st.activeSource < 0 || (st.srcFade.getCurrent() <= 0.0f && ! st.srcFade.isSmoothing()))
            {
                st.activeSource = wanted;
                st.srcFade.setImmediate (0.0f);
                st.srcFade.setTarget (wanted >= 0 ? 1.0f : 0.0f);
            }
            else
            {
                st.srcFade.setTarget (0.0f);
            }
        }
        else
        {
            st.srcFade.setTarget (wanted >= 0 ? 1.0f : 0.0f);
        }

        // ---- gain targets
        const float inTarget = dbToGain (p.trimDb.get()) * (p.polarity.getBool() ? -1.0f : 1.0f);
        
        // Check Mute Groups
        bool muteGroupMuted = false;
        const int mgMask = p.muteGroups.getInt();
        if (mgMask > 0)
        {
            for (int mg = 0; mg < 6; ++mg)
            {
                if ((mgMask & (1 << mg)) && params.muteGroupActive[(size_t) mg].load (std::memory_order_relaxed))
                {
                    muteGroupMuted = true;
                    break;
                }
            }
        }

        // Check DCA Groups (Gain scaling and Mute)
        float dcaGain = 1.0f;
        bool dcaMuted = false;
        const int dcaMask = p.dcaGroups.getInt();
        if (dcaMask > 0)
        {
            for (int d = 0; d < 8; ++d)
            {
                if (dcaMask & (1 << d))
                {
                    auto& dca = params.dcas[(size_t) d];
                    if (dca.mute.getBool())
                        dcaMuted = true;
                    dcaGain *= faderDbToGain (dca.faderDb.get());
                }
            }
        }

        const bool muted = p.mute.getBool() || muteGroupMuted || dcaMuted;
        const float fader = faderDbToGain (p.faderDb.get()) * dcaGain * (muted ? 0.0f : 1.0f);
        const bool routed = p.mainAssign.getBool() && (! anySolo || p.solo.getBool());

        if (st.activeSource < 0)
        {
            // Silent channel: snap smoothers so a later fade-in is handled only by srcFade.
            st.inGain.setImmediate (inTarget);
            const auto pg = panMonoConstantPower (p.pan.get());
            st.postL.setImmediate (fader * pg.left);
            st.postR.setImmediate (fader * pg.right);
            st.route.setImmediate (routed ? 1.0f : 0.0f);
            st.activeWidth = 0;
            m.width.store (0, std::memory_order_relaxed);
            continue;
        }

        const int src = st.activeSource;
        const int slotCh = slotChannels[src];
        const float* L = slotData[src * 2];
        const float* R = (slotCh >= 2 && slotData[src * 2 + 1] != nullptr) ? slotData[src * 2 + 1] : L;
        const auto mode = (SourceMode) p.sourceMode.getInt();
        const int width = (slotCh >= 2 && mode == SourceMode::Native) ? 2 : 1;

        // ---- source selection into channel buffer
        if (width == 2)
        {
            std::memcpy (chL.data(), L, sizeof (float) * (size_t) n);
            std::memcpy (chR.data(), R, sizeof (float) * (size_t) n);
        }
        else if (slotCh < 2 || mode == SourceMode::LeftOnly || mode == SourceMode::Native)
        {
            std::memcpy (chL.data(), L, sizeof (float) * (size_t) n);
        }
        else if (mode == SourceMode::RightOnly)
        {
            std::memcpy (chL.data(), R, sizeof (float) * (size_t) n);
        }
        else // MonoSum
        {
            for (int i = 0; i < n; ++i) chL[(size_t) i] = 0.5f * (L[i] + R[i]);
        }

        // ---- trim x polarity x source fade
        st.inGain.setTarget (inTarget);
        if (width == 2)
        {
            for (int i = 0; i < n; ++i)
            {
                const float g = st.inGain.next() * st.srcFade.next();
                chL[(size_t) i] *= g;
                chR[(size_t) i] *= g;
            }
        }
        else
        {
            for (int i = 0; i < n; ++i)
                chL[(size_t) i] *= st.inGain.next() * st.srcFade.next();
        }

        // ---- Channel DSP: Delay -> HPF -> Gate -> 4-band EQ -> Compressor
        st.dsp.process (chL.data(), chR.data(), width, n, p.dsp, m.dsp);

        // ---- Channel Insert FX (Slots 1..8)
        const int chInsSlot = p.insertFxSlot.getInt();
        if (chInsSlot >= 1 && chInsSlot <= 8)
        {
            auto& insSlotParams = params.fxSlots[(size_t) (chInsSlot - 1)];
            const float* inR = (width == 2) ? chR.data() : chL.data();
            fxRack.processSlot (chInsSlot - 1, chL.data(), inR,
                                insBufL.data(), insBufR.data(), n, insSlotParams);
            std::memcpy (chL.data(), insBufL.data(), sizeof (float) * (size_t) n);
            if (width == 2)
                std::memcpy (chR.data(), insBufR.data(), sizeof (float) * (size_t) n);
        }

        // ---- Real-time 31-band acoustic spectrum analysis
        if ((int) c == rtaChannel.load (std::memory_order_relaxed))
        {
            processRta (chL.data(), chR.data(), width, n, (int) c);
        }

        // ---- pre-fader meter tap
        const float preL = blockPeak (chL.data(), n);
        const float preR = width == 2 ? blockPeak (chR.data(), n) : preL;
        m.preL.update (preL);
        m.preR.update (preR);
        if (preL >= 1.0f || preR >= 1.0f) m.clip.store (true, std::memory_order_relaxed);
        m.width.store (width, std::memory_order_relaxed);

        // ---- fader x mute x pan
        const auto pg = width == 2 ? balanceStereo (p.pan.get()) : panMonoConstantPower (p.pan.get());
        st.postL.setTarget (fader * pg.left);
        st.postR.setTarget (fader * pg.right);
        st.route.setTarget (routed ? 1.0f : 0.0f);
        st.activeWidth = width;

        float pkL = 0.0f, pkR = 0.0f;
        const float* inR = width == 2 ? chR.data() : chL.data();
        for (int i = 0; i < n; ++i)
        {
            const float l = chL[(size_t) i] * st.postL.next();
            const float r = inR[i] * st.postR.next();
            pkL = std::max (pkL, std::abs (l));
            pkR = std::max (pkR, std::abs (r));
            const float rg = st.route.next();
            sumL[(size_t) i] += l * rg;
            sumR[(size_t) i] += r * rg;

            if (! muted)
            {
                // MixBus 1..12 accumulation (Buses 1..12 are Mix Buses, NOT FX!)
                for (int b = 0; b < 12; ++b)
                {
                    if (! p.sendMutes[(size_t) b].getBool())
                    {
                        const float bDb = p.sends[(size_t) b].get();
                        if (bDb > -89.0f)
                        {
                            const float bGain = dbToGain (bDb);
                            busBufL[(size_t) b][(size_t) i] += l * bGain;
                            busBufR[(size_t) b][(size_t) i] += r * bGain;
                        }
                    }
                }

                // FX Sends 1..4 accumulation (Buses 13..16 are dedicated FX 1..4!)
                for (int s = 0; s < 4; ++s)
                {
                    const int fxBusIdx = 12 + s;
                    if (! p.sendMutes[(size_t) fxBusIdx].getBool())
                    {
                        const float sDb = p.sends[(size_t) fxBusIdx].get();
                        if (sDb > -89.0f)
                        {
                            const float sGain = dbToGain (sDb);
                            fxSendBufL[(size_t) s][(size_t) i] += l * sGain;
                            fxSendBufR[(size_t) s][(size_t) i] += r * sGain;
                        }
                    }
                }
            }
        }
        m.postL.update (pkL);
        m.postR.update (pkR);
    }

    // ---- Process MixBuses 1..12 (Master fader, mute, Main LR routing & metering)
    for (int b = 0; b < 12; ++b)
    {
        auto& busParam = params.buses[(size_t) b];

        // Check Mute Groups on bus
        bool busMgMuted = false;
        const int busMgMask = busParam.muteGroups.getInt();
        if (busMgMask > 0)
        {
            for (int mg = 0; mg < 6; ++mg)
            {
                if ((busMgMask & (1 << mg)) && params.muteGroupActive[(size_t) mg].load (std::memory_order_relaxed))
                {
                    busMgMuted = true;
                    break;
                }
            }
        }

        // Check DCA Groups on bus
        float busDcaGain = 1.0f;
        bool busDcaMuted = false;
        const int busDcaMask = busParam.dcaGroups.getInt();
        if (busDcaMask > 0)
        {
            for (int d = 0; d < 8; ++d)
            {
                if (busDcaMask & (1 << d))
                {
                    auto& dca = params.dcas[(size_t) d];
                    if (dca.mute.getBool())
                        busDcaMuted = true;
                    busDcaGain *= faderDbToGain (dca.faderDb.get());
                }
            }
        }

        // ---- Bus Insert FX (Slots 1..8)
        const int busInsSlot = busParam.insertFxSlot.getInt();
        if (busInsSlot >= 1 && busInsSlot <= 8)
        {
            auto& insSlotParams = params.fxSlots[(size_t) (busInsSlot - 1)];
            fxRack.processSlot (busInsSlot - 1,
                                busBufL[(size_t) b].data(), busBufR[(size_t) b].data(),
                                insBufL.data(), insBufR.data(), n, insSlotParams);
            std::memcpy (busBufL[(size_t) b].data(), insBufL.data(), sizeof (float) * (size_t) n);
            std::memcpy (busBufR[(size_t) b].data(), insBufR.data(), sizeof (float) * (size_t) n);
        }

        const bool busMuted = busParam.mute.getBool() || busMgMuted || busDcaMuted;
        const float busGain = busMuted ? 0.0f : (faderDbToGain (busParam.faderDb.get()) * busDcaGain);
        const bool busRoutedToMain = busParam.mainAssign.getBool() && (! anySolo || busParam.solo.getBool());
        const auto busPan = panMonoConstantPower (busParam.pan.get());

        float busPk = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            const float bL = busBufL[(size_t) b][(size_t) i] * busGain;
            const float bR = busBufR[(size_t) b][(size_t) i] * busGain;
            busPk = std::max (busPk, std::max (std::abs (bL), std::abs (bR)));

            if (busRoutedToMain)
            {
                sumL[(size_t) i] += bL * busPan.left  + bR * busPan.left;
                sumR[(size_t) i] += bL * busPan.right + bR * busPan.right;
            }
        }
        meters.buses[(size_t) b].update (busPk);
    }

    // ---- Process FX 1..4 (Send/Return Busses)
    for (int s = 0; s < 4; ++s)
    {
        auto& slotParams = params.fxSlots[(size_t) s];
        if (slotParams.insertMode.getInt() == 0) // Send/Return mode
        {
            // FX Send Master fader & mute (Buses 13..16)
            auto& sendBus = params.buses[(size_t) (12 + s)];
            const float sendTarget = sendBus.mute.getBool() ? 0.0f : faderDbToGain (sendBus.faderDb.get());
            fxSendGains[(size_t) s].setTarget (sendTarget);

            float sendPk = 0.0f;
            for (int i = 0; i < n; ++i)
            {
                const float g = fxSendGains[(size_t) s].next();
                fxSendBufL[(size_t) s][(size_t) i] *= g;
                fxSendBufR[(size_t) s][(size_t) i] *= g;
                sendPk = std::max (sendPk, std::max (std::abs (fxSendBufL[(size_t) s][(size_t) i]),
                                                     std::abs (fxSendBufR[(size_t) s][(size_t) i])));
            }
            meters.buses[(size_t) (12 + s)].update (sendPk);

            // Process through FX Rack Processor
            fxRack.processSlot (s, fxSendBufL[(size_t) s].data(), fxSendBufR[(size_t) s].data(),
                                fxRetBufL[(size_t) s].data(), fxRetBufR[(size_t) s].data(),
                                n, slotParams);

            // FX Returns (2 channels per FX slot: Left = 2*s, Right = 2*s + 1)
            const int rL = s * 2;
            const int rR = s * 2 + 1;
            auto& retPL = params.fxReturns[(size_t) rL];
            auto& retPR = params.fxReturns[(size_t) rR];

            const float retTargetL = retPL.mute.getBool() ? 0.0f : faderDbToGain (retPL.faderDb.get());
            const float retTargetR = retPR.mute.getBool() ? 0.0f : faderDbToGain (retPR.faderDb.get());
            fxReturnGains[(size_t) rL].setTarget (retTargetL);
            fxReturnGains[(size_t) rR].setTarget (retTargetR);

            const auto panL = panMonoConstantPower (retPL.pan.get());
            const auto panR = panMonoConstantPower (retPR.pan.get());

            const bool routeL = retPL.mainAssign.getBool() && (! anySolo || retPL.solo.getBool());
            const bool routeR = retPR.mainAssign.getBool() && (! anySolo || retPR.solo.getBool());

            float retPkL = 0.0f, retPkR = 0.0f;
            for (int i = 0; i < n; ++i)
            {
                const float retL = fxRetBufL[(size_t) s][(size_t) i] * fxReturnGains[(size_t) rL].next();
                const float retR = fxRetBufR[(size_t) s][(size_t) i] * fxReturnGains[(size_t) rR].next();

                retPkL = std::max (retPkL, std::abs (retL));
                retPkR = std::max (retPkR, std::abs (retR));

                if (routeL)
                {
                    sumL[(size_t) i] += retL * panL.left;
                    sumR[(size_t) i] += retL * panL.right;
                }
                if (routeR)
                {
                    sumL[(size_t) i] += retR * panR.left;
                    sumR[(size_t) i] += retR * panR.right;
                }
            }
            meters.fxReturns[(size_t) rL].update (retPkL);
            meters.fxReturns[(size_t) rR].update (retPkR);
        }
    }

    // ---- Main Bus Insert FX (Slots 1..8)
    const int mainInsSlot = params.main.insertFxSlot.getInt();
    if (mainInsSlot >= 1 && mainInsSlot <= 8)
    {
        auto& insSlotParams = params.fxSlots[(size_t) (mainInsSlot - 1)];
        fxRack.processSlot (mainInsSlot - 1, sumL.data(), sumR.data(),
                            insBufL.data(), insBufR.data(), n, insSlotParams);
        std::memcpy (sumL.data(), insBufL.data(), sizeof (float) * (size_t) n);
        std::memcpy (sumR.data(), insBufR.data(), sizeof (float) * (size_t) n);
    }

    // ---- main LR
    mainGain.setTarget (faderDbToGain (params.main.faderDb.get()) * (params.main.mute.getBool() ? 0.0f : 1.0f));
    float pkL = 0.0f, pkR = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const float g = mainGain.next();
        const float l = sumL[(size_t) i] * g;
        const float r = sumR[(size_t) i] * g;
        outL[i] = l;
        outR[i] = r;
        pkL = std::max (pkL, std::abs (l));
        pkR = std::max (pkR, std::abs (r));
    }
    meters.main.l.update (pkL);
    meters.main.r.update (pkR);
    if (pkL >= 1.0f || pkR >= 1.0f) meters.main.clip.store (true, std::memory_order_relaxed);

    // Update DCA group meters (highest peak among assigned channels)
    for (int d = 0; d < 8; ++d)
    {
        float dcaPk = 0.0f;
        const int mask = (1 << d);
        for (int c = 0; c < kMaxInputChannels; ++c)
        {
            if (params.channels[(size_t) c].dcaGroups.getInt() & mask)
            {
                dcaPk = std::max (dcaPk, meters.channels[(size_t) c].postL.read());
                dcaPk = std::max (dcaPk, meters.channels[(size_t) c].postR.read());
            }
        }
        meters.dcas[(size_t) d].update (dcaPk);
    }
}

} // namespace mps
