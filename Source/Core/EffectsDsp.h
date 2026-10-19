// Mix Practice Studio - X32-Mix FX configuration and DSP
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
// X32-Mix FX catalog in the order used by its effect picker.
enum class FxType : int
{
    HallReverb = 0,
    AmbienceReverb,
    RichPlateReverb,
    RoomReverb,
    ChamberReverb,
    PlateReverb,
    VintageReverb,
    VintageRoomReverb,
    GatedReverb,
    ReverseReverb,
    StereoDelay,
    TapDelay,
    RhythmDelay,
    StereoChorus,
    StereoFlanger,
    StereoPhaser,
    DimensionC,
    MoodFilter,
    RotarySpeakerCabinet,
    TremoloPanner,
    Suboctaver,
    DelayAndChamber,
    ChorusAndChamber,
    FlangerAndChamber,
    DelayAndChorus,
    DelayAndFlanger,
    ModulationDelay,
    DualGEQ,
    StereoGEQ,
    DualTruEQ,
    StereoTruEQ,
    DualDeEsser,
    StereoDeEsser,
    StereoXtecEQ1,
    DualXtecEQ1,
    StereoXtecEQ5,
    DualXtecEQ5,
    WaveDesigner,
    StereoPrecisionLimiter,
    Combinator,
    DualCombinator,
    FairCompressor,
    MsFairCompressor,
    DualFairCompressor,
    StereoLeisureCompressor,
    DualLeisureCompressor,
    UltimoCompressor,
    DualUltimoCompressor,
    DualEnhancer,
    StereoEnhancer,
    DualExciter,
    StereoExciter,
    StereoImager,
    EdisonEX1,
    SoundMaxer,
    DualGuitarAmp,
    StereoGuitarAmp,
    DualTubeStage,
    StereoTubeStage,
    DualPitch,
    StereoPitch,

    NumFxTypes = 61,

    // Legacy aliases retained for source and X32 scene compatibility.
    Ambience = AmbienceReverb,
    VintageRoom = VintageRoomReverb,
    GateReverb = GatedReverb,
    Tap3Delay = TapDelay,
    DimensionalChorus = DimensionC,
    RotarySpeaker = RotarySpeakerCabinet,
    PitchShifter = StereoPitch,
    LeisureCompressor = StereoLeisureCompressor,
    PrecisionLimiter = StereoPrecisionLimiter,
    XtecEq1 = StereoXtecEQ1,
    XtecEq5 = StereoXtecEQ5,
    EdisonEx1 = EdisonEX1,
    GuitarAmp = StereoGuitarAmp,
    TubeStage = StereoTubeStage,
    SubOctaver = Suboctaver
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
    static constexpr int kNumParameters = 18;

    Param fxType;       // FxType enum
    Param bypass;       // 0 = active, 1 = bypassed
    Param insertMode;   // 0 = send/return (or normal), 1 = insert
    Param mix;          // 0.0 .. 1.0 (dry/wet)

    // Dual Independent FX Inserts: [0] = Insert 1 (Left / In A), [1] = Insert 2 (Right / In B)
    // 0 = None, 1..32 = Ch 1..32, 101..116 = Bus 1..16, 201..206 = Mtx 1..6, 301 = Main LR
    std::array<Param, 2> insertTargets;

    // Three pages of six effect-specific parameters, matching X32-Mix.
    // e.g. Decay/Time, PreDelay/Feedback, Damping/Tone, Size/Depth, Speed/Ratio, etc.
    Param p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18;

    Param& parameter (int index) noexcept
    {
        Param* const all[] = { &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9,
                               &p10, &p11, &p12, &p13, &p14, &p15, &p16, &p17, &p18 };
        return *all[std::max (0, std::min (kNumParameters - 1, index))];
    }

    const Param& parameter (int index) const noexcept
    {
        const Param* const all[] = { &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9,
                                     &p10, &p11, &p12, &p13, &p14, &p15, &p16, &p17, &p18 };
        return *all[std::max (0, std::min (kNumParameters - 1, index))];
    }

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
                  float dampingNorm, float roomSizeNorm, float diffusionNorm,
                  float levelDb, float mixNorm);
    // Compatibility overload for callers that only provide the original controls.
    void process (float* chL, float* chR, int numSamples,
                  FxType type, float decaySec, float preDelayMs,
                  float dampingNorm, float roomSizeNorm, float mixNorm)
    {
        process (chL, chR, numSamples, type, decaySec, preDelayMs,
                 dampingNorm, roomSizeNorm, 70.0f, 0.0f, mixNorm);
    }

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

    // EMT 250 vintage 4.5kHz converter lowpass filters
    Biquad emtLpfL, emtLpfR;
    // Plate 100Hz rumble cut filters
    Biquad plateHpfL, plateHpfR;
    // Reverse reverb buffer
    std::vector<float> reverseBufL, reverseBufR;
    int reverseWrite = 0;
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

    // Dimension D dual inverse-phase BBD delay lines & pre/de-emphasis
    std::vector<float> dimBufL, dimBufR;
    int dimWrite = 0;
    Biquad dimPreL, dimPreR;
    Biquad dimPostL, dimPostR;

    // Leslie Rotary dual rotors (horn & drum)
    float rotorPhaseHorn = 0.0f, rotorPhaseDrum = 0.0f;

    // Moog Ladder 4-pole filter stages & envelope follower
    float moogStageL[4] = {0}, moogStageR[4] = {0};
    float moodEnv = 0.0f;

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
    // LA-2A dual-stage release envelope & T4 opto sidechain HF filter
    float la2aEnv = 0.0f, la2aTail = 0.0f;
    Biquad optoHpfL, optoHpfR;

    // 1176 FET fast envelope
    float fetEnv = 0.0f;

    // Fairchild Vari-Mu envelope
    float fairEnvL = 0.0f, fairEnvR = 0.0f;

    // Combinator 5-band crossover filters and envelope followers
    Biquad crossFiltL[4], crossFiltR[4];
    float bandEnvL[5] = {0}, bandEnvR[5] = {0};

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
                  FxType type, float p1, float p2, float p3, float p4, float p5, float p6, float mixNorm);

private:
    float sr = 48000.0f;
    // Pultec EQP-1A filters
    Biquad eqpLowBoostL, eqpLowBoostR;
    Biquad eqpLowAttenL, eqpLowAttenR;
    Biquad eqpHighBoostL, eqpHighBoostR;
    Biquad eqpHighAttenL, eqpHighAttenR;

    // Pultec MEQ-5 Midrange filters
    Biquad meqLowL, meqLowR;
    Biquad meqMidL, meqMidR;
    Biquad meqHighL, meqHighR;

    // Aphex Exciter HPF
    Biquad exciterHpfL, exciterHpfR;

    // BBE Sonic Maximizer 3-band phase alignment delay & crossover
    std::vector<float> bbeLowBufL, bbeLowBufR;
    int bbeWrite = 0;
    Biquad bbeLpfL, bbeLpfR;
    Biquad bbeHpfL, bbeHpfR;
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
    // Guitar Amp 3-band tone stack
    Biquad toneBassL, toneBassR;
    Biquad toneMidL, toneMidR;
    Biquad toneTrebleL, toneTrebleR;
    // Guitar Amp 4x12 Celestion speaker cabinet resonance & roll-off
    Biquad cabResoL, cabResoR;
    Biquad cabScoopL, cabScoopR;
    Biquad cabRollOffL, cabRollOffR;

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
