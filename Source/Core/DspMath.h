// Mix Practice Studio - shared DSP core (pure C++17, no JUCE dependency)
#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>

namespace mps
{
constexpr float kPi = 3.14159265358979323846f;

/** Anything at or below this is treated as "-inf dB" (gain 0). */
constexpr float kMinusInfDb = -144.0f;

/** Channel/main fader range. The bottom of travel (-90 dB) means -inf (fully off). */
constexpr float kFaderMinDb = -90.0f;
constexpr float kFaderMaxDb = 10.0f;

inline float clampf (float v, float lo, float hi) noexcept { return std::min (hi, std::max (lo, v)); }

inline float dbToGain (float db) noexcept
{
    return db <= kMinusInfDb ? 0.0f : std::pow (10.0f, db * 0.05f);
}

inline float gainToDb (float gain) noexcept
{
    gain = std::abs (gain);
    return gain <= 1.0e-7f ? kMinusInfDb : 20.0f * std::log10 (gain);
}

/** Fader dB -> linear gain. Bottom of the fader travel is a hard -inf. */
inline float faderDbToGain (float db) noexcept
{
    return db <= kFaderMinDb ? 0.0f : dbToGain (std::min (db, kFaderMaxDb));
}

/** Piece-wise linear fader taper, similar in feel to common digital consoles:
      pos 1.0    -> +10 dB      pos 0.75   ->   0 dB      pos 0.5 -> -10 dB
      pos 0.25   -> -30 dB      pos 0.0625 -> -60 dB      pos 0.0 -> -inf (-90)
    This is our own mapping of a familiar control feel; it is not claimed to match
    any specific hardware exactly. */
inline float faderPosToDb (float pos) noexcept
{
    pos = clampf (pos, 0.0f, 1.0f);
    if (pos >= 0.5f)    return pos * 40.0f - 30.0f;
    if (pos >= 0.25f)   return pos * 80.0f - 50.0f;
    if (pos >= 0.0625f) return pos * 160.0f - 70.0f;
    return pos * 480.0f - 90.0f;
}

inline float faderDbToPos (float db) noexcept
{
    if (db <= kFaderMinDb) return 0.0f;
    if (db >= kFaderMaxDb) return 1.0f;
    if (db >= -10.0f)      return (db + 30.0f) / 40.0f;
    if (db >= -30.0f)      return (db + 50.0f) / 80.0f;
    if (db >= -60.0f)      return (db + 70.0f) / 160.0f;
    return (db + 90.0f) / 480.0f;
}

struct PanGains { float left, right; };

/** Mono source -> stereo. Constant-power sin/cos law: -3 dB per side at centre,
    0 dB on the hard-panned side, -inf on the opposite side. pan in [-1, +1]. */
inline PanGains panMonoConstantPower (float pan) noexcept
{
    const float a = (clampf (pan, -1.0f, 1.0f) + 1.0f) * 0.25f * kPi; // 0 .. pi/2
    return { std::cos (a), std::sin (a) };
}

/** Stereo source balance: centre = unity on both sides; turning towards one side
    attenuates the opposite side with a cosine curve down to -inf at the end stop. */
inline PanGains balanceStereo (float pan) noexcept
{
    pan = clampf (pan, -1.0f, 1.0f);
    if (pan > 0.0f) return { std::cos (pan * 0.5f * kPi), 1.0f };
    if (pan < 0.0f) return { 1.0f, std::cos (-pan * 0.5f * kPi) };
    return { 1.0f, 1.0f };
}

/** Linear ramp smoother used for every gain that can change while audio runs
    (trim, polarity, fader, mute, pan, solo routing, source switching). */
class LinearSmoother
{
public:
    void reset (double sampleRate, double rampSeconds) noexcept
    {
        steps = std::max (1, (int) std::lround (sampleRate * rampSeconds));
        remaining = 0;
        current = target;
    }

    void setImmediate (float v) noexcept { current = target = v; remaining = 0; }

    void setTarget (float v) noexcept
    {
        if (v == target) return;
        target = v;
        remaining = steps;
        increment = (target - current) / (float) steps;
    }

    inline float next() noexcept
    {
        if (remaining > 0)
        {
            current += increment;
            if (--remaining == 0) current = target;
        }
        return current;
    }

    bool isSmoothing() const noexcept { return remaining > 0; }
    float getCurrent() const noexcept { return current; }
    float getTarget() const noexcept { return target; }

private:
    float current = 0.0f, target = 0.0f, increment = 0.0f;
    int steps = 1, remaining = 0;
};

struct ParamSpec
{
    float minValue = 0.0f, maxValue = 1.0f, defaultValue = 0.0f;
};

/** A lock-free float parameter. Booleans are stored as 0/1, enums/ints as whole numbers. */
class Param
{
public:
    Param() = default;
    explicit Param (ParamSpec s) : spec (s), value (s.defaultValue) {}

    void init (ParamSpec s) noexcept { spec = s; value.store (s.defaultValue, std::memory_order_relaxed); }

    float get() const noexcept      { return value.load (std::memory_order_relaxed); }
    bool  getBool() const noexcept  { return get() >= 0.5f; }
    int   getInt() const noexcept   { return (int) std::lround (get()); }

    void set (float v) noexcept     { value.store (clampf (v, spec.minValue, spec.maxValue), std::memory_order_relaxed); }
    void setBool (bool b) noexcept  { set (b ? 1.0f : 0.0f); }
    void toggle() noexcept          { setBool (! getBool()); }
    void resetToDefault() noexcept  { set (spec.defaultValue); }

    const ParamSpec& getSpec() const noexcept { return spec; }

private:
    ParamSpec spec;
    std::atomic<float> value { 0.0f };
};

/** Peak value written by the audio thread, read non-destructively by multiple UI meters. */
class AtomicPeak
{
public:
    void update (float p) noexcept
    {
        float old = v.load (std::memory_order_relaxed);
        float next = std::max (p, old * 0.93f);
        while (! v.compare_exchange_weak (old, next, std::memory_order_relaxed))
        {
            next = std::max (p, old * 0.93f);
        }
    }
    float read() const noexcept   { return v.load (std::memory_order_relaxed); }
    float peek() const noexcept   { return v.load (std::memory_order_relaxed); }
    float readAndReset() noexcept { return v.load (std::memory_order_relaxed); }
    void  reset() noexcept        { v.store (0.0f, std::memory_order_relaxed); }

private:
    std::atomic<float> v { 0.0f };
};

} // namespace mps
