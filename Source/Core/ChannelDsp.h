// Mix Practice Studio - Channel DSP processing (pure C++ audio engine).
//
// Includes:
//  - Input Delay (up to 500ms sample delay)
//  - High-Pass Filter (HPF / Low Cut: 12 dB/oct Butterworth, 20 Hz - 400 Hz)
//  - Expander / Noise Gate (Threshold, Range, Attack, Hold, Release, Gain Reduction meter)
//  - 4-Band Parametric EQ (Low Bell/Shelf, Mid1 Bell, Mid2 Bell, High Bell/Shelf)
//  - Dynamics / Compressor (Threshold, Ratio, Knee, Attack, Release, Makeup, Dry/Wet, GR meter)
#pragma once

#include "DspMath.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <vector>

namespace mps
{

//==============================================================================
// Biquad filter (Direct Form II Transposed for numerical stability)
struct BiquadCoeffs
{
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f; // normalized so a0 = 1.0

    // Evaluate magnitude response |H(e^{j w})| in dB at frequency f given sample rate sr
    float getMagnitudeResponseDb (float f, float sr) const noexcept
    {
        const float w = 2.0f * kPi * f / sr;
        const float cosw = std::cos (w);
        const float cos2w = std::cos (2.0f * w);
        const float sinw = std::sin (w);
        const float sin2w = std::sin (2.0f * w);

        const float numReal = b0 + b1 * cosw + b2 * cos2w;
        const float numImag = -b1 * sinw - b2 * sin2w;
        const float denReal = 1.0f + a1 * cosw + a2 * cos2w;
        const float denImag = -a1 * sinw - a2 * sin2w;

        const float numMagSq = numReal * numReal + numImag * numImag;
        const float denMagSq = denReal * denReal + denImag * denImag;

        if (denMagSq <= 1.0e-12f) return 0.0f;
        const float magSq = numMagSq / denMagSq;
        return 10.0f * std::log10 (std::max (1.0e-8f, magSq));
    }
};

class Biquad
{
public:
    void reset() noexcept { z1 = z2 = 0.0f; }

    inline float process (float x, const BiquadCoeffs& c) noexcept
    {
        const float y = c.b0 * x + z1;
        z1 = c.b1 * x - c.a1 * y + z2;
        z2 = c.b2 * x - c.a2 * y;
        return y;
    }

private:
    float z1 = 0.0f, z2 = 0.0f;
};

// Filter coefficient calculators (Audio EQ Cookbook by Robert Bristow-Johnson)
inline BiquadCoeffs calcHPF (float fc, float Q, float sr) noexcept
{
    BiquadCoeffs c;
    fc = clampf (fc, 10.0f, sr * 0.49f);
    const float w0 = 2.0f * kPi * fc / sr;
    const float cosw = std::cos (w0);
    const float sinw = std::sin (w0);
    const float alpha = sinw / (2.0f * std::max (0.01f, Q));
    const float a0 = 1.0f + alpha;

    c.b0 = ((1.0f + cosw) * 0.5f) / a0;
    c.b1 = (-(1.0f + cosw)) / a0;
    c.b2 = ((1.0f + cosw) * 0.5f) / a0;
    c.a1 = (-2.0f * cosw) / a0;
    c.a2 = (1.0f - alpha) / a0;
    return c;
}

inline BiquadCoeffs calcLPF (float fc, float Q, float sr) noexcept
{
    BiquadCoeffs c;
    fc = clampf (fc, 10.0f, sr * 0.49f);
    const float w0 = 2.0f * kPi * fc / sr;
    const float cosw = std::cos (w0);
    const float sinw = std::sin (w0);
    const float alpha = sinw / (2.0f * std::max (0.01f, Q));
    const float a0 = 1.0f + alpha;

    c.b0 = ((1.0f - cosw) * 0.5f) / a0;
    c.b1 = (1.0f - cosw) / a0;
    c.b2 = ((1.0f - cosw) * 0.5f) / a0;
    c.a1 = (-2.0f * cosw) / a0;
    c.a2 = (1.0f - alpha) / a0;
    return c;
}

inline BiquadCoeffs calcBell (float fc, float gainDb, float Q, float sr) noexcept
{
    BiquadCoeffs c;
    fc = clampf (fc, 10.0f, sr * 0.49f);
    const float A = std::pow (10.0f, gainDb * 0.025f); // sqrt of 10^(dB/20)
    const float w0 = 2.0f * kPi * fc / sr;
    const float cosw = std::cos (w0);
    const float sinw = std::sin (w0);
    const float alpha = sinw / (2.0f * std::max (0.05f, Q));
    const float a0 = 1.0f + alpha / A;

    c.b0 = (1.0f + alpha * A) / a0;
    c.b1 = (-2.0f * cosw) / a0;
    c.b2 = (1.0f - alpha * A) / a0;
    c.a1 = (-2.0f * cosw) / a0;
    c.a2 = (1.0f - alpha / A) / a0;
    return c;
}

inline BiquadCoeffs calcLowShelf (float fc, float gainDb, float Q, float sr) noexcept
{
    BiquadCoeffs c;
    fc = clampf (fc, 10.0f, sr * 0.49f);
    const float A = std::pow (10.0f, gainDb * 0.025f);
    const float w0 = 2.0f * kPi * fc / sr;
    const float cosw = std::cos (w0);
    const float sinw = std::sin (w0);
    const float alpha = sinw / (2.0f * std::max (0.05f, Q));
    const float twoSqrtAAlpha = 2.0f * std::sqrt (A) * alpha;

    const float a0 = (A + 1.0f) + (A - 1.0f) * cosw + twoSqrtAAlpha;
    c.b0 = (A * ((A + 1.0f) - (A - 1.0f) * cosw + twoSqrtAAlpha)) / a0;
    c.b1 = (2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw)) / a0;
    c.b2 = (A * ((A + 1.0f) - (A - 1.0f) * cosw - twoSqrtAAlpha)) / a0;
    c.a1 = (-2.0f * ((A - 1.0f) + (A + 1.0f) * cosw)) / a0;
    c.a2 = ((A + 1.0f) + (A - 1.0f) * cosw - twoSqrtAAlpha) / a0;
    return c;
}

