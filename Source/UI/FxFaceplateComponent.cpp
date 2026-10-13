// Mix Practice Studio - Hardware Simulated FX Faceplate Component Implementation
#include "FxFaceplateComponent.h"

namespace mps
{

namespace
{
struct GenericFxControl
{
    const char* name;
    double minimum;
    double maximum;
    double step;
    const char* unit;
    bool logarithmic = false;
    bool mix = false;
    int parameterIndex = -1;
};

GenericFxControl getFxControlForType (FxType type, int index);
int getFxParameterCountForType (FxType type);

GenericFxControl getGenericFxControl (FxCategory category, int index)
{
    static const GenericFxControl reverb[] = {
        { "PRE DEL", 0.0, 200.0, 1.0, "ms" }, { "DECAY", 0.1, 20.0, 0.1, "s" },
        { "SIZE", 0.0, 100.0, 1.0, "%" }, { "DAMPING", 0.0, 100.0, 1.0, "%" },
        { "DIFFUSE", 0.0, 100.0, 1.0, "%" }, { "LEVEL", -40.0, 10.0, 0.5, "dB" },
        { "LO CUT", 20.0, 1000.0, 5.0, "Hz", true }, { "HI CUT", 1000.0, 20000.0, 100.0, "Hz", true },
        { "BASS MULT", 0.0, 200.0, 1.0, "%" }, { "SPREAD", 0.0, 100.0, 1.0, "%" },
        { "SHAPE", 0.0, 100.0, 1.0, "%" }, { "MOD SPEED", 0.0, 10.0, 0.1, "Hz" },
        { "TAIL GAIN", -60.0, 6.0, 0.5, "dB" }, { "ECHO DELAY", 0.0, 1000.0, 1.0, "ms" },
        { "FEEDBACK", 0.0, 100.0, 1.0, "%" }, { "REFL DELAY", 0.0, 1000.0, 1.0, "ms" },
        { "REFL GAIN", -60.0, 6.0, 0.5, "dB" }, { "MOD DEPTH", 0.0, 100.0, 1.0, "%" }
    };
    static const GenericFxControl delay[] = {
        { "TIME", 0.0, 3000.0, 1.0, "ms" }, { "LO CUT", 20.0, 1000.0, 5.0, "Hz", true },
        { "HI CUT", 1000.0, 20000.0, 100.0, "Hz", true }, { "FACTOR L", 0.0, 4.0, 0.01, "x" },
        { "FACTOR R", 0.0, 4.0, 0.01, "x" }, { "OFFSET LR", -100.0, 100.0, 1.0, "ms" },
        { "FEED LO CUT", 20.0, 1000.0, 5.0, "Hz", true }, { "FEED HI CUT", 1000.0, 20000.0, 100.0, "Hz", true },
        { "FEED L", 0.0, 100.0, 1.0, "%" }, { "FEED R", 0.0, 100.0, 1.0, "%" },
        { "MODE", 0.0, 3.0, 1.0, "" }, { "CROSS FEED", 0.0, 1.0, 1.0, "" },
        { "TIME A", 0.0, 3000.0, 1.0, "ms" }, { "GAIN A", -60.0, 6.0, 0.5, "dB" },
        { "PAN A", -100.0, 100.0, 1.0, "%" }, { "TIME B", 0.0, 3000.0, 1.0, "ms" },
        { "GAIN B", -60.0, 6.0, 0.5, "dB" }, { "PAN B", -100.0, 100.0, 1.0, "%" }
    };
    static const GenericFxControl modulation[] = {
        { "SPEED", 0.01, 20.0, 0.01, "Hz", true }, { "DEPTH", 0.0, 100.0, 1.0, "%" },
        { "PHASE", 0.0, 360.0, 1.0, "deg" }, { "FEEDBACK", -100.0, 100.0, 1.0, "%" },
        { "DELAY", 0.0, 100.0, 0.1, "ms" }, { "WIDTH", 0.0, 100.0, 1.0, "%" },
        { "LO CUT", 20.0, 1000.0, 5.0, "Hz", true }, { "HI CUT", 1000.0, 20000.0, 100.0, "Hz", true },
        { "ENV ATTACK", 0.1, 1000.0, 1.0, "ms" }, { "ENV HOLD", 0.0, 1000.0, 1.0, "ms" },
        { "ENV RELEASE", 1.0, 2000.0, 1.0, "ms" }, { "RESONANCE", 0.0, 100.0, 1.0, "%" },
        { "ENV SPEED", 0.0, 100.0, 1.0, "%" }, { "ENV DEPTH", 0.0, 100.0, 1.0, "%" },
        { "LOW SPEED", 0.1, 20.0, 0.1, "Hz" }, { "HIGH SPEED", 0.1, 20.0, 0.1, "Hz" },
        { "ACCEL", 0.0, 100.0, 1.0, "%" }, { "BALANCE", -100.0, 100.0, 1.0, "%" }
    };
    static const GenericFxControl pitch[] = {
        { "SEMITONES", -12.0, 12.0, 0.01, "st" }, { "FINE", -100.0, 100.0, 1.0, "cent" },
        { "DELAY L", 0.0, 1000.0, 1.0, "ms" }, { "DELAY R", 0.0, 1000.0, 1.0, "ms" },
        { "FEEDBACK", 0.0, 100.0, 1.0, "%" }, { "MIX", 0.0, 100.0, 1.0, "%" }
    };
    static const GenericFxControl dynamics[] = {
        { "INPUT GAIN", -24.0, 24.0, 0.5, "dB" }, { "THRESHOLD", -60.0, 0.0, 0.5, "dB" },
        { "ATTACK", 0.05, 1000.0, 0.05, "ms", true }, { "RELEASE", 0.05, 2000.0, 0.05, "ms", true },
        { "OUTPUT GAIN", -24.0, 24.0, 0.5, "dB" }, { "RATIO", 1.0, 20.0, 0.1, ":1" },
        { "KNEE", 0.0, 20.0, 0.5, "dB" }, { "BIAS", -100.0, 100.0, 1.0, "%" },
        { "TIME CONST", 1.0, 6.0, 1.0, "" }, { "SQUEEZE", 0.0, 100.0, 1.0, "%" },
        { "STEREO LINK", 0.0, 1.0, 1.0, "" }, { "MIX", 0.0, 100.0, 1.0, "%" },
        { "BAND 1", -24.0, 24.0, 0.5, "dB" }, { "BAND 2", -24.0, 24.0, 0.5, "dB" },
        { "BAND 3", -24.0, 24.0, 0.5, "dB" }, { "BAND 4", -24.0, 24.0, 0.5, "dB" },
        { "BAND 5", -24.0, 24.0, 0.5, "dB" }, { "BAND 6", -24.0, 24.0, 0.5, "dB" }
    };
    static const GenericFxControl tubeEq[] = {
        { "GAIN", -12.0, 12.0, 0.5, "dB" }, { "LO BOOST", 0.0, 10.0, 0.1, "" },
        { "LO ATT", 0.0, 10.0, 0.1, "" }, { "LO FREQ", 20.0, 1000.0, 1.0, "Hz", true },
        { "MID GAIN", -12.0, 12.0, 0.5, "dB" }, { "MID FREQ", 200.0, 10000.0, 10.0, "Hz", true },
        { "HI BOOST", 0.0, 10.0, 0.1, "" }, { "HI ATT", 0.0, 10.0, 0.1, "" },
        { "HI FREQ", 1000.0, 20000.0, 100.0, "Hz", true }, { "BANDWIDTH", 0.0, 10.0, 0.1, "" },
        { "TRANSFORMER", 0.0, 1.0, 1.0, "" }, { "OUTPUT", -24.0, 24.0, 0.5, "dB" }
    };
    static const GenericFxControl utility[] = {
        { "OUTPUT GAIN", -24.0, 24.0, 0.5, "dB" }, { "BALANCE", -100.0, 100.0, 1.0, "%" },
        { "SPREAD", 0.0, 200.0, 1.0, "%" }, { "CENTER DIST", 0.0, 100.0, 1.0, "%" },
        { "DIRECT", 0.0, 100.0, 1.0, "%" }, { "RANGE", 0.0, 1.0, 1.0, "" },
        { "DRIVE", 0.0, 100.0, 1.0, "%" }, { "BASS", 0.0, 100.0, 1.0, "%" },
        { "MID", 0.0, 100.0, 1.0, "%" }, { "TREBLE", 0.0, 100.0, 1.0, "%" },
        { "PRESENCE", 0.0, 100.0, 1.0, "%" }, { "MASTER", 0.0, 100.0, 1.0, "%" },
        { "LOW FREQ", 20.0, 1000.0, 1.0, "Hz", true }, { "MID FREQ", 200.0, 10000.0, 10.0, "Hz", true },
        { "HI FREQ", 1000.0, 20000.0, 100.0, "Hz", true }, { "M/S INPUT", 0.0, 1.0, 1.0, "" },
        { "M/S OUTPUT", 0.0, 1.0, 1.0, "" }, { "CABINET", 0.0, 10.0, 1.0, "" }
    };

    const GenericFxControl* specs = utility;
    int count = (int) (sizeof (utility) / sizeof (utility[0]));
    switch (category)
    {
        case FxCategory::Reverb: specs = reverb; count = (int) (sizeof (reverb) / sizeof (reverb[0])); break;
        case FxCategory::Delay: specs = delay; count = (int) (sizeof (delay) / sizeof (delay[0])); break;
        case FxCategory::Modulation: specs = modulation; count = (int) (sizeof (modulation) / sizeof (modulation[0])); break;
        case FxCategory::Pitch: specs = pitch; count = (int) (sizeof (pitch) / sizeof (pitch[0])); break;
        case FxCategory::GraphicEQ: specs = tubeEq; count = (int) (sizeof (tubeEq) / sizeof (tubeEq[0])); break;
        case FxCategory::PremiumDynamics: specs = dynamics; count = (int) (sizeof (dynamics) / sizeof (dynamics[0])); break;
        case FxCategory::PremiumTubeEq: specs = tubeEq; count = (int) (sizeof (tubeEq) / sizeof (tubeEq[0])); break;
        case FxCategory::PremiumUtilityAmp: break;
    }
    return specs[std::max (0, std::min (count - 1, index))];
}

int getGenericFxParameterCount (FxCategory category)
{
    switch (category)
    {
        case FxCategory::Reverb: return 18;
        case FxCategory::Delay: return 18;
        case FxCategory::Modulation: return 18;
        case FxCategory::Pitch: return 6;
        case FxCategory::GraphicEQ: return 0;
        case FxCategory::PremiumDynamics: return 18;
        case FxCategory::PremiumTubeEq: return 12;
        case FxCategory::PremiumUtilityAmp: return 18;
    }
    return 18;
}

GenericFxControl getFxControlForType (FxType type, int index)
{
    // The combined algorithms expose their own pages in X32-Mix.  Keeping
    // these definitions per type prevents a generic delay/reverb page from
    // silently showing the wrong units or writing the wrong parameter.
    static const GenericFxControl delayChamber[] = {
        { "DELAY", 0.0, 3000.0, 1.0, "ms" }, { "FEEDBACK", 0.0, 95.0, 1.0, "%" },
        { "CHAMBER", 0.0, 100.0, 1.0, "%" }, { "DECAY", 0.1, 20.0, 0.1, "s" },
        { "PRE DEL", 0.0, 250.0, 1.0, "ms" }, { "DAMPING", 0.0, 100.0, 1.0, "%" },
        { "MIX", 0.0, 100.0, 1.0, "%", false, true }
    };
    static const GenericFxControl modChamber[] = {
        { "SPEED", 0.01, 20.0, 0.01, "Hz", true }, { "DEPTH", 0.0, 100.0, 1.0, "%" },
        { "CHORUS", 0.0, 100.0, 1.0, "%" }, { "DECAY", 0.1, 20.0, 0.1, "s" },
        { "PRE DEL", 0.0, 250.0, 1.0, "ms" }, { "DAMPING", 0.0, 100.0, 1.0, "%" },
        { "MIX", 0.0, 100.0, 1.0, "%", false, true }
    };
    static const GenericFxControl delayMod[] = {
        { "TIME", 0.0, 3000.0, 1.0, "ms" }, { "FEEDBACK", 0.0, 95.0, 1.0, "%" },
        { "MOD SPEED", 0.01, 20.0, 0.01, "Hz", true }, { "MOD DEPTH", 0.0, 100.0, 1.0, "%" },
        { "LO CUT", 20.0, 1000.0, 5.0, "Hz", true }, { "HI CUT", 1000.0, 20000.0, 100.0, "Hz", true },
        { "MIX", 0.0, 100.0, 1.0, "%", false, true }
    };
    static const GenericFxControl deEsser[] = {
        { "FREQUENCY", 2000.0, 20000.0, 10.0, "Hz", true },
        { "THRESHOLD", -60.0, 0.0, 0.5, "dB" }, { "RATIO", 1.0, 20.0, 0.1, ":1" },
        { "ATTACK", 0.05, 100.0, 0.05, "ms", true }, { "RELEASE", 5.0, 1000.0, 1.0, "ms", true },
        { "MIX", 0.0, 100.0, 1.0, "%", false, true }
    };
    static const GenericFxControl reverbControls[] = {
        { "PRE DELAY", 0.0, 250.0, 1.0, "ms", false, false, 1 },
        { "DECAY", 0.1, 20.0, 0.1, "s", false, false, 0 },
        { "DAMPING", 0.0, 100.0, 1.0, "%", false, false, 2 },
        { "SIZE", 0.0, 100.0, 1.0, "%", false, false, 3 },
        { "DIFFUSION", 0.0, 100.0, 1.0, "%", false, false, 4 },
        { "LEVEL", -40.0, 10.0, 0.5, "dB", false, false, 5 },
        { "MIX", 0.0, 100.0, 1.0, "%", false, true }
    };
    static const GenericFxControl xtecControls[] = {
        { "LO BOOST", 0.0, 10.0, 0.1, "dB", false, false, 0 },
        { "LO FREQ", 20.0, 1000.0, 1.0, "Hz", true, false, 1 },
        { "LO ATTEN", 0.0, 10.0, 0.1, "dB", false, false, 2 },
        { "HI WIDTH", 0.0, 10.0, 0.1, "", false, false, 3 },
        { "HI BOOST", 0.0, 10.0, 0.1, "dB", false, false, 4 },
        { "HI FREQ", 1000.0, 20000.0, 100.0, "Hz", true, false, 5 }
    };
    static const GenericFxControl limiterControls[] = {
        { "IN GAIN", 0.0, 24.0, 0.1, "dB", false, false, 0 },
        { "OUT GAIN", -12.0, 0.0, 0.1, "dB", false, false, 1 },
        { "SQUEEZE", 0.0, 100.0, 1.0, "%", false, false, 5 },
        { "ATTACK", 0.1, 100.0, 0.1, "ms", false, false, 3 },
        { "RELEASE", 10.0, 1000.0, 1.0, "ms", false, false, 2 },
        { "KNEE", 0.0, 1.0, 0.01, "", false, false, 4 }
    };
    static const GenericFxControl la2aControls[] = {
        { "GAIN", 0.0, 100.0, 1.0, "%", false, false, 1 },
        { "PEAK REDUCT", 0.0, 100.0, 1.0, "%", false, false, 0 }
    };
    static const GenericFxControl fairControls[] = {
        { "THRESHOLD", 0.0, 100.0, 1.0, "%", false, false, 0 },
        { "INPUT GAIN", 0.0, 100.0, 1.0, "%", false, false, 1 },
        { "TIME CONST", 1.0, 6.0, 1.0, "", false, false, 2 },
        { "OUTPUT", 0.0, 100.0, 1.0, "%", false, false, 3 }
    };
    static const GenericFxControl ampControls[] = {
        { "DRIVE", 0.0, 100.0, 1.0, "%", false, false, 0 },
        { "BASS", 0.0, 100.0, 1.0, "%", false, false, 1 },
        { "MIDDLE", 0.0, 100.0, 1.0, "%", false, false, 2 },
        { "TREBLE", 0.0, 100.0, 1.0, "%", false, false, 3 },
        { "PRESENCE", 0.0, 100.0, 1.0, "%", false, false, 4 },
        { "MASTER", 0.0, 100.0, 1.0, "%", false, false, 5 }
    };
    static const GenericFxControl tubeControls[] = {
        { "DRIVE", 0.0, 100.0, 1.0, "%", false, false, 0 },
        { "BIAS", 0.0, 100.0, 1.0, "%", false, false, 2 },
        { "SATURATION", 0.0, 100.0, 1.0, "%", false, false, 3 },
        { "OUTPUT", -20.0, 20.0, 0.5, "dB", false, false, 1 }
    };
    static const GenericFxControl stereoDelayControls[] = {
        { "TIME L", 10.0, 2000.0, 1.0, "ms", false, false, 0 },
        { "TIME R", 10.0, 2000.0, 1.0, "ms", false, false, 1 },
        { "FEEDBACK", 0.0, 95.0, 1.0, "%", false, false, 2 },
        { "PING PONG", 0.0, 1.0, 1.0, "", false, false, 3 },
        { "HI CUT", 0.0, 100.0, 1.0, "%", false, false, 4 },
        { "MIX", 0.0, 100.0, 1.0, "%", false, true }
    };
    static const GenericFxControl enhancerControls[] = {
        { "BASS", -100.0, 100.0, 1.0, "%", false, false, 0 },
        { "PROCESS", 0.0, 100.0, 1.0, "%", false, false, 1 },
        { "TREBLE", -100.0, 100.0, 1.0, "%", false, false, 2 },
        { "MIX", 0.0, 100.0, 1.0, "%", false, true }
    };
    static const GenericFxControl exciterControls[] = {
        { "HARMONICS", 0.0, 100.0, 1.0, "%", false, false, 0 },
        { "TUNE", 0.0, 100.0, 1.0, "%", false, false, 1 },
        { "MIX", 0.0, 100.0, 1.0, "%", false, true }
    };
    static const GenericFxControl imagerControls[] = {
        { "ST SPREAD", 0.0, 200.0, 1.0, "%", false, false, 0 },
        { "CENTER DIST", 0.0, 100.0, 1.0, "%", false, false, 1 },
        { "BALANCE", -100.0, 100.0, 1.0, "%", false, false, 2 },
        { "MIX", 0.0, 100.0, 1.0, "%", false, true }
    };
    static const GenericFxControl soundMaxerControls[] = {
        { "LO CONTOUR", 0.0, 100.0, 1.0, "%", false, false, 0 },
        { "PROCESS", 0.0, 100.0, 1.0, "%", false, false, 1 },
        { "MIX", 0.0, 100.0, 1.0, "%", false, true }
    };

    const GenericFxControl* specs = nullptr;
    int count = 0;
    switch (type)
    {
        case FxType::HallReverb: case FxType::AmbienceReverb: case FxType::RichPlateReverb:
        case FxType::RoomReverb: case FxType::ChamberReverb: case FxType::PlateReverb:
        case FxType::VintageReverb: case FxType::VintageRoomReverb: case FxType::GatedReverb:
        case FxType::ReverseReverb:
            specs = reverbControls; count = 7; break;
        case FxType::StereoXtecEQ1: case FxType::DualXtecEQ1:
        case FxType::StereoXtecEQ5: case FxType::DualXtecEQ5:
            specs = xtecControls; count = 6; break;
        case FxType::StereoPrecisionLimiter:
            specs = limiterControls; count = 6; break;
        case FxType::StereoLeisureCompressor: case FxType::DualLeisureCompressor:
            specs = la2aControls; count = 2; break;
        case FxType::FairCompressor: case FxType::MsFairCompressor: case FxType::DualFairCompressor:
            specs = fairControls; count = 4; break;
        case FxType::StereoGuitarAmp: case FxType::DualGuitarAmp:
            specs = ampControls; count = 6; break;
        case FxType::StereoTubeStage: case FxType::DualTubeStage:
            specs = tubeControls; count = 4; break;
        case FxType::StereoDelay:
            specs = stereoDelayControls; count = 6; break;
        case FxType::StereoEnhancer: case FxType::DualEnhancer:
            specs = enhancerControls; count = 4; break;
        case FxType::StereoExciter: case FxType::DualExciter:
            specs = exciterControls; count = 3; break;
        case FxType::StereoImager: case FxType::EdisonEX1:
            specs = imagerControls; count = 4; break;
        case FxType::SoundMaxer:
            specs = soundMaxerControls; count = 3; break;
        case FxType::DelayAndChamber: case FxType::DelayAndChorus: case FxType::DelayAndFlanger:
            specs = delayChamber; count = 7; break;
        case FxType::ChorusAndChamber: case FxType::FlangerAndChamber:
            specs = modChamber; count = 7; break;
        case FxType::ModulationDelay:
            specs = delayMod; count = 7; break;
        case FxType::DualDeEsser: case FxType::StereoDeEsser:
            specs = deEsser; count = 6; break;
        default: break;
    }
    if (specs != nullptr)
        return specs[juce::jlimit (0, count - 1, index)];
    return getGenericFxControl (getFxMetadata (type).category, index);
}

int getFxParameterCountForType (FxType type)
{
    switch (type)
    {
        case FxType::HallReverb: case FxType::AmbienceReverb: case FxType::RichPlateReverb:
        case FxType::RoomReverb: case FxType::ChamberReverb: case FxType::PlateReverb:
        case FxType::VintageReverb: case FxType::VintageRoomReverb: case FxType::GatedReverb:
        case FxType::ReverseReverb: return 7;
        case FxType::StereoXtecEQ1: case FxType::DualXtecEQ1:
        case FxType::StereoXtecEQ5: case FxType::DualXtecEQ5: return 6;
        case FxType::StereoPrecisionLimiter: case FxType::StereoDelay: return 6;
        case FxType::StereoLeisureCompressor: case FxType::DualLeisureCompressor: return 2;
        case FxType::FairCompressor: case FxType::MsFairCompressor: case FxType::DualFairCompressor: return 4;
        case FxType::StereoGuitarAmp: case FxType::DualGuitarAmp: return 6;
        case FxType::StereoTubeStage: case FxType::DualTubeStage: return 4;
        case FxType::StereoEnhancer: case FxType::DualEnhancer: return 4;
        case FxType::StereoExciter: case FxType::DualExciter: return 3;
        case FxType::StereoImager: case FxType::EdisonEX1: return 4;
        case FxType::SoundMaxer: return 3;
        case FxType::DelayAndChamber: case FxType::ChorusAndChamber:
        case FxType::FlangerAndChamber: case FxType::DelayAndChorus:
        case FxType::DelayAndFlanger: case FxType::ModulationDelay:
            return 7;
        case FxType::DualDeEsser: case FxType::StereoDeEsser:
            return 6;
        default:
            return getGenericFxParameterCount (getFxMetadata (type).category);
    }
}

bool isDualFxType (FxType type)
{
    switch (type)
    {
        case FxType::DualGEQ: case FxType::DualTruEQ: case FxType::DualDeEsser:
        case FxType::DualXtecEQ1: case FxType::DualXtecEQ5: case FxType::DualCombinator:
        case FxType::DualFairCompressor: case FxType::DualLeisureCompressor:
        case FxType::DualUltimoCompressor: case FxType::DualEnhancer: case FxType::DualExciter:
        case FxType::DualGuitarAmp: case FxType::DualTubeStage: case FxType::DualPitch:
            return true;
        default: return false;
    }
}
}

FxFaceplateComponent::FxFaceplateComponent (AudioEngine& eng)
    : engine (eng)
{
    dialParameterIndices.fill (-1);

    // 1. Setup dynamic console dials
    for (int i = 0; i < kMaxDials; ++i)
    {
        dials[(size_t) i].setTitle ("PARAM");
        dials[(size_t) i].onValueChanged = [this, i] (double val) {
            if (ignoreCallbacks) return;
            auto& p = engine.getMixerParams().fxSlots[(size_t) currentSlot];
            if (dialParameterIndices[(size_t) i] == -2)
            {
                p.mix.set ((float) (val * 0.01));
                if (onParametersChanged) onParametersChanged();
                return;
            }
            if (dialParameterIndices[(size_t) i] >= 0)
            {
                p.parameter (dialParameterIndices[(size_t) i]).set ((float) val);
                if (onParametersChanged) onParametersChanged();
                return;
            }
            const auto type = (FxType) (int) p.fxType.getInt();
            const auto meta = getFxMetadata (type);

            // Check if knob is mapped to MIX
            bool isMix = false;
            if (meta.category == FxCategory::Reverb && i == 5) isMix = true;
            else if (type == FxType::StereoDelay && i == 6) isMix = true;
            else if (type == FxType::Tap3Delay && i == 4) isMix = true;
            else if (type == FxType::RhythmDelay && i == 3) isMix = true;
            else if (type == FxType::StereoChorus && i == 5) isMix = true;
            else if (type == FxType::StereoFlanger && i == 5) isMix = true;
            else if (type == FxType::StereoPhaser && i == 5) isMix = true;
            else if (type == FxType::MoodFilter && i == 5) isMix = true;
            else if (type == FxType::RotarySpeaker && i == 5) isMix = true;
            else if (type == FxType::TremoloPanner && i == 5) isMix = true;
            else if (type == FxType::PitchShifter && i == 3) isMix = true;
            else if ((type == FxType::StereoExciter || type == FxType::DualExciter) && i == 2) isMix = true;
            else if ((type == FxType::StereoEnhancer || type == FxType::DualEnhancer) && i == 3) isMix = true;
            else if ((type == FxType::StereoImager || type == FxType::EdisonEx1) && i == 3) isMix = true;

            if (isMix)
            {
                p.mix.set ((float) (val * 0.01));
                if (onParametersChanged) onParametersChanged();
                return;
            }
            if (i < FxSlotParams::kNumParameters)
                p.parameter (i).set ((float) val);
            if (onParametersChanged) onParametersChanged();
        };
        addChildComponent (dials[(size_t) i]);
    }

    for (int i = 0; i < (int) parameterPageButtons.size(); ++i)
    {
        auto& button = parameterPageButtons[(size_t) i];
        button.setButtonText (juce::String (i + 1));
        button.setClickingTogglesState (false);
        button.addListener (this);
        addChildComponent (button);
    }

    // 2. Setup 6 Vertical Faders (Cream Reverbs & Precision Limiter)
    for (int i = 0; i < 6; ++i)
    {
        auto& s = faders[(size_t) i];
        s.setSliderStyle (juce::Slider::LinearVertical);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.addListener (this);
        addChildComponent (s);

        auto& l = faderLabels[(size_t) i];
        l.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        l.setJustificationType (juce::Justification::centred);
        l.setColour (juce::Label::textColourId, ConsoleLookAndFeel::accentCyan);
        addChildComponent (l);

        auto& vl = faderValLabels[(size_t) i];
        vl.setFont (juce::FontOptions (9.5f));
        vl.setJustificationType (juce::Justification::centred);
        vl.setColour (juce::Label::textColourId, ConsoleLookAndFeel::textMuted);
        addChildComponent (vl);
    }

    // 3. Setup Reverb Preset Ribbon Buttons (Hall, Ambience, Plate, Room, Chamber, Concert)
    const char* ribbonNames[6] = { "HALL", "AMBIENCE", "PLATE", "ROOM", "CHAMBER", "CONCERT" };
    for (int i = 0; i < 6; ++i)
    {
        reverbRibbonButtons[(size_t) i].setButtonText (ribbonNames[i]);
        reverbRibbonButtons[(size_t) i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2b3038));
        reverbRibbonButtons[(size_t) i].addListener (this);
        addChildComponent (reverbRibbonButtons[(size_t) i]);
    }

