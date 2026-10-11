// Mix Practice Studio - Professional Console Effects DSP (41 FX Processors)
#pragma once

#include "DspMath.h"
#include "ChannelDsp.h"
#include <array>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <memory>

namespace mps
{

//==============================================================================
// 41 Comprehensive FX Types matching Professional Console Specifications
enum class FxType : int
{
    // 1. Rack 1-4: Reverb (1..7)
    HallReverb = 0,         // 1. Hall Reverb (Default FX 1)
    PlateReverb,            // 2. Plate Reverb
    VintageReverb,          // 3. Vintage Reverb (EMT250)
    VintageRoom,            // 4. Vintage Room
    Ambience,               // 5. Ambience (Default FX 2)
    GateReverb,             // 6. Gate Reverb
    ReverseReverb,          // 7. Reverse Reverb

    // 2. Rack 1-4: Delay (8..10)
    StereoDelay,            // 8. Stereo Delay (Default FX 3)
    Tap3Delay,              // 9. 3-Tap Delay
    RhythmDelay,            // 10. Rhythm Delay

    // 3. Rack 1-4: Modulation (11..17)
    StereoChorus,           // 11. Stereo Chorus (Default FX 4)
    DimensionalChorus,      // 12. Dimensional Chorus (Roland Dimension D)
    StereoFlanger,          // 13. Stereo Flanger
    StereoPhaser,           // 14. Stereo Phaser
    MoodFilter,             // 15. Mood Filter
    RotarySpeaker,          // 16. Rotary Speaker
    TremoloPanner,          // 17. Tremolo / Panner

    // 4. Rack 1-4: Pitch (18)
    PitchShifter,           // 18. Pitch Shifter

    // 5. Rack 5-8: Graphic EQ (19..22)
    StereoGEQ,              // 19. Stereo GEQ (31-band stereo linked)
    DualGEQ,                // 20. Dual GEQ (31-band dual mono - Default FX 5-8)
    StereoTruEQ,            // 21. Stereo TruEQ
    DualTruEQ,              // 22. Dual TruEQ

    // 6. Premium FX: Dynamics / Compressors (23..30)
    LeisureCompressor,      // 23. Leisure Compressor (Teletronix LA-2A)
    UltimoCompressor,       // 24. Ultimo Compressor (Urei 1176)
    FairCompressor,         // 25. Fair Compressor (Fairchild 670 Stereo)
    MsFairCompressor,       // 26. M/S Fair Compressor (Fairchild 670 Mid/Side)
    Combinator,             // 27. Combinator (5-Band Multiband Compressor)
    PrecisionLimiter,       // 28. Precision Limiter (Brickwall peak limiter)
    WaveDesigner,           // 29. Wave Designer (Transient Designer)
    DualDeEsser,            // 30. Dual De-Esser

    // 7. Premium FX: Tube EQ / Sound Enhancers (31..36)
    XtecEq1,                // 31. XTEC EQ1 (Pultec EQP-1a)
    XtecEq5,                // 32. XTEC EQ5 (Pultec MEQ-5)
    StereoExciter,          // 33. Stereo Exciter (Harmonic Exciter)
    DualExciter,            // 34. Dual Exciter
    StereoEnhancer,         // 35. Stereo Enhancer (Dynamic Spectral Enhancer)
    DualEnhancer,           // 36. Dual Enhancer

    // 8. Premium FX: Utilities & Guitar Amp (37..41)
    EdisonEx1,              // 37. Edison EX1 (Stereo Field Manipulator)
    StereoImager,           // 38. Stereo Imager (Stereo Width & Balance)
    GuitarAmp,              // 39. Guitar Amp (Preamplifier & Cabinet Simulator)
    TubeStage,              // 40. Tube Stage (Analog Triode Saturation)
    SubOctaver,             // 41. Sub Octaver (-1 Octave Sub-bass Generator)

    NumFxTypes = 41
};

enum class FxCategory : int
{
    Reverb = 0,
    Delay,
    Modulation,
    Pitch,
    GraphicEQ,
    PremiumDynamics,
    PremiumTubeEq,
    PremiumUtilityAmp
};

struct FxMetadata
{
    FxType type;
    const char* name;
    const char* shortName;
    const char* categoryName;
    FxCategory category;
    const char* hardwareEmulation;
    bool allowedInRack1To4;
    bool allowedInRack5To8;
};

const FxMetadata& getFxMetadata (FxType type) noexcept;
int getNumFxTypes() noexcept;

// ISO Standard 31 Graphic EQ Center Frequencies (Hz)
constexpr std::array<float, 31> kGeqFrequencies = {
    20.0f, 25.0f, 31.5f, 40.0f, 50.0f, 63.0f, 80.0f, 100.0f,
    125.0f, 160.0f, 200.0f, 250.0f, 315.0f, 400.0f, 500.0f, 630.0f,
    800.0f, 1000.0f, 1250.0f, 1600.0f, 2000.0f, 2500.0f, 3150.0f, 4000.0f,
    5000.0f, 6300.0f, 8000.0f, 10000.0f, 12500.0f, 16000.0f, 20000.0f
};

//==============================================================================
// FX Slot Parameter Set
struct FxSlotParams
{
    Param fxType;       // FxType enum
    Param bypass;       // 0 = active, 1 = bypassed
    Param insertMode;   // 0 = send/return (or normal), 1 = insert
    Param mix;          // 0.0 .. 1.0 (dry/wet)

