// Mix Practice Studio - mixer parameter model (single source of truth).
//
// Every mixer value lives in exactly one std::atomic<float>. The UI (Home, Detail, ...)
// reads/writes these atomics; the audio thread reads them once per block. There are no
// copies per page, so changing pages can never lose or jump a value.
#pragma once

#include "DspMath.h"
#include "ChannelDsp.h"
#include "EffectsDsp.h"
#include <array>
#include <atomic>

namespace mps
{
constexpr int kMaxInputChannels = 32;   // input channel strips
constexpr int kMaxTracks        = 32;   // loaded audio files (track slots)

/** How a channel strip consumes its source track. */
enum class SourceMode : int
{
    Native   = 0,  // mono file -> mono strip, stereo file -> stereo strip (pan acts as balance)
    MonoSum  = 1,  // stereo file summed to mono: (L + R) * 0.5
    LeftOnly = 2,  // only left channel of a stereo file
    RightOnly= 3   // only right channel of a stereo file
};

struct ChannelParams
{
    Param source;      // -1 = none, otherwise track slot index
    Param sourceMode;  // SourceMode
    Param trimDb;      // digital trim (NOT an analogue preamp)
    Param polarity;    // 1 = inverted
    Param faderDb;     // kFaderMinDb (-inf) .. +10
    Param pan;         // -1 .. +1
    Param mute;
    Param solo;
    Param mainAssign;  // routed to Main LR
    Param monoAssign;  // routed to Mono/Center
    Param automixGroup, automixWeightDb;
    Param phantom48v;  // +48V phantom power simulation

    // Full DSP chain (Delay, HPF, Gate, EQ, Compressor)
    ChannelDspParams dsp;

    // MixBus Sends 1..16 (Buses 1..12 and Buses 13..16 / FX 1..4)
    std::array<Param, 16> sends;
    std::array<Param, 16> sendMutes;
    std::array<Param, 8>  sendTaps;

    // Channel FX Insert (0 = None, 1..8 = FX 1..8)
    Param insertFxSlot;

    // Mute Groups assignment bitmask (bit 0..5 = Mute Group 1..6)
    Param muteGroups;

    // DCA Groups assignment bitmask (bit 0..7 = DCA 1..8)
    Param dcaGroups;

    ChannelParams()
    {
        source.init     ({ -1.0f, (float) (kMaxTracks - 1), -1.0f });
        sourceMode.init ({ 0.0f, 3.0f, 0.0f });
        trimDb.init     ({ -12.0f, 60.0f, 0.0f }); // Full digital preamp gain range: -12 dB to +60 dB
        polarity.init   ({ 0.0f, 1.0f, 0.0f });
        faderDb.init    ({ kFaderMinDb, kFaderMaxDb, 0.0f });
        pan.init        ({ -1.0f, 1.0f, 0.0f });
        mute.init       ({ 0.0f, 1.0f, 0.0f });
        solo.init       ({ 0.0f, 1.0f, 0.0f });
        mainAssign.init ({ 0.0f, 1.0f, 1.0f });
        monoAssign.init ({ 0.0f, 1.0f, 0.0f });
        automixGroup.init ({ 0.0f, 2.0f, 0.0f });
        automixWeightDb.init ({ -12.0f, 12.0f, 0.0f });
        phantom48v.init ({ 0.0f, 1.0f, 0.0f });

        for (auto& s : sends)
            s.init ({ -90.0f, 10.0f, -90.0f });

        for (auto& sm : sendMutes)
            sm.init ({ 0.0f, 1.0f, 0.0f });

        for (auto& st : sendTaps)
            st.init ({ 0.0f, 5.0f, 4.0f });

        insertFxSlot.init ({ 0.0f, 8.0f, 0.0f });
        muteGroups.init   ({ 0.0f, 63.0f, 0.0f });
        dcaGroups.init    ({ 0.0f, 255.0f, 0.0f });
    }

    void resetMixValues() noexcept
    {
        sourceMode.resetToDefault(); trimDb.resetToDefault(); polarity.resetToDefault();
        faderDb.resetToDefault(); pan.resetToDefault(); mute.resetToDefault();
        solo.resetToDefault(); mainAssign.resetToDefault(); monoAssign.resetToDefault();
        phantom48v.resetToDefault(); automixGroup.resetToDefault(); automixWeightDb.resetToDefault();
        for (auto& s : sends) s.resetToDefault();
        for (auto& sm : sendMutes) sm.resetToDefault();
        for (auto& st : sendTaps) st.resetToDefault();
        insertFxSlot.resetToDefault();
    }
};

struct MainParams
{
    std::array<Param, 6> matrixSends;
    Param faderDb;
    Param mute;
    Param insertFxSlot; // 0 = None, 1..8 = FX 1..8

