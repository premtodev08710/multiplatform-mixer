// Mix Practice Studio - Professional Console Effects DSP Implementation
#include "EffectsDsp.h"

namespace mps
{

//==============================================================================
// Metadata Table for all 41 FX
static const FxMetadata kFxMetadataTable[41] = {
    // Rack 1-4 Reverbs (1..7)
    { FxType::HallReverb,        "Hall Reverb",        "HALL",   "Reverb",     FxCategory::Reverb, "Concert Hall with smooth early reflections and rich decay", true, false },
    { FxType::PlateReverb,       "Plate Reverb",       "PLATE",  "Reverb",     FxCategory::Reverb, "Classic mechanical steel plate reverberation", true, false },
    { FxType::VintageReverb,     "Vintage Reverb",     "VNTG",   "Reverb",     FxCategory::Reverb, "EMT250 first electronic digital reverberator emulation", true, false },
    { FxType::VintageRoom,       "Vintage Room",       "ROOM",   "Reverb",     FxCategory::Reverb, "Acoustic studio live room with natural decay", true, false },
    { FxType::Ambience,          "Ambience",           "AMBI",   "Reverb",     FxCategory::Reverb, "Transparent short space tail for drums & instruments", true, false },
    { FxType::GateReverb,        "Gate Reverb",        "GATE",   "Reverb",     FxCategory::Reverb, "1980s punchy gated snare drum reverberation", true, false },
    { FxType::ReverseReverb,     "Reverse Reverb",     "REVRS",  "Reverb",     FxCategory::Reverb, "Swell build-up backwards reverberation", true, false },

    // Rack 1-4 Delays (8..10)
    { FxType::StereoDelay,       "Stereo Delay",       "DLY",    "Delay",      FxCategory::Delay, "Dual channel tempo synced stereo echo with ping-pong", true, false },
    { FxType::Tap3Delay,         "3-Tap Delay",        "3TAP",   "Delay",      FxCategory::Delay, "3 independent spatial delay taps (L/C/R)", true, false },
    { FxType::RhythmDelay,       "Rhythm Delay",       "RHYTHM", "Delay",      FxCategory::Delay, "Musical rhythmic multi-tap pattern echo", true, false },

    // Rack 1-4 Modulation (11..17)
    { FxType::StereoChorus,      "Stereo Chorus",      "CHORUS", "Modulation", FxCategory::Modulation, "Wide dual-LFO multi-voice pitch detuning chorus", true, false },
    { FxType::DimensionalChorus, "Dimensional Chorus", "DIM-D",  "Modulation", FxCategory::Modulation, "Roland Dimension D analog spatial enhancer", true, false },
    { FxType::StereoFlanger,     "Stereo Flanger",     "FLANGE", "Modulation", FxCategory::Modulation, "Regenerative comb-filter jet flanging effect", true, false },
    { FxType::StereoPhaser,      "Stereo Phaser",      "PHASER", "Modulation", FxCategory::Modulation, "Multi-stage allpass phase cancellation sweep", true, false },
    { FxType::MoodFilter,        "Mood Filter",        "FILTER", "Modulation", FxCategory::Modulation, "Dynamic envelope follower & resonant LFO filter", true, false },
    { FxType::RotarySpeaker,     "Rotary Speaker",     "ROTARY", "Modulation", FxCategory::Modulation, "Leslie dual spinning horn and drum simulator", true, false },
    { FxType::TremoloPanner,     "Tremolo / Panner",   "TREM",   "Modulation", FxCategory::Modulation, "Amplitude tremolo and synchronized auto-panner", true, false },

    // Rack 1-4 Pitch (18)
    { FxType::PitchShifter,      "Pitch Shifter",      "PITCH",  "Pitch",      FxCategory::Pitch, "Granular real-time pitch transposition (-12 to +12 semitones)", true, false },

    // Rack 5-8 Graphic EQ (19..22)
    { FxType::StereoGEQ,         "Stereo GEQ",         "ST GEQ", "Graphic EQ", FxCategory::GraphicEQ, "31-band ISO stereo graphic equalizer", false, true },
    { FxType::DualGEQ,           "Dual GEQ",           "DUAL GEQ","Graphic EQ",FxCategory::GraphicEQ, "31-band ISO dual independent mono graphic equalizer", false, true },
    { FxType::StereoTruEQ,       "Stereo TruEQ",       "ST TRUEQ","Graphic EQ",FxCategory::GraphicEQ, "Constant-Q true frequency response graphic equalizer", false, true },
    { FxType::DualTruEQ,         "Dual TruEQ",         "DL TRUEQ","Graphic EQ",FxCategory::GraphicEQ, "Dual constant-Q true response graphic equalizer", false, true },

    // Premium FX: Dynamics (23..30)
    { FxType::LeisureCompressor, "Leisure Compressor", "LA-2A",  "Dynamics",   FxCategory::PremiumDynamics, "Teletronix LA-2A Optical tube leveling amplifier", true, true },
    { FxType::UltimoCompressor,  "Ultimo Compressor",  "1176",   "Dynamics",   FxCategory::PremiumDynamics, "Urei 1176 LN fast FET peak limiter & compressor", true, true },
    { FxType::FairCompressor,    "Fair Compressor",    "FAIR-670","Dynamics",  FxCategory::PremiumDynamics, "Fairchild 670 stereo Vari-Mu tube compressor", true, true },
    { FxType::MsFairCompressor,  "M/S Fair Compressor","MS-FAIR","Dynamics",   FxCategory::PremiumDynamics, "Fairchild 670 Mid/Side independent tube compressor", true, true },
    { FxType::Combinator,        "Combinator",         "COMBIN", "Dynamics",   FxCategory::PremiumDynamics, "5-band crossover broadcast mastering multiband compressor", true, true },
    { FxType::PrecisionLimiter,  "Precision Limiter",  "LIMIT",  "Dynamics",   FxCategory::PremiumDynamics, "Transparent brickwall lookahead peak mastering limiter", true, true },
    { FxType::WaveDesigner,      "Wave Designer",      "WAVE",   "Dynamics",   FxCategory::PremiumDynamics, "SPL Transient Designer attack/sustain shaper", true, true },
    { FxType::DualDeEsser,       "Dual De-Esser",      "DE-ESS", "Dynamics",   FxCategory::PremiumDynamics, "Dual channel dynamic vocal sibilance suppressor", true, true },

    // Premium FX: Tube EQ / Enhancers (31..36)
    { FxType::XtecEq1,           "XTEC EQ1",           "EQP-1A", "Tube EQ",    FxCategory::PremiumTubeEq, "Pultec EQP-1A passive tube program equalizer", true, true },
    { FxType::XtecEq5,           "XTEC EQ5",           "MEQ-5",  "Tube EQ",    FxCategory::PremiumTubeEq, "Pultec MEQ-5 passive tube midrange equalizer", true, true },
    { FxType::StereoExciter,     "Stereo Exciter",     "EXCITER","Enhancer",   FxCategory::PremiumTubeEq, "Aphex Aural Exciter harmonic sheen generator", true, true },
    { FxType::DualExciter,       "Dual Exciter",       "DL EXCIT","Enhancer",  FxCategory::PremiumTubeEq, "Dual channel independent harmonic sheen generator", true, true },
    { FxType::StereoEnhancer,    "Stereo Enhancer",    "ENHANCE","Enhancer",   FxCategory::PremiumTubeEq, "BBE Sonic Maximizer dynamic harmonic contourer", true, true },
    { FxType::DualEnhancer,      "Dual Enhancer",      "DL ENH", "Enhancer",   FxCategory::PremiumTubeEq, "Dual channel independent dynamic harmonic contourer", true, true },

    // Premium FX: Utilities & Guitar Amp (37..41)
    { FxType::EdisonEx1,         "Edison EX1",         "EDISON", "Utility",    FxCategory::PremiumUtilityAmp, "Stereo field spatial manipulator & mono-bass maker", true, true },
    { FxType::StereoImager,      "Stereo Imager",      "IMAGER", "Utility",    FxCategory::PremiumUtilityAmp, "Stereo width expansion & acoustic balance", true, true },
    { FxType::GuitarAmp,         "Guitar Amp",         "AMP",    "Guitar Amp", FxCategory::PremiumUtilityAmp, "Vintage tube preamplifier and 4x12 cabinet simulator", true, true },
    { FxType::TubeStage,         "Tube Stage",         "TUBE",   "Utility",    FxCategory::PremiumUtilityAmp, "Analog triode valve saturation & harmonic warmth", true, true },
    { FxType::SubOctaver,        "Sub Octaver",        "OCTAVE", "Utility",    FxCategory::PremiumUtilityAmp, "Analog -1 octave sub-bass synthesizer generator", true, true }
};

const FxMetadata& getFxMetadata (FxType type) noexcept
{
    const int idx = (int) type;
    if (idx >= 0 && idx < 41) return kFxMetadataTable[idx];
    return kFxMetadataTable[0];
}

int getNumFxTypes() noexcept { return 41; }

//==============================================================================
// FxSlotParams
FxSlotParams::FxSlotParams()
{
    fxType.init     ({ 0.0f, 40.0f, 0.0f });
    bypass.init     ({ 0.0f, 1.0f, 0.0f }); // 0 = active
    insertMode.init ({ 0.0f, 1.0f, 0.0f }); // 0 = Send/Return, 1 = Insert
    mix.init        ({ 0.0f, 1.0f, 1.0f }); // 1.0 (100% wet for send/return)

    p1.init ({ 0.0f, 100.0f, 50.0f });
    p2.init ({ 0.0f, 100.0f, 50.0f });
    p3.init ({ 0.0f, 100.0f, 50.0f });
    p4.init ({ 0.0f, 100.0f, 50.0f });
    p5.init ({ 0.0f, 100.0f, 50.0f });
    p6.init ({ 0.0f, 100.0f, 50.0f });
    p7.init ({ 0.0f, 100.0f, 50.0f });
    p8.init ({ 0.0f, 100.0f, 50.0f });

    for (auto& g : geqGainsL) g.init ({ -15.0f, 15.0f, 0.0f });
    for (auto& g : geqGainsR) g.init ({ -15.0f, 15.0f, 0.0f });
}

void FxSlotParams::setDefaultsForType (FxType type)
{
    fxType.set ((float) (int) type);
    switch (type)
    {
        case FxType::HallReverb:
            mix.set (1.0f); p1.set (2.5f);  p2.set (20.0f); p3.set (40.0f); p4.set (70.0f); break; // Decay 2.5s, Pre 20ms, Damp 40%, Size 70%
        case FxType::PlateReverb:
            mix.set (1.0f); p1.set (1.8f);  p2.set (10.0f); p3.set (20.0f); p4.set (80.0f); break;
        case FxType::VintageReverb:
            mix.set (1.0f); p1.set (3.0f);  p2.set (30.0f); p3.set (50.0f); p4.set (65.0f); break;
        case FxType::VintageRoom:
            mix.set (1.0f); p1.set (1.2f);  p2.set (5.0f);  p3.set (30.0f); p4.set (50.0f); break;
        case FxType::Ambience:
            mix.set (1.0f); p1.set (0.6f);  p2.set (0.0f);  p3.set (25.0f); p4.set (40.0f); break;
        case FxType::GateReverb:
            mix.set (1.0f); p1.set (1.5f);  p2.set (120.0f);p3.set (20.0f); p4.set (50.0f); break; // Decay 1.5s, GateHold 120ms
        case FxType::ReverseReverb:
            mix.set (1.0f); p1.set (2.0f);  p2.set (250.0f);p3.set (30.0f); p4.set (60.0f); break;
        case FxType::StereoDelay:
            mix.set (1.0f); p1.set (350.0f);p2.set (500.0f);p3.set (35.0f); p4.set (0.0f);  break; // TimeL 350ms, TimeR 500ms, Fdbk 35%, PingPong
        case FxType::Tap3Delay:
            mix.set (1.0f); p1.set (250.0f);p2.set (500.0f);p3.set (750.0f);p4.set (30.0f); break;
        case FxType::StereoChorus:
            mix.set (0.5f); p1.set (1.2f);  p2.set (60.0f); p3.set (20.0f); p4.set (10.0f); break; // Speed 1.2Hz, Depth 60%
        case FxType::DimensionalChorus:
            mix.set (0.5f); p1.set (2.0f);  p2.set (1.0f);  break; // Mode 2
        case FxType::StereoFlanger:
            mix.set (0.5f); p1.set (0.4f);  p2.set (70.0f); p3.set (60.0f); break;
        case FxType::StereoPhaser:
            mix.set (0.5f); p1.set (0.8f);  p2.set (75.0f); p3.set (50.0f); break;
        case FxType::PitchShifter:
            mix.set (0.5f); p1.set (0.0f);  break; // 0 semitones
        case FxType::StereoGEQ:
        case FxType::DualGEQ:
        case FxType::StereoTruEQ:
        case FxType::DualTruEQ:
            mix.set (1.0f);
            for (auto& g : geqGainsL) g.set (0.0f);
            for (auto& g : geqGainsR) g.set (0.0f);
            break;
        case FxType::LeisureCompressor: // LA-2A
            mix.set (1.0f); p1.set (40.0f); p2.set (40.0f); break; // Peak Reduction 40, Gain 40
        case FxType::UltimoCompressor:  // 1176
            mix.set (1.0f); p1.set (30.0f); p2.set (20.0f); p3.set (4.0f); p4.set (20.0f); break; // Input, Output, Ratio 4:1, Attack
        case FxType::GuitarAmp:
            mix.set (1.0f); p1.set (50.0f); p2.set (50.0f); p3.set (50.0f); p4.set (50.0f); break; // Drive, Bass, Mid, Treble
        case FxType::TubeStage:
            mix.set (1.0f); p1.set (35.0f); p2.set (0.0f); break; // Drive, Output
        case FxType::SubOctaver:
            mix.set (0.4f); p1.set (80.0f); break; // Lowpass 80Hz
        default:
            mix.set (0.5f); p1.set (50.0f); p2.set (50.0f); break;
    }
}

//==============================================================================
// ReverbDsp Implementation
void ReverbDsp::prepare (double sampleRate)
{
    sr = (float) sampleRate;
    const int preDelayLen = (int) (sr * 0.3f); // 300 ms predelay max
    preDelayBufL.assign ((size_t) preDelayLen, 0.0f);
    preDelayBufR.assign ((size_t) preDelayLen, 0.0f);

    // Prime number delay lengths tuned for 48kHz
    const int combLengthsL[kNumCombs] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
    const int combLengthsR[kNumCombs] = { 1139, 1211, 1293, 1373, 1443, 1507, 1579, 1639 };
    const float scale = (float) (sampleRate / 48000.0);

    for (int i = 0; i < kNumCombs; ++i)
    {
        const int lenL = std::max (16, (int) std::lround ((float) combLengthsL[i] * scale));
        const int lenR = std::max (16, (int) std::lround ((float) combLengthsR[i] * scale));
        combBufL[i].assign ((size_t) lenL, 0.0f);
        combBufR[i].assign ((size_t) lenR, 0.0f);
    }

    const int allpassLengths[kNumAllPasses] = { 225, 341, 441, 556 };
    for (int i = 0; i < kNumAllPasses; ++i)
    {
        const int len = std::max (8, (int) std::lround ((float) allpassLengths[i] * scale));
        allpassBufL[i].assign ((size_t) len, 0.0f);
        allpassBufR[i].assign ((size_t) len, 0.0f);
    }

    reset();
}

void ReverbDsp::reset()
{
    std::fill (preDelayBufL.begin(), preDelayBufL.end(), 0.0f);
    std::fill (preDelayBufR.begin(), preDelayBufR.end(), 0.0f);
    preDelayWritePos = 0;

    for (int i = 0; i < kNumCombs; ++i)
    {
        std::fill (combBufL[i].begin(), combBufL[i].end(), 0.0f);
        std::fill (combBufR[i].begin(), combBufR[i].end(), 0.0f);
        combPosL[i] = combPosR[i] = 0;
        combFilterStateL[i] = combFilterStateR[i] = 0.0f;
    }

    for (int i = 0; i < kNumAllPasses; ++i)
    {
        std::fill (allpassBufL[i].begin(), allpassBufL[i].end(), 0.0f);
        std::fill (allpassBufR[i].begin(), allpassBufR[i].end(), 0.0f);
        allpassPosL[i] = allpassPosR[i] = 0;
    }

    lfoPhase = 0.0f;
    gateCounter = 0;
}

void ReverbDsp::process (float* chL, float* chR, int numSamples,
                         FxType type, float decaySec, float preDelayMs,
                         float dampingNorm, float roomSizeNorm, float mixNorm)
{
    const int preDelaySamples = clampf ((preDelayMs * 0.001f * sr), 0.0f, (float) preDelayBufL.size() - 1);
    const float feedback = clampf (std::exp (-3.0f / (std::max (0.1f, decaySec) * (sr / 1400.0f))), 0.2f, 0.98f);
    const float dampCoeff = clampf (dampingNorm * 0.01f, 0.05f, 0.9f);
    const float wetGain = mixNorm;
    const float dryGain = 1.0f - wetGain;

    const bool isGate = (type == FxType::GateReverb);
    const int gateHoldSamples = (int) (std::max (0.02f, decaySec * 0.1f) * sr);

    for (int i = 0; i < numSamples; ++i)
    {
        const float inL = chL[i];
        const float inR = chR[i];

        // 1. Pre-delay
        preDelayBufL[(size_t) preDelayWritePos] = inL;
        preDelayBufR[(size_t) preDelayWritePos] = inR;

        int readIdx = preDelayWritePos - preDelaySamples;
        if (readIdx < 0) readIdx += (int) preDelayBufL.size();

        const float pdInL = preDelayBufL[(size_t) readIdx];
        const float pdInR = preDelayBufR[(size_t) readIdx];

        if (++preDelayWritePos >= (int) preDelayBufL.size())
            preDelayWritePos = 0;

        // 2. Parallel Comb Filters
        float sumCombL = 0.0f;
        float sumCombR = 0.0f;

        for (int c = 0; c < kNumCombs; ++c)
        {
            // Left Comb
            auto& bufL = combBufL[c];
            const float outCombL = bufL[(size_t) combPosL[c]];
            combFilterStateL[c] = outCombL * (1.0f - dampCoeff) + combFilterStateL[c] * dampCoeff;
            bufL[(size_t) combPosL[c]] = pdInL + combFilterStateL[c] * feedback;
            if (++combPosL[c] >= (int) bufL.size()) combPosL[c] = 0;
            sumCombL += outCombL;

            // Right Comb
            auto& bufR = combBufR[c];
            const float outCombR = bufR[(size_t) combPosR[c]];
            combFilterStateR[c] = outCombR * (1.0f - dampCoeff) + combFilterStateR[c] * dampCoeff;
            bufR[(size_t) combPosR[c]] = pdInR + combFilterStateR[c] * feedback;
            if (++combPosR[c] >= (int) bufR.size()) combPosR[c] = 0;
            sumCombR += outCombR;
        }

        sumCombL *= 0.125f;
        sumCombR *= 0.125f;

        // 3. Series All-Pass Diffusers
        float apL = sumCombL;
        float apR = sumCombR;

        for (int a = 0; a < kNumAllPasses; ++a)
        {
            auto& bufL = allpassBufL[a];
            const float bufOutL = bufL[(size_t) allpassPosL[a]];
            const float newL = apL + bufOutL * 0.5f;
            bufL[(size_t) allpassPosL[a]] = apL - newL * 0.5f;
            apL = bufOutL;
            if (++allpassPosL[a] >= (int) bufL.size()) allpassPosL[a] = 0;

            auto& bufR = allpassBufR[a];
            const float bufOutR = bufR[(size_t) allpassPosR[a]];
            const float newR = apR + bufOutR * 0.5f;
            bufR[(size_t) allpassPosR[a]] = apR - newR * 0.5f;
            apR = bufOutR;
            if (++allpassPosR[a] >= (int) bufR.size()) allpassPosR[a] = 0;
        }

        // 4. Gate / Envelope check
        if (isGate)
        {
            const float level = std::max (std::abs (inL), std::abs (inR));
            if (level > 0.05f) gateCounter = gateHoldSamples;
            if (gateCounter > 0) --gateCounter;
            else { apL = 0.0f; apR = 0.0f; }
        }

        // 5. Output mix
        chL[i] = inL * dryGain + apL * wetGain;
        chR[i] = inR * dryGain + apR * wetGain;
    }
}

//==============================================================================
// DelayDsp Implementation
void DelayDsp::prepare (double sampleRate)
{
    sr = (float) sampleRate;
    const int maxLen = (int) (sr * 2.5f); // 2.5 seconds max
    delayBufL.assign ((size_t) maxLen, 0.0f);
    delayBufR.assign ((size_t) maxLen, 0.0f);
    reset();
}

void DelayDsp::reset()
{
    std::fill (delayBufL.begin(), delayBufL.end(), 0.0f);
    std::fill (delayBufR.begin(), delayBufR.end(), 0.0f);
    writePos = 0;
    lpStateL = lpStateR = 0.0f;
}

void DelayDsp::process (float* chL, float* chR, int numSamples,
                        FxType type, float timeLMs, float timeRMs,
                        float feedbackNorm, float crossMix, float highDamp, float mixNorm)
{
    const int maxLen = (int) delayBufL.size();
    const int dSamplesL = clampf (timeLMs * 0.001f * sr, 1.0f, (float) (maxLen - 1));
    const int dSamplesR = clampf (timeRMs * 0.001f * sr, 1.0f, (float) (maxLen - 1));
    const float fdbk = clampf (feedbackNorm * 0.01f, 0.0f, 0.95f);
    const float damp = clampf (highDamp * 0.01f, 0.0f, 0.8f);
    const float wet = mixNorm;
    const float dry = 1.0f - wet;

    for (int i = 0; i < numSamples; ++i)
    {
        const float inL = chL[i];
        const float inR = chR[i];

        int rIdxL = writePos - dSamplesL;
        if (rIdxL < 0) rIdxL += maxLen;
        int rIdxR = writePos - dSamplesR;
        if (rIdxR < 0) rIdxR += maxLen;

        const float delayedL = delayBufL[(size_t) rIdxL];
        const float delayedR = delayBufR[(size_t) rIdxR];

        // Damp filter
        lpStateL = delayedL * (1.0f - damp) + lpStateL * damp;
        lpStateR = delayedR * (1.0f - damp) + lpStateR * damp;

        // Feedback into buffer
        if (type == FxType::Tap3Delay)
        {
            // 3-Tap delay: Tap 1 left, Tap 2 center, Tap 3 right
            int tap2Idx = writePos - (dSamplesL + dSamplesR) / 2;
            if (tap2Idx < 0) tap2Idx += maxLen;
            const float tap2 = delayBufL[(size_t) tap2Idx] * 0.5f;

            delayBufL[(size_t) writePos] = inL + lpStateL * fdbk;
            delayBufR[(size_t) writePos] = inR + lpStateR * fdbk;

            chL[i] = inL * dry + (delayedL + tap2) * wet;
            chR[i] = inR * dry + (delayedR + tap2) * wet;
        }
        else
        {
            // Ping-pong or standard stereo
            const float fbL = crossMix > 0.5f ? lpStateR : lpStateL;
            const float fbR = crossMix > 0.5f ? lpStateL : lpStateR;

            delayBufL[(size_t) writePos] = inL + fbL * fdbk;
            delayBufR[(size_t) writePos] = inR + fbR * fdbk;

            chL[i] = inL * dry + delayedL * wet;
            chR[i] = inR * dry + delayedR * wet;
        }

        if (++writePos >= maxLen) writePos = 0;
    }
}

//==============================================================================
// ModulationDsp Implementation
void ModulationDsp::prepare (double sampleRate)
{
    sr = (float) sampleRate;
    const int maxLen = (int) (sr * 0.05f); // 50 ms max modulation delay
    modDelayBufL.assign ((size_t) maxLen, 0.0f);
    modDelayBufR.assign ((size_t) maxLen, 0.0f);
    reset();
}

void ModulationDsp::reset()
{
    std::fill (modDelayBufL.begin(), modDelayBufL.end(), 0.0f);
    std::fill (modDelayBufR.begin(), modDelayBufR.end(), 0.0f);
    modWritePos = 0;
    lfoPhase = 0.0f;
    std::fill (std::begin (phaserZ1L), std::end (phaserZ1L), 0.0f);
    std::fill (std::begin (phaserZ1R), std::end (phaserZ1R), 0.0f);
    moodBiquadL.reset();
    moodBiquadR.reset();
}

void ModulationDsp::process (float* chL, float* chR, int numSamples,
                             FxType type, float speedHz, float depthNorm,
                             float feedbackNorm, float paramCustom, float mixNorm)
{
    const int maxLen = (int) modDelayBufL.size();
    const float lfoInc = (speedHz * 2.0f * kPi) / sr;
    const float wet = mixNorm;
    const float dry = 1.0f - wet;

    for (int i = 0; i < numSamples; ++i)
    {
        const float inL = chL[i];
        const float inR = chR[i];

        lfoPhase += lfoInc;
        if (lfoPhase >= 2.0f * kPi) lfoPhase -= 2.0f * kPi;

        const float lfoL = std::sin (lfoPhase);
        const float lfoR = std::sin (lfoPhase + kPi * 0.5f); // 90 degree stereo phase

        if (type == FxType::TremoloPanner)
        {
            // Tremolo / Auto-Panner
            const float modDepth = depthNorm * 0.01f;
            const float gainL = 1.0f - modDepth * (0.5f + 0.5f * lfoL);
            const float gainR = 1.0f - modDepth * (0.5f + 0.5f * (paramCustom > 0.5f ? -lfoL : lfoR));
            chL[i] = inL * (dry + gainL * wet);
            chR[i] = inR * (dry + gainR * wet);
        }
        else if (type == FxType::StereoPhaser)
        {
            // 4-stage allpass phaser
            const float apCoeff = 0.5f + 0.4f * lfoL;
            float sL = inL;
            float sR = inR;
            for (int stage = 0; stage < 4; ++stage)
            {
                const float yL = apCoeff * sL + phaserZ1L[stage];
                phaserZ1L[stage] = sL - apCoeff * yL;
                sL = yL;

                const float yR = apCoeff * sR + phaserZ1R[stage];
                phaserZ1R[stage] = sR - apCoeff * yR;
                sR = yR;
            }
            chL[i] = inL * dry + (inL + sL * 0.5f) * wet;
            chR[i] = inR * dry + (inR + sR * 0.5f) * wet;
        }
        else
        {
            // Chorus / Flanger / Dimension D / Rotary
            const float baseDelayMs = (type == FxType::StereoFlanger) ? 1.5f : 15.0f;
            const float modRangeMs = (type == FxType::StereoFlanger) ? 1.0f : 8.0f * (depthNorm * 0.01f);
            const float fdbk = (type == FxType::StereoFlanger) ? clampf (feedbackNorm * 0.01f, -0.9f, 0.9f) : 0.0f;

            const float dL = (baseDelayMs + modRangeMs * lfoL) * 0.001f * sr;
            const float dR = (baseDelayMs + modRangeMs * lfoR) * 0.001f * sr;

            int idxL = modWritePos - (int) dL;
            if (idxL < 0) idxL += maxLen;
            int idxR = modWritePos - (int) dR;
            if (idxR < 0) idxR += maxLen;

            const float outModL = modDelayBufL[(size_t) idxL];
            const float outModR = modDelayBufR[(size_t) idxR];

            modDelayBufL[(size_t) modWritePos] = inL + outModL * fdbk;
            modDelayBufR[(size_t) modWritePos] = inR + outModR * fdbk;

            if (++modWritePos >= maxLen) modWritePos = 0;

            chL[i] = inL * dry + outModL * wet;
            chR[i] = inR * dry + outModR * wet;
        }
    }
}

//==============================================================================
// PitchShifterDsp Implementation
void PitchShifterDsp::prepare (double sampleRate)
{
    sr = (float) sampleRate;
    ringBufL.assign (kWindowSize * 2, 0.0f);
    ringBufR.assign (kWindowSize * 2, 0.0f);
    reset();
}

void PitchShifterDsp::reset()
{
    std::fill (ringBufL.begin(), ringBufL.end(), 0.0f);
    std::fill (ringBufR.begin(), ringBufR.end(), 0.0f);
    writePos = 0;
    readPos1 = 0.0f;
    readPos2 = (float) (kWindowSize / 2);
}

void PitchShifterDsp::process (float* chL, float* chR, int numSamples,
                               float semitones, float mixNorm)
{
    const float pitchRatio = std::pow (2.0f, semitones / 12.0f);
    const float rate = 1.0f - pitchRatio;
    const int bufSize = (int) ringBufL.size();
    const float wet = mixNorm;
    const float dry = 1.0f - wet;

    for (int i = 0; i < numSamples; ++i)
    {
        const float inL = chL[i];
        const float inR = chR[i];

        ringBufL[(size_t) writePos] = inL;
        ringBufR[(size_t) writePos] = inR;

        // Two cross-fading grain taps
        readPos1 += rate;
        if (readPos1 >= (float) kWindowSize) readPos1 -= (float) kWindowSize;
        if (readPos1 < 0.0f) readPos1 += (float) kWindowSize;

        readPos2 += rate;
        if (readPos2 >= (float) kWindowSize) readPos2 -= (float) kWindowSize;
        if (readPos2 < 0.0f) readPos2 += (float) kWindowSize;

        const float win1 = 0.5f * (1.0f - std::cos (2.0f * kPi * readPos1 / (float) kWindowSize));
        const float win2 = 0.5f * (1.0f - std::cos (2.0f * kPi * readPos2 / (float) kWindowSize));

        int tap1 = writePos - (int) readPos1;
        if (tap1 < 0) tap1 += bufSize;
        int tap2 = writePos - (int) readPos2;
        if (tap2 < 0) tap2 += bufSize;

        const float outShiftL = ringBufL[(size_t) tap1] * win1 + ringBufL[(size_t) tap2] * win2;
        const float outShiftR = ringBufR[(size_t) tap1] * win1 + ringBufR[(size_t) tap2] * win2;

        if (++writePos >= bufSize) writePos = 0;

        chL[i] = inL * dry + outShiftL * wet;
        chR[i] = inR * dry + outShiftR * wet;
    }
}

//==============================================================================
// GraphicEqDsp Implementation (31 ISO Bands)
void GraphicEqDsp::prepare (double sampleRate)
{
    sr = (float) sampleRate;
    reset();
}

void GraphicEqDsp::reset()
{
    for (auto& b : bandsL) b.reset();
    for (auto& b : bandsR) b.reset();
    lastGainsL.fill (-999.0f);
    lastGainsR.fill (-999.0f);
}

void GraphicEqDsp::process (float* chL, float* chR, int numSamples,
                            const std::array<Param, 31>& gainsL,
                            const std::array<Param, 31>& gainsR,
                            bool linkedStereo)
{
    // Update any changed band filter coefficients
    for (size_t b = 0; b < 31; ++b)
    {
        const float gL = gainsL[b].get();
        if (std::abs (gL - lastGainsL[b]) > 0.05f)
        {
            lastGainsL[b] = gL;
            coeffsL[b] = calcBell (kGeqFrequencies[b], gL, 4.3f, sr); // Q=4.3 for 1/3-octave
        }

        const float gR = linkedStereo ? gL : gainsR[b].get();
        if (std::abs (gR - lastGainsR[b]) > 0.05f)
        {
            lastGainsR[b] = gR;
            coeffsR[b] = calcBell (kGeqFrequencies[b], gR, 4.3f, sr);
        }
    }

    // Process all 31 bands in series
    for (size_t b = 0; b < 31; ++b)
    {
        if (std::abs (lastGainsL[b]) > 0.01f)
        {
            for (int i = 0; i < numSamples; ++i)
                chL[i] = bandsL[b].process (chL[i], coeffsL[b]);
        }

        if (std::abs (lastGainsR[b]) > 0.01f)
        {
            for (int i = 0; i < numSamples; ++i)
                chR[i] = bandsR[b].process (chR[i], coeffsR[b]);
        }
    }
}

//==============================================================================
// PremiumDynamicsDsp Implementation (LA-2A, 1176, Fairchild, Limiter, Wave Designer)
void PremiumDynamicsDsp::prepare (double sampleRate)
{
    sr = (float) sampleRate;
    lookaheadBufL.assign ((size_t) (sr * 0.01f), 0.0f); // 10ms lookahead
    lookaheadBufR.assign ((size_t) (sr * 0.01f), 0.0f);
    reset();
}

void PremiumDynamicsDsp::reset()
{
    la2aEnv = la2aTail = fetEnv = fairEnvL = fairEnvR = 0.0f;
    limiterGain = 1.0f;
    fastEnvL = slowEnvL = fastEnvR = slowEnvR = 0.0f;
    lookaheadWrite = 0;
    std::fill (lookaheadBufL.begin(), lookaheadBufL.end(), 0.0f);
    std::fill (lookaheadBufR.begin(), lookaheadBufR.end(), 0.0f);
}

void PremiumDynamicsDsp::process (float* chL, float* chR, int numSamples,
                                  FxType type, float p1, float p2, float p3, float p4, float mixNorm)
{
    const float wet = mixNorm;
    const float dry = 1.0f - wet;

    if (type == FxType::LeisureCompressor) // LA-2A
    {
        // p1 = Peak Reduction (0..100), p2 = Output Gain (0..100)
        const float threshold = -40.0f + (100.0f - p1) * 0.4f;
        const float makeup = dbToGain ((p2 - 40.0f) * 0.4f);

        for (int i = 0; i < numSamples; ++i)
        {
            const float inPeak = std::max (std::abs (chL[i]), std::abs (chR[i]));
            const float inDb = gainToDb (inPeak);
            const float over = std::max (0.0f, inDb - threshold);

            // Optical two-stage release (fast 60ms first half, slow 1.5s tail)
            const float attCoeff = std::exp (-1.0f / (0.010f * sr));
            const float relCoeff1 = std::exp (-1.0f / (0.060f * sr));
            const float relCoeff2 = std::exp (-1.0f / (1.500f * sr));

            if (over > la2aEnv) la2aEnv = over + attCoeff * (la2aEnv - over);
            else la2aEnv = over + relCoeff1 * (la2aEnv - over);

            la2aTail = la2aEnv + relCoeff2 * (la2aTail - la2aEnv);
            const float compDb = -(la2aEnv * 0.6f + la2aTail * 0.4f);
            const float gain = dbToGain (compDb) * makeup;

            chL[i] = chL[i] * dry + (chL[i] * gain) * wet;
            chR[i] = chR[i] * dry + (chR[i] * gain) * wet;
        }
    }
    else if (type == FxType::UltimoCompressor) // 1176
    {
        // p1 = Input, p2 = Output, p3 = Ratio (4, 8, 12, 20), p4 = Attack (us)
        const float drive = dbToGain ((p1 - 30.0f) * 0.5f);
        const float makeup = dbToGain ((p2 - 20.0f) * 0.5f);
        const float ratio = std::max (2.0f, p3);

        const float attSec = 0.000020f + (p4 * 0.01f) * 0.000800f; // 20us to 800us fast FET
        const float attCoeff = std::exp (-1.0f / (attSec * sr));
        const float relCoeff = std::exp (-1.0f / (0.100f * sr));

        for (int i = 0; i < numSamples; ++i)
        {
            const float drivenL = chL[i] * drive;
            const float drivenR = chR[i] * drive;
            const float inPeak = std::max (std::abs (drivenL), std::abs (drivenR));
            const float inDb = gainToDb (inPeak);
            const float over = std::max (0.0f, inDb - (-20.0f));

            if (over > fetEnv) fetEnv = over + attCoeff * (fetEnv - over);
            else fetEnv = over + relCoeff * (fetEnv - over);

            const float grDb = (fetEnv * (1.0f - 1.0f / ratio));
            const float g = dbToGain (-grDb) * makeup;

            // Subtle FET soft clipping saturation
            const float satL = std::tanh (drivenL * g);
            const float satR = std::tanh (drivenR * g);

            chL[i] = chL[i] * dry + satL * wet;
            chR[i] = chR[i] * dry + satR * wet;
        }
    }
    else if (type == FxType::WaveDesigner) // Transient Shaper
    {
        // p1 = Attack (-100% to +100%), p2 = Sustain (-100% to +100%)
        const float attMult = dbToGain (p1 * 0.12f);
        const float susMult = dbToGain (p2 * 0.12f);

        const float fastCoeff = std::exp (-1.0f / (0.005f * sr));
        const float slowCoeff = std::exp (-1.0f / (0.080f * sr));

        for (int i = 0; i < numSamples; ++i)
        {
            const float aL = std::abs (chL[i]);
            fastEnvL = aL + fastCoeff * (fastEnvL - aL);
            slowEnvL = aL + slowCoeff * (slowEnvL - aL);

            const float transL = std::max (0.0f, fastEnvL - slowEnvL);
            const float susL = slowEnvL;
            const float modL = (transL * attMult + susL * susMult) / std::max (0.001f, transL + susL);

            chL[i] = chL[i] * dry + (chL[i] * modL) * wet;
            chR[i] = chR[i] * dry + (chR[i] * modL) * wet;
        }
    }
    else // Limiter / Other
    {
        const float threshLinear = dbToGain (-std::max (0.1f, p1 * 0.1f));
        for (int i = 0; i < numSamples; ++i)
        {
            const float pk = std::max (std::abs (chL[i]), std::abs (chR[i]));
            if (pk > threshLinear)
                limiterGain = threshLinear / pk;
            else
                limiterGain += (1.0f - limiterGain) * 0.005f;

            chL[i] = chL[i] * dry + (chL[i] * limiterGain) * wet;
            chR[i] = chR[i] * dry + (chR[i] * limiterGain) * wet;
        }
    }
}

//==============================================================================
// PremiumTubeEqDsp Implementation (Pultec, Exciter, Enhancer)
void PremiumTubeEqDsp::prepare (double sampleRate)
{
    sr = (float) sampleRate;
    reset();
}

void PremiumTubeEqDsp::reset()
{
    eqpLowBoostL.reset(); eqpLowBoostR.reset();
    eqpLowAttenL.reset(); eqpLowAttenR.reset();
    eqpHighBoostL.reset(); eqpHighBoostR.reset();
    eqpHighAttenL.reset(); eqpHighAttenR.reset();
    exciterHpfL.reset(); exciterHpfR.reset();
}

void PremiumTubeEqDsp::process (float* chL, float* chR, int numSamples,
                                FxType type, float p1, float p2, float p3, float p4, float mixNorm)
{
    const float wet = mixNorm;
    const float dry = 1.0f - wet;

    if (type == FxType::XtecEq1) // Pultec EQP-1a
    {
        // Pultec Low Boost (p1) & Attenuate (p2), High Boost (p3) & Attenuate (p4)
        const auto cBoost = calcLowShelf (60.0f, p1 * 0.12f, 0.707f, sr);
        const auto cAtten = calcLowShelf (60.0f, -p2 * 0.12f, 0.707f, sr);
        const auto cHigh = calcBell (8000.0f, p3 * 0.12f, 1.2f, sr);

        for (int i = 0; i < numSamples; ++i)
        {
            float sL = eqpLowBoostL.process (chL[i], cBoost);
            sL = eqpLowAttenL.process (sL, cAtten);
            sL = eqpHighBoostL.process (sL, cHigh);

            float sR = eqpLowBoostR.process (chR[i], cBoost);
            sR = eqpLowAttenR.process (sR, cAtten);
            sR = eqpHighBoostR.process (sR, cHigh);

            chL[i] = chL[i] * dry + sL * wet;
            chR[i] = chR[i] * dry + sR * wet;
        }
    }
    else if (type == FxType::StereoExciter || type == FxType::DualExciter) // Aphex Exciter
    {
        // High-pass sidechain at 4 kHz with 2nd harmonic generator
        const auto cHpf = calcHPF (4000.0f, 0.707f, sr);
        const float drive = dbToGain (p1 * 0.15f);

        for (int i = 0; i < numSamples; ++i)
        {
            const float hL = exciterHpfL.process (chL[i], cHpf);
            const float hR = exciterHpfR.process (chR[i], cHpf);

            // Asymmetric harmonic saturation
            const float harmL = (hL * hL + std::sin (hL * 1.5f)) * drive;
            const float harmR = (hR * hR + std::sin (hR * 1.5f)) * drive;

            chL[i] = chL[i] + harmL * (wet * 0.4f);
            chR[i] = chR[i] + harmR * (wet * 0.4f);
        }
    }
    else
    {
        // Enhancer
        for (int i = 0; i < numSamples; ++i)
        {
            chL[i] = chL[i] * dry + std::tanh (chL[i] * 1.2f) * wet;
            chR[i] = chR[i] * dry + std::tanh (chR[i] * 1.2f) * wet;
        }
    }
}

//==============================================================================
// PremiumUtilityAmpDsp Implementation (Guitar Amp, Tube Stage, Sub Octaver)
void PremiumUtilityAmpDsp::prepare (double sampleRate)
{
    sr = (float) sampleRate;
    reset();
}

void PremiumUtilityAmpDsp::reset()
{
    cabFilt1L.reset(); cabFilt1R.reset();
    cabFilt2L.reset(); cabFilt2R.reset();
    lastInSignL = 1.0f;
    subCycleCountL = 0;
    subSquareL = 1.0f;
    subLpfL.reset(); subLpfR.reset();
}

void PremiumUtilityAmpDsp::process (float* chL, float* chR, int numSamples,
                                    FxType type, float p1, float p2, float p3, float p4, float mixNorm)
{
    const float wet = mixNorm;
    const float dry = 1.0f - wet;

    if (type == FxType::GuitarAmp)
    {
        // p1 = Drive, p2 = Bass, p3 = Mid, p4 = Treble
        const float drive = dbToGain (p1 * 0.25f);
        const auto cCabLow = calcBell (90.0f, (p2 - 50.0f) * 0.2f, 1.2f, sr); // 90 Hz punch
        const auto cCabHigh = calcHPF (70.0f, 0.707f, sr);                     // low cut cab

        for (int i = 0; i < numSamples; ++i)
        {
            // Tube Preamp Distortion
            const float distL = std::tanh (chL[i] * drive + 0.1f) - 0.1f;
            const float distR = std::tanh (chR[i] * drive + 0.1f) - 0.1f;

            // 4x12 Speaker Cabinet Filter
            float cabL = cabFilt1L.process (distL, cCabLow);
            cabL = cabFilt2L.process (cabL, cCabHigh);
            float cabR = cabFilt1R.process (distR, cCabLow);
            cabR = cabFilt2R.process (cabR, cCabHigh);

            chL[i] = chL[i] * dry + cabL * wet;
            chR[i] = chR[i] * dry + cabR * wet;
        }
    }
    else if (type == FxType::TubeStage)
    {
        // Analog Triode Saturation
        const float drive = dbToGain (p1 * 0.2f);
        for (int i = 0; i < numSamples; ++i)
        {
            const float xL = chL[i] * drive;
            const float satL = (xL - 0.15f * xL * xL) / (1.0f + std::abs (xL));
            const float xR = chR[i] * drive;
            const float satR = (xR - 0.15f * xR * xR) / (1.0f + std::abs (xR));

            chL[i] = chL[i] * dry + satL * wet;
            chR[i] = chR[i] * dry + satR * wet;
        }
    }
    else if (type == FxType::SubOctaver)
    {
        // -1 Octave sub-bass generator
        const auto cSubLpf = calcHPF (20.0f, 0.707f, sr);
        for (int i = 0; i < numSamples; ++i)
        {
            const float sign = chL[i] >= 0.0f ? 1.0f : -1.0f;
            if (sign != lastInSignL && sign > 0.0f)
            {
                if (++subCycleCountL >= 2)
                {
                    subCycleCountL = 0;
                    subSquareL = -subSquareL;
                }
            }
            lastInSignL = sign;

            const float subSignal = subLpfL.process (subSquareL * std::abs (chL[i]), cSubLpf);
            chL[i] = chL[i] + subSignal * (wet * 0.7f);
            chR[i] = chR[i] + subSignal * (wet * 0.7f);
        }
    }
    else if (type == FxType::EdisonEx1 || type == FxType::StereoImager)
    {
        // Stereo Width & Mid/Side balance
        const float width = p1 * 0.02f; // 0..2 (1.0 = normal)
        for (int i = 0; i < numSamples; ++i)
        {
            const float mid = 0.5f * (chL[i] + chR[i]);
            const float side = 0.5f * (chL[i] - chR[i]) * width;
            chL[i] = mid + side;
            chR[i] = mid - side;
        }
    }
}

//==============================================================================
// Unified FxSlotProcessor
void FxSlotProcessor::prepare (double sampleRate)
{
    sr = sampleRate;
    reverb.prepare (sampleRate);
    delay.prepare (sampleRate);
    modulation.prepare (sampleRate);
    pitchShifter.prepare (sampleRate);
    graphicEq.prepare (sampleRate);
    dynamics.prepare (sampleRate);
    tubeEq.prepare (sampleRate);
    utilityAmp.prepare (sampleRate);
    reset();
}

void FxSlotProcessor::reset()
{
    reverb.reset();
    delay.reset();
    modulation.reset();
    pitchShifter.reset();
    graphicEq.reset();
    dynamics.reset();
    tubeEq.reset();
    utilityAmp.reset();
}

void FxSlotProcessor::process (const float* inL, const float* inR,
                              float* outL, float* outR,
                              int numSamples, FxSlotParams& params)
{
    if (inL == nullptr || outL == nullptr || numSamples <= 0) return;
    const float* srcR = (inR != nullptr) ? inR : inL;

    // Track input peak meters
    float inPkL = 0.0f, inPkR = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        inPkL = std::max (inPkL, std::abs (inL[i]));
        inPkR = std::max (inPkR, std::abs (srcR[i]));
    }
    inPeakL.update (inPkL);
    inPeakR.update (inPkR);

