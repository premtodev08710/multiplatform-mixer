// Mix Practice Studio - mixing engine (pure C++, real-time safe process()).
//
// Signal flow implemented in Phase 1 (per input channel):
//
//   Track slot --> Source mode (native / mono sum / L / R)
//              --> Source-switch fade (10 ms)
//              --> Digital Trim x Polarity       (smoothed, 20 ms)
//              --> [Phase 2: Delay -> HPF -> Gate -> EQ -> Compressor]   (not processed yet)
//              --> PRE-FADER METER TAP (+ clip detect >= 0 dBFS)
//              --> Fader x Mute x Pan            (smoothed, 20 ms)
//              --> POST-FADER METER TAP
//              --> Main LR assign x Solo-in-place gate (smoothed, 20 ms)
//              --> Main LR sum --> Main fader x Main mute --> MAIN METER --> output
//
// Solo: "solo in place, after fader/pan". If any channel is soloed, only soloed channels
//       reach the output. Mute always wins (a muted + soloed channel stays silent).
// Pan:  mono strips use constant-power (-3 dB centre); stereo strips use balance.
#pragma once

#include "MixerParams.h"
#include <vector>

namespace mps
{
class MixEngine
{
public:
    MixEngine (MixerParams& params, MixerMeters& meters);

    /** Allocates scratch memory. Call from a non-audio thread before processing starts. */
    void prepare (double sampleRate, int maxBlockSize);

    int getMaxBlockSize() const noexcept { return maxBlock; }
    double getSampleRate() const noexcept { return sampleRate; }
    FxRackManager& getFxRack() noexcept { return fxRack; }

    /** Real-time safe. slotData[slot * 2 + ch] for ch 0/1; slotChannels[slot] is 0 (empty),
        1 (mono) or 2 (stereo). numSamples must be <= maxBlockSize. */
    void process (const float* const* slotData, const int* slotChannels,
                  int numSamples, float* outL, float* outR) noexcept;

    void setRtaChannel (int ch) noexcept { rtaChannel.store (ch, std::memory_order_relaxed); }
    int getRtaChannel() const noexcept { return rtaChannel.load (std::memory_order_relaxed); }

    static constexpr double kGainRampSeconds   = 0.020;
    static constexpr double kSourceFadeSeconds = 0.010;

private:
    struct ChannelState
    {
        LinearSmoother inGain;          // trim * polarity
        LinearSmoother srcFade;         // source switching fade
        LinearSmoother postL, postR;    // fader * mute * pan
        LinearSmoother route;           // main assign * solo gate
        ChannelDspProcessor dsp;
        int activeSource = -1;
        int activeWidth = 0;
    };

    MixerParams& params;
    MixerMeters& meters;
    double sampleRate = 48000.0;
    int maxBlock = 0;
    std::vector<ChannelState> states;
    std::vector<float> chL, chR, sumL, sumR;
    LinearSmoother mainGain;

    FxRackManager fxRack;
    std::array<std::vector<float>, 12> busBufL, busBufR;
    std::array<std::vector<float>, 4> fxSendBufL, fxSendBufR;
    std::array<std::vector<float>, 4> fxRetBufL, fxRetBufR;
    std::vector<float> insBufL, insBufR;
    std::array<LinearSmoother, 8> fxReturnGains;
    std::array<LinearSmoother, 4> fxSendGains;

    std::atomic<int> rtaChannel { 0 };
    std::array<Biquad, 31> rtaFilters;
    std::array<BiquadCoeffs, 31> rtaCoeffs;
    void initRtaFilters (double sr) noexcept;
    void processRta (const float* l, const float* r, int width, int n, int ch) noexcept;
};

} // namespace mps
