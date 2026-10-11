// Mix Practice Studio - offline windowed-sinc sample-rate converter (pure C++).
//
// Used when an imported file's sample rate differs from the audio device rate. The file is
// converted ONCE in a background job to a 32-bit float cache file at the engine rate, so the
// real-time transport always streams sample-aligned material and never resamples live.
//
// The kernel is symmetric (zero phase): output sample n is centred exactly on input time
// t = n * inRate / outRate. Converted tracks therefore stay time-aligned with native tracks
// (no interpolator latency to compensate).
#pragma once

#include <cstdint>
#include <vector>

namespace mps
{
class SincResampler
{
public:
    SincResampler (double inRate, double outRate, int zeroCrossings = 24, int tableResolution = 512);

    double getRatio() const noexcept { return ratio; }               // input samples per output sample
    int64_t getOutputLength (int64_t inputLength) const noexcept;

    /** Input range [inStart, inStart + inCount) needed to compute outputs [outStart, outStart + numOut). */
    void getInputRange (int64_t outStart, int numOut, int64_t& inStart, int64_t& inCount) const noexcept;

    /** input holds samples [inStart, inStart + inCount) (callers zero-pad outside the file). */
    void process (const float* input, int64_t inStart, int64_t inCount,
                  float* out, int64_t outStart, int numOut) const noexcept;

private:
    float kernel (double x) const noexcept;   // x in input samples

    double ratio = 1.0;     // in / out
    double cutoff = 1.0;    // normalised to input Nyquist
    double halfWidth = 0.0; // in input samples
    int zeroCrossings = 24;
    int tableRes = 512;
    std::vector<float> table;
};

} // namespace mps