    // 4. Setup 1176 Ratio buttons
    const char* ratioLabels[5] = { "4:1", "8:1", "12:1", "20:1", "ALL" };
    for (int i = 0; i < 5; ++i)
    {
        ratioButtons[i].setButtonText (ratioLabels[i]);
        ratioButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2b3038));
        ratioButtons[i].setClickingTogglesState (true);
        ratioButtons[i].setRadioGroupId (7001);
        ratioButtons[i].addListener (this);
        addChildComponent (ratioButtons[i]);
    }
    ratioButtons[0].setToggleState (true, juce::dontSendNotification);

    // 5. Setup Dimension D / C buttons [OFF] [1] [2] [3] [4]
    const char* dimLabels[5] = { "OFF", "1", "2", "3", "4" };
    for (int i = 0; i < 5; ++i)
    {
        dimButtons[i].setButtonText (dimLabels[i]);
        dimButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2b3038));
        dimButtons[i].setClickingTogglesState (true);
        dimButtons[i].setRadioGroupId (7002);
        dimButtons[i].addListener (this);
        addChildComponent (dimButtons[i]);
    }
    dimButtons[2].setToggleState (true, juce::dontSendNotification);

    // 6. Setup 31 GEQ Sliders
    for (int b = 0; b < 31; ++b)
    {
        auto& s = geqSliders[(size_t) b];
        s.setSliderStyle (juce::Slider::LinearVertical);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRange (-15.0, 15.0, 0.5);
        s.setValue (0.0, juce::dontSendNotification);
        s.addListener (this);
        addChildComponent (s);

        auto& l = geqLabels[(size_t) b];
        const float f = kGeqFrequencies[(size_t) b];
        l.setText (f >= 1000.0f ? juce::String (f * 0.001f, 1) + "k" : juce::String ((int) f), juce::dontSendNotification);
        l.setFont (juce::FontOptions (8.0f));
        l.setJustificationType (juce::Justification::centred);
        l.setColour (juce::Label::textColourId, ConsoleLookAndFeel::textMuted);
        addChildComponent (l);
    }

    geqFlatButton.addListener (this);
    addChildComponent (geqFlatButton);

    geqChAButton.setClickingTogglesState (true);
    geqChAButton.setRadioGroupId (7003);
    geqChAButton.addListener (this);
    geqChAButton.setToggleState (true, juce::dontSendNotification);
    addChildComponent (geqChAButton);

    geqChBButton.setClickingTogglesState (true);
    geqChBButton.setRadioGroupId (7003);
    geqChBButton.addListener (this);
    addChildComponent (geqChBButton);

    // 7. General Switches
    powerSwitch.setClickingTogglesState (true);
    powerSwitch.setColour (juce::TextButton::buttonColourId, ConsoleLookAndFeel::accentGreen.darker (0.55f));
    powerSwitch.addListener (this);
    addAndMakeVisible (powerSwitch);

    bypassSwitch.setClickingTogglesState (true);
    bypassSwitch.setColour (juce::TextButton::buttonColourId, ConsoleLookAndFeel::accentRed.darker (0.55f));
    bypassSwitch.addListener (this);
    addAndMakeVisible (bypassSwitch);

    stereoLinkSwitch.setClickingTogglesState (true);
    stereoLinkSwitch.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2b3038));
    stereoLinkSwitch.addListener (this);
    addChildComponent (stereoLinkSwitch);

    rotarySpeedSwitch.setClickingTogglesState (true);
    rotarySpeedSwitch.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2b3038));
    rotarySpeedSwitch.addListener (this);
    addChildComponent (rotarySpeedSwitch);

    startTimerHz (30); // 30 Hz animation & metering refresh
}