    MainParams()
    {
        for (auto& send : matrixSends) send.init ({ -90.0f, 10.0f, -90.0f });
        // Standard console startup: Main sits at 0.0 dB (Unity Gain)
        faderDb.init ({ kFaderMinDb, kFaderMaxDb, 0.0f });
        mute.init    ({ 0.0f, 1.0f, 0.0f });
        insertFxSlot.init ({ 0.0f, 8.0f, 0.0f });
    }
};

struct FxReturnParams
{
    Param faderDb;
    Param pan;
    Param mute;
    Param solo;
    Param mainAssign;
    Param monoAssign;
    Param dcaGroups;
    Param muteGroups;
    ChannelDspParams dsp;

    FxReturnParams()
    {
        faderDb.init ({ kFaderMinDb, kFaderMaxDb, 0.0f }); // 0 dB unity gain default
        pan.init     ({ -1.0f, 1.0f, 0.0f });
        mute.init    ({ 0.0f, 1.0f, 0.0f });
        solo.init    ({ 0.0f, 1.0f, 0.0f });
        mainAssign.init ({ 0.0f, 1.0f, 1.0f });
        monoAssign.init ({ 0.0f, 1.0f, 0.0f });
        dcaGroups.init  ({ 0.0f, 255.0f, 0.0f });
        muteGroups.init ({ 0.0f, 63.0f, 0.0f });
    }
};

struct BusParams
{
    std::array<Param, 6> matrixSends;
    Param faderDb;
    Param mute;
    Param pan;
    Param solo;
    Param mainAssign;
    Param monoAssign;
    Param insertFxSlot;
    Param dcaGroups;
    Param muteGroups;
    ChannelDspParams dsp;

    BusParams()
    {
        for (auto& send : matrixSends) send.init ({ -90.0f, 10.0f, -90.0f });
        faderDb.init ({ kFaderMinDb, kFaderMaxDb, 0.0f }); // 0 dB unity gain
        mute.init    ({ 0.0f, 1.0f, 0.0f });
        pan.init     ({ -1.0f, 1.0f, 0.0f });
        solo.init    ({ 0.0f, 1.0f, 0.0f });
        // Bus sends must not duplicate the direct channel mix until explicitly assigned.
        mainAssign.init ({ 0.0f, 1.0f, 0.0f });
        monoAssign.init ({ 0.0f, 1.0f, 0.0f });
        insertFxSlot.init ({ 0.0f, 8.0f, 0.0f });
        dcaGroups.init  ({ 0.0f, 255.0f, 0.0f });
        muteGroups.init ({ 0.0f, 63.0f, 0.0f });
    }
};

struct AuxParams
{
    Param faderDb;
    Param mute;
    Param pan;
    Param solo;
    Param trimDb;
    Param polarity;
    Param phantom48v;
    Param source;
    Param mainAssign;
    Param monoAssign;
    Param dcaGroups;
    Param muteGroups;
    ChannelDspParams dsp;

    AuxParams()
    {
        faderDb.init ({ kFaderMinDb, kFaderMaxDb, 0.0f });
        mute.init    ({ 0.0f, 1.0f, 0.0f });
        pan.init     ({ -1.0f, 1.0f, 0.0f });
        solo.init    ({ 0.0f, 1.0f, 0.0f });
        trimDb.init  ({ -12.0f, 60.0f, 0.0f });
        polarity.init({ 0.0f, 1.0f, 0.0f });
        phantom48v.init({ 0.0f, 1.0f, 0.0f });
        source.init  ({ -1.0f, 31.0f, -1.0f });
        mainAssign.init ({ 0.0f, 1.0f, 1.0f });
        monoAssign.init ({ 0.0f, 1.0f, 0.0f });
        dcaGroups.init  ({ 0.0f, 255.0f, 0.0f });
        muteGroups.init ({ 0.0f, 63.0f, 0.0f });
    }
};

struct MatrixParams
{
    Param faderDb;
    Param mute;
    Param pan;
    Param solo;
    Param polarity;
    Param insertFxSlot;
    ChannelDspParams dsp;

    MatrixParams()
    {
        faderDb.init ({ kFaderMinDb, kFaderMaxDb, 0.0f });
        mute.init    ({ 0.0f, 1.0f, 0.0f });
        pan.init     ({ -1.0f, 1.0f, 0.0f });
        solo.init    ({ 0.0f, 1.0f, 0.0f });
        polarity.init({ 0.0f, 1.0f, 0.0f });
        insertFxSlot.init ({ 0.0f, 8.0f, 0.0f });
    }
};

struct DcaParams
{
    Param faderDb;
    Param mute;