inline BiquadCoeffs calcHighShelf (float fc, float gainDb, float Q, float sr) noexcept
{
    BiquadCoeffs c;
    fc = clampf (fc, 10.0f, sr * 0.49f);
    const float A = std::pow (10.0f, gainDb * 0.025f);
    const float w0 = 2.0f * kPi * fc / sr;
    const float cosw = std::cos (w0);
    const float sinw = std::sin (w0);
    const float alpha = sinw / (2.0f * std::max (0.05f, Q));
    const float twoSqrtAAlpha = 2.0f * std::sqrt (A) * alpha;

    const float a0 = (A + 1.0f) - (A - 1.0f) * cosw + twoSqrtAAlpha;
    c.b0 = (A * ((A + 1.0f) + (A - 1.0f) * cosw + twoSqrtAAlpha)) / a0;
    c.b1 = (-2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw)) / a0;
    c.b2 = (A * ((A + 1.0f) + (A - 1.0f) * cosw - twoSqrtAAlpha)) / a0;
    c.a1 = (2.0f * ((A - 1.0f) - (A + 1.0f) * cosw)) / a0;
    c.a2 = ((A + 1.0f) - (A - 1.0f) * cosw - twoSqrtAAlpha) / a0;
    return c;
}

//==============================================================================
// Delay buffer
class DelayLine
{
public:
    void prepare (double sampleRate, float maxDelayMs = 500.0f)
    {
        const int maxSamples = (int) std::ceil (sampleRate * maxDelayMs * 0.001f) + 16;
        buffer.assign ((size_t) maxSamples, 0.0f);
        writeIndex = 0;
        sr = (float) sampleRate;
    }

    void reset() noexcept
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writeIndex = 0;
    }

    inline float process (float input, float delayMs) noexcept
    {
        if (buffer.empty()) return input;
        const int delaySamples = clampf (delayMs * 0.001f * sr, 0.0f, (float) buffer.size() - 2.0f);
        buffer[(size_t) writeIndex] = input;
        int readIndex = writeIndex - delaySamples;
        if (readIndex < 0) readIndex += (int) buffer.size();
        writeIndex = (writeIndex + 1) % (int) buffer.size();
        return buffer[(size_t) readIndex];
    }

private:
    std::vector<float> buffer;
    int writeIndex = 0;
    float sr = 48000.0f;
};