FxFaceplateComponent::~FxFaceplateComponent()
{
    stopTimer();
}

void FxFaceplateComponent::setSlot (int slotIndex)
{
    if (slotIndex >= 0 && slotIndex < 8)
    {
        currentSlot = slotIndex;
        currentParameterPage = 0;
        refreshFromSlot();
        resized();
        repaint();
    }
}

void FxFaceplateComponent::refreshFromSlot()
{
    ignoreCallbacks = true;
    dialParameterIndices.fill (-1);
    auto& p = engine.getMixerParams().fxSlots[(size_t) currentSlot];
    const auto type = (FxType) (int) p.fxType.getInt();
    const auto meta = getFxMetadata (type);

    powerSwitch.setToggleState (! p.bypass.getBool(), juce::dontSendNotification);
    powerSwitch.setButtonText (p.bypass.getBool() ? "OFF" : "ON");
    bypassSwitch.setToggleState (p.bypass.getBool(), juce::dontSendNotification);

    // Hide all dynamic components by default
    for (auto& d : dials) d.setVisible (false);
    for (auto& b : parameterPageButtons) b.setVisible (false);
    for (auto& f : faders) f.setVisible (false);
    for (auto& fl : faderLabels) fl.setVisible (false);
    for (auto& fv : faderValLabels) fv.setVisible (false);
    for (auto& b : reverbRibbonButtons) b.setVisible (false);
    for (auto& b : ratioButtons) b.setVisible (false);
    for (auto& b : dimButtons) b.setVisible (false);
    for (auto& s : geqSliders) s.setVisible (false);
    for (auto& l : geqLabels) l.setVisible (false);
    geqFlatButton.setVisible (false);
    geqChAButton.setVisible (false);
    geqChBButton.setVisible (false);
    stereoLinkSwitch.setVisible (false);
    rotarySpeedSwitch.setVisible (false);

    // One X32-inspired control surface for every algorithm: six rotary
    // encoders per page, identical page keys and the same readout styling.
    // Graphic EQ keeps its natural 31-band fader surface.
    if (meta.category == FxCategory::GraphicEQ)
    {
        for (int b = 0; b < 31; ++b)
        {
            const float value = activeGeqChannel == 0 ? p.geqGainsL[(size_t) b].get()
                                                       : p.geqGainsR[(size_t) b].get();
            geqSliders[(size_t) b].setVisible (true);
            geqSliders[(size_t) b].setValue (value, juce::dontSendNotification);
            geqLabels[(size_t) b].setVisible (true);
        }
        geqFlatButton.setVisible (true);
        geqChAButton.setVisible (true);
        geqChBButton.setVisible (true);
    }
    else
    {
        const int parameterCount = getFxParameterCountForType (type);
        const int pageCount = std::max (1, (parameterCount + 5) / 6);
        currentParameterPage = juce::jlimit (0, pageCount - 1, currentParameterPage);

        for (int page = 0; page < pageCount; ++page)
        {
            auto& button = parameterPageButtons[(size_t) page];
            button.setVisible (true);
            button.setToggleState (currentParameterPage == page, juce::dontSendNotification);
            button.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff272c33));
            button.setColour (juce::TextButton::textColourOffId, ConsoleLookAndFeel::textMuted);
            button.setColour (juce::TextButton::textColourOnId, ConsoleLookAndFeel::accentCyan);
        }

        for (int dialIndex = 0; dialIndex < 6; ++dialIndex)
        {
            const int parameterIndex = currentParameterPage * 6 + dialIndex;
            if (parameterIndex >= parameterCount) break;

            const auto spec = getFxControlForType (type, parameterIndex);
            const int mappedIndex = spec.parameterIndex >= 0 ? spec.parameterIndex : parameterIndex;
            auto& dial = dials[(size_t) dialIndex];
            dial.setVisible (true);
            dial.setTitle (spec.name);
            dial.setRange (spec.minimum, spec.maximum, spec.step, spec.logarithmic);
            dial.setUnit (spec.unit);
            dial.setValue (spec.mix ? p.mix.get() * 100.0f
                                    : p.parameter (mappedIndex).get(), juce::dontSendNotification);
            dialParameterIndices[(size_t) dialIndex] = spec.mix ? -2 : mappedIndex;
        }
    }

    ignoreCallbacks = false;
    resized();
    repaint();
    return;

    // 1. Cream Fader Reverb (Hall, Ambience, Vintage Room, Chamber)
    const bool isCreamReverb = (type == FxType::HallReverb || type == FxType::AmbienceReverb
                                || type == FxType::RoomReverb || type == FxType::ChamberReverb
                                || type == FxType::VintageRoomReverb);
    if (isCreamReverb)
    {
        const char* fTitles[6] = { "PRE DEL", "DECAY", "SIZE", "DAMPING", "DIFFUSE", "LEVEL" };
        const double fMins[6]  = { 0.0, 0.2, 0.0, 0.0, 0.0, -40.0 };
        const double fMaxs[6]  = { 250.0, 10.0, 100.0, 100.0, 100.0, 10.0 };
        const double fVals[6]  = { p.p2.get(), p.p1.get(), p.p4.get(), p.p3.get(), 70.0, p.mix.get() * 10.0 };

        for (int i = 0; i < 6; ++i)
        {
            faders[(size_t) i].setVisible (true);
            faders[(size_t) i].setRange (fMins[i], fMaxs[i], 0.1);
            faders[(size_t) i].setValue (fVals[i], juce::dontSendNotification);

            faderLabels[(size_t) i].setVisible (true);
            faderLabels[(size_t) i].setText (fTitles[i], juce::dontSendNotification);

            faderValLabels[(size_t) i].setVisible (true);
            if (i == 0) faderValLabels[(size_t) i].setText (juce::String (fVals[i], 0) + " ms", juce::dontSendNotification);
            else if (i == 1) faderValLabels[(size_t) i].setText (juce::String (fVals[i], 1) + " s", juce::dontSendNotification);
            else if (i == 5) faderValLabels[(size_t) i].setText (juce::String (fVals[i], 1) + " dB", juce::dontSendNotification);
            else faderValLabels[(size_t) i].setText (juce::String (fVals[i], 0) + " %", juce::dontSendNotification);
        }

        for (auto& b : reverbRibbonButtons) b.setVisible (true);
    }
    // 2. 31-Band Graphic EQ
    else if (meta.category == FxCategory::GraphicEQ)
    {
        for (int b = 0; b < 31; ++b)
        {
            const float val = (activeGeqChannel == 0) ? p.geqGainsL[(size_t) b].get() : p.geqGainsR[(size_t) b].get();
            geqSliders[(size_t) b].setVisible (true);
            geqSliders[(size_t) b].setValue (val, juce::dontSendNotification);
            geqLabels[(size_t) b].setVisible (true);
        }
        geqFlatButton.setVisible (true);
        geqChAButton.setVisible (true);
        geqChBButton.setVisible (true);
    }
    // 3. Precision Limiter (6 Vertical Faders)
    else if (type == FxType::PrecisionLimiter)
    {
        const char* fTitles[6] = { "IN GAIN", "OUT GAIN", "SQUEEZE", "ATTACK", "RELEASE", "KNEE" };
        const double fMins[6]  = { 0.0, -12.0, 0.0, 0.1, 10.0, 0.0 };
        const double fMaxs[6]  = { 24.0, 0.0, 100.0, 100.0, 1000.0, 1.0 };
        const double fVals[6]  = { p.p1.get(), p.p2.get(), p.p6.get(), p.p4.get(), p.p3.get(), p.p5.get() };

        for (int i = 0; i < 6; ++i)
        {
            faders[(size_t) i].setVisible (true);
            faders[(size_t) i].setRange (fMins[i], fMaxs[i], 0.1);
            faders[(size_t) i].setValue (fVals[i], juce::dontSendNotification);

            faderLabels[(size_t) i].setVisible (true);
            faderLabels[(size_t) i].setText (fTitles[i], juce::dontSendNotification);

            faderValLabels[(size_t) i].setVisible (true);
            faderValLabels[(size_t) i].setText (juce::String (fVals[i], 1), juce::dontSendNotification);
        }
        stereoLinkSwitch.setVisible (true);
    }
    // 4. Dimension D / C (Pushbutton Bank)
    else if (type == FxType::DimensionalChorus)
    {
        for (auto& b : dimButtons) b.setVisible (true);
        dials[0].setVisible (true); dials[0].setTitle ("INTENSITY"); dials[0].setRange (0.0, 100.0, 1.0); dials[0].setValue (p.p2.get(), juce::dontSendNotification);
        dials[1].setVisible (true); dials[1].setTitle ("MIX (%)");   dials[1].setRange (0.0, 100.0, 1.0); dials[1].setValue (p.mix.get() * 100.0, juce::dontSendNotification);
    }
    // 5. Urei 1176 LN (Ratio Pushbuttons + Knobs)
    else if (type == FxType::UltimoCompressor)
    {
        for (auto& b : ratioButtons) b.setVisible (true);
        dials[0].setVisible (true); dials[0].setTitle ("INPUT");   dials[0].setRange (0.0, 100.0, 1.0); dials[0].setValue (p.p1.get(), juce::dontSendNotification);
        dials[1].setVisible (true); dials[1].setTitle ("OUTPUT");  dials[1].setRange (0.0, 100.0, 1.0); dials[1].setValue (p.p2.get(), juce::dontSendNotification);
        dials[2].setVisible (true); dials[2].setTitle ("ATTACK");  dials[2].setRange (1.0, 100.0, 1.0); dials[2].setValue (p.p4.get(), juce::dontSendNotification);
        dials[3].setVisible (true); dials[3].setTitle ("RELEASE"); dials[3].setRange (1.0, 100.0, 1.0); dials[3].setValue (50.0, juce::dontSendNotification);
    }
    // 6. Wave Designer (Dual Channels A & B: Attack, Sustain, Gain)
    else if (type == FxType::WaveDesigner)
    {
        dials[0].setVisible (true); dials[0].setTitle ("A ATTACK"); dials[0].setRange (-100.0, 100.0, 1.0); dials[0].setValue (p.p1.get(), juce::dontSendNotification);
        dials[1].setVisible (true); dials[1].setTitle ("A SUSTAIN");dials[1].setRange (-100.0, 100.0, 1.0); dials[1].setValue (p.p2.get(), juce::dontSendNotification);
        dials[2].setVisible (true); dials[2].setTitle ("A GAIN");   dials[2].setRange (-12.0, 12.0, 0.5);   dials[2].setValue (p.p3.get(), juce::dontSendNotification);

        dials[3].setVisible (true); dials[3].setTitle ("B ATTACK"); dials[3].setRange (-100.0, 100.0, 1.0); dials[3].setValue (p.p1.get(), juce::dontSendNotification);
        dials[4].setVisible (true); dials[4].setTitle ("B SUSTAIN");dials[4].setRange (-100.0, 100.0, 1.0); dials[4].setValue (p.p2.get(), juce::dontSendNotification);
        dials[5].setVisible (true); dials[5].setTitle ("B GAIN");   dials[5].setRange (-12.0, 12.0, 0.5);   dials[5].setValue (p.p3.get(), juce::dontSendNotification);
    }
    // 7. Rotary Speaker (Woodgrain panel)
    else if (type == FxType::RotarySpeaker)
    {
        dials[0].setVisible (true); dials[0].setTitle ("LO-SPEED"); dials[0].setRange (0.5, 12.0, 0.1); dials[0].setValue (p.p1.get(), juce::dontSendNotification);
        dials[1].setVisible (true); dials[1].setTitle ("HI-SPEED"); dials[1].setRange (0.5, 8.0, 0.1);  dials[1].setValue (p.p2.get(), juce::dontSendNotification);
        dials[2].setVisible (true); dials[2].setTitle ("ACCEL");    dials[2].setRange (0.0, 100.0, 1.0); dials[2].setValue (50.0, juce::dontSendNotification);
        dials[3].setVisible (true); dials[3].setTitle ("DISTANCE"); dials[3].setRange (0.0, 100.0, 1.0); dials[3].setValue (50.0, juce::dontSendNotification);
        dials[4].setVisible (true); dials[4].setTitle ("BALANCE");  dials[4].setRange (-50.0, 50.0, 1.0);dials[4].setValue (0.0, juce::dontSendNotification);
        dials[5].setVisible (true); dials[5].setTitle ("MIX (%)");  dials[5].setRange (0.0, 100.0, 1.0); dials[5].setValue (p.mix.get() * 100.0, juce::dontSendNotification);
        rotarySpeedSwitch.setVisible (true);
    }
    // 8. Stereo Chorus, Flanger, Phaser, Mood Filter, Tremolo Panner (2 Rows of Knobs)
    else if (type == FxType::StereoChorus || type == FxType::StereoFlanger || type == FxType::StereoPhaser ||
             type == FxType::MoodFilter || type == FxType::TremoloPanner)
    {
        // Top row (0..5)
        dials[0].setVisible (true); dials[0].setTitle ("SPEED");    dials[0].setRange (0.1, 10.0, 0.1); dials[0].setValue (p.p1.get(), juce::dontSendNotification);
        dials[1].setVisible (true); dials[1].setTitle ("WIDTH L");  dials[1].setRange (0.0, 100.0, 1.0); dials[1].setValue (p.p2.get(), juce::dontSendNotification);
        dials[2].setVisible (true); dials[2].setTitle ("WIDTH R");  dials[2].setRange (0.0, 100.0, 1.0); dials[2].setValue (p.p2.get(), juce::dontSendNotification);
        dials[3].setVisible (true); dials[3].setTitle ("DELAY L");  dials[3].setRange (0.0, 100.0, 1.0); dials[3].setValue (20.0, juce::dontSendNotification);
        dials[4].setVisible (true); dials[4].setTitle ("DELAY R");  dials[4].setRange (0.0, 100.0, 1.0); dials[4].setValue (20.0, juce::dontSendNotification);
        dials[5].setVisible (true); dials[5].setTitle ("MIX (%)");  dials[5].setRange (0.0, 100.0, 1.0); dials[5].setValue (p.mix.get() * 100.0, juce::dontSendNotification);

        // Bottom row (6..10)
        dials[6].setVisible (true); dials[6].setTitle ("LO CUT");   dials[6].setRange (20.0, 500.0, 5.0); dials[6].setValue (100.0, juce::dontSendNotification);
        dials[7].setVisible (true); dials[7].setTitle ("HI CUT");   dials[7].setRange (2000.0, 20000.0, 100.0); dials[7].setValue (12000.0, juce::dontSendNotification);
        dials[8].setVisible (true); dials[8].setTitle ("PHASE");    dials[8].setRange (0.0, 180.0, 5.0); dials[8].setValue (90.0, juce::dontSendNotification);
        dials[9].setVisible (true); dials[9].setTitle ("FEED L");   dials[9].setRange (0.0, 95.0, 1.0);  dials[9].setValue (p.p3.get(), juce::dontSendNotification);
        dials[10].setVisible (true); dials[10].setTitle ("FEED R"); dials[10].setRange (0.0, 95.0, 1.0); dials[10].setValue (p.p3.get(), juce::dontSendNotification);
    }
    // 9. Stereo Delay (Cross-Feed CRT)
    else if (type == FxType::StereoDelay)
    {
        dials[0].setVisible (true); dials[0].setTitle ("TIME L (MS)"); dials[0].setRange (10.0, 2000.0, 1.0); dials[0].setValue (p.p1.get(), juce::dontSendNotification);
        dials[1].setVisible (true); dials[1].setTitle ("TIME R (MS)"); dials[1].setRange (10.0, 2000.0, 1.0); dials[1].setValue (p.p2.get(), juce::dontSendNotification);
        dials[2].setVisible (true); dials[2].setTitle ("FEEDBACK");    dials[2].setRange (0.0, 95.0, 1.0);   dials[2].setValue (p.p3.get(), juce::dontSendNotification);
        dials[3].setVisible (true); dials[3].setTitle ("PING-PONG");   dials[3].setRange (0.0, 1.0, 1.0);    dials[3].setValue (p.p4.get(), juce::dontSendNotification);
        dials[4].setVisible (true); dials[4].setTitle ("HI-CUT");      dials[4].setRange (0.0, 100.0, 1.0);  dials[4].setValue (p.p5.get(), juce::dontSendNotification);
        dials[5].setVisible (true); dials[5].setTitle ("FEED MODE");   dials[5].setRange (0.0, 3.0, 1.0);    dials[5].setValue (1.0, juce::dontSendNotification);
        dials[6].setVisible (true); dials[6].setTitle ("MIX (%)");     dials[6].setRange (0.0, 100.0, 1.0);  dials[6].setValue (p.mix.get() * 100.0, juce::dontSendNotification);
    }
    // 10. Plate / Gated / Reverse Reverb (Wide LCD with Bottom Knobs)
    else if (type == FxType::RichPlateReverb || type == FxType::PlateReverb
             || type == FxType::GatedReverb || type == FxType::ReverseReverb)
    {
        dials[0].setVisible (true); dials[0].setTitle ("PRE-DELAY"); dials[0].setRange (0.0, 250.0, 1.0);  dials[0].setValue (p.p2.get(), juce::dontSendNotification);
        dials[1].setVisible (true); dials[1].setTitle ("DECAY (S)"); dials[1].setRange (0.2, 10.0, 0.1);   dials[1].setValue (p.p1.get(), juce::dontSendNotification);
        dials[2].setVisible (true); dials[2].setTitle ("SIZE");      dials[2].setRange (0.0, 100.0, 1.0);  dials[2].setValue (p.p4.get(), juce::dontSendNotification);
        dials[3].setVisible (true); dials[3].setTitle ("DIFFUSE");   dials[3].setRange (0.0, 100.0, 1.0);  dials[3].setValue (70.0, juce::dontSendNotification);
        dials[4].setVisible (true); dials[4].setTitle ("LO CUT");    dials[4].setRange (20.0, 500.0, 5.0); dials[4].setValue (80.0, juce::dontSendNotification);
        dials[5].setVisible (true); dials[5].setTitle ("HI CUT");    dials[5].setRange (2000.0, 20000.0, 100.0); dials[5].setValue (10000.0, juce::dontSendNotification);
        dials[6].setVisible (true); dials[6].setTitle ("MOD DEP");   dials[6].setRange (0.0, 100.0, 1.0);  dials[6].setValue (30.0, juce::dontSendNotification);
        dials[7].setVisible (true); dials[7].setTitle ("MIX (%)");   dials[7].setRange (0.0, 100.0, 1.0);  dials[7].setValue (p.mix.get() * 100.0, juce::dontSendNotification);
    }
    // 11. Combinator (Multiband Master)
    else if (type == FxType::Combinator)
    {
        dials[0].setVisible (true); dials[0].setTitle ("MIX (%)");   dials[0].setRange (0.0, 100.0, 1.0);  dials[0].setValue (p.mix.get() * 100.0, juce::dontSendNotification);
        dials[1].setVisible (true); dials[1].setTitle ("ATTACK");    dials[1].setRange (1.0, 100.0, 1.0);  dials[1].setValue (25.0, juce::dontSendNotification);
        dials[2].setVisible (true); dials[2].setTitle ("RELEASE");   dials[2].setRange (10.0, 1000.0, 5.0);dials[2].setValue (150.0, juce::dontSendNotification);
        dials[3].setVisible (true); dials[3].setTitle ("X-OVER");    dials[3].setRange (100.0, 8000.0, 50.0); dials[3].setValue (1000.0, juce::dontSendNotification);
        dials[4].setVisible (true); dials[4].setTitle ("RATIO");     dials[4].setRange (1.5, 10.0, 0.5);   dials[4].setValue (p.p3.get(), juce::dontSendNotification);
        dials[5].setVisible (true); dials[5].setTitle ("THRESHOLD"); dials[5].setRange (-40.0, 0.0, 0.5);  dials[5].setValue (p.p2.get(), juce::dontSendNotification);
    }
    // 12. Pultec XTEC EQ1 & EQ5
    else if (type == FxType::XtecEq1 || type == FxType::XtecEq5
             || type == FxType::DualXtecEQ1 || type == FxType::DualXtecEQ5)
    {
        dials[0].setVisible (true); dials[0].setTitle ("LO BOOST"); dials[0].setRange (0.0, 10.0, 0.1); dials[0].setValue (p.p1.get(), juce::dontSendNotification);
        dials[1].setVisible (true); dials[1].setTitle ("LO FREQ"); dials[1].setRange (20.0, 1000.0, 1.0, true); dials[1].setValue (p.p2.get(), juce::dontSendNotification);
        dials[2].setVisible (true); dials[2].setTitle ("LO ATTEN"); dials[2].setRange (0.0, 10.0, 0.1); dials[2].setValue (p.p3.get(), juce::dontSendNotification);
        dials[3].setVisible (true); dials[3].setTitle ("HI WIDTH"); dials[3].setRange (0.0, 10.0, 0.1); dials[3].setValue (p.p4.get(), juce::dontSendNotification);
        dials[4].setVisible (true); dials[4].setTitle ("HI BOOST"); dials[4].setRange (0.0, 10.0, 0.1); dials[4].setValue (p.p5.get(), juce::dontSendNotification);
        dials[5].setVisible (true); dials[5].setTitle ("HI FREQ"); dials[5].setRange (1000.0, 20000.0, 100.0, true); dials[5].setValue (p.p6.get(), juce::dontSendNotification);
    }
    // 13. Teletronix LA-2A
    else if (type == FxType::LeisureCompressor)
    {
        dials[0].setVisible (true); dials[0].setTitle ("GAIN");        dials[0].setRange (0.0, 100.0, 1.0); dials[0].setValue (p.p2.get(), juce::dontSendNotification);
        dials[1].setVisible (true); dials[1].setTitle ("PEAK REDUCT"); dials[1].setRange (0.0, 100.0, 1.0); dials[1].setValue (p.p1.get(), juce::dontSendNotification);
    }
    // 14. Fairchild 670
    else if (type == FxType::FairCompressor || type == FxType::MsFairCompressor
             || type == FxType::DualFairCompressor)
    {
        dials[0].setVisible (true); dials[0].setTitle ("THRESHOLD");  dials[0].setRange (0.0, 100.0, 1.0); dials[0].setValue (p.p1.get(), juce::dontSendNotification);
        dials[1].setVisible (true); dials[1].setTitle ("INPUT GAIN"); dials[1].setRange (0.0, 100.0, 1.0); dials[1].setValue (p.p2.get(), juce::dontSendNotification);
        dials[2].setVisible (true); dials[2].setTitle ("TIME CONST"); dials[2].setRange (1.0, 6.0, 1.0);   dials[2].setValue (p.p3.get(), juce::dontSendNotification);
        dials[3].setVisible (true); dials[3].setTitle ("OUTPUT");     dials[3].setRange (0.0, 100.0, 1.0); dials[3].setValue (p.p4.get(), juce::dontSendNotification);
    }
    // 15. Lead 100 Guitar Amp
    else if (type == FxType::GuitarAmp || type == FxType::DualGuitarAmp)
    {
        dials[0].setVisible (true); dials[0].setTitle ("DRIVE");    dials[0].setRange (0.0, 100.0, 1.0); dials[0].setValue (p.p1.get(), juce::dontSendNotification);
        dials[1].setVisible (true); dials[1].setTitle ("BASS");     dials[1].setRange (0.0, 100.0, 1.0); dials[1].setValue (p.p2.get(), juce::dontSendNotification);
        dials[2].setVisible (true); dials[2].setTitle ("MIDDLE");   dials[2].setRange (0.0, 100.0, 1.0); dials[2].setValue (p.p3.get(), juce::dontSendNotification);
        dials[3].setVisible (true); dials[3].setTitle ("TREBLE");   dials[3].setRange (0.0, 100.0, 1.0); dials[3].setValue (p.p4.get(), juce::dontSendNotification);
        dials[4].setVisible (true); dials[4].setTitle ("PRESENCE"); dials[4].setRange (0.0, 100.0, 1.0); dials[4].setValue (p.p5.get(), juce::dontSendNotification);
        dials[5].setVisible (true); dials[5].setTitle ("MASTER");   dials[5].setRange (0.0, 100.0, 1.0); dials[5].setValue (p.p6.get(), juce::dontSendNotification);
    }
    // 16. Tube Stage (12AX7 Valve Warmth)
    else if (type == FxType::TubeStage || type == FxType::DualTubeStage)
    {
        dials[0].setVisible (true); dials[0].setTitle ("DRIVE");      dials[0].setRange (0.0, 100.0, 1.0); dials[0].setValue (p.p1.get(), juce::dontSendNotification);
        dials[1].setVisible (true); dials[1].setTitle ("BIAS");       dials[1].setRange (0.0, 100.0, 1.0); dials[1].setValue (p.p3.get(), juce::dontSendNotification);
        dials[2].setVisible (true); dials[2].setTitle ("SATURATION"); dials[2].setRange (0.0, 100.0, 1.0); dials[2].setValue (p.p4.get(), juce::dontSendNotification);
        dials[3].setVisible (true); dials[3].setTitle ("OUTPUT");     dials[3].setRange (-20.0, 20.0, 0.5);dials[3].setValue (p.p2.get(), juce::dontSendNotification);
    }
    // 17. Stereo Enhancer / Aphex Exciter
    else if (type == FxType::StereoEnhancer || type == FxType::DualEnhancer)
    {
        dials[0].setVisible (true); dials[0].setTitle ("BASS");     dials[0].setRange (-100.0, 100.0, 1.0); dials[0].setValue (p.p1.get(), juce::dontSendNotification);
        dials[1].setVisible (true); dials[1].setTitle ("PROCESS");  dials[1].setRange (0.0, 100.0, 1.0);   dials[1].setValue (p.p2.get(), juce::dontSendNotification);
        dials[2].setVisible (true); dials[2].setTitle ("TREBLE");  dials[2].setRange (-100.0, 100.0, 1.0); dials[2].setValue (p.p3.get(), juce::dontSendNotification);
        dials[3].setVisible (true); dials[3].setTitle ("MIX (%)");  dials[3].setRange (0.0, 100.0, 1.0);   dials[3].setValue (p.mix.get() * 100.0, juce::dontSendNotification);
    }
    else if (type == FxType::StereoExciter || type == FxType::DualExciter)
    {
        dials[0].setVisible (true); dials[0].setTitle ("HARMONICS"); dials[0].setRange (0.0, 100.0, 1.0); dials[0].setValue (p.p1.get(), juce::dontSendNotification);
        dials[1].setVisible (true); dials[1].setTitle ("TUNE");     dials[1].setRange (0.0, 100.0, 1.0); dials[1].setValue (p.p2.get(), juce::dontSendNotification);
        dials[2].setVisible (true); dials[2].setTitle ("MIX (%)"); dials[2].setRange (0.0, 100.0, 1.0); dials[2].setValue (p.mix.get() * 100.0, juce::dontSendNotification);
    }
    else if (type == FxType::StereoImager || type == FxType::EdisonEx1)
    {
        dials[0].setVisible (true); dials[0].setTitle ("ST SPREAD");  dials[0].setRange (0.0, 200.0, 1.0); dials[0].setValue (p.p1.get(), juce::dontSendNotification);
        dials[1].setVisible (true); dials[1].setTitle ("CENTER DIST");dials[1].setRange (0.0, 100.0, 1.0); dials[1].setValue (p.p2.get(), juce::dontSendNotification);
        dials[2].setVisible (true); dials[2].setTitle ("BALANCE");    dials[2].setRange (-100.0, 100.0, 1.0); dials[2].setValue (p.p3.get(), juce::dontSendNotification);
        dials[3].setVisible (true); dials[3].setTitle ("MIX (%)");    dials[3].setRange (0.0, 100.0, 1.0); dials[3].setValue (p.mix.get() * 100.0, juce::dontSendNotification);
    }
    // X32-Mix algorithms without a dedicated faceplate use the same six
    // controls per page as the X32 editor, with up to three pages.
    else
    {
        const int parameterCount = getFxParameterCountForType (type);
        const int pageCount = std::max (1, (parameterCount + 5) / 6);
        const int controlsPerChannel = std::max (1, (parameterCount + 1) / 2);
        currentParameterPage = juce::jlimit (0, pageCount - 1, currentParameterPage);
        for (int i = 0; i < pageCount; ++i)
        {
            auto& pageButton = parameterPageButtons[(size_t) i];
            pageButton.setVisible (true);
            pageButton.setToggleState (currentParameterPage == i, juce::dontSendNotification);
        }

        for (int i = 0; i < 6; ++i)
        {
            const int parameterIndex = currentParameterPage * 6 + i;
            if (parameterIndex >= parameterCount) break;

            auto spec = getFxControlForType (type, parameterIndex);
            juce::String title;
            if (isDualFxType (type))
            {
                spec = getFxControlForType (type, parameterIndex % controlsPerChannel);
                title = juce::String (parameterIndex < controlsPerChannel ? "A " : "B ") + spec.name;
            }
            else title = spec.name;

            auto& dial = dials[(size_t) i];
            dial.setVisible (true);
            dial.setTitle (title);
            dial.setRange (spec.minimum, spec.maximum, spec.step, spec.logarithmic);
            dial.setUnit (spec.unit);
            dial.setValue (spec.mix ? p.mix.get() * 100.0f
                                    : p.parameter (parameterIndex).get(), juce::dontSendNotification);
            dialParameterIndices[(size_t) i] = spec.mix ? -2 : parameterIndex;
        }
    }

    ignoreCallbacks = false;
}

