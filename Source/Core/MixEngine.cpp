#include "MixEngine.h"
#include <cstring>

namespace mps
{
MixEngine::MixEngine (MixerParams& p, MixerMeters& m) : params (p), meters (m) {}

void MixEngine::prepare (double sr, int maxBlockSize)
{
    sampleRate = sr > 0.0 ? sr : 48000.0;
    maxBlock = std::max (4096, maxBlockSize);

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
        s.automix.reset (sampleRate, kGainRampSeconds);
        s.automix.setImmediate (1.0f);
        s.inGain.setImmediate (1.0f);
        s.srcFade.setImmediate (0.0f);
        s.dsp.prepare (sampleRate);
        s.activeSource = -1;
        s.activeWidth = 0;
    }

    monitorL.assign ((size_t) maxBlock, 0.0f);
    monitorR.assign ((size_t) maxBlock, 0.0f);
    monitorGain.reset (sampleRate, kGainRampSeconds);
    monitorGain.setImmediate (faderDbToGain (params.monitorLevelDb.get()));
    mainGain.reset (sampleRate, kGainRampSeconds);
    mainGain.setImmediate (faderDbToGain (params.main.faderDb.get()) * (params.main.mute.getBool() ? 0.0f : 1.0f));

    fxRack.prepare (sampleRate, maxBlock);
    monoBuffer.assign ((size_t) maxBlock, 0.0f);
    automixLevels.fill (0.0f);
    for (auto& dsp : auxDsp) dsp.prepare (sampleRate);
    for (auto& dsp : busDsp) dsp.prepare (sampleRate);
    for (auto& dsp : matrixDsp) dsp.prepare (sampleRate);
    for (auto& dsp : fxReturnDsp) dsp.prepare (sampleRate);
    for (auto& tap : sendTapBuffers) tap.assign ((size_t) maxBlock, 0.0f);
    for (int i = 0; i < 16; ++i)
    {
        busBufL[i].assign ((size_t) maxBlock, 0.0f);
        busBufR[i].assign ((size_t) maxBlock, 0.0f);
    }
    for (int i = 0; i < 6; ++i)
    {
        mtxBufL[i].assign ((size_t) maxBlock, 0.0f);
        mtxBufR[i].assign ((size_t) maxBlock, 0.0f);
    }
    for (int i = 0; i < 4; ++i)
    {
        fxSendBufL[i].assign ((size_t) maxBlock, 0.0f);
        fxSendBufR[i].assign ((size_t) maxBlock, 0.0f);
        fxRetBufL[i].assign ((size_t) maxBlock, 0.0f);
        fxRetBufR[i].assign ((size_t) maxBlock, 0.0f);
    }
    insBufL.assign ((size_t) maxBlock, 0.0f);
    insBufR.assign ((size_t) maxBlock, 0.0f);

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
    if (ch < 0 || ch >= kMaxInputChannels + 30 || n <= 0) return;
    auto& cm = ch < kMaxInputChannels ? meters.channels[(size_t) ch]
             : ch < kMaxInputChannels + 16 ? meters.busDetails[(size_t) (ch - kMaxInputChannels)]
             : ch < kMaxInputChannels + 22 ? meters.matrixDetails[(size_t) (ch - kMaxInputChannels - 16)]
             : meters.fxReturnDetails[(size_t) (ch - kMaxInputChannels - 22)];
    if (activeRtaTarget != ch)
    {
        for (auto& filter : rtaFilters) filter.reset();
        for (auto& band : cm.rtaBands) band.reset();
        activeRtaTarget = ch;
    }

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
    for (int b = 0; b < 16; ++b)
    {
        std::memset (busBufL[(size_t) b].data(), 0, sizeof (float) * (size_t) n);
        std::memset (busBufR[(size_t) b].data(), 0, sizeof (float) * (size_t) n);
    }
    for (int m = 0; m < 6; ++m)
    {
        std::memset (mtxBufL[(size_t) m].data(), 0, sizeof (float) * (size_t) n);
        std::memset (mtxBufR[(size_t) m].data(), 0, sizeof (float) * (size_t) n);
    }
    for (int s = 0; s < 4; ++s)
    {
        std::memset (fxSendBufL[(size_t) s].data(), 0, sizeof (float) * (size_t) n);
        std::memset (fxSendBufR[(size_t) s].data(), 0, sizeof (float) * (size_t) n);
    }