    // Bypass check
    if (params.bypass.getBool())
    {
        std::memcpy (outL, inL, sizeof (float) * (size_t) numSamples);
        std::memcpy (outR, srcR, sizeof (float) * (size_t) numSamples);
        outPeakL.update (inPkL);
        outPeakR.update (inPkR);
        return;
    }

    // Prepare work buffer
    if ((int) workL.size() < numSamples)
    {
        workL.resize ((size_t) numSamples);
        workR.resize ((size_t) numSamples);
    }
    std::memcpy (workL.data(), inL, sizeof (float) * (size_t) numSamples);
    std::memcpy (workR.data(), srcR, sizeof (float) * (size_t) numSamples);

    const auto type = (FxType) (int) params.fxType.getInt();
    const auto meta = getFxMetadata (type);
    const float mixVal = params.mix.get();

    // Route to specialized DSP processor
    switch (meta.category)
    {
        case FxCategory::Reverb:
            reverb.process (workL.data(), workR.data(), numSamples, type,
                            params.p1.get(), params.p2.get(), params.p3.get(), params.p4.get(), mixVal);
            break;

        case FxCategory::Delay:
            delay.process (workL.data(), workR.data(), numSamples, type,
                           params.p1.get(), params.p2.get(), params.p3.get(), params.p4.get(), params.p5.get(), mixVal);
            break;

        case FxCategory::Modulation:
            modulation.process (workL.data(), workR.data(), numSamples, type,
                                params.p1.get(), params.p2.get(), params.p3.get(), params.p4.get(), mixVal);
            break;

        case FxCategory::Pitch:
            pitchShifter.process (workL.data(), workR.data(), numSamples, params.p1.get(), mixVal);
            break;

        case FxCategory::GraphicEQ:
            graphicEq.process (workL.data(), workR.data(), numSamples,
                               params.geqGainsL, params.geqGainsR, (type == FxType::StereoGEQ || type == FxType::StereoTruEQ));
            break;

        case FxCategory::PremiumDynamics:
            dynamics.process (workL.data(), workR.data(), numSamples, type,
                              params.p1.get(), params.p2.get(), params.p3.get(), params.p4.get(), mixVal);
            break;

        case FxCategory::PremiumTubeEq:
            tubeEq.process (workL.data(), workR.data(), numSamples, type,
                            params.p1.get(), params.p2.get(), params.p3.get(), params.p4.get(), mixVal);
            break;

        case FxCategory::PremiumUtilityAmp:
            utilityAmp.process (workL.data(), workR.data(), numSamples, type,
                                params.p1.get(), params.p2.get(), params.p3.get(), params.p4.get(), mixVal);
            break;
    }

    // Copy to output & update output peak meters
    float outPkL = 0.0f, outPkR = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        outL[i] = workL[(size_t) i];
        outR[i] = workR[(size_t) i];
        outPkL = std::max (outPkL, std::abs (outL[i]));
        outPkR = std::max (outPkR, std::abs (outR[i]));
    }
    outPeakL.update (outPkL);
    outPeakR.update (outPkR);
}

//==============================================================================
// FxRackManager Implementation
void FxRackManager::prepare (double sampleRate, int maxBlockSize)
{
    (void) maxBlockSize;
    for (int i = 0; i < kNumSlots; ++i)
        slots[(size_t) i].prepare (sampleRate);
}

void FxRackManager::reset()
{
    for (int i = 0; i < kNumSlots; ++i)
        slots[(size_t) i].reset();
}

} // namespace mps