void FxFaceplateComponent::timerCallback()
{
    // Realtime meters polling
    auto& fxRack = engine.getMixEngine().getFxRack();
    auto& slot = fxRack.getSlot (currentSlot);
    const float pkL = slot.getOutPeakL();
    const float pkR = slot.getOutPeakR();

    vuTargetL = clampf (pkL * 0.92f, 0.0f, 1.0f);
    vuNeedleL += (vuTargetL - vuNeedleL) * 0.28f;

    vuTargetR = clampf (pkR * 0.92f, 0.0f, 1.0f);
    vuNeedleR += (vuTargetR - vuNeedleR) * 0.28f;

    tubeGlowPhase += 0.06f;
    if (tubeGlowPhase > 2.0f * kPi) tubeGlowPhase -= 2.0f * kPi;

    lfoPhase += 0.08f;
    if (lfoPhase > 2.0f * kPi) lfoPhase -= 2.0f * kPi;

    // Combinator 5-band bounce
    for (int b = 0; b < 5; ++b)
    {
        const float target = clampf (vuNeedleL * (1.1f - b * 0.15f), 0.0f, 1.0f);
        combMeters[(size_t) b] += (target - combMeters[(size_t) b]) * 0.35f;
    }

    repaint();
}

void FxFaceplateComponent::buttonClicked (juce::Button* b)
{
    auto& p = engine.getMixerParams().fxSlots[(size_t) currentSlot];

    for (int i = 0; i < (int) parameterPageButtons.size(); ++i)
    {
        if (b == &parameterPageButtons[(size_t) i])
        {
            currentParameterPage = i;
            refreshFromSlot();
            resized();
            repaint();
            return;
        }
    }

    if (b == &powerSwitch || b == &bypassSwitch)
    {
        p.bypass.toggle();
        refreshFromSlot();
        if (onParametersChanged) onParametersChanged();
        return;
    }

    // Reverb Ribbon Buttons: Switch between Hall, Ambience, Plate, Room, Chamber, Concert
    for (int i = 0; i < 6; ++i)
    {
        if (b == &reverbRibbonButtons[(size_t) i])
        {
            FxType targetType = FxType::HallReverb;
            switch (i)
            {
                case 0: targetType = FxType::HallReverb; break;
                case 1: targetType = FxType::Ambience; break;
                case 2: targetType = FxType::PlateReverb; break;
                case 3: targetType = FxType::VintageRoom; break;
                case 4: targetType = FxType::VintageReverb; break;
                case 5: targetType = FxType::HallReverb; break;
            }
            if (onTypeChangeRequested) onTypeChangeRequested (targetType);
            else
            {
                p.setDefaultsForType (targetType);
                refreshFromSlot();
                repaint();
                if (onParametersChanged) onParametersChanged();
            }
            return;
        }
    }

    // Graphic EQ Buttons
    if (b == &geqFlatButton)
    {
        for (int i = 0; i < 31; ++i)
        {
            if (activeGeqChannel == 0) p.geqGainsL[(size_t) i].set (0.0f);
            else p.geqGainsR[(size_t) i].set (0.0f);
            geqSliders[(size_t) i].setValue (0.0, juce::dontSendNotification);
        }
        if (onParametersChanged) onParametersChanged();
        return;
    }
    if (b == &geqChAButton)
    {
        activeGeqChannel = 0;
        refreshFromSlot();
        return;
    }
    if (b == &geqChBButton)
    {
        activeGeqChannel = 1;
        refreshFromSlot();
        return;
    }

    // 1176 Ratio Buttons
    for (int i = 0; i < 5; ++i)
    {
        if (b == &ratioButtons[i])
        {
            const float ratios[5] = { 4.0f, 8.0f, 12.0f, 20.0f, 20.0f };
            p.p3.set (ratios[i]);
            if (onParametersChanged) onParametersChanged();
            return;
        }
    }

    // Dimension D / C Buttons
    for (int i = 0; i < 5; ++i)
    {
        if (b == &dimButtons[i])
        {
            p.p1.set ((float) i); // 0=OFF, 1=1, 2=2, 3=3, 4=4
            if (onParametersChanged) onParametersChanged();
            return;
        }
    }
}