    const int monitorMode = params.monitorMode.getInt();
    const bool soloSelected = params.anySolo();
    const bool anySolo = monitorMode == 0 && soloSelected;
    // When a channel is soloed, its configured sends must follow it to the
    // corresponding FX return. Otherwise the dry channel is audible while
    // its reverb/delay settings disappear because the return is not soloed.
    std::array<bool, 4> soloFxContext {};
    if (anySolo)
    {
        for (int c = 0; c < kMaxInputChannels; ++c)
        {
            const auto& cp = params.channels[(size_t) c];
            if (! cp.solo.getBool()) continue;
            for (int fx = 0; fx < 4; ++fx)
            {
                const int bus = 12 + fx;
                soloFxContext[(size_t) fx] = soloFxContext[(size_t) fx]
                    || (! cp.sendMutes[(size_t) bus].getBool() && cp.sends[(size_t) bus].get() > -89.0f);
            }
        }
        for (int fx = 0; fx < 4; ++fx)
            soloFxContext[(size_t) fx] = soloFxContext[(size_t) fx]
                || params.buses[(size_t) (12 + fx)].solo.getBool();
    }
    std::fill_n (monitorL.data(), n, 0.0f);
    std::fill_n (monitorR.data(), n, 0.0f);

    std::fill_n (monoBuffer.data(), n, 0.0f);
    std::array<float, 2> automixTotals {};
    const float envelope = std::exp (-(float)n / ((float)sampleRate * 0.05f));
    for (int c = 0; c < kMaxInputChannels; ++c)
    {
        auto& p = params.channels[(size_t)c];
        const int source = p.source.getInt(), group = p.automixGroup.getInt();
        float energy = 0.0f;
        bool muted = p.mute.getBool();
        for (int g = 0; g < 6; ++g)
            muted = muted || ((p.muteGroups.getInt() & (1 << g)) && params.muteGroupActive[(size_t)g].load());
        for (int d = 0; d < 8; ++d)
            muted = muted || ((p.dcaGroups.getInt() & (1 << d)) && params.dcas[(size_t)d].mute.getBool());
        if (!muted && source >= 0 && source < kMaxTracks && slotChannels[source] > 0 && slotData[source*2])
            for (int i = 0; i < n; ++i)
            {
                const float l = slotData[source*2][i];
                const float r = slotChannels[source] > 1 && slotData[source*2+1] ? slotData[source*2+1][i] : l;
                const auto mode = (SourceMode)p.sourceMode.getInt();
                const float detector = mode == SourceMode::RightOnly ? r : mode == SourceMode::MonoSum ? (l+r)*0.5f
                    : mode == SourceMode::LeftOnly ? l : std::max(std::abs(l),std::abs(r));
                energy += detector * detector;
            }
        const float level = std::sqrt (energy / (float)n) * dbToGain(p.trimDb.get() + p.automixWeightDb.get());
        auto& previous = automixLevels[(size_t)c];
        previous = muted ? 0.0f : level + envelope * (previous - level);
        if (group > 0) automixTotals[(size_t)(group-1)] += previous;
    }

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
        const int autoGroup = p.automixGroup.getInt();
        const bool autoEnabled = autoGroup > 0 && params.automixEnabled[(size_t)(autoGroup-1)].getBool();
        const float autoTarget = autoEnabled ? automixLevels[(size_t)c] / std::max(1.0e-12f, automixTotals[(size_t)(autoGroup-1)]) : 1.0f;
        st.automix.setTarget (autoTarget);
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
        float* tapPointers[6];
        for (int t = 0; t < 6; ++t) tapPointers[t] = sendTapBuffers[(size_t) t].data();
        st.dsp.process (chL.data(), chR.data(), width, n, p.dsp, m.dsp, tapPointers);

