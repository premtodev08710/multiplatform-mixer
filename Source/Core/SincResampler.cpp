#include "SincResampler.h"
#include <algorithm>
#include <cmath>

namespace mps
{
constexpr double kPiD = 3.14159265358979323846;

static double besselI0 (double x) noexcept
{
    double sum = 1.0, term = 1.0;
    const double q = x * x * 0.25;
    for (int k = 1; k < 64; ++k)
    {
        term *= q / ((double) k * (double) k);
        sum += term;
        if (term < sum * 1.0e-12) break;
    }
    return sum;
}

SincResampler::SincResampler (double inRate, double outRate, int zc, int res)
    : zeroCrossings (std::max (4, zc)), tableRes (std::max (32, res))
{
    ratio = inRate / outRate;
    // Anti-alias when downsampling; small guard band so the window transition stays below Nyquist.
    cutoff = (outRate < inRate ? outRate / inRate : 1.0) * 0.97;
    halfWidth = (double) zeroCrossings / cutoff;

    const double beta = 9.0;
    const double i0beta = besselI0 (beta);
    const int size = zeroCrossings * tableRes + 2;
    table.resize ((size_t) size);
    for (int i = 0; i < size; ++i)
    {
        const double u = (double) i / (double) tableRes;          // in zero crossings
        const double r = u / (double) zeroCrossings;
        double w = 0.0;
        if (r < 1.0) w = besselI0 (beta * std::sqrt (1.0 - r * r)) / i0beta;
        const double s = u == 0.0 ? 1.0 : std::sin (kPiD * u) / (kPiD * u);
        table[(size_t) i] = (float) (s * w);
    }
}

int64_t SincResampler::getOutputLength (int64_t inputLength) const noexcept
{
    if (inputLength <= 0) return 0;
    return (int64_t) std::ceil ((double) inputLength / ratio - 1.0e-9);
}

void SincResampler::getInputRange (int64_t outStart, int numOut, int64_t& inStart, int64_t& inCount) const noexcept
{
    const double t0 = (double) outStart * ratio;
    const double t1 = (double) (outStart + std::max (1, numOut) - 1) * ratio;
    inStart = (int64_t) std::floor (t0 - halfWidth);
    const int64_t inEnd = (int64_t) std::ceil (t1 + halfWidth) + 1;
    inCount = inEnd - inStart;
}

float SincResampler::kernel (double x) const noexcept
{
    const double u = std::abs (x) * cutoff;          // zero crossings
    if (u >= (double) zeroCrossings) return 0.0f;
    const double pos = u * (double) tableRes;
    const auto i = (size_t) pos;
    const double frac = pos - (double) i;
    const double v = (double) table[i] + frac * ((double) table[i + 1] - (double) table[i]);
    return (float) (v * cutoff);
}

void SincResampler::process (const float* input, int64_t inStart, int64_t inCount,
                             float* out, int64_t outStart, int numOut) const noexcept
{
    const int64_t inEnd = inStart + inCount;
    for (int j = 0; j < numOut; ++j)
    {
        const double t = (double) (outStart + j) * ratio;
        const int64_t kLo = std::max (inStart, (int64_t) std::ceil (t - halfWidth));
        const int64_t kHi = std::min (inEnd - 1, (int64_t) std::floor (t + halfWidth));
        double acc = 0.0;
        for (int64_t k = kLo; k <= kHi; ++k)
            acc += (double) input[k - inStart] * (double) kernel (t - (double) k);
        out[j] = (float) acc;
    }
}

} // namespace mps