void FxFaceplateComponent::sliderValueChanged (juce::Slider* slider)
{
    if (ignoreCallbacks) return;
    auto& p = engine.getMixerParams().fxSlots[(size_t) currentSlot];
    const auto type = (FxType) (int) p.fxType.getInt();

    // 31-Band GEQ Sliders
    for (int b = 0; b < 31; ++b)
    {
        if (slider == &geqSliders[(size_t) b])
        {
            const float val = (float) slider->getValue();
            if (activeGeqChannel == 0) p.geqGainsL[(size_t) b].set (val);
            else p.geqGainsR[(size_t) b].set (val);
            if (onParametersChanged) onParametersChanged();
            return;
        }
    }

    // 6 Vertical Faders
    for (int i = 0; i < 6; ++i)
    {
        if (slider == &faders[(size_t) i])
        {
            const float val = (float) slider->getValue();
            if (type == FxType::PrecisionLimiter)
            {
                switch (i)
                {
                    case 0: p.p1.set (val); break; // Input Gain
                    case 1: p.p2.set (val); break; // Output Gain
                    case 3: p.p4.set (val); break; // Attack
                    case 4: p.p3.set (val); break; // Release
                    case 5: p.p5.set (val); break; // Knee
                }
            }
            else // Cream Reverbs
            {
                switch (i)
                {
                    case 0: p.p2.set (val); break; // Pre Delay
                    case 1: p.p1.set (val); break; // Decay
                    case 2: p.p4.set (val); break; // Size
                    case 3: p.p3.set (val); break; // Damping
                    case 4: p.p5.set (val); break; // Diffusion
                    case 5: p.p6.set (val); break; // Level
                }
            }
            // Update value label
            if (i == 0) faderValLabels[(size_t) i].setText (juce::String (val, 0) + " ms", juce::dontSendNotification);
            else if (i == 1) faderValLabels[(size_t) i].setText (juce::String (val, 1) + " s", juce::dontSendNotification);
            else faderValLabels[(size_t) i].setText (juce::String (val, 0) + " %", juce::dontSendNotification);
            if (onParametersChanged) onParametersChanged();
            return;
        }
    }
}

//==============================================================================
// Layout Methods
void FxFaceplateComponent::resized()
{
    const auto area = getLocalBounds();
    const int totalW = getWidth();
    const int totalH = getHeight();

    powerSwitch.setBounds (totalW - 85, totalH - 34, 72, 24);
    bypassSwitch.setBounds (totalW - 165, totalH - 34, 72, 24);

    auto& p = engine.getMixerParams().fxSlots[(size_t) currentSlot];
    const auto type = (FxType) (int) p.fxType.getInt();
    const auto meta = getFxMetadata (type);

    if (meta.category == FxCategory::GraphicEQ)
        layoutGEQ (area);
    else if (parameterPageButtons[0].isVisible())
        layoutGenericRack (area);
    else if (type == FxType::HallReverb || type == FxType::AmbienceReverb
             || type == FxType::RoomReverb || type == FxType::ChamberReverb
             || type == FxType::VintageRoomReverb)
        layoutCreamReverb (area);
    else if (type == FxType::RichPlateReverb || type == FxType::PlateReverb
             || type == FxType::GatedReverb || type == FxType::ReverseReverb)
        layoutLcdReverb (area);
    else if (type == FxType::VintageReverb)
        layoutVintageReverb (area);
    else if (type == FxType::StereoDelay)
        layoutStereoDelay (area);
    else if (type == FxType::StereoChorus || type == FxType::StereoFlanger || type == FxType::StereoPhaser)
        layoutChorusFlangerPhaser (area);
    else if (type == FxType::DimensionalChorus)
        layoutDimensionC (area);
    else if (type == FxType::MoodFilter)
        layoutMoodFilter (area);
    else if (type == FxType::RotarySpeaker)
        layoutRotarySpeaker (area);
    else if (type == FxType::TremoloPanner)
        layoutTremoloPanner (area);
    else if (type == FxType::SubOctaver)
        layoutSuboctaver (area);
    else if (type == FxType::WaveDesigner)
        layoutWaveDesigner (area);
    else if (type == FxType::PrecisionLimiter)
        layoutPrecisionLimiter (area);
    else if (type == FxType::Combinator)
        layoutCombinator (area);
    else if (type == FxType::XtecEq1 || type == FxType::XtecEq5
             || type == FxType::DualXtecEQ1 || type == FxType::DualXtecEQ5)
        layoutXtecEq (area);
    else if (type == FxType::LeisureCompressor)
        layoutLA2A (area);
    else if (type == FxType::UltimoCompressor)
        layout1176 (area);
    else if (type == FxType::FairCompressor || type == FxType::MsFairCompressor
             || type == FxType::DualFairCompressor)
        layoutFairchild (area);
    else if (type == FxType::GuitarAmp || type == FxType::DualGuitarAmp)
        layoutGuitarAmp (area);
    else if (type == FxType::TubeStage || type == FxType::DualTubeStage)
        layoutTubeStage (area);
    else
        layoutGenericRack (area);
}

// 1. Cream Reverb Layout (6 Horizontal/Vertical Faders + Bottom Ribbon)
void FxFaceplateComponent::layoutCreamReverb (juce::Rectangle<int> area)
{
    const int startX = area.getX() + 45;
    const int availW = area.getWidth() - 90;
    const int colW = availW / 6;
    const int faderY = area.getY() + 52;
    const int faderH = area.getHeight() - 115;

    for (int i = 0; i < 6; ++i)
    {
        const int colX = startX + i * colW;
        faderLabels[(size_t) i].setBounds (colX, area.getY() + 25, colW, 20);
        faders[(size_t) i].setBounds (colX + colW / 2 - 20, faderY, 40, faderH);
        faderValLabels[(size_t) i].setBounds (colX, faderY + faderH + 2, colW, 16);
    }

    // Bottom Preset Ribbon Buttons
    const int ribbonY = area.getBottom() - 36;
    const int ribbonW = std::min (130, availW / 6);
    const int ribbonStartX = area.getX() + (area.getWidth() - ribbonW * 6) / 2;
    for (int i = 0; i < 6; ++i)
        reverbRibbonButtons[(size_t) i].setBounds (ribbonStartX + i * ribbonW, ribbonY, ribbonW - 2, 26);
}

// 2. LCD Reverb Layout (Center LCD Screen + Knobs Below)
void FxFaceplateComponent::layoutLcdReverb (juce::Rectangle<int> area)
{
    const int dialW = 95;
    const int dialH = 100;
    const int dialY = area.getY() + 105;
    const int startX = area.getX() + 35;
    for (int i = 0; i < 8; ++i)
        dials[(size_t) i].setBounds (startX + i * 115, dialY, dialW, dialH);
}

// 3. Vintage EMT 250 Reverb
void FxFaceplateComponent::layoutVintageReverb (juce::Rectangle<int> area)
{
    const int startX = area.getX() + 180;
    for (int i = 0; i < 4; ++i)
        dials[(size_t) i].setBounds (startX + i * 140, area.getY() + 85, 110, 115);
}

// 4. Stereo Delay (CRT Screen in Center + Surrounding Knobs)
void FxFaceplateComponent::layoutStereoDelay (juce::Rectangle<int> area)
{
    const int dialW = 95;
    const int dialH = 100;
    const int dialY = area.getY() + 105;
    const int startX = area.getX() + 35;
    for (int i = 0; i < 7; ++i)
        dials[(size_t) i].setBounds (startX + i * 115, dialY, dialW, dialH);
}

// 5. Modulation Racks (Chorus / Flanger / Phaser: 2 Rows of 6 Knobs)
void FxFaceplateComponent::layoutChorusFlangerPhaser (juce::Rectangle<int> area)
{
    const int dialW = 90;
    const int dialH = 92;
    const int startX = area.getX() + 45;
    const int row1Y = area.getY() + 36;
    const int row2Y = area.getY() + 130;

    // Row 1 (0..5)
    for (int i = 0; i < 6; ++i)
        dials[(size_t) i].setBounds (startX + i * 115, row1Y, dialW, dialH);

    // Row 2 (6..10)
    for (int i = 0; i < 5; ++i)
        dials[(size_t) (i + 6)].setBounds (startX + i * 115, row2Y, dialW, dialH);
}

// 6. Dimension-C / Dimension D (Pushbutton Array)
void FxFaceplateComponent::layoutDimensionC (juce::Rectangle<int> area)
{
    const int btnW = 55;
    const int btnH = 38;
    const int startX = area.getCentreX() - 140;
    for (int i = 0; i < 5; ++i)
        dimButtons[i].setBounds (startX + i * 58, area.getY() + 90, btnW, btnH);

    dials[0].setBounds (area.getX() + 40, area.getY() + 70, 105, 110);
    dials[1].setBounds (area.getRight() - 250, area.getY() + 70, 105, 110);
}

// 7. Mood Filter
void FxFaceplateComponent::layoutMoodFilter (juce::Rectangle<int> area)
{
    layoutChorusFlangerPhaser (area);
}

// 8. Rotary Speaker (Woodgrain panel)
void FxFaceplateComponent::layoutRotarySpeaker (juce::Rectangle<int> area)
{
    const int startX = area.getX() + 65;
    for (int i = 0; i < 6; ++i)
        dials[(size_t) i].setBounds (startX + i * 115, area.getY() + 70, 100, 105);

    rotarySpeedSwitch.setBounds (startX + 15, area.getBottom() - 40, 110, 26);
}

// 9. Tremolo Panner
void FxFaceplateComponent::layoutTremoloPanner (juce::Rectangle<int> area)
{
    layoutChorusFlangerPhaser (area);
}

// 10. Suboctaver (Channels A & B)
void FxFaceplateComponent::layoutSuboctaver (juce::Rectangle<int> area)
{
    const int startX = area.getX() + 80;
    for (int i = 0; i < 4; ++i)
        dials[(size_t) i].setBounds (startX + i * 140, area.getY() + 75, 105, 110);
}

// 11. 31-Band GEQ
void FxFaceplateComponent::layoutGEQ (juce::Rectangle<int> area)
{
    geqFlatButton.setBounds (24, 24, 55, 24);
    geqChAButton.setBounds (85, 24, 85, 24);
    geqChBButton.setBounds (175, 24, 85, 24);

    const int sliderStartX = 24;
    const int sliderAvailW = area.getWidth() - 48;
    const int sliderW = sliderAvailW / 31;
    const int sliderY = 56;
    const int sliderH = area.getHeight() - sliderY - 42;

    for (int b = 0; b < 31; ++b)
    {
        geqSliders[(size_t) b].setBounds (sliderStartX + b * sliderW, sliderY, sliderW - 2, sliderH);
        geqLabels[(size_t) b].setBounds (sliderStartX + b * sliderW, sliderY + sliderH + 2, sliderW, 14);
    }
}

// 12. De-Esser (Dual Circular Bezels)
void FxFaceplateComponent::layoutDeEsser (juce::Rectangle<int> area)
{
    const int cx = area.getCentreX();
    dials[0].setBounds (cx - 260, area.getY() + 65, 115, 120);
    dials[1].setBounds (cx + 145, area.getY() + 65, 115, 120);
}

// 13. Xtec EQ1 & EQ5 (Pultec)
void FxFaceplateComponent::layoutXtecEq (juce::Rectangle<int> area)
{
    const int startX = area.getX() + 35;
    for (int i = 0; i < 6; ++i)
        dials[(size_t) i].setBounds (startX + i * 115, area.getY() + 75, 105, 110);
}

// 14. Wave Designer (Dual Channel Pods: Attack, Sustain, Gain)
void FxFaceplateComponent::layoutWaveDesigner (juce::Rectangle<int> area)
{
    const int halfW = (area.getWidth() - 80) / 2;
    const int dialW = 105;
    const int dialH = 110;
    const int dialY = area.getY() + 65;

    // Channel A Pod
    const int startA = area.getX() + 45;
    dials[0].setBounds (startA + 15, dialY, dialW, dialH);
    dials[1].setBounds (startA + 15 + dialW + 10, dialY, dialW, dialH);
    dials[2].setBounds (startA + 15 + (dialW + 10) * 2, dialY, dialW, dialH);

    // Channel B Pod
    const int startB = startA + halfW + 10;
    dials[3].setBounds (startB + 15, dialY, dialW, dialH);
    dials[4].setBounds (startB + 15 + dialW + 10, dialY, dialW, dialH);
    dials[5].setBounds (startB + 15 + (dialW + 10) * 2, dialY, dialW, dialH);
}