//==============================================================================
// Gate DSP
class GateDsp
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = (float) sampleRate;
        envelope = 0.0f;
        gain = 1.0f;
        holdCounter = 0;
    }

    void reset() noexcept
    {
        envelope = 0.0f;
        gain = 1.0f;
        holdCounter = 0;
    }

    // Process mono or stereo detector, returns linear gain multiplier
    inline float processGain (float xDetector, bool enabled, float threshDb, float rangeDb,
                              float attackMs, float holdMs, float releaseMs) noexcept
    {
        if (! enabled) return 1.0f;

        const float attackCoeff = std::exp (-1.0f / (std::max (0.1f, attackMs) * 0.001f * sr));
        const float releaseCoeff = std::exp (-1.0f / (std::max (1.0f, releaseMs) * 0.001f * sr));
        const int holdSamples = (int) std::lround (holdMs * 0.001f * sr);

        const float inputLevel = std::abs (xDetector);
        const float inputDb = gainToDb (inputLevel);

        const float targetGainDb = (inputDb >= threshDb) ? 0.0f : rangeDb; // rangeDb is negative (e.g. -60 dB)
        const float targetGain = dbToGain (targetGainDb);

        if (inputDb >= threshDb)
        {
            holdCounter = holdSamples;
            // Attack (opening)
            gain = targetGain + attackCoeff * (gain - targetGain);
        }
        else if (holdCounter > 0)
        {
            --holdCounter;
            // Gate remains held open
        }
        else
        {
            // Release (closing)
            gain = targetGain + releaseCoeff * (gain - targetGain);
        }

        return gain;
    }

private:
    float sr = 48000.0f;
    float envelope = 0.0f;
    float gain = 1.0f;
    int holdCounter = 0;
};

//==============================================================================
// Compressor DSP
class CompressorDsp
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = (float) sampleRate;
        envelope = 0.0f;
        gain = 1.0f;
    }

    void reset() noexcept
    {
        envelope = 0.0f;
        gain = 1.0f;
    }

    // Process detector and returns gain reduction in dB (> 0) and linear gain multiplier
    inline float processGain (float xDetector, bool enabled, float threshDb, float ratio,
                              float kneeDb, float attackMs, float releaseMs,
                              float& outGainReductionDb) noexcept
    {
        if (! enabled)
        {
            outGainReductionDb = 0.0f;
            return 1.0f;
        }

        const float attackCoeff = std::exp (-1.0f / (std::max (0.1f, attackMs) * 0.001f * sr));
        const float releaseCoeff = std::exp (-1.0f / (std::max (1.0f, releaseMs) * 0.001f * sr));

        const float inputLevel = std::abs (xDetector);
        const float inputDb = gainToDb (inputLevel);

        // Soft-knee compression characteristic
        float targetGainDb = 0.0f;
        const float halfKnee = kneeDb * 0.5f;

        if (kneeDb > 0.0f && inputDb > (threshDb - halfKnee) && inputDb < (threshDb + halfKnee))
        {
            // Inside the knee region
            const float delta = inputDb - threshDb + halfKnee;
            const float compDb = threshDb + (inputDb - threshDb) + ((1.0f / ratio - 1.0f) * (delta * delta)) / (2.0f * kneeDb);
            targetGainDb = compDb - inputDb;
        }
        else if (inputDb > threshDb)
        {
            // Above knee: full ratio
            const float compDb = threshDb + (inputDb - threshDb) / ratio;
            targetGainDb = compDb - inputDb;
        }

        const float targetGain = dbToGain (targetGainDb);

        // Envelope follower on gain
        if (targetGain < gain)
            gain = targetGain + attackCoeff * (gain - targetGain);
        else
            gain = targetGain + releaseCoeff * (gain - targetGain);

        outGainReductionDb = -gainToDb (gain);
        if (outGainReductionDb < 0.0f) outGainReductionDb = 0.0f;

        return gain;
    }

private:
    float sr = 48000.0f;
    float envelope = 0.0f;
    float gain = 1.0f;
};

//==============================================================================
// Full Channel DSP instance (Input Delay + HPF + Gate + 4-Band EQ + Compressor)
enum class EqBandType : int { Bell = 0, LowShelf = 1, HighShelf = 2 };

struct ChannelDspParams
{
    // Delay
    Param delayMs;      // 0 .. 500 ms
    Param delayBypass;