    DcaParams()
    {
        faderDb.init ({ kFaderMinDb, kFaderMaxDb, 0.0f }); // 0 dB unity gain default
        mute.init    ({ 0.0f, 1.0f, 0.0f });
    }
};

struct MixerParams
{
    std::array<ChannelParams, kMaxInputChannels> channels;
    MainParams main;
    // Audition/output pair: 0 Main LR, 1..16 Bus, 17..22 Matrix.
    Param outputSource;
    Param monoFaderDb, monoMute;
    std::array<Param, 2> automixEnabled;
    // Main monitoring, solo PFL, or solo AFL; monitoring never changes Main meters.
    Param monitorMode, monitorLevelDb, monitorDim, monitorDimDb, monitorMute, monitorMono;
    std::array<FxSlotParams, 8> fxSlots;
    std::array<FxReturnParams, 8> fxReturns;
    std::array<BusParams, 16> buses;
    std::array<AuxParams, 8> auxes;
    std::array<MatrixParams, 6> matrices;
    std::array<DcaParams, 8> dcas;
    std::array<std::atomic<bool>, 16> channelPairLinked;
    std::array<std::atomic<bool>, 6> muteGroupActive;

    MixerParams()
    {
        outputSource.init ({ 0.0f, 23.0f, 0.0f });
        monoFaderDb.init ({ -90.0f, 10.0f, 0.0f });
        monoMute.init ({ 0.0f, 1.0f, 0.0f });
        for (auto& enabled : automixEnabled) enabled.init ({ 0.0f, 1.0f, 0.0f });
        monitorMode.init ({ 0.0f, 2.0f, 0.0f });
        monitorLevelDb.init ({ -90.0f, 10.0f, 0.0f });
        monitorDim.init ({ 0.0f, 1.0f, 0.0f });
        monitorDimDb.init ({ -40.0f, 0.0f, -20.0f });
        monitorMute.init ({ 0.0f, 1.0f, 0.0f });
        monitorMono.init ({ 0.0f, 1.0f, 0.0f });
        for (auto& l : channelPairLinked) l.store (false, std::memory_order_relaxed);
        for (auto& m : muteGroupActive)    m.store (false, std::memory_order_relaxed);

        // Match standard X32 console default FX assignments (Reverbs, Delays, Modulation, Insert Dynamics)
        fxSlots[0].setDefaultsForType (FxType::HallReverb);             // FX 1: Hall Reverb (Bus 13)
        fxSlots[1].setDefaultsForType (FxType::RichPlateReverb);        // FX 2: Plate Reverb (Bus 14)
        fxSlots[2].setDefaultsForType (FxType::StereoDelay);            // FX 3: Stereo Delay (Bus 15)
        fxSlots[3].setDefaultsForType (FxType::StereoChorus);           // FX 4: Stereo Chorus (Bus 16)
        fxSlots[4].setDefaultsForType (FxType::StereoEnhancer);         // FX 5: Stereo Enhancer (Insert)
        fxSlots[5].setDefaultsForType (FxType::StereoXtecEQ1);          // FX 6: Pultec Tube EQ (Insert)
        fxSlots[6].setDefaultsForType (FxType::StereoLeisureCompressor); // FX 7: LA-2A Optical Comp (Insert)
        fxSlots[7].setDefaultsForType (FxType::MsFairCompressor);       // FX 8: Fairchild 670 Comp (Insert)
        for (int i = 4; i < 8; ++i)
            fxSlots[(size_t) i].insertMode.setBool (true); // FX 5-8 are insert-only

        // Defaults: FX 1-4 on Bus 13-16; FX 5-8 insert on Ch 1-2, Bus 1-2, Mtx 1-2, Main LR
        fxSlots[0].insertTargets[0].set (113.0f); fxSlots[0].insertTargets[1].set (113.0f);
        fxSlots[1].insertTargets[0].set (114.0f); fxSlots[1].insertTargets[1].set (114.0f);
        fxSlots[2].insertTargets[0].set (115.0f); fxSlots[2].insertTargets[1].set (115.0f);
        fxSlots[3].insertTargets[0].set (116.0f); fxSlots[3].insertTargets[1].set (116.0f);

        fxSlots[4].insertTargets[0].set (1.0f);   fxSlots[4].insertTargets[1].set (2.0f);
        fxSlots[5].insertTargets[0].set (101.0f); fxSlots[5].insertTargets[1].set (102.0f);
        fxSlots[6].insertTargets[0].set (201.0f); fxSlots[6].insertTargets[1].set (202.0f);
        fxSlots[7].insertTargets[0].set (301.0f); fxSlots[7].insertTargets[1].set (301.0f);

        // Returns 1L, 2L, 3L, 4L default pan hard-left (-1.0)
        // Returns 1R, 2R, 3R, 4R default pan hard-right (+1.0)
        for (int s = 0; s < 4; ++s)
        {
            fxReturns[(size_t) (s * 2)].pan.init ({ -1.0f, 1.0f, -1.0f });
            fxReturns[(size_t) (s * 2 + 1)].pan.init ({ -1.0f, 1.0f, 1.0f });
        }
    }