        // ---- Channel Insert FX (Slots 1..8)
        const int chInsSlot = p.insertFxSlot.getInt();
        if (chInsSlot >= 1 && chInsSlot <= 8)
        {
            auto& insSlotParams = params.fxSlots[(size_t) (chInsSlot - 1)];
            const int t0 = insSlotParams.insertTargets[0].getInt();
            const int t1 = insSlotParams.insertTargets[1].getInt();
            const int myCode = (int) c + 1;

            if (t0 == myCode && t1 != myCode && t1 != 0)
            {
                fxRack.processSlot (chInsSlot - 1, chL.data(), chL.data(),
                                    insBufL.data(), insBufR.data(), n, insSlotParams);
                std::memcpy (chL.data(), insBufL.data(), sizeof (float) * (size_t) n);
            }
            else if (t1 == myCode && t0 != myCode && t0 != 0)
            {
                fxRack.processSlot (chInsSlot - 1, chL.data(), chL.data(),
                                    insBufL.data(), insBufR.data(), n, insSlotParams);
                std::memcpy (chL.data(), insBufR.data(), sizeof (float) * (size_t) n);
            }
            else
            {
                const float* inR = (width == 2) ? chR.data() : chL.data();
                fxRack.processSlot (chInsSlot - 1, chL.data(), inR,
                                    insBufL.data(), insBufR.data(), n, insSlotParams);
                std::memcpy (chL.data(), insBufL.data(), sizeof (float) * (size_t) n);
                if (width == 2)
                    std::memcpy (chR.data(), insBufR.data(), sizeof (float) * (size_t) n);
            }
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
            const float autoGain = st.automix.next();
            const float l = chL[(size_t) i] * st.postL.next() * autoGain;
            const float r = inR[i] * st.postR.next() * autoGain;
            if (p.monoAssign.getBool() && (!anySolo || p.solo.getBool()))
                monoBuffer[(size_t)i] += 0.5f * (chL[(size_t)i] + inR[i]) * fader * autoGain;
            pkL = std::max (pkL, std::abs (l));
            pkR = std::max (pkR, std::abs (r));
            if (p.solo.getBool())
            {
                monitorL[(size_t) i] += monitorMode == 1 ? chL[(size_t) i] * pg.left : l;
                monitorR[(size_t) i] += monitorMode == 1 ? inR[i] * pg.right : r;
            }
            const float rg = st.route.next();
            sumL[(size_t) i] += l * rg;
            sumR[(size_t) i] += r * rg;

            if (! muted && (! anySolo || p.solo.getBool()))
            {
                for (int b = 0; b < 16; ++b)
                {
                    if (p.sendMutes[(size_t) b].getBool()) continue;
                    const int tap = p.sendTaps[(size_t) (b / 2)].getInt();
                    const float level = tap == 5 ? 1.0f : faderDbToGain (p.sends[(size_t) b].get());
                    float sendL = l, sendR = r;
                    if (tap < 3)
                    {
                        sendL = sendTapBuffers[(size_t) (tap * 2)][(size_t) i] * pg.left;
                        sendR = sendTapBuffers[(size_t) (tap * 2 + 1)][(size_t) i] * pg.right;
                    }
                    else if (tap == 3)
                    {
                        sendL = chL[(size_t) i] * pg.left;
                        sendR = inR[i] * pg.right;
                    }
                    busBufL[(size_t) b][(size_t) i] += sendL * level;
                    busBufR[(size_t) b][(size_t) i] += sendR * level;
                }
            }
        }
        m.postL.update (pkL);
        m.postR.update (pkR);
    }

    // Aux strips consume an explicitly selected file source; disconnected by default.
    for (int a = 0; a < 8; ++a)
    {
        auto& p = params.auxes[(size_t)a]; const int source = p.source.getInt();
        if (source < 0 || source >= kMaxTracks || slotChannels[source] == 0)
        { meters.auxes[(size_t)a].update (0); continue; }
        const int width = slotChannels[source] > 1 ? 2 : 1;
        const float gain = dbToGain(p.trimDb.get()) * (p.polarity.getBool() ? -1.0f : 1.0f);
        for (int i = 0; i < n; ++i)
        {
            chL[(size_t)i] = slotData[source*2][i] * gain;
            chR[(size_t)i] = width == 2 && slotData[source*2+1] ? slotData[source*2+1][i]*gain : chL[(size_t)i];
        }
        auxDsp[(size_t)a].process(chL.data(), chR.data(), width, n, p.dsp, auxDspMeters[(size_t)a]);
        bool muted = p.mute.getBool(); float fader = faderDbToGain(p.faderDb.get());
        for (int g = 0; g < 6; ++g) muted = muted || ((p.muteGroups.getInt() & (1<<g)) && params.muteGroupActive[(size_t)g].load());
        for (int d = 0; d < 8; ++d) if (p.dcaGroups.getInt() & (1<<d))
        { muted = muted || params.dcas[(size_t)d].mute.getBool(); fader *= faderDbToGain(params.dcas[(size_t)d].faderDb.get()); }
        if (muted) fader = 0;
        const auto pan = width == 2 ? balanceStereo(p.pan.get()) : panMonoConstantPower(p.pan.get());
        float peak = 0;
        for (int i = 0; i < n; ++i)
        {
            const float dryL = chL[(size_t)i], dryR = width == 2 ? chR[(size_t)i] : dryL;
            const float l = dryL*fader*pan.left, r = dryR*fader*pan.right;
            peak = std::max(peak, std::max(std::abs(l),std::abs(r)));
            if (p.mainAssign.getBool() && (!anySolo || p.solo.getBool())) { sumL[(size_t)i] += l; sumR[(size_t)i] += r; }
            if (p.monoAssign.getBool() && (!anySolo || p.solo.getBool())) monoBuffer[(size_t)i] += (dryL+dryR)*0.5f*fader;
            if (p.solo.getBool())
            {
                monitorL[(size_t)i] += monitorMode == 1 ? dryL*pan.left : l;
                monitorR[(size_t)i] += monitorMode == 1 ? dryR*pan.right : r;
            }
        }
        meters.auxes[(size_t)a].update(peak);
    }

    // ---- Process MixBuses 1..16 (Master fader, mute, insert FX, Main LR routing & metering)
    for (int b = 0; b < 16; ++b)
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

        busDsp[(size_t) b].process (busBufL[(size_t) b].data(), busBufR[(size_t) b].data(),
                                    2, n, busParam.dsp, meters.busDetails[(size_t) b].dsp);

        if (rtaChannel.load (std::memory_order_relaxed) == kMaxInputChannels + b)
            processRta (busBufL[(size_t) b].data(), busBufR[(size_t) b].data(), 2, n, kMaxInputChannels + b);

        // ---- Bus Insert FX (Slots 1..8)
        const int busInsSlot = busParam.insertFxSlot.getInt();
        if (busInsSlot >= 1 && busInsSlot <= 8
            && !(b >= 12 && busInsSlot == b - 11 && !params.fxSlots[(size_t) (b - 12)].insertMode.getBool()))
        {
            auto& insSlotParams = params.fxSlots[(size_t) (busInsSlot - 1)];
            const int t0 = insSlotParams.insertTargets[0].getInt();
            const int t1 = insSlotParams.insertTargets[1].getInt();
            const int myCode = 100 + (b + 1);

            if (t0 == myCode && t1 != myCode && t1 != 0)
            {
                fxRack.processSlot (busInsSlot - 1,
                                    busBufL[(size_t) b].data(), busBufL[(size_t) b].data(),
                                    insBufL.data(), insBufR.data(), n, insSlotParams);
                std::memcpy (busBufL[(size_t) b].data(), insBufL.data(), sizeof (float) * (size_t) n);
                std::memcpy (busBufR[(size_t) b].data(), insBufL.data(), sizeof (float) * (size_t) n);
            }
            else if (t1 == myCode && t0 != myCode && t0 != 0)
            {
                fxRack.processSlot (busInsSlot - 1,
                                    busBufR[(size_t) b].data(), busBufR[(size_t) b].data(),
                                    insBufL.data(), insBufR.data(), n, insSlotParams);
                std::memcpy (busBufL[(size_t) b].data(), insBufR.data(), sizeof (float) * (size_t) n);
                std::memcpy (busBufR[(size_t) b].data(), insBufR.data(), sizeof (float) * (size_t) n);
            }
            else
            {
                fxRack.processSlot (busInsSlot - 1,
                                    busBufL[(size_t) b].data(), busBufR[(size_t) b].data(),
                                    insBufL.data(), insBufR.data(), n, insSlotParams);
                std::memcpy (busBufL[(size_t) b].data(), insBufL.data(), sizeof (float) * (size_t) n);
                std::memcpy (busBufR[(size_t) b].data(), insBufR.data(), sizeof (float) * (size_t) n);
            }
        }

        const bool busMuted = busParam.mute.getBool() || busMgMuted || busDcaMuted;
        const float busGain = busMuted ? 0.0f : (faderDbToGain (busParam.faderDb.get()) * busDcaGain);
        const bool fxSendBus = b >= 12 && !params.fxSlots[(size_t) (b - 12)].insertMode.getBool();
        const bool busRoutedToMain = !fxSendBus && busParam.mainAssign.getBool() && (! anySolo || busParam.solo.getBool());
        const auto busBal = balanceStereo (busParam.pan.get());

        float busPk = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            if (busParam.solo.getBool() && monitorMode == 1)
            {
                monitorL[(size_t) i] += busBufL[(size_t) b][(size_t) i];
                monitorR[(size_t) i] += busBufR[(size_t) b][(size_t) i];
            }
            const float bL = busBufL[(size_t) b][(size_t) i] * busGain;
            const float bR = busBufR[(size_t) b][(size_t) i] * busGain;
            busPk = std::max (busPk, std::max (std::abs (bL), std::abs (bR)));

            busBufL[(size_t) b][(size_t) i] = bL * busBal.left;
            busBufR[(size_t) b][(size_t) i] = bR * busBal.right;
            for (int matrix = 0; matrix < 6; ++matrix)
            {
                const float send = faderDbToGain (busParam.matrixSends[(size_t) matrix].get());
                mtxBufL[(size_t) matrix][(size_t) i] += bL * busBal.left * send;
                mtxBufR[(size_t) matrix][(size_t) i] += bR * busBal.right * send;
            }
            if (busParam.solo.getBool() && monitorMode == 2)
            {
                monitorL[(size_t) i] += bL * busBal.left;
                monitorR[(size_t) i] += bR * busBal.right;
            }
            if (fxSendBus)
            {
                fxSendBufL[(size_t) (b - 12)][(size_t) i] = bL * busBal.left;
                fxSendBufR[(size_t) (b - 12)][(size_t) i] = bR * busBal.right;
            }
            if (busParam.monoAssign.getBool() && (!anySolo || busParam.solo.getBool()))
                monoBuffer[(size_t)i] += (bL+bR)*0.5f;
            if (busRoutedToMain)
            {
                sumL[(size_t) i] += bL * busBal.left;
                sumR[(size_t) i] += bR * busBal.right;
            }
        }
        meters.buses[(size_t) b].update (busPk);
    }

    // ---- Process FX 1..4 (Send/Return Busses when in Send/Return mode)
    for (int s = 0; s < 4; ++s)
    {
        auto& slotParams = params.fxSlots[(size_t) s];
        if (slotParams.insertMode.getInt() == 0) // Send/Return mode
        {
            // Process through FX Rack Processor
            fxRack.processSlot (s, fxSendBufL[(size_t) s].data(), fxSendBufR[(size_t) s].data(),
                                fxRetBufL[(size_t) s].data(), fxRetBufR[(size_t) s].data(),
                                n, slotParams);

            // FX Returns (2 channels per FX slot: Left = 2*s, Right = 2*s + 1)
            const int rL = s * 2;
            const int rR = s * 2 + 1;
            auto& retPL = params.fxReturns[(size_t) rL];
            auto& retPR = params.fxReturns[(size_t) rR];

            // Apply FX Return DSP (4-band Parametric EQ, High Pass, Low Pass, Polarity)
            fxReturnDsp[(size_t) rL].process (fxRetBufL[(size_t) s].data(), nullptr, 1, n,
                                              retPL.dsp, fxReturnDspMeters[(size_t) rL]);
            fxReturnDsp[(size_t) rR].process (fxRetBufR[(size_t) s].data(), nullptr, 1, n,
                                              retPR.dsp, fxReturnDspMeters[(size_t) rR]);

            const int fxRtaBase = kMaxInputChannels + 22;
            if (rtaChannel.load (std::memory_order_relaxed) == fxRtaBase + rL)
                processRta (fxRetBufL[(size_t) s].data(), nullptr, 1, n, fxRtaBase + rL);
            if (rtaChannel.load (std::memory_order_relaxed) == fxRtaBase + rR)
                processRta (fxRetBufR[(size_t) s].data(), nullptr, 1, n, fxRtaBase + rR);

            const float retTargetL = retPL.mute.getBool() ? 0.0f : faderDbToGain (retPL.faderDb.get());
            const float retTargetR = retPR.mute.getBool() ? 0.0f : faderDbToGain (retPR.faderDb.get());
            fxReturnGains[(size_t) rL].setTarget (retTargetL);
            fxReturnGains[(size_t) rR].setTarget (retTargetR);

            const auto panL = panMonoConstantPower (retPL.pan.get());
            const auto panR = panMonoConstantPower (retPR.pan.get());

            const bool fxSolo = anySolo && soloFxContext[(size_t) s];
            const bool routeL = retPL.mainAssign.getBool() && (! anySolo || retPL.solo.getBool() || fxSolo);
            const bool routeR = retPR.mainAssign.getBool() && (! anySolo || retPR.solo.getBool() || fxSolo);

            float retPkL = 0.0f, retPkR = 0.0f;
            for (int i = 0; i < n; ++i)
            {
                const float retL = fxRetBufL[(size_t) s][(size_t) i] * fxReturnGains[(size_t) rL].next();
                const float retR = fxRetBufR[(size_t) s][(size_t) i] * fxReturnGains[(size_t) rR].next();

                retPkL = std::max (retPkL, std::abs (retL));
                retPkR = std::max (retPkR, std::abs (retR));

                if (retPL.solo.getBool())
                {
                    const float sample = monitorMode == 1 ? fxRetBufL[(size_t) s][(size_t) i] : retL;
                    monitorL[(size_t) i] += sample * panL.left;
                    monitorR[(size_t) i] += sample * panL.right;
                }
                if (retPR.solo.getBool())
                {
                    const float sample = monitorMode == 1 ? fxRetBufR[(size_t) s][(size_t) i] : retR;
                    monitorL[(size_t) i] += sample * panR.left;
                    monitorR[(size_t) i] += sample * panR.right;
                }
                if (retPL.monoAssign.getBool() && (!anySolo || retPL.solo.getBool())) monoBuffer[(size_t)i] += retL;
                if (retPR.monoAssign.getBool() && (!anySolo || retPR.solo.getBool())) monoBuffer[(size_t)i] += retR;
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

    // ---- Process Matrices 1..6 (Fed from Main LR and Buses, with Insert FX and metering)
    for (int m = 0; m < 6; ++m)
    {
        auto& mtxParam = params.matrices[(size_t) m];
        const float mainSend = faderDbToGain (params.main.matrixSends[(size_t) m].get())
            * faderDbToGain (params.main.faderDb.get()) * (params.main.mute.getBool() ? 0.0f : 1.0f);
        for (int i = 0; i < n; ++i)
        {
            mtxBufL[(size_t) m][(size_t) i] += sumL[(size_t) i] * mainSend;
            mtxBufR[(size_t) m][(size_t) i] += sumR[(size_t) i] * mainSend;
        }
        matrixDsp[(size_t) m].process (mtxBufL[(size_t) m].data(), mtxBufR[(size_t) m].data(),
                                      2, n, mtxParam.dsp, meters.matrixDetails[(size_t) m].dsp);

        if (rtaChannel.load (std::memory_order_relaxed) == kMaxInputChannels + 16 + m)
            processRta (mtxBufL[(size_t) m].data(), mtxBufR[(size_t) m].data(), 2, n, kMaxInputChannels + 16 + m);

        const int mtxInsSlot = mtxParam.insertFxSlot.getInt();
        if (mtxInsSlot >= 1 && mtxInsSlot <= 8)
        {
            auto& insSlotParams = params.fxSlots[(size_t) (mtxInsSlot - 1)];
            const int t0 = insSlotParams.insertTargets[0].getInt();
            const int t1 = insSlotParams.insertTargets[1].getInt();
            const int myCode = 200 + (m + 1);

            if (t0 == myCode && t1 != myCode && t1 != 0)
            {
                fxRack.processSlot (mtxInsSlot - 1,
                                    mtxBufL[(size_t) m].data(), mtxBufL[(size_t) m].data(),
                                    insBufL.data(), insBufR.data(), n, insSlotParams);
                std::memcpy (mtxBufL[(size_t) m].data(), insBufL.data(), sizeof (float) * (size_t) n);
                std::memcpy (mtxBufR[(size_t) m].data(), insBufL.data(), sizeof (float) * (size_t) n);
            }
            else if (t1 == myCode && t0 != myCode && t0 != 0)
            {
                fxRack.processSlot (mtxInsSlot - 1,
                                    mtxBufR[(size_t) m].data(), mtxBufR[(size_t) m].data(),
                                    insBufL.data(), insBufR.data(), n, insSlotParams);
                std::memcpy (mtxBufL[(size_t) m].data(), insBufR.data(), sizeof (float) * (size_t) n);
                std::memcpy (mtxBufR[(size_t) m].data(), insBufR.data(), sizeof (float) * (size_t) n);
            }
            else
            {
                fxRack.processSlot (mtxInsSlot - 1,
                                    mtxBufL[(size_t) m].data(), mtxBufR[(size_t) m].data(),
                                    insBufL.data(), insBufR.data(), n, insSlotParams);
                std::memcpy (mtxBufL[(size_t) m].data(), insBufL.data(), sizeof (float) * (size_t) n);
                std::memcpy (mtxBufR[(size_t) m].data(), insBufR.data(), sizeof (float) * (size_t) n);
            }
        }

        const bool mtxMuted = mtxParam.mute.getBool();
        const float mtxGain = mtxMuted ? 0.0f : faderDbToGain (mtxParam.faderDb.get())
            * (mtxParam.polarity.getBool() ? -1.0f : 1.0f);
        const auto matrixBalance = balanceStereo (mtxParam.pan.get());
        float mtxPk = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            if (mtxParam.solo.getBool() && monitorMode == 1)
            {
                monitorL[(size_t) i] += mtxBufL[(size_t) m][(size_t) i];
                monitorR[(size_t) i] += mtxBufR[(size_t) m][(size_t) i];
            }
            const float mL = mtxBufL[(size_t) m][(size_t) i] *= mtxGain * matrixBalance.left;
            const float mR = mtxBufR[(size_t) m][(size_t) i] *= mtxGain * matrixBalance.right;
            if (mtxParam.solo.getBool() && monitorMode == 2)
            {
                monitorL[(size_t) i] += mL;
                monitorR[(size_t) i] += mR;
            }
            mtxPk = std::max (mtxPk, std::max (std::abs (mL), std::abs (mR)));
        }
        meters.matrices[(size_t) m].update (mtxPk);
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

    // Route the selected stereo bus to the hardware output pair after metering Main.
    const int outputSource = params.outputSource.getInt();
    float monoPeak = 0;
    const float monoGain = params.monoMute.getBool() ? 0.0f : faderDbToGain(params.monoFaderDb.get());
    for (int i = 0; i < n; ++i) { monoBuffer[(size_t)i] *= monoGain; monoPeak = std::max(monoPeak,std::abs(monoBuffer[(size_t)i])); }
    meters.mainC.update(monoPeak);
    if (outputSource == 23)
    { std::copy_n(monoBuffer.data(), n, outL); std::copy_n(monoBuffer.data(), n, outR); }
    else if (outputSource > 0)
    {
        const float* selectedL = outputSource <= 16 ? busBufL[(size_t) (outputSource - 1)].data()
                                                   : mtxBufL[(size_t) (outputSource - 17)].data();
        const float* selectedR = outputSource <= 16 ? busBufR[(size_t) (outputSource - 1)].data()
                                                   : mtxBufR[(size_t) (outputSource - 17)].data();
        std::memcpy (outL, selectedL, sizeof (float) * (size_t) n);
        std::memcpy (outR, selectedR, sizeof (float) * (size_t) n);
    }

    const float monitorTarget = params.monitorMute.getBool() ? 0.0f
        : faderDbToGain (params.monitorLevelDb.get())
            * (params.monitorDim.getBool() ? dbToGain (params.monitorDimDb.get()) : 1.0f);
    monitorGain.setTarget (monitorTarget);
    for (int i = 0; i < n; ++i)
    {
        float l = monitorMode > 0 && soloSelected ? monitorL[(size_t) i] : outL[i];
        float r = monitorMode > 0 && soloSelected ? monitorR[(size_t) i] : outR[i];
        if (params.monitorMono.getBool()) l = r = (l + r) * 0.5f;
        const float gain = monitorGain.next();
        outL[i] = l * gain; outR[i] = r * gain;
    }

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