    // HPF
    Param hpfFreq;      // 20 .. 400 Hz
    Param hpfBypass;

    // Gate
    Param gateBypass;
    Param gateThreshDb; // -80 .. 0 dB
    Param gateRangeDb;  // -60 .. 0 dB
    Param gateAttackMs; // 0.1 .. 100 ms
    Param gateHoldMs;   // 0 .. 1000 ms
    Param gateReleaseMs;// 10 .. 2000 ms

    // 4-Band Parametric EQ
    Param eqBypass;
    // Band 0: Low
    Param eqLowType;    // 0: Bell, 1: LowShelf
    Param eqLowFreq;    // 20 .. 2000 Hz
    Param eqLowGainDb;  // -15 .. +15 dB
    Param eqLowQ;       // 0.3 .. 10.0
    // Band 1: Mid 1
    Param eqMid1Freq;   // 50 .. 5000 Hz
    Param eqMid1GainDb; // -15 .. +15 dB
    Param eqMid1Q;      // 0.3 .. 10.0
    // Band 2: Mid 2
    Param eqMid2Freq;   // 200 .. 10000 Hz
    Param eqMid2GainDb; // -15 .. +15 dB
    Param eqMid2Q;      // 0.3 .. 10.0
    // Band 3: High
    Param eqHighType;   // 0: Bell, 2: HighShelf
    Param eqHighFreq;   // 1000 .. 20000 Hz
    Param eqHighGainDb; // -15 .. +15 dB
    Param eqHighQ;      // 0.3 .. 10.0

    // Dynamics / Compressor
    Param compBypass;
    Param compThreshDb; // -60 .. 0 dB
    Param compRatio;    // 1.0 .. 20.0
    Param compKneeDb;   // 0.0 .. 12.0 dB
    Param compAttackMs; // 0.1 .. 100 ms
    Param compReleaseMs;// 10 .. 1000 ms
    Param compMakeupDb; // 0 .. 24 dB
    Param compMix;      // 0.0 .. 1.0 (dry/wet)

    ChannelDspParams()
    {
        delayMs.init ({ 0.0f, 500.0f, 0.0f });
        delayBypass.init ({ 0.0f, 1.0f, 1.0f }); // 1 = bypassed

        hpfFreq.init ({ 20.0f, 400.0f, 80.0f });
        hpfBypass.init ({ 0.0f, 1.0f, 1.0f });   // default bypassed

        gateBypass.init ({ 0.0f, 1.0f, 1.0f });
        gateThreshDb.init ({ -80.0f, 0.0f, -40.0f });
        gateRangeDb.init ({ -60.0f, 0.0f, -60.0f });
        gateAttackMs.init ({ 0.1f, 100.0f, 1.0f });
        gateHoldMs.init ({ 0.0f, 1000.0f, 20.0f });
        gateReleaseMs.init ({ 10.0f, 2000.0f, 100.0f });

        eqBypass.init ({ 0.0f, 1.0f, 0.0f });   // 0 = Active (enabled by default)
        eqLowType.init ({ 0.0f, 2.0f, 1.0f });   // LowShelf
        eqLowFreq.init ({ 20.0f, 2000.0f, 100.0f });
        eqLowGainDb.init ({ -15.0f, 15.0f, 0.0f });
        eqLowQ.init ({ 0.3f, 10.0f, 0.707f });

        eqMid1Freq.init ({ 50.0f, 5000.0f, 500.0f });
        eqMid1GainDb.init ({ -15.0f, 15.0f, 0.0f });
        eqMid1Q.init ({ 0.3f, 10.0f, 1.0f });

        eqMid2Freq.init ({ 200.0f, 10000.0f, 2500.0f });
        eqMid2GainDb.init ({ -15.0f, 15.0f, 0.0f });
        eqMid2Q.init ({ 0.3f, 10.0f, 1.0f });

        eqHighType.init ({ 0.0f, 2.0f, 2.0f });  // HighShelf
        eqHighFreq.init ({ 1000.0f, 20000.0f, 8000.0f });
        eqHighGainDb.init ({ -15.0f, 15.0f, 0.0f });
        eqHighQ.init ({ 0.3f, 10.0f, 0.707f });

        compBypass.init ({ 0.0f, 1.0f, 1.0f });
        compThreshDb.init ({ -60.0f, 0.0f, -20.0f });
        compRatio.init ({ 1.0f, 20.0f, 3.0f });
        compKneeDb.init ({ 0.0f, 12.0f, 2.0f });
        compAttackMs.init ({ 0.1f, 100.0f, 10.0f });
        compReleaseMs.init ({ 10.0f, 1000.0f, 100.0f });
        compMakeupDb.init ({ 0.0f, 24.0f, 0.0f });
        compMix.init ({ 0.0f, 1.0f, 1.0f });
    }