// 15. Precision Limiter (6 Vertical Faders)
void FxFaceplateComponent::layoutPrecisionLimiter (juce::Rectangle<int> area)
{
    const int startX = area.getX() + 50;
    const int availW = area.getWidth() - 100;
    const int colW = availW / 6;
    const int faderY = area.getY() + 54;
    const int faderH = area.getHeight() - 110;

    for (int i = 0; i < 6; ++i)
    {
        const int colX = startX + i * colW;
        faderLabels[(size_t) i].setBounds (colX, area.getY() + 28, colW, 20);
        faders[(size_t) i].setBounds (colX + colW / 2 - 20, faderY, 40, faderH);
        faderValLabels[(size_t) i].setBounds (colX, faderY + faderH + 2, colW, 16);
    }
    stereoLinkSwitch.setBounds (startX + 10, area.getBottom() - 36, 100, 24);
}

// 16. Combinator
void FxFaceplateComponent::layoutCombinator (juce::Rectangle<int> area)
{
    const int startX = area.getX() + 35;
    for (int i = 0; i < 6; ++i)
        dials[(size_t) i].setBounds (startX + i * 110, area.getY() + 75, 100, 105);
}

// 17. Teletronix LA-2A
void FxFaceplateComponent::layoutLA2A (juce::Rectangle<int> area)
{
    const int cx = area.getCentreX();
    const int dialSize = 135;
    const int dialY = area.getY() + 65;
    dials[0].setBounds (cx - 280, dialY, dialSize, dialSize);
    dials[1].setBounds (cx + 145, dialY, dialSize, dialSize);
}

// 18. Urei 1176 LN
void FxFaceplateComponent::layout1176 (juce::Rectangle<int> area)
{
    const int dialSize = 105;
    const int dialY = area.getY() + 75;
    const int startX = area.getX() + 30;

    dials[0].setBounds (startX, dialY, dialSize, dialSize);
    dials[1].setBounds (startX + 115, dialY, dialSize, dialSize);
    dials[2].setBounds (startX + 230, dialY, dialSize, dialSize);
    dials[3].setBounds (startX + 345, dialY, dialSize, dialSize);

    const int rBtnX = startX + 470;
    for (int i = 0; i < 5; ++i)
        ratioButtons[i].setBounds (rBtnX + i * 44, dialY + 28, 40, 32);
}

// 19. Fairchild 670
void FxFaceplateComponent::layoutFairchild (juce::Rectangle<int> area)
{
    const int dialSize = 105;
    const int dialY = area.getY() + 75;
    const int startX = area.getX() + 35;
    for (int i = 0; i < 4; ++i)
        dials[(size_t) i].setBounds (startX + i * 120, dialY, dialSize, dialSize);
}

// 20. Lead 100 Guitar Amp
void FxFaceplateComponent::layoutGuitarAmp (juce::Rectangle<int> area)
{
    const int startX = area.getX() + 25;
    for (int i = 0; i < 6; ++i)
        dials[(size_t) i].setBounds (startX + i * 115, area.getY() + 75, 100, 105);
}

// 21. Tube Stage
void FxFaceplateComponent::layoutTubeStage (juce::Rectangle<int> area)
{
    const int cx = area.getCentreX();
    const int dialSize = 105;
    const int dialY = area.getY() + 75;

    dials[0].setBounds (cx - 320, dialY, dialSize, dialSize);
    dials[1].setBounds (cx - 200, dialY, dialSize, dialSize);
    dials[2].setBounds (cx + 95, dialY, dialSize, dialSize);
    dials[3].setBounds (cx + 215, dialY, dialSize, dialSize);
}

// 22. Generic Studio Rack
void FxFaceplateComponent::layoutGenericRack (juce::Rectangle<int> area)
{
    const int margin = 24;
    const int gap = 8;
    const int dialW = std::max (72, (area.getWidth() - margin * 2 - gap * 5) / 6);
    const int visibleDialCount = std::max (1, (int) std::count_if (dials.begin(), dials.end(),
                                                                    [] (const auto& dial) { return dial.isVisible(); }));
    const int totalDialW = dialW * visibleDialCount + gap * (visibleDialCount - 1);
    const int startX = area.getX() + (area.getWidth() - totalDialW) / 2;
    const int dialY = area.getY() + 62;
    const int dialH = std::max (84, area.getHeight() - 118);
    int visibleDialIndex = 0;
    for (int i = 0; i < 6; ++i)
        if (dials[(size_t) i].isVisible())
            dials[(size_t) i].setBounds (startX + visibleDialIndex++ * (dialW + gap), dialY, dialW, dialH);

    const int buttonW = 34;
    const int buttonGap = 6;
    const int pageCount = (int) std::count_if (parameterPageButtons.begin(), parameterPageButtons.end(),
                                                [] (const auto& button) { return button.isVisible(); });
    const int totalButtonW = pageCount * buttonW + std::max (0, pageCount - 1) * buttonGap;
    const int buttonStartX = area.getCentreX() - totalButtonW / 2;
    for (int i = 0; i < (int) parameterPageButtons.size(); ++i)
        if (parameterPageButtons[(size_t) i].isVisible())
            parameterPageButtons[(size_t) i].setBounds (buttonStartX + i * (buttonW + buttonGap),
                                                        area.getBottom() - 38, buttonW, 24);
}

//==============================================================================
// Specialized Painting Implementations matching user reference images
void FxFaceplateComponent::paint (juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat();
    auto& p = engine.getMixerParams().fxSlots[(size_t) currentSlot];
    const auto type = (FxType) (int) p.fxType.getInt();
    const auto meta = getFxMetadata (type);

    if (meta.category == FxCategory::GraphicEQ)
        paintGraphicEq (g, area, meta.name);
    else
        paintGenericStudioRack (g, area, meta.name, "X32 FX PROCESSOR");

    g.setColour (juce::Colour (0xff080a0c));
    g.drawRoundedRectangle (area.reduced (1.0f), 5.0f, 4.0f);
    g.setColour (ConsoleLookAndFeel::borderSubtle);
    g.drawRoundedRectangle (area.reduced (3.0f), 4.0f, 1.0f);
    g.setColour (ConsoleLookAndFeel::accentCyan.withAlpha (0.55f));
    g.fillRect (area.getX() + 12.0f, area.getY() + 3.0f, std::min (150.0f, area.getWidth() - 24.0f), 2.0f);
    drawRackMountScrews (g, area);
    return;

    if (meta.category == FxCategory::GraphicEQ)
        paintGraphicEq (g, area, meta.name);
    else if (type == FxType::HallReverb || type == FxType::AmbienceReverb
             || type == FxType::RoomReverb || type == FxType::ChamberReverb
             || type == FxType::VintageRoomReverb)
        paintCreamReverb (g, area, meta.name);
    else if (type == FxType::RichPlateReverb || type == FxType::PlateReverb)
        paintLcdReverb (g, area, "PLATE REVERB", juce::Colour (0xff0088ff)); // Electric Blue LCD
    else if (type == FxType::GateReverb)
        paintLcdReverb (g, area, "GATED REVERB", juce::Colour (0xff10c850)); // Emerald Green LCD
    else if (type == FxType::ReverseReverb)
        paintLcdReverb (g, area, "REVERSE REVERB", juce::Colour (0xff10c850)); // Emerald Green LCD
    else if (type == FxType::VintageReverb)
        paintVintageReverb (g, area);
    else if (type == FxType::StereoDelay)
        paintStereoDelay (g, area);
    else if (type == FxType::StereoChorus)
        paintModulationRack (g, area, "Stereo Chorus", juce::Colour (0xff1fa0c8)); // Cyan/Sky Blue
    else if (type == FxType::StereoFlanger)
        paintModulationRack (g, area, "Stereo Flanger", juce::Colour (0xff861b9e)); // Magenta/Purple
    else if (type == FxType::StereoPhaser)
        paintModulationRack (g, area, "Stereo Phaser", juce::Colour (0xff3da832)); // Lime Green
    else if (type == FxType::DimensionalChorus)
        paintDimensionC (g, area);
    else if (type == FxType::MoodFilter)
        paintMoodFilter (g, area);
    else if (type == FxType::RotarySpeaker)
        paintRotarySpeaker (g, area);
    else if (type == FxType::TremoloPanner)
        paintTremoloPanner (g, area);
    else if (type == FxType::SubOctaver)
        paintSuboctaver (g, area);
    else if (type == FxType::DualDeEsser || type == FxType::StereoExciter)
        paintDeEsser (g, area, true);
    else if (type == FxType::XtecEq1 || type == FxType::XtecEq5
             || type == FxType::DualXtecEQ1 || type == FxType::DualXtecEQ5)
        paintXtecEq (g, area, meta.name);
    else if (type == FxType::WaveDesigner)
        paintWaveDesigner (g, area);
    else if (type == FxType::PrecisionLimiter)
        paintPrecisionLimiter (g, area);
    else if (type == FxType::Combinator)
        paintCombinator (g, area);
    else if (type == FxType::LeisureCompressor)
        paintLA2A (g, area);
    else if (type == FxType::UltimoCompressor)
        paint1176 (g, area);
    else if (type == FxType::FairCompressor || type == FxType::MsFairCompressor
             || type == FxType::DualFairCompressor)
        paintFairchild (g, area);
    else if (type == FxType::GuitarAmp || type == FxType::DualGuitarAmp)
        paintGuitarAmp (g, area);
    else if (type == FxType::TubeStage || type == FxType::DualTubeStage)
        paintTubeStage (g, area);
    else
        paintGenericStudioRack (g, area, meta.name, meta.hardwareEmulation);

    // Consistent X32-style rack shell around every algorithm faceplate.
    g.setColour (juce::Colour (0xff0a0c0f).withAlpha (0.70f));
    g.drawRoundedRectangle (area.reduced (1.0f), 5.0f, 5.0f);
    g.setColour (ConsoleLookAndFeel::borderSubtle.withAlpha (0.85f));
    g.drawRoundedRectangle (area.reduced (3.0f), 4.0f, 1.0f);
    g.setColour (ConsoleLookAndFeel::accentCyan.withAlpha (0.32f));
    g.fillRect (area.getX() + 12.0f, area.getY() + 3.0f, std::min (180.0f, area.getWidth() - 24.0f), 2.0f);

    drawRackMountScrews (g, area);
}

// 1. Authentic Cream/Beige Fader Reverb (Image 1 reference)
void FxFaceplateComponent::paintCreamReverb (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& activeName)
{
    // Warm Ivory / Cream Background
    juce::ColourGradient creamGrad (juce::Colour (0xffe8e1cf), area.getX(), area.getY(),
                                    juce::Colour (0xffd8cfb8), area.getX(), area.getBottom(), false);
    g.setGradientFill (creamGrad);
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colour (0xffb8ab90));
    g.drawRoundedRectangle (area, 4.0f, 1.5f);

    // Top Dark Header Banner
    const juce::Rectangle<float> topBanner (area.getX() + 4.0f, area.getY() + 4.0f, area.getWidth() - 8.0f, 40.0f);
    g.setColour (juce::Colour (0xff24201a));
    g.fillRoundedRectangle (topBanner, 3.0f);

    // Model Name
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xffded6c0));
    g.drawText (activeName.toUpperCase(), (int) topBanner.getX() + 15, (int) topBanner.getY() + 10, 220, 20, juce::Justification::left);

    // Bottom Dark Ribbon Bar
    const juce::Rectangle<float> bottomRibbon (area.getX() + 4.0f, area.getBottom() - 44.0f, area.getWidth() - 8.0f, 38.0f);
    g.setColour (juce::Colour (0xff1f1b16));
    g.fillRoundedRectangle (bottomRibbon, 3.0f);

    // Draw Fader Slots Guides
    const float startX = area.getX() + 45.0f;
    const float colW = (area.getWidth() - 90.0f) / 6.0f;
    const float faderY = area.getY() + 52.0f;
    const float faderH = area.getHeight() - 115.0f;

    for (int i = 0; i < 6; ++i)
    {
        const float cx = startX + i * colW + colW * 0.5f;
        // Dark recessed fader track line
        g.setColour (juce::Colour (0xff1a1714));
        g.fillRoundedRectangle (cx - 3.0f, faderY + 8.0f, 6.0f, faderH - 16.0f, 2.0f);

        // Fader ticks
        g.setColour (juce::Colour (0xff8a7e6c));
        for (float ty = faderY + 12.0f; ty <= faderY + faderH - 12.0f; ty += 14.0f)
        {
            g.drawHorizontalLine ((int) ty, cx - 14.0f, cx - 5.0f);
            g.drawHorizontalLine ((int) ty, cx + 5.0f, cx + 14.0f);
        }
    }
}

// 2. Plate / Gated / Reverse Reverb with Glowing LCD (Image 1 & 2 reference)
void FxFaceplateComponent::paintLcdReverb (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& title, juce::Colour lcdColor)
{
    // Matte Black Chassis
    juce::ColourGradient blackGrad (juce::Colour (0xff1a1d22), area.getX(), area.getY(),
                                    juce::Colour (0xff0f1114), area.getX(), area.getBottom(), false);
    g.setGradientFill (blackGrad);
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colour (0xff323a48));
    g.drawRoundedRectangle (area, 4.0f, 1.2f);

    // Center Large Wide LCD Display Screen
    const juce::Rectangle<float> lcdBox (area.getCentreX() - 150.0f, area.getY() + 20.0f, 300.0f, 60.0f);
    g.setColour (juce::Colour (0xff080a0d));
    g.fillRoundedRectangle (lcdBox, 4.0f);
    g.setColour (juce::Colour (0xff2b3340));
    g.drawRoundedRectangle (lcdBox, 4.0f, 1.2f);

    // Illuminated Screen Glass
    auto innerGlass = lcdBox.reduced (3.0f);
    g.setColour (lcdColor);
    g.fillRoundedRectangle (innerGlass, 3.0f);

    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xff00150a));
    g.drawText (title, (int) innerGlass.getX(), (int) innerGlass.getY(), (int) innerGlass.getWidth(), (int) innerGlass.getHeight(), juce::Justification::centred);

    // Output Meter on Right
    const juce::Rectangle<float> meterBox (area.getRight() - 210.0f, area.getY() + 20.0f, 120.0f, 60.0f);
    drawAnalogVuMeter (g, meterBox, vuNeedleL, "LEVEL");
}

// 3. Vintage Reverb (EMT 250 Hallgerät - Image 1 reference)
void FxFaceplateComponent::paintVintageReverb (juce::Graphics& g, juce::Rectangle<float> area)
{
    juce::ColourGradient darkGrad (juce::Colour (0xff22252a), area.getX(), area.getY(),
                                   juce::Colour (0xff141619), area.getX(), area.getBottom(), false);
    g.setGradientFill (darkGrad);
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colour (0xff3d4450));
    g.drawRoundedRectangle (area, 4.0f, 1.2f);

    // Header Title Silkscreen
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    g.setColour (ConsoleLookAndFeel::accentCyan);
    g.drawText ("Vintage Reverb / Hallgerät", (int) area.getX() + 180, (int) area.getY() + 18, 300, 20, juce::Justification::centred);

    // Left Dual Meter Ladders
    const juce::Rectangle<float> meterBox (area.getX() + 45.0f, area.getY() + 25.0f, 90.0f, area.getHeight() - 50.0f);
    g.setColour (juce::Colour (0xff0d0f12));
    g.fillRoundedRectangle (meterBox, 3.0f);

    // Right Red Accent Bar
    g.setColour (juce::Colour (0xffcc2222));
    g.fillRoundedRectangle (area.getRight() - 36.0f, area.getY() + 20.0f, 12.0f, area.getHeight() - 40.0f, 3.0f);
}