    // 8 Generic parameters adaptable per effect
    // e.g. Decay/Time, PreDelay/Feedback, Damping/Tone, Size/Depth, Speed/Ratio, etc.
    Param p1, p2, p3, p4, p5, p6, p7, p8;

    // 31-Band Graphic EQ Gains (-15 dB to +15 dB, 0 dB center)
    std::array<Param, 31> geqGainsL;
    std::array<Param, 31> geqGainsR;

    FxSlotParams();
    void setDefaultsForType (FxType type);
};

//==============================================================================
// Algorithmic Stereo Reverb Processor (Schroeder-Moorer / FDN with Modulation & Envelopes)
class ReverbDsp
{
public:
    void prepare (double sampleRate);
    void reset();

    // Process stereo buffer in-place
    void process (float* chL, float* chR, int numSamples,
                  FxType type, float decaySec, float preDelayMs,
                  float dampingNorm, float roomSizeNorm, float mixNorm);

private:
    float sr = 48000.0f;

    // Pre-delay ring buffer
    std::vector<float> preDelayBufL, preDelayBufR;
    int preDelayWritePos = 0;

    // 8 Parallel Comb Filters with prime delay lengths
    static constexpr int kNumCombs = 8;
    std::vector<float> combBufL[kNumCombs];
    std::vector<float> combBufR[kNumCombs];
    int combPosL[kNumCombs] = {0}, combPosR[kNumCombs] = {0};
    float combFilterStateL[kNumCombs] = {0.0f}, combFilterStateR[kNumCombs] = {0.0f};

    // 4 Series All-Pass Diffusers
    static constexpr int kNumAllPasses = 4;
    std::vector<float> allpassBufL[kNumAllPasses], allpassBufR[kNumAllPasses];
    int allpassPosL[kNumAllPasses] = {0}, allpassPosR[kNumAllPasses] = {0};

    // LFO for Vintage EMT250 modulation
    float lfoPhase = 0.0f;

    // Envelope for Gate Reverb and Reverse Reverb
    int gateCounter = 0;
};

//==============================================================================
// Delay Processor (Stereo, 3-Tap, Rhythm)
class DelayDsp
{
public:
    void prepare (double sampleRate);
    void reset();

    void process (float* chL, float* chR, int numSamples,
                  FxType type, float timeLMs, float timeRMs,
                  float feedbackNorm, float crossMix, float highDamp, float mixNorm);

private:
    float sr = 48000.0f;
    std::vector<float> delayBufL, delayBufR;
    int writePos = 0;
    float lpStateL = 0.0f, lpStateR = 0.0f;
};

//==============================================================================
// Modulation Processor (Stereo Chorus, Dimension D, Flanger, Phaser, Mood Filter, Rotary, Tremolo)
class ModulationDsp
{
public:
    void prepare (double sampleRate);
    void reset();

    void process (float* chL, float* chR, int numSamples,
                  FxType type, float speedHz, float depthNorm,
                  float feedbackNorm, float paramCustom, float mixNorm);

private:
    float sr = 48000.0f;
    std::vector<float> modDelayBufL, modDelayBufR;
    int modWritePos = 0;
    float lfoPhase = 0.0f;

    // Phaser 4-stage allpass filters
    float phaserZ1L[4] = {0}, phaserZ1R[4] = {0};

    // Mood Filter biquad
    Biquad moodBiquadL, moodBiquadR;
};

//==============================================================================
// Pitch Shifter Processor (-12 to +12 semitones)
class PitchShifterDsp
{
public:
    void prepare (double sampleRate);
    void reset();

    void process (float* chL, float* chR, int numSamples,
                  float semitones, float mixNorm);

private:
    float sr = 48000.0f;
    std::vector<float> ringBufL, ringBufR;
    int writePos = 0;
    float readPos1 = 0.0f, readPos2 = 0.0f;
    static constexpr int kWindowSize = 2048;
};

//==============================================================================
// 31-Band ISO Graphic Equalizer Processor (Stereo GEQ & Dual GEQ)
class GraphicEqDsp
{
public:
    void prepare (double sampleRate);
    void reset();

    void process (float* chL, float* chR, int numSamples,
                  const std::array<Param, 31>& gainsL,
                  const std::array<Param, 31>& gainsR,
                  bool linkedStereo);

private:
    float sr = 48000.0f;
    std::array<Biquad, 31> bandsL, bandsR;
    std::array<float, 31> lastGainsL {}, lastGainsR {};
    std::array<BiquadCoeffs, 31> coeffsL {}, coeffsR {};
};

//==============================================================================
// Premium Dynamics Processor (LA-2A, 1176, Fairchild 670, Combinator, Limiter, Wave Designer, De-Esser)
class PremiumDynamicsDsp
{
public:
    void prepare (double sampleRate);
    void reset();