    void resetToDefaults() noexcept
    {
        delayMs.resetToDefault(); delayBypass.resetToDefault();
        hpfFreq.resetToDefault(); hpfBypass.resetToDefault();
        gateBypass.resetToDefault(); gateThreshDb.resetToDefault();
        gateRangeDb.resetToDefault(); gateAttackMs.resetToDefault();
        gateHoldMs.resetToDefault(); gateReleaseMs.resetToDefault();
        eqBypass.resetToDefault();
        eqLowType.resetToDefault(); eqLowFreq.resetToDefault(); eqLowGainDb.resetToDefault(); eqLowQ.resetToDefault();
        eqMid1Freq.resetToDefault(); eqMid1GainDb.resetToDefault(); eqMid1Q.resetToDefault();
        eqMid2Freq.resetToDefault(); eqMid2GainDb.resetToDefault(); eqMid2Q.resetToDefault();
        eqHighType.resetToDefault(); eqHighFreq.resetToDefault(); eqHighGainDb.resetToDefault(); eqHighQ.resetToDefault();
        compBypass.resetToDefault(); compThreshDb.resetToDefault(); compRatio.resetToDefault();
        compKneeDb.resetToDefault(); compAttackMs.resetToDefault(); compReleaseMs.resetToDefault();
        compMakeupDb.resetToDefault(); compMix.resetToDefault();
    }
};

struct ChannelDspMeters
{
    std::atomic<float> gateGainReductionDb { 0.0f };
    std::atomic<float> compGainReductionDb { 0.0f };
};

