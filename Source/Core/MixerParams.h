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

    // Full DSP chain (Delay, HPF, Gate, EQ, Compressor)
    ChannelDspParams dsp;

    // FX Sends (FX 1, FX 2, FX 3, FX 4)
    std::array<Param, 4> sends;

    // Channel FX Insert (0 = None, 1..8 = FX 1..8)
    Param insertFxSlot;

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

        for (auto& s : sends)
            s.init ({ -90.0f, 10.0f, -90.0f });

        insertFxSlot.init ({ 0.0f, 8.0f, 0.0f });
    }

    void resetMixValues() noexcept
    {
        sourceMode.resetToDefault(); trimDb.resetToDefault(); polarity.resetToDefault();
        faderDb.resetToDefault(); pan.resetToDefault(); mute.resetToDefault();
        solo.resetToDefault(); mainAssign.resetToDefault();
        for (auto& s : sends) s.resetToDefault();
        insertFxSlot.resetToDefault();
    }
};

struct MainParams
{
    Param faderDb;
    Param mute;
    Param insertFxSlot; // 0 = None, 1..8 = FX 1..8

    MainParams()
    {
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

    FxReturnParams()
    {
        faderDb.init ({ kFaderMinDb, kFaderMaxDb, 0.0f }); // 0 dB unity gain default
        pan.init     ({ -1.0f, 1.0f, 0.0f });
        mute.init    ({ 0.0f, 1.0f, 0.0f });
        solo.init    ({ 0.0f, 1.0f, 0.0f });
    }
};

struct BusParams
{
    Param faderDb;
    Param mute;
    Param pan;

    BusParams()
    {
        faderDb.init ({ kFaderMinDb, kFaderMaxDb, 0.0f }); // 0 dB unity gain
        mute.init    ({ 0.0f, 1.0f, 0.0f });
        pan.init     ({ -1.0f, 1.0f, 0.0f });
    }
};

struct AuxParams
{
    Param faderDb;
    Param mute;
    Param pan;

    AuxParams()
    {
        faderDb.init ({ kFaderMinDb, kFaderMaxDb, 0.0f });
        mute.init    ({ 0.0f, 1.0f, 0.0f });
        pan.init     ({ -1.0f, 1.0f, 0.0f });
    }
};

struct MatrixParams
{
    Param faderDb;
    Param mute;
    Param pan;

    MatrixParams()
    {
        faderDb.init ({ kFaderMinDb, kFaderMaxDb, 0.0f });
        mute.init    ({ 0.0f, 1.0f, 0.0f });
        pan.init     ({ -1.0f, 1.0f, 0.0f });
    }
};

struct MixerParams
{
    std::array<ChannelParams, kMaxInputChannels> channels;
    MainParams main;
    std::array<FxSlotParams, 8> fxSlots;
    std::array<FxReturnParams, 8> fxReturns;
    std::array<BusParams, 16> buses;
    std::array<AuxParams, 8> auxes;
    std::array<MatrixParams, 6> matrices;

    MixerParams()
    {
        // Setup console default effect assignments
        fxSlots[0].setDefaultsForType (FxType::HallReverb);   // FX 1
        fxSlots[1].setDefaultsForType (FxType::Ambience);     // FX 2
        fxSlots[2].setDefaultsForType (FxType::StereoDelay);  // FX 3
        fxSlots[3].setDefaultsForType (FxType::StereoChorus); // FX 4
        fxSlots[4].setDefaultsForType (FxType::DualGEQ);      // FX 5
        fxSlots[5].setDefaultsForType (FxType::DualGEQ);      // FX 6
        fxSlots[6].setDefaultsForType (FxType::DualGEQ);      // FX 7
        fxSlots[7].setDefaultsForType (FxType::DualGEQ);      // FX 8

        // Returns 1L, 2L, 3L, 4L default pan hard-left (-1.0)
        // Returns 1R, 2R, 3R, 4R default pan hard-right (+1.0)
        for (int s = 0; s < 4; ++s)
        {
            fxReturns[(size_t) (s * 2)].pan.init ({ -1.0f, 1.0f, -1.0f });
            fxReturns[(size_t) (s * 2 + 1)].pan.init ({ -1.0f, 1.0f, 1.0f });
        }
    }

    bool anySolo() const noexcept
    {
        for (auto& c : channels)
            if (c.solo.getBool()) return true;
        for (auto& r : fxReturns)
            if (r.solo.getBool()) return true;
        return false;
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
};

struct MainMeters
{
    AtomicPeak l, r;                    // final output after main fader
    std::atomic<bool> clip { false };   // output sample >= 0 dBFS (will clip at the DAC)
};

struct MixerMeters
{
    std::array<ChannelMeters, kMaxInputChannels> channels;
    MainMeters main;
    std::array<AtomicPeak, 8> fxReturns;
    std::array<AtomicPeak, 16> buses;
    std::array<AtomicPeak, 8> auxes;
    std::array<AtomicPeak, 6> matrices;
    AtomicPeak mainC;
};

} // namespace mps
