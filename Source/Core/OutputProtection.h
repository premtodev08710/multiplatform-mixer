#pragma once
#include <algorithm>
#include <cmath>

namespace mps
{
// Stereo-linked studio peak protection for monitored/recorded output only.
// Unity below full scale (100% bit-accurate and linear).
// Overload reduces both channels smoothly with studio attack & release envelopes,
// preventing audio-rate waveform chopping, harmonic buzzing, and digital crackle.
class OutputProtection
{
public:
    void prepare (double sampleRate) noexcept
    {
        const double sr = std::max (1000.0, sampleRate);
        gain = 1.0f;
        env = 0.0f;
        inputPeak = 0.0f;
        // ~1 ms fast attack prevents overshoot while eliminating harsh instantaneous kinks
        attack = (float) (1.0 - std::exp (-1.0 / (0.001 * sr)));
        // ~100 ms musical release for transparent recovery without pumping or fluttering
        release = (float) (1.0 - std::exp (-1.0 / (0.100 * sr)));
    }

    float getInputPeak() const noexcept { return inputPeak; }

    void process (float* left, float* right, int count) noexcept
    {
        inputPeak = 0.0f;
        for (int i = 0; i < count; ++i)
        {
            const float l = std::isfinite (left[i]) ? left[i] : 0.0f;
            const float r = std::isfinite (right[i]) ? right[i] : 0.0f;
            const float peak = std::max (std::abs (l), std::abs (r));
            inputPeak = std::max (inputPeak, peak);

            // Envelope follower with fast attack & smooth musical release
            if (peak > env)
                env += attack * (peak - env);
            else
                env += release * (peak - env);

            const float targetGain = env > 1.0f ? (1.0f / env) : 1.0f;
            if (targetGain < gain)
                gain += attack * (targetGain - gain);
            else
                gain += release * (targetGain - gain);

            float outL = l * gain;
            float outR = r * gain;

            // Absolute ceiling protection: ensure samples never exceed full scale [-1.0f, +1.0f]
            const float p = std::max (std::abs (outL), std::abs (outR));
            if (p > 1.0f)
            {
                const float ceilingGain = 1.0f / p;
                outL *= ceilingGain;
                outR *= ceilingGain;
            }

            left[i] = outL;
            right[i] = outR;
        }
    }
private:
    float inputPeak = 0.0f;
    float gain = 1.0f, env = 0.0f;
    float attack = 0.0f, release = 0.0f;
};
}
