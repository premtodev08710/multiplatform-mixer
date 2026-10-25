// Mix Practice Studio - mixing engine (pure C++, real-time safe process()).
//
// Input strips: source -> trim/polarity -> delay/HPF/gate/EQ/compressor
// -> insert -> fader/pan/AutoMix -> Main/Mono and selectable bus-send taps.
// Buses feed Matrix sends and Main/Mono; output audition and PFL/AFL monitoring
// are selected after independent Main metering. Scratch buffers allocate in prepare().
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
        LinearSmoother route;
        LinearSmoother automix;           // main assign * solo gate
        ChannelDspProcessor dsp;
        int activeSource = -1;
        int activeWidth = 0;
    };

    MixerParams& params;
    MixerMeters& meters;
    double sampleRate = 48000.0;
    int maxBlock = 0;
    std::vector<ChannelState> states;
    std::array<ChannelDspProcessor, 8> auxDsp;
    std::array<ChannelDspMeters, 8> auxDspMeters;
    std::array<float, 32> automixLevels {};
    std::vector<float> monoBuffer;
    std::vector<float> chL, chR, sumL, sumR;
    LinearSmoother mainGain, monitorGain;
    std::vector<float> monitorL, monitorR;

    std::array<ChannelDspProcessor, 16> busDsp;

    std::array<ChannelDspProcessor, 6> matrixDsp;

    std::array<ChannelDspProcessor, 8> fxReturnDsp;
    std::array<ChannelDspMeters, 8> fxReturnDspMeters;

    std::array<std::vector<float>, 6> sendTapBuffers;
    FxRackManager fxRack;
    std::array<std::vector<float>, 16> busBufL, busBufR;
    std::array<std::vector<float>, 6> mtxBufL, mtxBufR;
    std::array<std::vector<float>, 4> fxSendBufL, fxSendBufR;
    std::array<std::vector<float>, 4> fxRetBufL, fxRetBufR;
    std::vector<float> insBufL, insBufR;
    std::array<LinearSmoother, 8> fxReturnGains;
    std::array<LinearSmoother, 4> fxSendGains;

    // RTA targets: inputs 0..31, buses 32..47, matrices 48..53.
    std::atomic<int> rtaChannel { 0 };
    int activeRtaTarget = -1;
    std::array<Biquad, 31> rtaFilters;
    std::array<BiquadCoeffs, 31> rtaCoeffs;
    void initRtaFilters (double sr) noexcept;
    void processRta (const float* l, const float* r, int width, int n, int ch) noexcept;
};

} // namespace mps