// 4. Stereo Delay with Green CRT Schematic (Image 2 reference)
void FxFaceplateComponent::paintStereoDelay (juce::Graphics& g, juce::Rectangle<float> area)
{
    juce::ColourGradient delayGrad (juce::Colour (0xff1c1f24), area.getX(), area.getY(),
                                    juce::Colour (0xff101215), area.getX(), area.getBottom(), false);
    g.setGradientFill (delayGrad);
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colour (0xff323a48));
    g.drawRoundedRectangle (area, 4.0f, 1.2f);

    // Center Green LCD Crossfeed Schematic Screen
    const juce::Rectangle<float> crtBox (area.getCentreX() - 100.0f, area.getY() + 18.0f, 200.0f, 75.0f);
    g.setColour (juce::Colour (0xff0a100a));
    g.fillRoundedRectangle (crtBox, 4.0f);
    g.setColour (juce::Colour (0xff1f4020));
    g.drawRoundedRectangle (crtBox, 4.0f, 1.2f);

    auto innerCrt = crtBox.reduced (3.0f);
    g.setColour (juce::Colour (0xff30b530));
    g.fillRoundedRectangle (innerCrt, 3.0f);

    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xff002005));
    g.drawText ("Cross Feed Mode", (int) innerCrt.getX(), (int) innerCrt.getY() + 8, (int) innerCrt.getWidth(), 16, juce::Justification::centred);

    // Arrow schematic
    g.drawText ("L ---> Feed R", (int) innerCrt.getX() + 20, (int) innerCrt.getY() + 30, 160, 16, juce::Justification::left);
    g.drawText ("R ---> Feed L", (int) innerCrt.getX() + 20, (int) innerCrt.getY() + 48, 160, 16, juce::Justification::left);
}

// 5. Colorful Modulation Racks (Chorus Cyan, Flanger Magenta, Phaser Green - Image 2)
void FxFaceplateComponent::paintModulationRack (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& title, juce::Colour bodyColour)
{
    // Anodized Aluminum Vivid Body
    juce::ColourGradient modGrad (bodyColour.brighter (0.1f), area.getX(), area.getY(),
                                  bodyColour.darker (0.25f), area.getX(), area.getBottom(), false);
    g.setGradientFill (modGrad);
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (bodyColour.brighter (0.3f));
    g.drawRoundedRectangle (area, 4.0f, 1.5f);

    // Black Recessed Center Panel for Knobs
    const juce::Rectangle<float> innerWell (area.getX() + 35.0f, area.getY() + 25.0f, area.getWidth() - 70.0f, area.getHeight() - 50.0f);
    g.setColour (juce::Colour (0xff121418));
    g.fillRoundedRectangle (innerWell, 5.0f);
    g.setColour (juce::Colour (0xff252a32));
    g.drawRoundedRectangle (innerWell, 5.0f, 1.2f);

    // Title and Speed Stripes
    g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    g.setColour (juce::Colours::white);
    g.drawText (title, (int) area.getX() + 20, (int) area.getY() + 10, 150, 20, juce::Justification::left);

    // Pulsing LFO Rate LED
    const float lfoGlow = 0.5f + 0.5f * std::sin (lfoPhase);
    g.setColour (juce::Colour (0xffffffff).withMultipliedAlpha (lfoGlow));
    g.fillEllipse (area.getX() + 160.0f, area.getY() + 15.0f, 10.0f, 10.0f);
}

// 6. Dimension-C / Dimension D (Image 3 reference)
void FxFaceplateComponent::paintDimensionC (juce::Graphics& g, juce::Rectangle<float> area)
{
    juce::ColourGradient dimGrad (juce::Colour (0xff1c1f24), area.getX(), area.getY(),
                                  juce::Colour (0xff101215), area.getX(), area.getBottom(), false);
    g.setGradientFill (dimGrad);
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colour (0xff323a48));
    g.drawRoundedRectangle (area, 4.0f, 1.2f);

    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xffff8800)); // Orange logo
    g.drawText ("Dimensional Chorus", (int) area.getCentreX() + 40, (int) area.getY() + 18, 220, 22, juce::Justification::left);

    // Pushbutton plate housing
    const juce::Rectangle<float> btnPlate (area.getCentreX() - 150.0f, area.getY() + 80.0f, 300.0f, 60.0f);
    g.setColour (juce::Colour (0xff0a0c0f));
    g.fillRoundedRectangle (btnPlate, 4.0f);
    g.setColour (juce::Colour (0xff282f3a));
    g.drawRoundedRectangle (btnPlate, 4.0f, 1.0f);
}

// 7. Mood Filter (Image 3 reference)
void FxFaceplateComponent::paintMoodFilter (juce::Graphics& g, juce::Rectangle<float> area)
{
    // Deep Teal / Cyan borders
    g.setColour (juce::Colour (0xff0f2b32));
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colour (0xff00d4d8));
    g.drawRoundedRectangle (area, 4.0f, 1.8f);

    g.setFont (juce::FontOptions (24.0f, juce::Font::italic));
    g.setColour (juce::Colour (0xff00e5ff));
    g.drawText ("mood", (int) area.getRight() - 140, (int) area.getY() + 20, 100, 30, juce::Justification::centred);
}

// 8. Rotary Speaker Woodgrain (Image 3 reference)
void FxFaceplateComponent::paintRotarySpeaker (juce::Graphics& g, juce::Rectangle<float> area)
{
    // Rich Mahogany / Cherry Woodgrain Gradient
    juce::ColourGradient woodGrad (juce::Colour (0xff421812), area.getX(), area.getY(),
                                   juce::Colour (0xff260a06), area.getX(), area.getBottom(), false);
    g.setGradientFill (woodGrad);
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colour (0xff652b20));
    g.drawRoundedRectangle (area, 4.0f, 1.5f);

    // Side Cooling Louvers/Vents
    g.setColour (juce::Colour (0xff140503));
    for (float vy = area.getY() + 50.0f; vy <= area.getBottom() - 50.0f; vy += 10.0f)
    {
        g.fillRoundedRectangle (area.getX() + 14.0f, vy, 35.0f, 4.0f, 2.0f);
        g.fillRoundedRectangle (area.getRight() - 49.0f, vy, 35.0f, 4.0f, 2.0f);
    }

    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xffd8c2aa));
    g.drawText ("Rotary Speaker", (int) area.getX() + 65, (int) area.getY() + 20, 200, 22, juce::Justification::left);
}

// 9. Tremolo Panner Mint Green (Image 3 reference)
void FxFaceplateComponent::paintTremoloPanner (juce::Graphics& g, juce::Rectangle<float> area)
{
    juce::ColourGradient mintGrad (juce::Colour (0xff2ea87a), area.getX(), area.getY(),
                                   juce::Colour (0xff1e7855), area.getX(), area.getBottom(), false);
    g.setGradientFill (mintGrad);
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colour (0xff4ad49e));
    g.drawRoundedRectangle (area, 4.0f, 1.5f);

    // Inner Recessed Knob Panel
    const juce::Rectangle<float> innerWell (area.getX() + 35.0f, area.getY() + 25.0f, area.getWidth() - 70.0f, area.getHeight() - 50.0f);
    g.setColour (juce::Colour (0xff121418));
    g.fillRoundedRectangle (innerWell, 5.0f);

    g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    g.setColour (juce::Colours::white);
    g.drawText ("Stereo Tremolo", (int) area.getX() + 20, (int) area.getY() + 10, 160, 20, juce::Justification::left);
}

// 10. Suboctaver Terracotta (Image 3 reference)
void FxFaceplateComponent::paintSuboctaver (juce::Graphics& g, juce::Rectangle<float> area)
{
    juce::ColourGradient terraGrad (juce::Colour (0xff8c4a42), area.getX(), area.getY(),
                                    juce::Colour (0xff65322c), area.getX(), area.getBottom(), false);
    g.setGradientFill (terraGrad);
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colour (0xffad655c));
    g.drawRoundedRectangle (area, 4.0f, 1.5f);

    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.setColour (juce::Colours::white);
    g.drawText ("Sub Octaver", (int) area.getX() + 25, (int) area.getY() + 20, 160, 22, juce::Justification::left);
}

// 11. Dual / Stereo De-Esser (Image 4 & 5 reference)
void FxFaceplateComponent::paintDeEsser (juce::Graphics& g, juce::Rectangle<float> area, bool isDual)
{
    (void) isDual;
    // Brushed White/Silver Aluminum Chassis
    juce::ColourGradient silverGrad (juce::Colour (0xffeaeef5), area.getX(), area.getY(),
                                     juce::Colour (0xffc4cbd6), area.getX(), area.getBottom(), false);
    g.setGradientFill (silverGrad);
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colour (0xff8a94a2));
    g.drawRoundedRectangle (area, 4.0f, 1.5f);

    // Central Black DSS Badge
    const float cx = area.getCentreX();
    const float cy = area.getY() + 45.0f;
    g.setColour (juce::Colour (0xff14161a));
    g.fillRoundedRectangle (cx - 30.0f, cy - 16.0f, 60.0f, 32.0f, 4.0f);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.setColour (juce::Colours::white);
    g.drawText ("dss", (int) cx - 25, (int) cy - 14, 50, 28, juce::Justification::centred);

    // Dual Circular Dark Dial Bezels on Left and Right
    const float bY = area.getY() + 125.0f;
    g.setColour (juce::Colour (0xff14161a));
    g.fillEllipse (cx - 210.0f, bY - 60.0f, 120.0f, 120.0f);
    g.fillEllipse (cx + 90.0f, bY - 60.0f, 120.0f, 120.0f);
}

// 12. Xtec EQ1 & EQ5 (Pultec Sea-Teal Faceplate - Image 5 reference)
void FxFaceplateComponent::paintXtecEq (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& title)
{
    juce::ColourGradient pultecGrad (juce::Colour (0xff3c6670), area.getX(), area.getY(),
                                     juce::Colour (0xff26464e), area.getX(), area.getBottom(), false);
    g.setGradientFill (pultecGrad);
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colour (0xff5a8e9c));
    g.drawRoundedRectangle (area, 4.0f, 1.5f);

    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xffe6f4f8));
    g.drawText (title, (int) area.getX() + 25, (int) area.getY() + 14, 250, 22, juce::Justification::left);

    // Red Jewel Pilot Light
    const float jewelX = area.getRight() - 50.0f;
    const float jewelY = area.getY() + 28.0f;
    g.setColour (juce::Colour (0xffcc0000));
    g.fillEllipse (jewelX - 8.0f, jewelY - 8.0f, 16.0f, 16.0f);
    g.setColour (juce::Colour (0xffff6666));
    g.fillEllipse (jewelX - 3.0f, jewelY - 3.0f, 6.0f, 6.0f);
}

// 13. Wave Designer (Royal Blue Deep Indigo - Image 5 reference)
void FxFaceplateComponent::paintWaveDesigner (juce::Graphics& g, juce::Rectangle<float> area)
{
    juce::ColourGradient waveGrad (juce::Colour (0xff1c2980), area.getX(), area.getY(),
                                   juce::Colour (0xff0f1754), area.getX(), area.getBottom(), false);
    g.setGradientFill (waveGrad);
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colour (0xff3b4fb5));
    g.drawRoundedRectangle (area, 4.0f, 1.5f);

    // Channel A and Channel B Outlined Boxes
    const float halfW = (area.getWidth() - 80.0f) / 2.0f;
    const float boxY = area.getY() + 45.0f;
    const float boxH = area.getHeight() - 75.0f;

    // Box A
    const juce::Rectangle<float> boxA (area.getX() + 35.0f, boxY, halfW, boxH);
    g.setColour (juce::Colour (0xff25359e));
    g.fillRoundedRectangle (boxA, 4.0f);
    g.setColour (juce::Colour (0xff6377e0));
    g.drawRoundedRectangle (boxA, 4.0f, 1.5f);

    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.setColour (juce::Colours::white);
    g.drawText ("A", (int) boxA.getX() + 12, (int) boxA.getY() + 8, 25, 25, juce::Justification::centred);

    // Box B
    const juce::Rectangle<float> boxB (area.getX() + 45.0f + halfW, boxY, halfW, boxH);
    g.setColour (juce::Colour (0xff25359e));
    g.fillRoundedRectangle (boxB, 4.0f);
    g.setColour (juce::Colour (0xff6377e0));
    g.drawRoundedRectangle (boxB, 4.0f, 1.5f);

    g.drawText ("B", (int) boxB.getX() + 12, (int) boxB.getY() + 8, 25, 25, juce::Justification::centred);
}

// 14. Precision Limiter (Brushed Light Gray Aluminum - Image 5 reference)
void FxFaceplateComponent::paintPrecisionLimiter (juce::Graphics& g, juce::Rectangle<float> area)
{
    juce::ColourGradient limGrad (juce::Colour (0xffd8dde4), area.getX(), area.getY(),
                                  juce::Colour (0xffb8bfc9), area.getX(), area.getBottom(), false);
    g.setGradientFill (limGrad);
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colour (0xff8a94a2));
    g.drawRoundedRectangle (area, 4.0f, 1.5f);

    g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xff1c222b));
    g.drawText ("STEREO PRECISION LIMITER", (int) area.getX() + 25, (int) area.getY() + 10, 300, 20, juce::Justification::left);

    // Red Brickwall LIMIT LED
    const float ledX = area.getRight() - 50.0f;
    const float ledY = area.getY() + 25.0f;
    const bool isClipping = (vuNeedleL > 0.85f);
    g.setColour (isClipping ? juce::Colour (0xffff1111) : juce::Colour (0xff440000));
    g.fillEllipse (ledX - 8.0f, ledY - 8.0f, 16.0f, 16.0f);
    g.setColour (isClipping ? juce::Colours::white : juce::Colour (0xff220000));
    g.fillEllipse (ledX - 3.0f, ledY - 3.0f, 6.0f, 6.0f);
}

