#pragma once
#include <algorithm>
#include <cmath>

namespace mps
{
// Stereo-linked emergency peak protection for monitored/recorded output only.
// Unity below full scale; overload reduces both channels together, then recovers
// over 100 ms. Mixer buses and their clip meters retain their original levels.
class OutputProtection
{
public:
    void prepare (double sampleRate) noexcept
    {
        gain = 1.0f;
        release = (float) (1.0 - std::exp (-1.0 / (0.1 * std::max (1.0, sampleRate))));
    }

    void process (float* left, float* right, int count) noexcept
    {
        for (int i = 0; i < count; ++i)
        {
            const float l = std::isfinite (left[i]) ? left[i] : 0.0f;
            const float r = std::isfinite (right[i]) ? right[i] : 0.0f;
            const float peak = std::max (std::abs (l), std::abs (r));
            const float required = peak > 1.0f ? 1.0f / peak : 1.0f;
            gain = std::min (required, gain + release * (1.0f - gain));
            left[i] = l * gain;
            right[i] = r * gain;
        }
    }
private:
    float gain = 1.0f, release = 0.0f;
};
}