    void resetToDefaults()
    {
        for (auto& c : channels) c.resetMixValues();
        for (auto& r : fxReturns)
        {
            r.faderDb.resetToDefault();
            r.mute.resetToDefault();
            r.solo.resetToDefault();
            r.mainAssign.resetToDefault();
        }
        for (auto& b : buses)
        {
            b.faderDb.resetToDefault();
            b.mute.resetToDefault();
            b.solo.resetToDefault();
            b.mainAssign.resetToDefault();
            b.insertFxSlot.resetToDefault();
        }
        for (auto& m : matrices)
        {
            m.faderDb.resetToDefault();
            m.mute.resetToDefault();
            m.solo.resetToDefault();
            m.insertFxSlot.resetToDefault();
        }
        main.faderDb.resetToDefault();
        main.mute.resetToDefault();
        main.insertFxSlot.resetToDefault();

        fxSlots[0].setDefaultsForType (FxType::HallReverb);
        fxSlots[1].setDefaultsForType (FxType::RichPlateReverb);
        fxSlots[2].setDefaultsForType (FxType::StereoDelay);
        fxSlots[3].setDefaultsForType (FxType::StereoChorus);
        fxSlots[4].setDefaultsForType (FxType::StereoEnhancer);
        fxSlots[5].setDefaultsForType (FxType::StereoXtecEQ1);
        fxSlots[6].setDefaultsForType (FxType::StereoLeisureCompressor);
        fxSlots[7].setDefaultsForType (FxType::MsFairCompressor);
        for (int i = 4; i < 8; ++i)
            fxSlots[(size_t) i].insertMode.setBool (true);

        for (int s = 0; s < 4; ++s)
        {
            fxReturns[(size_t) (s * 2)].pan.set (-1.0f);
            fxReturns[(size_t) (s * 2 + 1)].pan.set (1.0f);
        }
    }

    bool anySolo() const noexcept
    {
        for (auto& c : channels)
            if (c.solo.getBool()) return true;
        for (auto& r : fxReturns)
            if (r.solo.getBool()) return true;
        for (auto& b : buses)
            if (b.solo.getBool()) return true;
        for (auto& a : auxes)
            if (a.solo.getBool()) return true;
        for (auto& m : matrices)
            if (m.solo.getBool()) return true;
        return false;
    }

    bool isChannelLinked (int ch) const noexcept
    {
        if (ch >= 0 && ch < kMaxInputChannels)
            return channelPairLinked[(size_t) (ch / 2)].load (std::memory_order_relaxed);
        return false;
    }

    int getLinkedChannel (int ch) const noexcept
    {
        return (ch % 2 == 0) ? (ch + 1) : (ch - 1);
    }
};

//==============================================================================
struct ChannelMeters
{
    AtomicPeak preL, preR;    // pre-fader: after trim/polarity and channel processing
    AtomicPeak postL, postR;  // post-fader: after fader, mute and pan (before solo/main routing)
    std::atomic<bool> clip { false };   // pre-fader tap reached >= 0 dBFS
    std::atomic<int>  width { 0 };      // 0 = no signal source, 1 = mono, 2 = stereo
    ChannelDspMeters dsp;               // Gate GR and Comp GR
    std::array<AtomicPeak, 31> rtaBands; // Real-time 31-band acoustic spectrum tap
};

struct MainMeters
{
    AtomicPeak l, r;                    // final output after main fader
    std::atomic<bool> clip { false };   // output sample >= 0 dBFS (will clip at the DAC)
};

struct MixerMeters
{
    std::array<ChannelMeters, kMaxInputChannels> channels;
    std::array<ChannelMeters, 16> busDetails;
    std::array<ChannelMeters, 6> matrixDetails;
    MainMeters main;
    std::array<AtomicPeak, 8> fxReturns;
    std::array<AtomicPeak, 16> buses;
    std::array<AtomicPeak, 8> auxes;
    std::array<AtomicPeak, 6> matrices;
    std::array<AtomicPeak, 8> dcas;
    AtomicPeak mainC;
};

} // namespace mps