class ChannelDspProcessor
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        delayL.prepare (sampleRate);
        delayR.prepare (sampleRate);
        gate.prepare (sampleRate);
        comp.prepare (sampleRate);
        reset();
    }

    void reset()
    {
        delayL.reset(); delayR.reset();
        hpfL.reset(); hpfR.reset();
        gate.reset();
        for (auto& b : eqL) b.reset();
        for (auto& b : eqR) b.reset();
        comp.reset();
    }

    // Process in-place on float buffers chL and chR (or chL only if width == 1)
    void process (float* chL, float* chR, int width, int numSamples,
                  const ChannelDspParams& p, ChannelDspMeters& m,
                  float* const* taps = nullptr) noexcept
    {
        if (numSamples <= 0) return;

        // 1. Input Delay
        const bool delayActive = ! p.delayBypass.getBool();
        const float dMs = p.delayMs.get();
        if (delayActive && dMs > 0.01f)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                chL[i] = delayL.process (chL[i], dMs);
                if (width == 2) chR[i] = delayR.process (chR[i], dMs);
            }
        }

        // 2. HPF (Low Cut)
        const bool hpfActive = ! p.hpfBypass.getBool();
        if (hpfActive)
        {
            const auto coeffs = calcHPF (p.hpfFreq.get(), 0.7071f, sr);
            for (int i = 0; i < numSamples; ++i)
            {
                chL[i] = hpfL.process (chL[i], coeffs);
                if (width == 2) chR[i] = hpfR.process (chR[i], coeffs);
            }
        }

        // Optional preallocated send taps: input/low cut, pre EQ, post EQ.
        if (taps != nullptr)
            for (int i = 0; i < numSamples; ++i)
            {
                taps[0][i] = chL[i];
                taps[1][i] = width == 2 ? chR[i] : chL[i];
            }

        // 3. Gate
        const bool gateActive = ! p.gateBypass.getBool();
        float maxGateGr = 0.0f;
        if (gateActive)
        {
            const float thresh = p.gateThreshDb.get();
            const float range = p.gateRangeDb.get();
            const float att = p.gateAttackMs.get();
            const float hold = p.gateHoldMs.get();
            const float rel = p.gateReleaseMs.get();

            for (int i = 0; i < numSamples; ++i)
            {
                const float det = (width == 2) ? std::max (std::abs (chL[i]), std::abs (chR[i])) : std::abs (chL[i]);
                const float g = gate.processGain (det, true, thresh, range, att, hold, rel);
                chL[i] *= g;
                if (width == 2) chR[i] *= g;
                const float grDb = -gainToDb (g);
                maxGateGr = std::max (maxGateGr, grDb);
            }
        }
        m.gateGainReductionDb.store (maxGateGr, std::memory_order_relaxed);

        // Optional preallocated send taps: input/low cut, pre EQ, post EQ.
        if (taps != nullptr)
            for (int i = 0; i < numSamples; ++i)
            {
                taps[2][i] = chL[i];
                taps[3][i] = width == 2 ? chR[i] : chL[i];
            }

        // 4. 4-Band Parametric EQ
        const bool eqActive = ! p.eqBypass.getBool();
        if (eqActive)
        {
            // Calculate coefficients for 4 bands
            std::array<BiquadCoeffs, 4> coeffs;
            const auto lowType = (EqBandType) p.eqLowType.getInt();
            coeffs[0] = (lowType == EqBandType::LowShelf)
                ? calcLowShelf (p.eqLowFreq.get(), p.eqLowGainDb.get(), p.eqLowQ.get(), sr)
                : calcBell (p.eqLowFreq.get(), p.eqLowGainDb.get(), p.eqLowQ.get(), sr);

            coeffs[1] = calcBell (p.eqMid1Freq.get(), p.eqMid1GainDb.get(), p.eqMid1Q.get(), sr);
            coeffs[2] = calcBell (p.eqMid2Freq.get(), p.eqMid2GainDb.get(), p.eqMid2Q.get(), sr);

            const auto highType = (EqBandType) p.eqHighType.getInt();
            coeffs[3] = (highType == EqBandType::HighShelf)
                ? calcHighShelf (p.eqHighFreq.get(), p.eqHighGainDb.get(), p.eqHighQ.get(), sr)
                : calcBell (p.eqHighFreq.get(), p.eqHighGainDb.get(), p.eqHighQ.get(), sr);

            for (int b = 0; b < 4; ++b)
            {
                for (int i = 0; i < numSamples; ++i)
                {
                    chL[i] = eqL[(size_t) b].process (chL[i], coeffs[(size_t) b]);
                    if (width == 2) chR[i] = eqR[(size_t) b].process (chR[i], coeffs[(size_t) b]);
                }
            }
        }

        // Optional preallocated send taps: input/low cut, pre EQ, post EQ.
        if (taps != nullptr)
            for (int i = 0; i < numSamples; ++i)
            {
                taps[4][i] = chL[i];
                taps[5][i] = width == 2 ? chR[i] : chL[i];
            }

        // 5. Dynamics / Compressor
        const bool compActive = ! p.compBypass.getBool();
        float maxCompGr = 0.0f;
        if (compActive)
        {
            const float thresh = p.compThreshDb.get();
            const float ratio = p.compRatio.get();
            const float knee = p.compKneeDb.get();
            const float att = p.compAttackMs.get();
            const float rel = p.compReleaseMs.get();
            const float makeupGain = dbToGain (p.compMakeupDb.get());
            const float wet = p.compMix.get();
            const float dry = 1.0f - wet;

            for (int i = 0; i < numSamples; ++i)
            {
                const float dryL = chL[i];
                const float dryR = (width == 2) ? chR[i] : dryL;
                const float det = (width == 2) ? std::max (std::abs (dryL), std::abs (dryR)) : std::abs (dryL);

                float grDb = 0.0f;
                const float g = comp.processGain (det, true, thresh, ratio, knee, att, rel, grDb);
                maxCompGr = std::max (maxCompGr, grDb);

                const float wetL = dryL * g * makeupGain;
                chL[i] = dryL * dry + wetL * wet;

                if (width == 2)
                {
                    const float wetR = dryR * g * makeupGain;
                    chR[i] = dryR * dry + wetR * wet;
                }
            }
        }
        m.compGainReductionDb.store (maxCompGr, std::memory_order_relaxed);
    }

private:
    float sr = 48000.0f;
    DelayLine delayL, delayR;
    Biquad hpfL, hpfR;
    GateDsp gate;
    std::array<Biquad, 4> eqL, eqR;
    CompressorDsp comp;
};

} // namespace mps