// 15. Combinator (5-Band Rainbow Spectral Meters - Image 5 reference)
void FxFaceplateComponent::paintCombinator (juce::Graphics& g, juce::Rectangle<float> area)
{
    // White/Silver Left Faceplate
    juce::ColourGradient combGrad (juce::Colour (0xffe2e6ed), area.getX(), area.getY(),
                                   juce::Colour (0xffc5ccd6), area.getX(), area.getBottom(), false);
    g.setGradientFill (combGrad);
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colour (0xff828c9b));
    g.drawRoundedRectangle (area, 4.0f, 1.5f);

    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xff1b2028));
    g.drawText ("COMBINATOR", (int) area.getX() + 25, (int) area.getY() + 12, 200, 22, juce::Justification::left);

    // Right Side: 5 Vertical Rainbow Spectral LED Ladders
    const float startMeterX = area.getRight() - 250.0f;
    const float meterTopY = area.getY() + 35.0f;
    const float meterH = area.getHeight() - 75.0f;

    const juce::Colour rainbowColors[5] = {
        juce::Colour (0xffff8800), // Band 1: Orange
        juce::Colour (0xffffdd00), // Band 2: Yellow
        juce::Colour (0xff00d455), // Band 3: Green
        juce::Colour (0xff00b4ff), // Band 4: Cyan
        juce::Colour (0xffd822ff)  // Band 5: Magenta
    };
    const char* freqTags[5] = { "80", "250", "1k", "3k", "10k" };

    for (int b = 0; b < 5; ++b)
    {
        const float bx = startMeterX + b * 42.0f;
        // Dark track
        g.setColour (juce::Colour (0xff101317));
        g.fillRect (bx, meterTopY, 26.0f, meterH);

        // Active ladder height
        const float fillH = std::max (4.0f, combMeters[(size_t) b] * (meterH - 8.0f));
        g.setColour (rainbowColors[b]);
        g.fillRect (bx + 3.0f, meterTopY + meterH - 4.0f - fillH, 20.0f, fillH);

        // Frequency Label
        g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        g.setColour (juce::Colour (0xff323a48));
        g.drawText (freqTags[b], (int) bx, (int) (meterTopY + meterH + 2.0f), 26, 14, juce::Justification::centred);
    }
}

// 16. 31-Band ISO Graphic EQ (Image 4 reference)
void FxFaceplateComponent::paintGraphicEq (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& title)
{
    juce::ColourGradient geqGrad (juce::Colour (0xff1c2026), area.getX(), area.getY(),
                                  juce::Colour (0xff0f1114), area.getX(), area.getBottom(), false);
    g.setGradientFill (geqGrad);
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colour (0xff323a48));
    g.drawRoundedRectangle (area, 4.0f, 1.2f);

    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.setColour (juce::Colours::white);
    g.drawText (title.toUpperCase(), (int) area.getX() + 280, (int) area.getY() + 18, 300, 22, juce::Justification::left);

    // Center 0 dB Detent Reference Line across faders
    const float zeroY = area.getY() + 56.0f + (area.getHeight() - 98.0f) * 0.5f;
    g.setColour (juce::Colour (0x4400d4aa));
    g.drawHorizontalLine ((int) zeroY, area.getX() + 24.0f, area.getRight() - 24.0f);
}

// 17. Teletronix LA-2A
void FxFaceplateComponent::paintLA2A (juce::Graphics& g, juce::Rectangle<float> area)
{
    juce::ColourGradient silverGrad (juce::Colour (0xffe2e5eb), area.getX(), area.getY(),
                                     juce::Colour (0xffb8bec9), area.getX(), area.getBottom(), false);
    g.setGradientFill (silverGrad);
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colour (0xff7a8290));
    g.drawRoundedRectangle (area, 4.0f, 1.5f);

    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xff1c2026));
    g.drawText ("TELETRONIX", (int) area.getX() + 28, (int) area.getY() + 14, 200, 22, juce::Justification::left);

    const juce::Rectangle<float> meterBox (area.getCentreX() - 110.0f, area.getY() + 25.0f, 220.0f, 150.0f);
    drawAnalogVuMeter (g, meterBox, vuNeedleL, "GAIN REDUCTION");
}

// 18. Urei 1176 LN
void FxFaceplateComponent::paint1176 (juce::Graphics& g, juce::Rectangle<float> area)
{
    juce::ColourGradient blackGrad (juce::Colour (0xff1a1d22), area.getX(), area.getY(),
                                    juce::Colour (0xff0e1013), area.getX(), area.getBottom(), false);
    g.setGradientFill (blackGrad);
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colour (0xff323a48));
    g.drawRoundedRectangle (area, 4.0f, 1.5f);

    g.setColour (juce::Colour (0xff0088cc));
    g.fillRect (area.getX() + 24.0f, area.getY() + 14.0f, 6.0f, 40.0f);

    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.setColour (juce::Colours::white);
    g.drawText ("1176 LN", (int) area.getX() + 38, (int) area.getY() + 14, 180, 24, juce::Justification::left);

    const juce::Rectangle<float> meterBox (area.getRight() - 250.0f, area.getY() + 20.0f, 220.0f, 150.0f);
    drawAnalogVuMeter (g, meterBox, vuNeedleL, "GR / OUTPUT");
}

// 19. Fairchild 670
void FxFaceplateComponent::paintFairchild (juce::Graphics& g, juce::Rectangle<float> area)
{
    juce::ColourGradient fairGrad (juce::Colour (0xff353e37), area.getX(), area.getY(),
                                   juce::Colour (0xff202622), area.getX(), area.getBottom(), false);
    g.setGradientFill (fairGrad);
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colour (0xff4b594e));
    g.drawRoundedRectangle (area, 4.0f, 1.5f);

    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xffd8eedc));
    g.drawText ("FAIRCHILD 670", (int) area.getX() + 28, (int) area.getY() + 14, 250, 24, juce::Justification::left);

    const float meterW = 160.0f;
    const float meterH = 120.0f;
    const juce::Rectangle<float> meterL (area.getRight() - 360.0f, area.getY() + 20.0f, meterW, meterH);
    const juce::Rectangle<float> meterR (area.getRight() - 190.0f, area.getY() + 20.0f, meterW, meterH);
    drawAnalogVuMeter (g, meterL, vuNeedleL, "CH 1 / MID");
    drawAnalogVuMeter (g, meterR, vuNeedleR, "CH 2 / SIDE");
}

// 20. Lead 100 Guitar Amp
void FxFaceplateComponent::paintGuitarAmp (juce::Graphics& g, juce::Rectangle<float> area)
{
    juce::ColourGradient goldGrad (juce::Colour (0xffcca045), area.getX(), area.getY(),
                                   juce::Colour (0xff8c6a24), area.getX(), area.getBottom(), false);
    g.setGradientFill (goldGrad);
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colour (0xffffe082));
    g.drawRoundedRectangle (area, 4.0f, 1.5f);

    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xff221704));
    g.drawText ("LEAD 100", (int) area.getX() + 28, (int) area.getY() + 14, 180, 24, juce::Justification::left);

    const float jewelX = area.getRight() - 50.0f;
    const float jewelY = area.getY() + 28.0f;
    const float glow = 0.7f + 0.3f * std::sin (tubeGlowPhase);
    g.setColour (juce::Colour (0xffff2222).withMultipliedAlpha (glow));
    g.fillEllipse (jewelX - 10.0f, jewelY - 10.0f, 20.0f, 20.0f);
    g.setColour (juce::Colours::white);
    g.fillEllipse (jewelX - 3.0f, jewelY - 3.0f, 6.0f, 6.0f);
}

// 21. Tube Stage (12AX7 Valve Warmth)
void FxFaceplateComponent::paintTubeStage (juce::Graphics& g, juce::Rectangle<float> area)
{
    paintGenericStudioRack (g, area, "TUBE STAGE", "12AX7 DUAL TRIODE VALVE WARMTH & TAPE HARMONICS");

    const juce::Rectangle<float> tubeWindow (area.getCentreX() - 75.0f, area.getY() + 65.0f, 150.0f, 130.0f);
    drawVacuumTubeGlow (g, tubeWindow, 0.8f + 0.2f * std::sin (tubeGlowPhase));
}

// 22. Generic Studio Rack
void FxFaceplateComponent::paintGenericStudioRack (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& title, const juce::String& subTitle)
{
    juce::ColourGradient rackGrad (juce::Colour (0xff20242a), area.getX(), area.getY(),
                                   juce::Colour (0xff101215), area.getX(), area.getBottom(), false);
    g.setGradientFill (rackGrad);
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (ConsoleLookAndFeel::borderSubtle);
    g.drawRoundedRectangle (area, 4.0f, 1.0f);

    const juce::Rectangle<float> header (area.getX() + 16.0f, area.getY() + 12.0f,
                                         std::max (180.0f, area.getWidth() - 32.0f), 40.0f);
    g.setColour (juce::Colour (0xff080a0d));
    g.fillRoundedRectangle (header, 3.0f);
    g.setColour (juce::Colour (0xff303640));
    g.drawRoundedRectangle (header, 3.0f, 1.0f);
    g.setColour (ConsoleLookAndFeel::accentCyan);
    g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    g.drawText (title.toUpperCase(), (int) header.getX() + 12, (int) header.getY() + 3,
                (int) header.getWidth() - 230, 19, juce::Justification::left);
    g.setColour (ConsoleLookAndFeel::textMuted);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText (subTitle.toUpperCase(), (int) header.getX() + 12, (int) header.getY() + 22,
                (int) header.getWidth() - 230, 14, juce::Justification::left);

    const juce::Rectangle<float> meterBox (area.getRight() - 188.0f, area.getY() + 19.0f, 156.0f, 24.0f);
    g.setColour (juce::Colour (0xff090b0d));
    g.fillRoundedRectangle (meterBox, 2.0f);
    g.setColour (juce::Colour (0xff303640));
    g.drawRoundedRectangle (meterBox, 2.0f, 1.0f);
    const float meterWidth = (meterBox.getWidth() - 6.0f) * clampf (vuNeedleL, 0.0f, 1.0f);
    g.setColour (vuNeedleL > 0.88f ? ConsoleLookAndFeel::accentRed : ConsoleLookAndFeel::accentGreen);
    g.fillRoundedRectangle (meterBox.getX() + 3.0f, meterBox.getY() + 4.0f,
                            meterWidth, meterBox.getHeight() - 8.0f, 1.0f);
}

//==============================================================================
// Helpers
void FxFaceplateComponent::drawAnalogVuMeter (juce::Graphics& g, juce::Rectangle<float> bounds, float needlePos01, const juce::String& label)
{
    g.setColour (juce::Colour (0xff0a0c0e));
    g.fillRoundedRectangle (bounds, 5.0f);
    g.setColour (juce::Colour (0xff4a5465));
    g.drawRoundedRectangle (bounds, 5.0f, 1.2f);

    auto scaleArea = bounds.reduced (5.0f);
    juce::ColourGradient lampGrad (juce::Colour (0xfffff6d6), scaleArea.getCentreX(), scaleArea.getY(),
                                  juce::Colour (0xffe8dcaf), scaleArea.getCentreX(), scaleArea.getBottom(), false);
    g.setGradientFill (lampGrad);
    g.fillRoundedRectangle (scaleArea, 3.0f);

    const float pivotX = scaleArea.getCentreX();
    const float pivotY = scaleArea.getBottom() + 25.0f;
    const float needleLen = scaleArea.getHeight() + 12.0f;

    g.setColour (juce::Colour (0xff1a1d22));
    g.setFont (juce::FontOptions (8.0f, juce::Font::bold));
    const struct { float pos; const char* txt; } ticks[] = {
        { 0.0f, "-20" }, { 0.25f, "-10" }, { 0.5f, "-7" }, { 0.7f, "-3" }, { 0.85f, "0" }, { 1.0f, "+3" }
    };
    for (const auto& t : ticks)
    {
        const float angle = -0.55f + t.pos * 1.10f;
        const float tx = pivotX + std::sin (angle) * (needleLen - 12.0f);
        const float ty = pivotY - std::cos (angle) * (needleLen - 12.0f);
        g.drawText (t.txt, (int) tx - 10, (int) ty - 6, 20, 12, juce::Justification::centred);
    }

    const float needleAngle = -0.55f + needlePos01 * 1.10f;
    const float tipX = pivotX + std::sin (needleAngle) * needleLen;
    const float tipY = pivotY - std::cos (needleAngle) * needleLen;

    g.setColour (juce::Colour (0xffcc1111));
    g.drawLine (pivotX, pivotY - 15.0f, tipX, tipY, 1.8f);

    g.setColour (juce::Colour (0xff222222));
    g.fillEllipse (pivotX - 6.0f, scaleArea.getBottom() - 6.0f, 12.0f, 12.0f);

    g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
    g.setColour (juce::Colour (0xff333333));
    g.drawText (label, (int) scaleArea.getX(), (int) scaleArea.getY() + 4, (int) scaleArea.getWidth(), 12, juce::Justification::centred);
}

void FxFaceplateComponent::drawDigitalLcdDisplay (juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& mainText, const juce::String& subText, juce::Colour bgTint)
{
    g.setColour (juce::Colour (0xff0a0d12));
    g.fillRoundedRectangle (bounds, 5.0f);
    g.setColour (juce::Colour (0xff323c4a));
    g.drawRoundedRectangle (bounds, 5.0f, 1.2f);

    auto inner = bounds.reduced (3.0f);
    juce::ColourGradient lcdGrad (bgTint, inner.getX(), inner.getY(),
                                  bgTint.darker (0.4f), inner.getX(), inner.getBottom(), false);
    g.setGradientFill (lcdGrad);
    g.fillRoundedRectangle (inner, 3.0f);

    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xff55d8ff));
    g.drawText (mainText, (int) inner.getX() + 10, (int) inner.getY() + 5, (int) inner.getWidth() - 20, 20, juce::Justification::left);

    g.setFont (juce::FontOptions (10.0f));
    g.setColour (juce::Colour (0xff88e5ff));
    g.drawText (subText, (int) inner.getX() + 10, (int) inner.getY() + 27, (int) inner.getWidth() - 20, 16, juce::Justification::left);
}

void FxFaceplateComponent::drawVacuumTubeGlow (juce::Graphics& g, juce::Rectangle<float> bounds, float intensity)
{
    g.setColour (juce::Colour (0xff0a0806));
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (juce::Colour (0xff554838));
    g.drawRoundedRectangle (bounds, 8.0f, 2.0f);

    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float r = 45.0f * intensity;

    juce::ColourGradient tubeGrad (juce::Colour (0xffff9900).withMultipliedAlpha (intensity), cx, cy,
                                   juce::Colour (0x00ff4400), cx + r, cy + r, true);
    g.setGradientFill (tubeGrad);
    g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);

    g.setColour (juce::Colour (0xffffeeaa));
    g.drawLine (cx - 15.0f, cy + 20.0f, cx - 8.0f, cy - 20.0f, 2.0f);
    g.drawLine (cx - 8.0f, cy - 20.0f, cx, cy - 10.0f, 2.0f);
    g.drawLine (cx, cy - 10.0f, cx + 8.0f, cy - 20.0f, 2.0f);
    g.drawLine (cx + 8.0f, cy - 20.0f, cx + 15.0f, cy + 20.0f, 2.0f);
}

void FxFaceplateComponent::drawRackMountScrews (juce::Graphics& g, juce::Rectangle<float> area)
{
    const float screwMargin = 10.0f;
    const juce::Point<float> screws[4] = {
        { area.getX() + screwMargin, area.getY() + screwMargin },
        { area.getRight() - screwMargin, area.getY() + screwMargin },
        { area.getX() + screwMargin, area.getBottom() - screwMargin },
        { area.getRight() - screwMargin, area.getBottom() - screwMargin }
    };

    for (const auto& sc : screws)
    {
        g.setColour (juce::Colour (0xff111418));
        g.fillEllipse (sc.x - 5.0f, sc.y - 5.0f, 10.0f, 10.0f);
        g.setColour (juce::Colour (0xff6a7280));
        g.drawEllipse (sc.x - 5.0f, sc.y - 5.0f, 10.0f, 10.0f, 1.2f);
        g.drawLine (sc.x - 3.0f, sc.y, sc.x + 3.0f, sc.y, 1.2f);
    }
}

} // namespace mps