    void process (float* chL, float* chR, int numSamples,
                  FxType type, float p1, float p2, float p3, float p4, float mixNorm);

private:
    float sr = 48000.0f;
    // LA-2A dual-stage release envelope
    float la2aEnv = 0.0f, la2aTail = 0.0f;
    // 1176 FET fast envelope
    float fetEnv = 0.0f;
    // Fairchild Vari-Mu envelope
    float fairEnvL = 0.0f, fairEnvR = 0.0f;
    // Limiter lookahead buffer
    std::vector<float> lookaheadBufL, lookaheadBufR;
    int lookaheadWrite = 0;
    float limiterGain = 1.0f;
    // Transient shaper (fast vs slow envelope)
    float fastEnvL = 0.0f, slowEnvL = 0.0f;
    float fastEnvR = 0.0f, slowEnvR = 0.0f;
    // De-Esser bandpass filter
    Biquad sibilanceFiltL, sibilanceFiltR;
    float deEssEnv = 0.0f;
};

//==============================================================================
// Premium Tube EQ & Enhancers Processor (Pultec EQP-1a, MEQ-5, Exciter, Enhancer)
class PremiumTubeEqDsp
{
public:
    void prepare (double sampleRate);
    void reset();

    void process (float* chL, float* chR, int numSamples,
                  FxType type, float p1, float p2, float p3, float p4, float mixNorm);

private:
    float sr = 48000.0f;
    Biquad eqpLowBoostL, eqpLowBoostR;
    Biquad eqpLowAttenL, eqpLowAttenR;
    Biquad eqpHighBoostL, eqpHighBoostR;
    Biquad eqpHighAttenL, eqpHighAttenR;
    Biquad exciterHpfL, exciterHpfR;
};

//==============================================================================
// Premium Utilities & Guitar Amp Processor (Edison EX1, Imager, Guitar Amp, Tube Stage, Sub Octaver)
class PremiumUtilityAmpDsp
{
public:
    void prepare (double sampleRate);
    void reset();

    void process (float* chL, float* chR, int numSamples,
                  FxType type, float p1, float p2, float p3, float p4, float mixNorm);

private:
    float sr = 48000.0f;
    // Guitar Amp tone stack & cab filters
    Biquad cabFilt1L, cabFilt1R;
    Biquad cabFilt2L, cabFilt2R;
    // Sub-octaver zero crossing tracker
    float lastInSignL = 1.0f;
    int subCycleCountL = 0;
    float subSquareL = 1.0f;
    Biquad subLpfL, subLpfR;
};

//==============================================================================
// Unified Single FX Rack Slot Processor
class FxSlotProcessor
{
public:
    void prepare (double sampleRate);
    void reset();

    void process (const float* inL, const float* inR,
                  float* outL, float* outR,
                  int numSamples, FxSlotParams& params);

    // Meter readings
    float getInPeakL()  noexcept { return inPeakL.readAndReset(); }
    float getInPeakR()  noexcept { return inPeakR.readAndReset(); }
    float getOutPeakL() noexcept { return outPeakL.readAndReset(); }
    float getOutPeakR() noexcept { return outPeakR.readAndReset(); }

    AtomicPeak* getInPeakLPtr()  noexcept { return &inPeakL; }
    AtomicPeak* getInPeakRPtr()  noexcept { return &inPeakR; }
    AtomicPeak* getOutPeakLPtr() noexcept { return &outPeakL; }
    AtomicPeak* getOutPeakRPtr() noexcept { return &outPeakR; }

private:
    double sr = 48000.0;
    AtomicPeak inPeakL, inPeakR;
    AtomicPeak outPeakL, outPeakR;

    std::vector<float> workL, workR;

    ReverbDsp reverb;
    DelayDsp delay;
    ModulationDsp modulation;
    PitchShifterDsp pitchShifter;
    GraphicEqDsp graphicEq;
    PremiumDynamicsDsp dynamics;
    PremiumTubeEqDsp tubeEq;
    PremiumUtilityAmpDsp utilityAmp;
};

//==============================================================================
// Complete 8-Slot Rack Manager (FX 1 - FX 8)
class FxRackManager
{
public:
    static constexpr int kNumSlots = 8;

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    FxSlotProcessor& getSlot (int index) noexcept { return slots[(size_t) index]; }

    void processSlot (int slotIndex,
                      const float* inL, const float* inR,
                      float* outL, float* outR,
                      int numSamples, FxSlotParams& params)
    {
        if (slotIndex >= 0 && slotIndex < kNumSlots)
            slots[(size_t) slotIndex].process (inL, inR, outL, outR, numSamples, params);
    }

private:
    std::array<FxSlotProcessor, kNumSlots> slots;
};

} // namespace mps
