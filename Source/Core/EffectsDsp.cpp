// Mix Practice Studio - Professional Console Effects DSP Implementation
#include "EffectsDsp.h"

namespace mps
{

//==============================================================================
// Retained for old source references while the public catalog follows X32-Mix.
[[maybe_unused]] static const FxMetadata kLegacyFxMetadataTable[41] = {
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

static const FxMetadata kFxMetadataTable[61] = {
    { FxType::HallReverb,              "Hall Reverb",              "HALL",       "Reverb",     FxCategory::Reverb, "Lexicon 480L Concert Hall algorithm with rich lush tail & spatial diffusion", true, true },
    { FxType::AmbienceReverb,          "Ambience Reverb",          "AMBI",       "Reverb",     FxCategory::Reverb, "Lexicon 480L Ambience room acoustic simulation for drums and percussion", true, true },
    { FxType::RichPlateReverb,         "Rich Plate Reverb",        "RICH PLATE", "Reverb",     FxCategory::Reverb, "EMT 140 / Lexicon Plate with dense metallic diffusion and fast build", true, true },
    { FxType::RoomReverb,              "Room Reverb",              "ROOM",       "Reverb",     FxCategory::Reverb, "Quantec QRS Acoustic Room Simulator with high reflection density", true, true },
    { FxType::ChamberReverb,           "Chamber Reverb",           "CHAMBER",    "Reverb",     FxCategory::Reverb, "Abbey Road / Capitol Studios acoustic echo chamber simulation", true, true },
    { FxType::PlateReverb,             "Plate Reverb",             "PLATE",      "Reverb",     FxCategory::Reverb, "Classic mechanical steel plate reverberation with bright dispersion", true, true },
    { FxType::VintageReverb,           "Vintage Reverb",           "VINTAGE",    "Reverb",     FxCategory::Reverb, "EMT 250 first electronic digital reverberator with 4.5kHz band-limiting", true, true },
    { FxType::VintageRoomReverb,       "Vintage Room Reverb",      "VINT ROOM",  "Reverb",     FxCategory::Reverb, "EMT 250 Space / Room acoustic simulation with tight reflections", true, true },
    { FxType::GatedReverb,             "Gated Reverb",             "GATED",      "Reverb",     FxCategory::Reverb, "1980s AMS RMX16 / SSL Non-Lin gated snare reverb with instant cut", true, true },
    { FxType::ReverseReverb,           "Reverse Reverb",           "REVERSE",    "Reverb",     FxCategory::Reverb, "Backward envelope swell reverberation with quadratic build-up", true, true },
    { FxType::StereoDelay,             "Stereo Delay",             "ST DELAY",   "Delay",      FxCategory::Delay, "TC Electronic 2290 dynamic stereo delay with cross-feedback & ping-pong", true, true },
    { FxType::TapDelay,                "Tap Delay",                "TAP",        "Delay",      FxCategory::Delay, "3-tap multi-position spatial delay (Left / Center / Right)", true, true },
    { FxType::RhythmDelay,             "Rhythm Delay",             "RHYTHM",     "Delay",      FxCategory::Delay, "Musical rhythmic multi-tap tempo sync pattern echo", true, true },
    { FxType::StereoChorus,            "Stereo Chorus",            "CHORUS",     "Modulation", FxCategory::Modulation, "TC Electronic stereo chorus with dual sine/tri LFO detune", true, true },
    { FxType::StereoFlanger,           "Stereo Flanger",           "FLANGER",    "Modulation", FxCategory::Modulation, "MXR / Eventide jet flanger with deep comb-filtering and regeneration", true, true },
    { FxType::StereoPhaser,            "Stereo Phaser",            "PHASER",     "Modulation", FxCategory::Modulation, "Electro-Harmonix Small Stone 4-stage optical allpass phase shifter", true, true },
    { FxType::DimensionC,              "Dimension-C",              "DIM-C",      "Modulation", FxCategory::Modulation, "Roland SDD-320 Dimension D analog spatial expander with dual BBD lines", true, true },
    { FxType::MoodFilter,              "Mood Filter",              "MOOD",       "Modulation", FxCategory::Modulation, "Moog MF-101 24dB 4-pole transistor ladder filter with envelope follower", true, true },
    { FxType::RotarySpeakerCabinet,    "Rotary Speaker",           "ROTARY",     "Modulation", FxCategory::Modulation, "Leslie 122/147 dual spinning horn & drum rotary cabinet with tube drive", true, true },
    { FxType::TremoloPanner,           "Tremolo Panner",           "TREMOLO",    "Modulation", FxCategory::Modulation, "Fender tube tremolo and auto-panning stereo modulation", true, true },
    { FxType::Suboctaver,              "Suboctaver",               "SUB OCT",    "Utility",    FxCategory::PremiumUtilityAmp, "Boss OC-2 analog dual flip-flop -1 octave sub-bass synthesizer", true, true },
    { FxType::DelayAndChamber,         "Delay & Chamber",          "DLY+CH",     "Delay",      FxCategory::Delay, "TC 2290 delay combined in series with acoustic chamber reverb", true, true },
    { FxType::ChorusAndChamber,        "Chorus & Chamber",         "CHO+CH",     "Modulation", FxCategory::Modulation, "Stereo chorus combined with acoustic chamber reverb", true, true },
    { FxType::FlangerAndChamber,       "Flanger & Chamber",        "FLG+CH",     "Modulation", FxCategory::Modulation, "Jet flanger combined with acoustic chamber reverb", true, true },
    { FxType::DelayAndChorus,          "Delay & Chorus",           "DLY+CHO",    "Delay",      FxCategory::Delay, "Stereo delay combined with wide multi-voice chorus", true, true },
    { FxType::DelayAndFlanger,         "Delay & Flanger",          "DLY+FLG",    "Delay",      FxCategory::Delay, "Stereo delay combined with jet regenerative flanger", true, true },
    { FxType::ModulationDelay,         "Modulation Delay",         "MOD DLY",    "Delay",      FxCategory::Delay, "Lexicon PCM-42 delay line with sinusoidal tape warble modulation", true, true },
    { FxType::DualGEQ,                 "Dual GEQ",                 "DUAL GEQ",  "Graphic EQ", FxCategory::GraphicEQ, "Klark Teknik DN360 dual 31-band proportional-Q graphic equalizer", true, true },
    { FxType::StereoGEQ,               "Stereo GEQ",               "ST GEQ",    "Graphic EQ", FxCategory::GraphicEQ, "Klark Teknik DN360 stereo 31-band proportional-Q graphic equalizer", true, true },
    { FxType::DualTruEQ,               "Dual Tru EQ",              "DUAL TRUEQ","Graphic EQ", FxCategory::GraphicEQ, "Klark Teknik DN9331 dual 31-band constant-Q true response equalizer", true, true },
    { FxType::StereoTruEQ,             "Stereo Tru EQ",            "ST TRUEQ", "Graphic EQ", FxCategory::GraphicEQ, "Klark Teknik DN9331 stereo 31-band constant-Q true response equalizer", true, true },
    { FxType::DualDeEsser,             "Dual DeEsser",             "DUAL DE-ESS","Dynamics", FxCategory::PremiumDynamics, "SPL dual-channel dynamic sibilance suppressor with phase cancellation", true, true },
    { FxType::StereoDeEsser,           "Stereo DeEsser",           "ST DE-ESS", "Dynamics",   FxCategory::PremiumDynamics, "SPL stereo dynamic vocal sibilance suppressor with phase cancellation", true, true },
    { FxType::StereoXtecEQ1,           "Stereo Xtec EQ1",          "ST XTEC 1", "Tube EQ",    FxCategory::PremiumTubeEq, "Pultec EQP-1A stereo passive tube program EQ with simultaneous boost/cut", true, true },
    { FxType::DualXtecEQ1,             "Dual Xtec EQ1",            "DUAL XTEC1","Tube EQ",    FxCategory::PremiumTubeEq, "Pultec EQP-1A dual mono passive tube program EQ with simultaneous boost/cut", true, true },
    { FxType::StereoXtecEQ5,           "Stereo Xtec EQ5",          "ST XTEC 5", "Tube EQ",    FxCategory::PremiumTubeEq, "Pultec MEQ-5 stereo passive tube midrange equalizer with peak/dip bands", true, true },
    { FxType::DualXtecEQ5,             "Dual Xtec EQ5",            "DUAL XTEC5","Tube EQ",    FxCategory::PremiumTubeEq, "Pultec MEQ-5 dual mono passive tube midrange equalizer with peak/dip bands", true, true },
    { FxType::WaveDesigner,            "Wave Designer",            "WAVE",      "Dynamics",   FxCategory::PremiumDynamics, "SPL Transient Designer attack and sustain envelope punch shaper", true, true },
    { FxType::StereoPrecisionLimiter,  "Stereo Precision Limiter", "LIMIT",     "Dynamics",   FxCategory::PremiumDynamics, "TC Electronic Finalizer transparent brickwall lookahead peak limiter", true, true },
    { FxType::Combinator,              "Combinator",               "COMBIN",    "Dynamics",   FxCategory::PremiumDynamics, "TC Electronic Finalizer 5-band broadcast multiband mastering compressor", true, true },
    { FxType::DualCombinator,          "Dual Combinator",          "DUAL COMB", "Dynamics",   FxCategory::PremiumDynamics, "TC Electronic Finalizer dual 5-band broadcast mastering compressor", true, true },
    { FxType::FairCompressor,          "Fair Comp",                "FAIR",      "Dynamics",   FxCategory::PremiumDynamics, "Fairchild 670 stereo Vari-Mu tube compressor with 6 time constants", true, true },
    { FxType::MsFairCompressor,        "M/S Fair Comp",            "M/S FAIR",  "Dynamics",   FxCategory::PremiumDynamics, "Fairchild 670 Mid/Side independent Vari-Mu tube compressor", true, true },
    { FxType::DualFairCompressor,      "Dual Fair Comp",           "DUAL FAIR", "Dynamics",   FxCategory::PremiumDynamics, "Fairchild 660 dual mono Vari-Mu tube compressor with 6 time constants", true, true },
    { FxType::StereoLeisureCompressor, "Leisure Comp",             "LEISURE",   "Dynamics",   FxCategory::PremiumDynamics, "Teletronix LA-2A stereo electro-optical T4 leveling amplifier", true, true },
    { FxType::DualLeisureCompressor,   "Dual Leisure Comp",        "DUAL LEIS", "Dynamics",   FxCategory::PremiumDynamics, "Teletronix LA-2A dual mono electro-optical T4 leveling amplifier", true, true },
    { FxType::UltimoCompressor,        "Ultimo Comp",              "ULTIMO",    "Dynamics",   FxCategory::PremiumDynamics, "Urei 1176LN stereo ultra-fast FET peak limiter with All-Buttons mode", true, true },
    { FxType::DualUltimoCompressor,    "Dual Ultimo Comp",         "DUAL ULT",  "Dynamics",   FxCategory::PremiumDynamics, "Urei 1176LN dual mono ultra-fast FET peak limiter with All-Buttons mode", true, true },
    { FxType::DualEnhancer,            "Dual Enhancer",            "DUAL ENH",  "Enhancer",   FxCategory::PremiumTubeEq, "BBE 882i dual mono Sonic Maximizer harmonic & phase alignment processor", true, true },
    { FxType::StereoEnhancer,          "Stereo Enhancer",          "ENHANCE",   "Enhancer",   FxCategory::PremiumTubeEq, "BBE 882i stereo Sonic Maximizer harmonic & phase alignment processor", true, true },
    { FxType::DualExciter,             "Dual Exciter",             "DUAL EXC",  "Enhancer",   FxCategory::PremiumTubeEq, "Aphex Aural Exciter Type C dual mono high-frequency harmonic sheen generator", true, true },
    { FxType::StereoExciter,           "Stereo Exciter",           "EXCITER",   "Enhancer",   FxCategory::PremiumTubeEq, "Aphex Aural Exciter Type C stereo high-frequency harmonic sheen generator", true, true },
    { FxType::StereoImager,            "Stereo Imager",            "IMAGER",    "Utility",    FxCategory::PremiumUtilityAmp, "SPL Vitalizer stereo field spatial expander and acoustic width enhancer", true, true },
    { FxType::EdisonEX1,               "Edison EX1",               "EDISON",    "Utility",    FxCategory::PremiumUtilityAmp, "Behringer Edison EX1 stereo spatial manipulator with mono-bass centering", true, true },
    { FxType::SoundMaxer,              "Sound Maxer",              "MAXER",     "Utility",    FxCategory::PremiumTubeEq, "BBE Sonic Maximizer dynamic contourer and high-frequency clarifier", true, true },
    { FxType::DualGuitarAmp,           "Dual GuitarAmp",           "DUAL AMP",  "Guitar Amp", FxCategory::PremiumUtilityAmp, "Marshall JCM800 / Fender Twin dual mono tube amp & 4x12 cab simulator", true, true },
    { FxType::StereoGuitarAmp,         "Stereo GuitarAmp",         "ST AMP",    "Guitar Amp", FxCategory::PremiumUtilityAmp, "Marshall JCM800 / Fender Twin stereo tube amp & 4x12 cab simulator", true, true },
    { FxType::DualTubeStage,           "Dual Tube Stage",          "DUAL TUBE", "Utility",    FxCategory::PremiumUtilityAmp, "Telefunken VF14 dual mono triode valve preamp saturation & warmth", true, true },
    { FxType::StereoTubeStage,         "Stereo Tube Stage",        "ST TUBE",   "Utility",    FxCategory::PremiumUtilityAmp, "Telefunken VF14 stereo triode valve preamp saturation & warmth", true, true },
    { FxType::DualPitch,               "Dual Pitch",               "DUAL PITCH","Pitch",      FxCategory::Pitch, "Eventide H3000 dual independent semitone pitch transposition harmonizer", true, true },
    { FxType::StereoPitch,             "Stereo Pitch",             "ST PITCH", "Pitch",      FxCategory::Pitch, "Eventide H3000 stereo micro-pitch detuner and semitone harmonizer", true, true },
};

const FxMetadata& getFxMetadata (FxType type) noexcept
{
    const int idx = (int) type;
    if (idx >= 0 && idx < 61) return kFxMetadataTable[idx];
    return kFxMetadataTable[0];
}

int getNumFxTypes() noexcept { return 61; }

bool usesIndependentFxParameters (FxType type) noexcept
{
    switch (type)
    {
        case FxType::DualXtecEQ1: case FxType::DualXtecEQ5:
        case FxType::DualExciter: case FxType::DualEnhancer:
        case FxType::DualGuitarAmp: case FxType::DualTubeStage:
        case FxType::DualLeisureCompressor: case FxType::DualUltimoCompressor:
        case FxType::DualFairCompressor: case FxType::DualCombinator: return true;
        default: return false;
    }
}

//==============================================================================
// FxSlotParams
FxSlotParams::FxSlotParams()
{
    parameterEncoding.init ({ 0.0f, 2.0f, 0.0f });
    rightParametersEnabled.init ({ 0.0f, 1.0f, 0.0f });
    for (auto& p : rightParameters) p.init ({ -24000.0f, 24000.0f, 0.5f });
    fxType.init     ({ 0.0f, 60.0f, 0.0f });
    bypass.init     ({ 0.0f, 1.0f, 0.0f }); // 0 = active
    insertMode.init ({ 0.0f, 1.0f, 0.0f }); // 0 = Send/Return, 1 = Insert
    mix.init        ({ 0.0f, 1.0f, 1.0f }); // 1.0 (100% wet for send/return)
    for (auto& t : insertTargets) t.init ({ 0.0f, 400.0f, 0.0f });

    p1.init ({ -24000.0f, 24000.0f, 50.0f });
    p2.init ({ -24000.0f, 24000.0f, 50.0f });
    p3.init ({ -24000.0f, 24000.0f, 50.0f });
    p4.init ({ -24000.0f, 24000.0f, 50.0f });
    p5.init ({ -24000.0f, 24000.0f, 50.0f });
    p6.init ({ -24000.0f, 24000.0f, 50.0f });
    p7.init ({ -24000.0f, 24000.0f, 50.0f });
    p8.init ({ -24000.0f, 24000.0f, 50.0f });
    p9.init ({ -24000.0f, 24000.0f, 50.0f });
    p10.init ({ -24000.0f, 24000.0f, 50.0f });
    p11.init ({ -24000.0f, 24000.0f, 50.0f });
    p12.init ({ -24000.0f, 24000.0f, 50.0f });
    p13.init ({ -24000.0f, 24000.0f, 50.0f });
    p14.init ({ -24000.0f, 24000.0f, 50.0f });
    p15.init ({ -24000.0f, 24000.0f, 50.0f });
    p16.init ({ -24000.0f, 24000.0f, 50.0f });
    p17.init ({ -24000.0f, 24000.0f, 50.0f });
    p18.init ({ -24000.0f, 24000.0f, 50.0f });

    for (auto& g : geqGainsL) g.init ({ -15.0f, 15.0f, 0.0f });
    for (auto& g : geqGainsR) g.init ({ -15.0f, 15.0f, 0.0f });
}

void FxSlotParams::setDefaultsForType (FxType type)
{
    parameterEncoding.set (1.0f);
    rightParametersEnabled.setBool (false);
    fxType.set ((float) (int) type);
    mix.set (1.0f);
    for (int i = 0; i < kNumParameters; ++i)
        parameter (i).resetToDefault();
    p7.set (20.0f);     // Default Lo Cut 20 Hz
    p8.set (18000.0f);  // Default Hi Cut 18 kHz
    p9.set (1.0f);      // Default Bass Mult 1.0x
    p10.set (100.0f);   // Default Spread 100%
    p11.set (25.0f);    // Default Spin 25%
    p12.set (30.0f);    // Default Mod Depth 30%
    p13.set (0.0f);     // Default Echo L 0 ms
    p14.set (0.0f);     // Default Echo R 0 ms
    p15.set (0.0f);     // Default Echo Feedback L 0%
    p16.set (0.0f);     // Default Echo Feedback R 0%
    for (auto& g : geqGainsL) g.set (0.0f);
    for (auto& g : geqGainsR) g.set (0.0f);
    switch (type)
    {
        case FxType::HallReverb:
            mix.set (1.0f); p1.set (2.5f);  p2.set (20.0f); p3.set (40.0f); p4.set (70.0f); p5.set (70.0f); p6.set (0.0f); break;
        case FxType::RichPlateReverb:
            mix.set (1.0f); p1.set (2.0f);  p2.set (10.0f); p3.set (25.0f); p4.set (80.0f); p5.set (75.0f); p6.set (0.0f); break;
        case FxType::RoomReverb:
            mix.set (1.0f); p1.set (1.2f);  p2.set (5.0f);  p3.set (35.0f); p4.set (50.0f); p5.set (65.0f); p6.set (0.0f); break;
        case FxType::ChamberReverb:
            mix.set (1.0f); p1.set (1.8f);  p2.set (15.0f); p3.set (30.0f); p4.set (65.0f); p5.set (70.0f); p6.set (0.0f); break;
        case FxType::PlateReverb:
            mix.set (1.0f); p1.set (1.8f);  p2.set (10.0f); p3.set (20.0f); p4.set (80.0f); p5.set (75.0f); p6.set (0.0f); break;
        case FxType::VintageReverb:
            mix.set (1.0f); p1.set (3.0f);  p2.set (30.0f); p3.set (50.0f); p4.set (65.0f); p5.set (65.0f); p6.set (0.0f); break;
        case FxType::VintageRoomReverb:
            mix.set (1.0f); p1.set (1.2f);  p2.set (5.0f);  p3.set (30.0f); p4.set (50.0f); p5.set (65.0f); p6.set (0.0f); break;
        case FxType::AmbienceReverb:
            mix.set (1.0f); p1.set (0.6f);  p2.set (0.0f);  p3.set (25.0f); p4.set (40.0f); p5.set (60.0f); p6.set (0.0f); break;
        case FxType::GatedReverb:
            mix.set (1.0f); p1.set (1.5f);  p2.set (120.0f);p3.set (20.0f); p4.set (50.0f); p5.set (70.0f); p6.set (0.0f); break;
        case FxType::ReverseReverb:
            mix.set (1.0f); p1.set (2.0f);  p2.set (250.0f);p3.set (30.0f); p4.set (60.0f); p5.set (70.0f); p6.set (0.0f); break;
        case FxType::StereoDelay:
        case FxType::ModulationDelay:
            mix.set (1.0f); p1.set (350.0f);p2.set (500.0f);p3.set (35.0f); p4.set (0.0f);  break;
        case FxType::TapDelay:
            mix.set (1.0f); p1.set (250.0f);p2.set (500.0f);p3.set (750.0f);p4.set (30.0f); break;
        case FxType::RhythmDelay:
            mix.set (1.0f); p1.set (200.0f);p2.set (400.0f);p3.set (600.0f);p4.set (40.0f); break;
        case FxType::StereoChorus:
        case FxType::ChorusAndChamber:
        case FxType::DelayAndChorus:
            mix.set (0.5f); p1.set (1.2f);  p2.set (60.0f); p3.set (20.0f); p4.set (10.0f); break;
        case FxType::DimensionC:
            mix.set (0.5f); p1.set (2.0f);  p2.set (50.0f); break; // Mode 2, 50% depth
        case FxType::StereoFlanger:
        case FxType::FlangerAndChamber:
        case FxType::DelayAndFlanger:
            mix.set (0.5f); p1.set (0.4f);  p2.set (70.0f); p3.set (60.0f); break;
        case FxType::StereoPhaser:
            mix.set (0.5f); p1.set (0.8f);  p2.set (75.0f); p3.set (50.0f); break;
        case FxType::MoodFilter:
            mix.set (0.6f); p1.set (2.5f);  p2.set (60.0f); p3.set (45.0f); break;
        case FxType::RotarySpeakerCabinet:
            mix.set (0.7f); p1.set (1.0f);  p2.set (80.0f); p3.set (0.0f);  break; // Slow rotor default
        case FxType::TremoloPanner:
            mix.set (0.5f); p1.set (3.0f);  p2.set (70.0f); break;
        case FxType::StereoPitch:
        case FxType::DualPitch:
            mix.set (0.5f); p1.set (0.0f);  break;
        case FxType::StereoGEQ:
        case FxType::DualGEQ:
        case FxType::StereoTruEQ:
        case FxType::DualTruEQ:
            mix.set (1.0f);
            for (auto& g : geqGainsL) g.set (0.0f);
            for (auto& g : geqGainsR) g.set (0.0f);
            break;
        case FxType::StereoLeisureCompressor:
        case FxType::DualLeisureCompressor:
            mix.set (1.0f); p1.set (40.0f); p2.set (40.0f); p3.set (0.0f); break; // Peak Reduction 40, Gain 40
        case FxType::UltimoCompressor:
        case FxType::DualUltimoCompressor:
            mix.set (1.0f); p1.set (30.0f); p2.set (20.0f); p3.set (4.0f); p4.set (20.0f); break; // Input, Output, Ratio 4:1, Attack
        case FxType::FairCompressor:
        case FxType::DualFairCompressor:
        case FxType::MsFairCompressor:
            mix.set (1.0f); p1.set (45.0f); p2.set (50.0f); p3.set (4.0f); p4.set (2.0f); break; // Pos 2 (0.5s release)
        case FxType::Combinator:
        case FxType::DualCombinator:
            mix.set (1.0f); p1.set (50.0f); p2.set (50.0f); break;
        case FxType::StereoPrecisionLimiter:
            mix.set (1.0f); p1.set (0.0f);  p2.set (0.0f);  p3.set (250.0f);p4.set (5.0f); p5.set (0.0f); p6.set (50.0f); break;
        case FxType::WaveDesigner:
            mix.set (1.0f); p1.set (0.0f); p2.set (0.0f); p3.set (0.0f); p4.set (0.0f); p5.set (0.0f); p6.set (0.0f); break; // Neutral transient shaper
        case FxType::StereoDeEsser:
        case FxType::DualDeEsser:
            mix.set (1.0f); p1.set (45.0f); break;
        case FxType::StereoXtecEQ1:
        case FxType::DualXtecEQ1:
            mix.set (1.0f); p1.set (3.0f);  p2.set (60.0f); p3.set (2.0f); p4.set (2.0f); p5.set (3.0f); p6.set (8000.0f); break;
        case FxType::StereoXtecEQ5:
        case FxType::DualXtecEQ5:
            mix.set (1.0f); p1.set (2.0f);  p2.set (500.0f);p3.set (2.0f); p4.set (1500.0f); p5.set (2.0f); p6.set (3000.0f); break;
        case FxType::StereoExciter:
        case FxType::DualExciter:
            mix.set (0.35f);p1.set (30.0f); break;
        case FxType::SoundMaxer:
        case FxType::StereoEnhancer:
        case FxType::DualEnhancer:
            mix.set (1.0f); p1.set (20.0f); p2.set (20.0f); p3.set (0.0f); break;
        case FxType::StereoGuitarAmp:
        case FxType::DualGuitarAmp:
            mix.set (1.0f); p1.set (45.0f); p2.set (50.0f); p3.set (50.0f); p4.set (50.0f); break; // Drive, Bass, Mid, Treble
        case FxType::StereoTubeStage:
        case FxType::DualTubeStage:
            mix.set (1.0f); p1.set (30.0f); p2.set (0.0f); p3.set (0.0f); p4.set (0.0f); p5.set (0.0f); p6.set (0.0f); break;
        case FxType::Suboctaver:
            mix.set (0.4f); p1.set (80.0f); break; // Lowpass 80Hz
        case FxType::EdisonEX1:
        case FxType::StereoImager:
            mix.set (1.0f); p1.set (50.0f); break; // 100% normal width
        default:
            mix.set (0.5f); p1.set (50.0f); p2.set (50.0f); break;
    }
}

static inline float readFractionalRing (const std::vector<float>& buf, int writePos, float delaySamples) noexcept
{
    const int maxLen = (int) buf.size();
    if (maxLen <= 1) return 0.0f;
    const float clamped = clampf (delaySamples, 1.0f, (float) (maxLen - 2));
    const int intDelay = (int) clamped;
    const float frac = clamped - (float) intDelay;
    int idx0 = writePos - intDelay;
    if (idx0 < 0) idx0 += maxLen;
    int idx1 = idx0 - 1;
    if (idx1 < 0) idx1 += maxLen;
    return buf[(size_t) idx0] + frac * (buf[(size_t) idx1] - buf[(size_t) idx0]);
}

//==============================================================================
// ReverbDsp Implementation
void ReverbDsp::prepare (double sampleRate)
{
    sr = (float) sampleRate;
    const int preDelayLen = (int) (sr * 0.35f); // 350 ms predelay max
    preDelayBufL.assign ((size_t) preDelayLen, 0.0f);
    preDelayBufR.assign ((size_t) preDelayLen, 0.0f);

    const int echoLen = (int) (sr * 0.6f); // 600 ms LARC Page 3 pre-echo max
    echoBufL.assign ((size_t) echoLen, 0.0f);
    echoBufR.assign ((size_t) echoLen, 0.0f);

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

    const int revLen = (int) (sr * 3.0f);
    reverseBufL.assign ((size_t) revLen, 0.0f);
    reverseBufR.assign ((size_t) revLen, 0.0f);

    reset();
}

void ReverbDsp::reset()
{
    std::fill (preDelayBufL.begin(), preDelayBufL.end(), 0.0f);
    std::fill (preDelayBufR.begin(), preDelayBufR.end(), 0.0f);
    preDelayWritePos = 0;

    std::fill (echoBufL.begin(), echoBufL.end(), 0.0f);
    std::fill (echoBufR.begin(), echoBufR.end(), 0.0f);
    echoWritePos = 0;

    for (int i = 0; i < kNumCombs; ++i)
    {
        std::fill (combBufL[i].begin(), combBufL[i].end(), 0.0f);
        std::fill (combBufR[i].begin(), combBufR[i].end(), 0.0f);
        combPosL[i] = combPosR[i] = 0;
        combFilterStateL[i] = combFilterStateR[i] = 0.0f;
        combBassL[i] = combBassR[i] = 0.0f;
    }

    for (int i = 0; i < kNumAllPasses; ++i)
    {
        std::fill (allpassBufL[i].begin(), allpassBufL[i].end(), 0.0f);
        std::fill (allpassBufR[i].begin(), allpassBufR[i].end(), 0.0f);
        allpassPosL[i] = allpassPosR[i] = 0;
    }

    std::fill (reverseBufL.begin(), reverseBufL.end(), 0.0f);
    std::fill (reverseBufR.begin(), reverseBufR.end(), 0.0f);
    reverseWrite = 0;
    emtLpfL.reset(); emtLpfR.reset();
    plateHpfL.reset(); plateHpfR.reset();
    loCutFiltL.reset(); loCutFiltR.reset();
    hiCutFiltL.reset(); hiCutFiltR.reset();
    bassShelfL.reset(); bassShelfR.reset();

    lfoPhase = 0.0f;
    gateCounter = 0;
}

void ReverbDsp::process (float* chL, float* chR, int numSamples,
                         FxType type, float decaySec, float preDelayMs,
                         float dampingNorm, float roomSizeNorm, float diffusionNorm,
                         float levelDb, float mixNorm,
                         float loCutHz, float hiCutHz,
                         float bassMult, float spreadNorm,
                         float spinNorm,
                         float echoLMs, float echoRMs,
                         float echoFeedLNorm, float echoFeedRNorm, float attackNorm)
{
    const int preDelaySamples = (int) clampf ((preDelayMs * 0.001f * sr), 0.0f, (float) preDelayBufL.size() - 1);
    // These are distinct algorithmic spaces, not measured hardware replicas.
    float geometry = 1.0f, stereoBleed = 0.18f;
    int diffuserCount = 4;
    switch (type)
    {
        case FxType::AmbienceReverb: geometry = 0.37f; diffuserCount = 2; stereoBleed = 0.36f; break;
        case FxType::RoomReverb: geometry = 0.61f; diffuserCount = 3; stereoBleed = 0.25f; break;
        case FxType::ChamberReverb: geometry = 0.83f; stereoBleed = 0.32f; break;
        case FxType::RichPlateReverb: geometry = 0.91f; stereoBleed = 0.45f; break;
        case FxType::PlateReverb: geometry = 0.73f; stereoBleed = 0.40f; break;
        case FxType::VintageRoomReverb: geometry = 0.54f; diffuserCount = 3; break;
        default: break;
    }
    const float bassPole = std::exp (-2.0f * kPi * 220.0f / sr);
    const float dampCoeff = clampf (dampingNorm * 0.01f, 0.05f, 0.9f);
    const float diffusionCoeff = clampf (diffusionNorm * 0.0095f, 0.0f, 0.95f);
    const float roomScale = geometry * (0.35f + 0.65f * clampf (roomSizeNorm * 0.01f, 0.0f, 1.0f));
    const float wetGain = mixNorm * dbToGain (clampf (levelDb, -40.0f, 10.0f));
    const float dryGain = 1.0f - clampf (mixNorm, 0.0f, 1.0f);

    const bool isGate = (type == FxType::GatedReverb);
    const bool isReverse = (type == FxType::ReverseReverb);
    const bool isVintage = (type == FxType::VintageReverb || type == FxType::VintageRoomReverb);
    const bool isPlate = (type == FxType::PlateReverb || type == FxType::RichPlateReverb);
    const int gateHoldSamples = (int) (std::max (0.04f, decaySec * 0.12f) * sr);

    const bool useLoCut = (loCutHz > 22.0f);
    const bool useHiCut = (hiCutHz < 17500.0f);

    const float spreadWidth = clampf (spreadNorm * 0.01f, 0.0f, 1.5f);
    const float spinHz = 0.15f + clampf (spinNorm * 0.01f, 0.0f, 1.0f) * 1.35f;

    const int echoMax = (int) echoBufL.size();
    const int echoSamplesL = (echoMax > 2 && echoLMs > 1.0f) ? (int) clampf (echoLMs * 0.001f * sr, 1.0f, (float) (echoMax - 1)) : 0;
    const int echoSamplesR = (echoMax > 2 && echoRMs > 1.0f) ? (int) clampf (echoRMs * 0.001f * sr, 1.0f, (float) (echoMax - 1)) : 0;
    const float echoFbL = clampf (echoFeedLNorm * 0.0085f, 0.0f, 0.85f);
    const float echoFbR = clampf (echoFeedRNorm * 0.0085f, 0.0f, 0.85f);

    // Colour filters for the algorithmic approximations
    const auto emtCoeffs = calcLPF (4500.0f, 0.707f, sr);  // Vintage colour (second-order low-pass, not a brickwall)
    const auto plateCoeffs = calcHPF (100.0f, 0.707f, sr); // Plate 100Hz rumble cut
    const auto loCutCoeffs = calcHPF (clampf (loCutHz, 20.0f, 1000.0f), 0.707f, sr);
    const auto hiCutCoeffs = calcLPF (clampf (hiCutHz, 800.0f, 20000.0f), 0.707f, sr);


    float loopGainL[kNumCombs], loopGainR[kNumCombs], bassGainL[kNumCombs], bassGainR[kNumCombs];
    for (int c = 0; c < kNumCombs; ++c)
    {
        const float lenL = std::max (1.0f, (float) combBufL[c].size() * roomScale);
        const float lenR = std::max (1.0f, (float) combBufR[c].size() * roomScale);
        loopGainL[c] = std::exp (-6.907755f * lenL / (sr * std::max (0.1f, decaySec)));
        loopGainR[c] = std::exp (-6.907755f * lenR / (sr * std::max (0.1f, decaySec)));
        bassGainL[c] = std::exp (-6.907755f * lenL / (sr * std::max (0.1f, decaySec * bassMult)));
        bassGainR[c] = std::exp (-6.907755f * lenR / (sr * std::max (0.1f, decaySec * bassMult)));
    }

    for (int i = 0; i < numSamples; ++i)
    {
        const float inL = chL[i];
        const float inR = chR[i];

        // A causal reverse reverb builds reflection energy toward the end of
        // its window. Read every tap at normal speed: never transpose the input.
        if (isReverse)
        {
            reverseBufL[(size_t) reverseWrite] = inL;
            reverseBufR[(size_t) reverseWrite] = inR;
            const int capacity = (int) reverseBufL.size();
            const float window = std::min (decaySec * sr, (float) capacity - preDelaySamples - 2.0f);
            float revL = 0.0f, revR = 0.0f;
            for (int tap = 1; tap <= 32; ++tap)
            {
                const float phase = (float) tap / 33.0f;
                const int offset = preDelaySamples + std::max (1, (int) (window * phase));
                const int read = (reverseWrite - offset + capacity) % capacity;
                const float rise = 1.0f + clampf(attackNorm * .01f,0.0f,1.0f) * 4.0f;
                const float weight = std::pow (phase, rise) * (rise + 1.0f) / 32.0f;
                revL += reverseBufL[(size_t) read] * weight;
                revR += reverseBufR[(size_t) read] * weight;
            }
            if (useLoCut) { revL = loCutFiltL.process (revL, loCutCoeffs); revR = loCutFiltR.process (revR, loCutCoeffs); }
            if (useHiCut) { revL = hiCutFiltL.process (revL, hiCutCoeffs); revR = hiCutFiltR.process (revR, hiCutCoeffs); }
            if (++reverseWrite >= capacity) reverseWrite = 0;
            chL[i] = inL * dryGain + revL * wetGain;
            chR[i] = inR * dryGain + revR * wetGain;
            continue;
        }

        // 1. Pre-delay
        preDelayBufL[(size_t) preDelayWritePos] = inL;
        preDelayBufR[(size_t) preDelayWritePos] = inR;

        int readIdx = preDelayWritePos - preDelaySamples;
        if (readIdx < 0) readIdx += (int) preDelayBufL.size();

        float pdInL = preDelayBufL[(size_t) readIdx];
        float pdInR = preDelayBufR[(size_t) readIdx];

        if (attackNorm > 0.0f)
        {
            float smearL = 0.0f, smearR = 0.0f;
            for (int tap = 0; tap < 8; ++tap)
            {
                const float offset = preDelaySamples + attackNorm * .001f * sr * (float) tap / 7.0f;
                const float weight = (float)(tap + 1) / 36.0f;
                smearL += readFractionalRing(preDelayBufL,preDelayWritePos,offset + 1.0f) * weight;
                smearR += readFractionalRing(preDelayBufR,preDelayWritePos,offset + 1.0f) * weight;
            }
            pdInL = smearL; pdInR = smearR;
        }
        if (++preDelayWritePos >= (int) preDelayBufL.size())
            preDelayWritePos = 0;

        // 1b. Optional LARC Page 3 pre-echo reflections
        if ((echoSamplesL > 0 || echoSamplesR > 0) && echoMax > 0)
        {
            int erL = echoWritePos - echoSamplesL;
            if (erL < 0) erL += echoMax;
            int erR = echoWritePos - echoSamplesR;
            if (erR < 0) erR += echoMax;

            const float eOutL = (echoSamplesL > 0) ? echoBufL[(size_t) erL] : 0.0f;
            const float eOutR = (echoSamplesR > 0) ? echoBufR[(size_t) erR] : 0.0f;

            echoBufL[(size_t) echoWritePos] = pdInL + eOutL * echoFbL;
            echoBufR[(size_t) echoWritePos] = pdInR + eOutR * echoFbR;
            if (++echoWritePos >= echoMax) echoWritePos = 0;

            pdInL += eOutL * 0.35f;
            pdInR += eOutR * 0.35f;
        }

        // Vintage EMT 250 / LARC Spin delay modulation to prevent metallic flutter
        if (isVintage || spinNorm > 1.0f)
        {
            lfoPhase += (spinHz * 2.0f * kPi) / sr;
            if (lfoPhase >= 2.0f * kPi) lfoPhase -= 2.0f * kPi;
        }

        // 2. Parallel Comb Filters
        float sumCombL = 0.0f;
        float sumCombR = 0.0f;
        const float modExcursion = isVintage ? 3.0f : (spinNorm * 0.025f);

        for (int c = 0; c < kNumCombs; ++c)
        {
            // Left Comb
            auto& bufL = combBufL[c];
            float delayL = std::max (1.0f, (float) bufL.size() * roomScale);
            if (c % 2 == 0) delayL += std::sin (lfoPhase + (float)c) * modExcursion;
            const float outCombL = readFractionalRing (bufL, combPosL[c], delayL);
            combFilterStateL[c] = outCombL * (1.0f - dampCoeff) + combFilterStateL[c] * dampCoeff;
            combBassL[c] = (1.0f - bassPole) * combFilterStateL[c] + bassPole * combBassL[c];
            bufL[(size_t) combPosL[c]] = pdInL * (1.0f - stereoBleed) + pdInR * stereoBleed
                                      + combFilterStateL[c] * loopGainL[c] + combBassL[c] * (bassGainL[c] - loopGainL[c]);
            if (++combPosL[c] >= (int) bufL.size()) combPosL[c] = 0;
            sumCombL += outCombL;

            // Right Comb
            auto& bufR = combBufR[c];
            float delayR = std::max (1.0f, (float) bufR.size() * roomScale);
            if (c % 2 != 0) delayR += std::cos (lfoPhase + (float)c) * modExcursion;
            const float outCombR = readFractionalRing (bufR, combPosR[c], delayR);
            combFilterStateR[c] = outCombR * (1.0f - dampCoeff) + combFilterStateR[c] * dampCoeff;
            combBassR[c] = (1.0f - bassPole) * combFilterStateR[c] + bassPole * combBassR[c];
            bufR[(size_t) combPosR[c]] = pdInR * (1.0f - stereoBleed) + pdInL * stereoBleed
                                      + combFilterStateR[c] * loopGainR[c] + combBassR[c] * (bassGainR[c] - loopGainR[c]);
            if (++combPosR[c] >= (int) bufR.size()) combPosR[c] = 0;
            sumCombR += outCombR;
        }

        sumCombL *= 0.125f;
        sumCombR *= 0.125f;

        // 3. Series All-Pass Diffusers
        float apL = sumCombL;
        float apR = sumCombR;

        for (int a = 0; a < diffuserCount; ++a)
        {
            auto& bufL = allpassBufL[a];
            const float bufOutL = bufL[(size_t) allpassPosL[a]];
            const float outL = -diffusionCoeff * apL + bufOutL;
            bufL[(size_t) allpassPosL[a]] = apL + diffusionCoeff * outL;
            apL = outL;
            if (++allpassPosL[a] >= (int) bufL.size()) allpassPosL[a] = 0;

            auto& bufR = allpassBufR[a];
            const float bufOutR = bufR[(size_t) allpassPosR[a]];
            const float outR = -diffusionCoeff * apR + bufOutR;
            bufR[(size_t) allpassPosR[a]] = apR + diffusionCoeff * outR;
            apR = outR;
            if (++allpassPosR[a] >= (int) bufR.size()) allpassPosR[a] = 0;
        }

        // Apply hardware brand & tone filters
        if (isVintage)
        {
            apL = emtLpfL.process (apL, emtCoeffs);
            apR = emtLpfR.process (apR, emtCoeffs);
        }
        else if (isPlate)
        {
            apL = plateHpfL.process (apL, plateCoeffs);
            apR = plateHpfR.process (apR, plateCoeffs);
        }

        if (useLoCut)
        {
            apL = loCutFiltL.process (apL, loCutCoeffs);
            apR = loCutFiltR.process (apR, loCutCoeffs);
        }
        if (useHiCut)
        {
            apL = hiCutFiltL.process (apL, hiCutCoeffs);
            apR = hiCutFiltR.process (apR, hiCutCoeffs);
        }

        // Stereo Spread adjustment
        if (std::abs (spreadWidth - 1.0f) > 0.02f)
        {
            const float mid = 0.5f * (apL + apR);
            const float side = 0.5f * (apL - apR) * spreadWidth;
            apL = mid + side;
            apR = mid - side;
        }

        // 4. Gate / Envelope check (AMS RMX16 / Phil Collins gated snare)
        if (isGate)
        {
            const float level = std::max (std::abs (inL), std::abs (inR));
            if (level > 0.04f) gateCounter = gateHoldSamples;
            if (gateCounter > 0)
            {
                --gateCounter;
                const int releaseSamples = (int) (0.015f * sr); // 15ms quick release
                if (gateCounter < releaseSamples)
                {
                    const float fade = (float) gateCounter / (float) releaseSamples;
                    apL *= fade;
                    apR *= fade;
                }
            }
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
    loCutL.reset(); loCutR.reset();
    hiCutL.reset(); hiCutR.reset();
}

void DelayDsp::process (float* chL, float* chR, int numSamples,
                        FxType type, float timeLMs, float timeRMs,
                        float feedbackNorm, float crossMix, float highDamp, float mixNorm,
                        float loCutHz, float hiCutHz)
{
    const int maxLen = (int) delayBufL.size();
    const int dSamplesL = (int) clampf (timeLMs * 0.001f * sr, 1.0f, (float) (maxLen - 1));
    const int dSamplesR = (int) clampf (timeRMs * 0.001f * sr, 1.0f, (float) (maxLen - 1));
    const float fdbk = clampf (feedbackNorm * 0.01f, 0.0f, 0.95f);
    const float damp = clampf (highDamp * 0.01f, 0.0f, 0.8f);
    const float wet = clampf (mixNorm, 0.0f, 1.0f);
    const float dry = 1.0f - wet;

    const bool useLoCut = (loCutHz > 22.0f);
    const bool useHiCut = (hiCutHz < 17500.0f);
    const auto cLo = calcHPF (clampf (loCutHz, 20.0f, 2000.0f), 0.707f, sr);
    const auto cHi = calcLPF (clampf (hiCutHz, 800.0f, 20000.0f), 0.707f, sr);

    for (int i = 0; i < numSamples; ++i)
    {
        const float inL = chL[i];
        const float inR = chR[i];

        int rIdxL = writePos - dSamplesL;
        if (rIdxL < 0) rIdxL += maxLen;
        int rIdxR = writePos - dSamplesR;
        if (rIdxR < 0) rIdxR += maxLen;

        float delayedL = delayBufL[(size_t) rIdxL];
        float delayedR = delayBufR[(size_t) rIdxR];

        if (useLoCut)
        {
            delayedL = loCutL.process (delayedL, cLo);
            delayedR = loCutR.process (delayedR, cLo);
        }
        if (useHiCut)
        {
            delayedL = hiCutL.process (delayedL, cHi);
            delayedR = hiCutR.process (delayedR, cHi);
        }

        // Damp filter
        lpStateL = delayedL * (1.0f - damp) + lpStateL * damp;
        lpStateR = delayedR * (1.0f - damp) + lpStateR * damp;

        // Feedback into buffer
        if (type == FxType::TapDelay)
        {
            // 3-Tap delay: Tap 1 left, Tap 2 center, Tap 3 right
            int tap2Idx = writePos - (dSamplesL + dSamplesR) / 2;
            if (tap2Idx < 0) tap2Idx += maxLen;
            const float tap2 = 0.5f * (delayBufL[(size_t) tap2Idx] + delayBufR[(size_t) tap2Idx]) * 0.5f;

            delayBufL[(size_t) writePos] = inL + lpStateL * fdbk;
            delayBufR[(size_t) writePos] = inR + lpStateR * fdbk;

            chL[i] = inL * dry + (delayedL + tap2) * wet;
            chR[i] = inR * dry + (delayedR + tap2) * wet;
        }
        else if (type == FxType::RhythmDelay)
        {
            // 4-Tap rhythmic pattern delay (1/4, 2/4, 3/4, 4/4 taps)
            int tapAIdx = writePos - std::max (1, dSamplesL / 2);
            if (tapAIdx < 0) tapAIdx += maxLen;
            int tapBIdx = writePos - std::max (1, (dSamplesL * 3) / 4);
            if (tapBIdx < 0) tapBIdx += maxLen;

            const float tapA = delayBufL[(size_t) tapAIdx] * 0.55f;
            const float tapB = delayBufR[(size_t) tapBIdx] * 0.45f;

            delayBufL[(size_t) writePos] = inL + lpStateL * fdbk;
            delayBufR[(size_t) writePos] = inR + lpStateR * fdbk;

            chL[i] = inL * dry + (delayedL + tapA) * wet;
            chR[i] = inR * dry + (delayedR + tapB) * wet;
        }
        else
        {
            // Ping-pong or cross-feedback stereo delay
            const float xMix = clampf (crossMix > 1.0f ? crossMix * 0.01f : crossMix, 0.0f, 1.0f);
            const float fbL = lpStateL * (1.0f - xMix) + lpStateR * xMix;
            const float fbR = lpStateR * (1.0f - xMix) + lpStateL * xMix;

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
    const int maxLen = (int) (sr * 0.06f); // 60 ms max modulation delay
    modDelayBufL.assign ((size_t) maxLen, 0.0f);
    modDelayBufR.assign ((size_t) maxLen, 0.0f);

    const int dimLen = (int) (sr * 0.06f);
    dimBufL.assign ((size_t) dimLen, 0.0f);
    dimBufR.assign ((size_t) dimLen, 0.0f);

    reset();
}

void ModulationDsp::reset()
{
    std::fill (modDelayBufL.begin(), modDelayBufL.end(), 0.0f);
    std::fill (modDelayBufR.begin(), modDelayBufR.end(), 0.0f);
    modWritePos = 0;
    lfoPhase = 0.0f;

    std::fill (dimBufL.begin(), dimBufL.end(), 0.0f);
    std::fill (dimBufR.begin(), dimBufR.end(), 0.0f);
    dimWrite = 0;
    dimPreL.reset(); dimPreR.reset();
    dimPostL.reset(); dimPostR.reset();

    rotorPhaseHorn = rotorPhaseDrum = 0.0f;
    rotorSpeedHorn = 0.4f; rotorSpeedDrum = 0.3f;
    hornHpfL.reset(); hornHpfR.reset();
    drumLpfL.reset(); drumLpfR.reset();

    for (int i = 0; i < 4; ++i) moogStageL[i] = moogStageR[i] = 0.0f;
    moodEnv = 0.0f;

    std::fill (std::begin (phaserZ1L), std::end (phaserZ1L), 0.0f);
    std::fill (std::begin (phaserZ1R), std::end (phaserZ1R), 0.0f);
    phaserFbL = phaserFbR = 0.0f;
    moodBiquadL.reset();
    moodBiquadR.reset();
}


void ModulationDsp::process (float* chL, float* chR, int numSamples,
                             FxType type, float speedHz, float depthNorm,
                             float feedbackNorm, float paramCustom, float mixNorm, float modeNorm, float widthR, float delayLMs, float delayRMs, float phaseDeg, float acceleration)
{
    const int maxLen = (int) modDelayBufL.size();
    const float wet = clampf (mixNorm, 0.0f, 1.0f);
    const float dry = 1.0f - wet;

    // 1. Roland SDD-320 Dimension D Modeling
    if (type == FxType::DimensionC)
    {
        const float dimLfoInc = (0.25f * 2.0f * kPi) / sr;
        const auto cPre = calcHighShelf (2500.0f, 3.5f, 0.707f, sr);
        const auto cPost = calcHighShelf (2500.0f, -3.5f, 0.707f, sr);
        const float depthMs = 0.4f + clampf (depthNorm * 0.01f, 0.0f, 1.5f) * 1.2f; // Mode 1..4 depth
        const int dimMax = (int) dimBufL.size();

        for (int i = 0; i < numSamples; ++i)
        {
            lfoPhase += dimLfoInc;
            if (lfoPhase >= 2.0f * kPi) lfoPhase -= 2.0f * kPi;

            // Pre-emphasis high boost (+3.5 dB)
            const float preInL = dimPreL.process (chL[i], cPre);
            const float preInR = dimPreR.process (chR[i], cPre);

            dimBufL[(size_t) dimWrite] = preInL;
            dimBufR[(size_t) dimWrite] = preInR;

            // BBD delay with inverted LFO modulation (+sin on Left, -sin on Right)
            const float modL = std::sin (lfoPhase);
            const float modR = -std::sin (lfoPhase);
            const float dSamplesL = (2.5f + depthMs * modL) * 0.001f * sr;
            const float dSamplesR = (2.5f + depthMs * modR) * 0.001f * sr;

            const float bbdL = readFractionalRing (dimBufL, dimWrite, dSamplesL);
            const float bbdR = readFractionalRing (dimBufR, dimWrite, dSamplesR);

            if (++dimWrite >= dimMax) dimWrite = 0;

            // Antiphase cross-bleed creates legendary spatial widening without vibrato pitch flutter
            const float bleed = paramCustom > 0.5f ? 0.35f : 0.0f;
            const float crossL = bbdL - bleed * bbdR;
            const float crossR = bbdR - bleed * bbdL;

            // Post-emphasis high cut (-3.5 dB) restores tonal balance
            const float outDimL = dimPostL.process (crossL, cPost);
            const float outDimR = dimPostR.process (crossR, cPost);

            chL[i] = chL[i] * dry + outDimL * wet;
            chR[i] = chR[i] * dry + outDimR * wet;
        }
        return;
    }

    // 2. Leslie 122/147 Rotary Speaker Modeling
    if (type == FxType::RotarySpeakerCabinet)
    {
        // speedHz = horn speed (or slow/fast switch), depthNorm = balance/depth
        const bool isFast = modeNorm > 0.5f;
        const float targetHornHz = isFast ? std::max (0.4f, paramCustom) : std::max (0.4f, speedHz);
        const float targetDrumHz = isFast ? 5.6f : std::max (0.3f, speedHz * 0.75f);
        const float slew = 1.0f - std::exp (-1.0f / (sr * (0.05f + acceleration * 3.0f)));
        const auto cHorn = calcHPF (800.0f, 0.707f, sr); // Horn gets highs (>800 Hz)
        const auto cDrum = calcLPF (800.0f, 0.707f, sr); // Drum gets lows (<800 Hz)
        const float hornBal = clampf (0.3f + (depthNorm * 0.01f) * 0.7f, 0.2f, 1.2f);

        for (int i = 0; i < numSamples; ++i)
        {
            rotorSpeedHorn += (targetHornHz - rotorSpeedHorn) * slew;
            rotorSpeedDrum += (targetDrumHz - rotorSpeedDrum) * slew * 0.6f;
            rotorPhaseHorn += rotorSpeedHorn * 2.0f * kPi / sr;
            if (rotorPhaseHorn >= 2.0f * kPi) rotorPhaseHorn -= 2.0f * kPi;
            rotorPhaseDrum += rotorSpeedDrum * 2.0f * kPi / sr;
            if (rotorPhaseDrum >= 2.0f * kPi) rotorPhaseDrum -= 2.0f * kPi;

            // Tube preamp saturation
            const float tubeInL = std::tanh (chL[i] * 1.25f);
            const float tubeInR = std::tanh (chR[i] * 1.25f);

            // Horn Doppler pitch shift + AM (30%)
            const float hornAmL = 1.0f + 0.30f * std::sin (rotorPhaseHorn);
            const float hornAmR = 1.0f + 0.30f * std::cos (rotorPhaseHorn);
            const float hornDelayMsL = 1.8f + 1.2f * std::sin (rotorPhaseHorn);
            const float hornDelayMsR = 1.8f + 1.2f * std::cos (rotorPhaseHorn);

            // Drum low-end phase rotation + AM (15%)
            const float drumAmL = 1.0f + 0.15f * std::sin (rotorPhaseDrum);
            const float drumAmR = 1.0f + 0.15f * std::cos (rotorPhaseDrum);

            // Write into modulation delay buffer
            modDelayBufL[(size_t) modWritePos] = tubeInL;
            modDelayBufR[(size_t) modWritePos] = tubeInR;

            const float dHornL = hornDelayMsL * 0.001f * sr;
            const float dHornR = hornDelayMsR * 0.001f * sr;

            const float delayedHornL = readFractionalRing (modDelayBufL, modWritePos, dHornL);
            const float delayedHornR = readFractionalRing (modDelayBufR, modWritePos, dHornR);

            if (++modWritePos >= maxLen) modWritePos = 0;

            // Dedicated crossover filters for Horn (HPF) and Drum (LPF)
            const float hL = hornHpfL.process (delayedHornL, cHorn) * hornAmL * hornBal;
            const float hR = hornHpfR.process (delayedHornR, cHorn) * hornAmR * hornBal;
            const float dL = drumLpfL.process (tubeInL, cDrum) * drumAmL;
            const float dR = drumLpfR.process (tubeInR, cDrum) * drumAmR;

            const float rotL = hL + dL;
            const float rotR = hR + dR;

            chL[i] = chL[i] * dry + rotL * wet;
            chR[i] = chR[i] * dry + rotR * wet;
        }
        return;
    }

    // 3. Moog MF-101 24dB 4-Pole Transistor Ladder Filter Modeling
    if (type == FxType::MoodFilter)
    {
        const float baseCutoff = clampf (paramCustom, 60.0f, 16000.0f);
        const float resAmount = clampf (feedbackNorm * 0.038f, 0.0f, 3.85f); // self-oscillates near 4.0
        const float envSensitivity = clampf (depthNorm * 0.01f, 0.0f, 1.0f);
        const float envAttackCoeff = std::exp (-1.0f / (0.005f * sr));
        const float envReleaseCoeff = std::exp (-1.0f / (0.120f * sr));
        const float lfoInc = (clampf (speedHz, 0.1f, 12.0f) * 2.0f * kPi) / sr;

        for (int i = 0; i < numSamples; ++i)
        {
            lfoPhase += lfoInc;
            if (lfoPhase >= 2.0f * kPi) lfoPhase -= 2.0f * kPi;

            const float inPk = std::max (std::abs (chL[i]), std::abs (chR[i]));
            if (inPk > moodEnv) moodEnv = inPk + envAttackCoeff * (moodEnv - inPk);
            else moodEnv = inPk + envReleaseCoeff * (moodEnv - inPk);

            const float lfoMod = 1.0f + 0.35f * envSensitivity * std::sin (lfoPhase);
            const float modCutoff = clampf ((baseCutoff + moodEnv * envSensitivity * 10000.0f) * lfoMod, 40.0f, 18000.0f);
            const float g = clampf (1.0f - std::exp (-(2.0f * kPi * modCutoff) / sr), 0.001f, 0.99f);

            // Left 4-Pole Ladder
            const float feedL = chL[i] - resAmount * moogStageL[3];
            moogStageL[0] += g * (std::tanh (feedL) - std::tanh (moogStageL[0]));
            moogStageL[1] += g * (std::tanh (moogStageL[0]) - std::tanh (moogStageL[1]));
            moogStageL[2] += g * (std::tanh (moogStageL[1]) - std::tanh (moogStageL[2]));
            moogStageL[3] += g * (std::tanh (moogStageL[2]) - std::tanh (moogStageL[3]));

            // Right 4-Pole Ladder (Fixed stage[2] input from stage[1])
            const float feedR = chR[i] - resAmount * moogStageR[3];
            moogStageR[0] += g * (std::tanh (feedR) - std::tanh (moogStageR[0]));
            moogStageR[1] += g * (std::tanh (moogStageR[0]) - std::tanh (moogStageR[1]));
            moogStageR[2] += g * (std::tanh (moogStageR[1]) - std::tanh (moogStageR[2]));
            moogStageR[3] += g * (std::tanh (moogStageR[2]) - std::tanh (moogStageR[3]));

            // Mode selection via paramCustom: LP (0..0.4), BP (0.4..0.75), HP (>0.75)
            float filtOutL = moogStageL[3];
            float filtOutR = moogStageR[3];
            if (modeNorm > 0.75f)
            {
                filtOutL = feedL - moogStageL[3];
                filtOutR = feedR - moogStageR[3];
            }
            else if (modeNorm > 0.4f)
            {
                filtOutL = (moogStageL[1] - moogStageL[3]) * 1.8f;
                filtOutR = (moogStageR[1] - moogStageR[3]) * 1.8f;
            }

            chL[i] = chL[i] * dry + filtOutL * wet;
            chR[i] = chR[i] * dry + filtOutR * wet;
        }
        return;
    }

    // 4. Standard Chorus, Flanger, Phaser, Tremolo
    const float lfoInc = (clampf (speedHz, 0.05f, 20.0f) * 2.0f * kPi) / sr;
    for (int i = 0; i < numSamples; ++i)
    {
        const float inL = chL[i];
        const float inR = chR[i];

        lfoPhase += lfoInc;
        if (lfoPhase >= 2.0f * kPi) lfoPhase -= 2.0f * kPi;

        const float lfoL = std::sin (lfoPhase);
        const float lfoR = std::sin (lfoPhase + phaseDeg * kPi / 180.0f);

        if (type == FxType::TremoloPanner)
        {
            // Tremolo / Auto-Panner
            const float modDepth = clampf (depthNorm * 0.01f, 0.0f, 1.0f);
            const auto wave = [paramCustom] (float phase) {
                if (paramCustom > 66.0f) return std::sin (phase) >= 0.0f ? 1.0f : -1.0f;
                if (paramCustom > 33.0f) return 2.0f / kPi * std::asin (std::sin (phase));
                return std::sin (phase);
            };
            const float gainL = 1.0f - modDepth * (0.5f + 0.5f * wave (lfoPhase));
            const float gainR = 1.0f - modDepth * (0.5f + 0.5f * wave (lfoPhase + feedbackNorm * kPi / 180.0f));
            chL[i] = inL * (dry + gainL * wet);
            chR[i] = inR * (dry + gainR * wet);
        }
        else if (type == FxType::StereoPhaser)
        {
            // 4-stage allpass phaser with resonance feedback
            const float depth01 = clampf (depthNorm * 0.01f, 0.0f, 1.0f);
            const float resoFb = clampf (feedbackNorm * 0.0075f, 0.0f, 0.75f);
            const float baseHz = paramCustom > 20.0f ? paramCustom : 700.0f;
            const float fL = clampf (baseHz * std::pow (4.0f, depth01 * lfoL), 20.0f, sr * 0.45f);
            const float fR = clampf (baseHz * std::pow (4.0f, depth01 * lfoR), 20.0f, sr * 0.45f);
            const float tanL = std::tan (kPi * fL / sr), tanR = std::tan (kPi * fR / sr);
            const float apCoeffL = (tanL - 1.0f) / (tanL + 1.0f);
            const float apCoeffR = (tanR - 1.0f) / (tanR + 1.0f);

            float sL = inL + phaserFbL * resoFb;
            float sR = inR + phaserFbR * resoFb;
            for (int stage = 0; stage < 4; ++stage)
            {
                const float yL = apCoeffL * sL + phaserZ1L[stage];
                phaserZ1L[stage] = sL - apCoeffL * yL;
                sL = yL;

                const float yR = apCoeffR * sR + phaserZ1R[stage];
                phaserZ1R[stage] = sR - apCoeffR * yR;
                sR = yR;
            }
            phaserFbL = std::tanh (sL);
            phaserFbR = std::tanh (sR);
            chL[i] = inL * dry + (inL + sL * 0.7f) * 0.7f * wet;
            chR[i] = inR * dry + (inR + sR * 0.7f) * 0.7f * wet;
        }
        else
        {
            // Chorus / Flanger with smooth fractional interpolation
            const bool isFlanger = (type == FxType::StereoFlanger || type == FxType::FlangerAndChamber || type == FxType::DelayAndFlanger);
            const float baseDelayMs = delayLMs >= 0.0f ? delayLMs : (isFlanger ? 1.8f : 12.0f);
            const float baseDelayRMs = delayRMs >= 0.0f ? delayRMs : baseDelayMs;
            const float modRangeMs = isFlanger ? (1.4f * clampf (depthNorm * 0.01f, 0.0f, 1.0f))
                                               : (6.5f * clampf (depthNorm * 0.01f, 0.0f, 1.0f));
            const float fdbk = isFlanger ? clampf (feedbackNorm * 0.0088f, -0.88f, 0.88f)
                                         : clampf (feedbackNorm * 0.0035f, -0.35f, 0.35f);

            const float dL = (baseDelayMs + modRangeMs * lfoL) * 0.001f * sr;
            const float rangeR = widthR < 0.0f ? modRangeMs : (isFlanger ? 1.4f : 6.5f) * clampf (widthR * 0.01f, 0.0f, 1.0f);
            const float dR = (baseDelayRMs + rangeR * lfoR) * 0.001f * sr;

            const float outModL = readFractionalRing (modDelayBufL, modWritePos, dL);
            const float outModR = readFractionalRing (modDelayBufR, modWritePos, dR);

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
    ringBufL.assign (kWindowSize * 4, 0.0f);
    ringBufR.assign (kWindowSize * 4, 0.0f);
    reset();
}

void PitchShifterDsp::reset()
{
    std::fill (ringBufL.begin(), ringBufL.end(), 0.0f);
    std::fill (ringBufR.begin(), ringBufR.end(), 0.0f);
    writePos = 0;
    readPos1 = 0.0f;
    readPos2 = (float) (kWindowSize / 2);
    readPos1B = 0.0f;
    readPos2B = (float) (kWindowSize / 2);
}

void PitchShifterDsp::process (float* chL, float* chR, int numSamples,
                               float semitones, float mixNorm)
{
    const float pitchRatio = std::pow (2.0f, clampf (semitones, -24.0f, 24.0f) / 12.0f);
    const float rate = 1.0f - pitchRatio;
    const int bufSize = (int) ringBufL.size();
    const float wet = clampf (mixNorm, 0.0f, 1.0f);
    const float dry = 1.0f - wet;

    for (int i = 0; i < numSamples; ++i)
    {
        const float inL = chL[i];
        const float inR = chR[i];

        ringBufL[(size_t) writePos] = inL;
        ringBufR[(size_t) writePos] = inR;

        // Two cross-fading Hann grain taps with fractional interpolation
        readPos1 += rate;
        if (readPos1 >= (float) kWindowSize) readPos1 -= (float) kWindowSize;
        if (readPos1 < 0.0f) readPos1 += (float) kWindowSize;

        readPos2 += rate;
        if (readPos2 >= (float) kWindowSize) readPos2 -= (float) kWindowSize;
        if (readPos2 < 0.0f) readPos2 += (float) kWindowSize;

        const float win1 = 0.5f * (1.0f - std::cos (2.0f * kPi * readPos1 / (float) kWindowSize));
        const float win2 = 0.5f * (1.0f - std::cos (2.0f * kPi * readPos2 / (float) kWindowSize));

        const float outShiftL = readFractionalRing (ringBufL, writePos, readPos1 + 1.0f) * win1
                              + readFractionalRing (ringBufL, writePos, readPos2 + 1.0f) * win2;
        const float outShiftR = readFractionalRing (ringBufR, writePos, readPos1 + 1.0f) * win1
                              + readFractionalRing (ringBufR, writePos, readPos2 + 1.0f) * win2;

        if (++writePos >= bufSize) writePos = 0;

        chL[i] = inL * dry + outShiftL * wet;
        chR[i] = inR * dry + outShiftR * wet;
    }
}

void PitchShifterDsp::processDual (float* chL, float* chR, int numSamples,
                                   float semi1, float cent1, float delay1Ms, float gain1Norm, float pan1Norm,
                                   float semi2, float cent2, float delay2Ms, float gain2Norm, float pan2Norm,
                                   float mixNorm)
{
    const float ratio1 = std::pow (2.0f, clampf (semi1 + cent1 * 0.01f, -24.0f, 24.0f) / 12.0f);
    const float ratio2 = std::pow (2.0f, clampf (semi2 + cent2 * 0.01f, -24.0f, 24.0f) / 12.0f);
    const float rate1 = 1.0f - ratio1;
    const float rate2 = 1.0f - ratio2;

    const int bufSize = (int) ringBufL.size();
    const float baseDelay1 = clampf (delay1Ms * 0.001f * sr, 1.0f, (float) (bufSize - kWindowSize - 4));
    const float baseDelay2 = clampf (delay2Ms * 0.001f * sr, 1.0f, (float) (bufSize - kWindowSize - 4));

    const float g1 = clampf (gain1Norm * 0.01f, 0.0f, 1.2f);
    const float g2 = clampf (gain2Norm * 0.01f, 0.0f, 1.2f);
    const auto pan1 = panMonoConstantPower (clampf ((pan1Norm - 50.0f) / 50.0f, -1.0f, 1.0f));
    const auto pan2 = panMonoConstantPower (clampf ((pan2Norm - 50.0f) / 50.0f, -1.0f, 1.0f));

    const float wet = clampf (mixNorm, 0.0f, 1.0f);
    const float dry = 1.0f - wet;

    for (int i = 0; i < numSamples; ++i)
    {
        const float inL = chL[i];
        const float inR = chR[i];

        ringBufL[(size_t) writePos] = inL;
        ringBufR[(size_t) writePos] = inR;

        readPos1 += rate1;
        if (readPos1 >= (float) kWindowSize) readPos1 -= (float) kWindowSize;
        if (readPos1 < 0.0f) readPos1 += (float) kWindowSize;

        readPos2 += rate1;
        if (readPos2 >= (float) kWindowSize) readPos2 -= (float) kWindowSize;
        if (readPos2 < 0.0f) readPos2 += (float) kWindowSize;

        readPos1B += rate2;
        if (readPos1B >= (float) kWindowSize) readPos1B -= (float) kWindowSize;
        if (readPos1B < 0.0f) readPos1B += (float) kWindowSize;

        readPos2B += rate2;
        if (readPos2B >= (float) kWindowSize) readPos2B -= (float) kWindowSize;
        if (readPos2B < 0.0f) readPos2B += (float) kWindowSize;

        const float win1 = 0.5f * (1.0f - std::cos (2.0f * kPi * readPos1 / (float) kWindowSize));
        const float win2 = 0.5f * (1.0f - std::cos (2.0f * kPi * readPos2 / (float) kWindowSize));
        const float win1B = 0.5f * (1.0f - std::cos (2.0f * kPi * readPos1B / (float) kWindowSize));
        const float win2B = 0.5f * (1.0f - std::cos (2.0f * kPi * readPos2B / (float) kWindowSize));

        const float v1 = (readFractionalRing (ringBufL, writePos, baseDelay1 + readPos1) * win1
                        + readFractionalRing (ringBufL, writePos, baseDelay1 + readPos2) * win2) * g1;
        const float v2 = (readFractionalRing (ringBufR, writePos, baseDelay2 + readPos1B) * win1B
                        + readFractionalRing (ringBufR, writePos, baseDelay2 + readPos2B) * win2B) * g2;

        if (++writePos >= bufSize) writePos = 0;

        chL[i] = inL * dry + (v1 * pan1.left + v2 * pan2.left) * wet;
        chR[i] = inR * dry + (v1 * pan1.right + v2 * pan2.right) * wet;
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
                            bool linkedStereo,
                            bool isTrueEq)
{
    const float qFactor = isTrueEq ? 5.2f : 4.318f; // TruEQ uses narrower constant-Q decoupling

    // Update any changed band filter coefficients
    for (size_t b = 0; b < 31; ++b)
    {
        float gL = gainsL[b].get();
        if (isTrueEq && b > 0 && b < 30)
        {
            // Constant-Q adjacent-band skirt compensation
            const float neighborSumL = gainsL[b - 1].get() + gainsL[b + 1].get();
            gL = clampf (gL - 0.14f * neighborSumL, -18.0f, 18.0f);
        }

        if (std::abs (gL - lastGainsL[b]) > 0.05f)
        {
            lastGainsL[b] = gL;
            coeffsL[b] = calcBell (kGeqFrequencies[b], gL, qFactor, sr);
        }

        float gR = linkedStereo ? gL : gainsR[b].get();
        if (! linkedStereo && isTrueEq && b > 0 && b < 30)
        {
            const float neighborSumR = gainsR[b - 1].get() + gainsR[b + 1].get();
            gR = clampf (gR - 0.14f * neighborSumR, -18.0f, 18.0f);
        }

        if (std::abs (gR - lastGainsR[b]) > 0.05f)
        {
            lastGainsR[b] = gR;
            coeffsR[b] = calcBell (kGeqFrequencies[b], gR, qFactor, sr);
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
// PremiumDynamicsDsp Implementation (LA-2A, 1176, Fairchild, Combinator, Limiter, Wave Designer, De-Esser)
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
    la2aEnvR = la2aTailR = fetEnvR = 0.0f;
    fairMemoryL = fairMemoryR = 0.0f;
    limiterGain = 1.0f;
    fastEnvL = slowEnvL = fastEnvR = slowEnvR = 0.0f;
    lookaheadWrite = 0;
    std::fill (lookaheadBufL.begin(), lookaheadBufL.end(), 0.0f);
    std::fill (lookaheadBufR.begin(), lookaheadBufR.end(), 0.0f);

    for (int i = 0; i < 4; ++i) { crossFiltL[i].reset(); crossFiltR[i].reset(); }
    for (int i = 0; i < 5; ++i) bandEnvL[i] = bandEnvR[i] = 0.0f;

    optoHpfL.reset(); optoHpfR.reset();
    sibilanceFiltL.reset(); sibilanceFiltR.reset();
    deEssEnv = 0.0f;
    deEssEnvR = 0.0f;
    deEssLowL.reset(); deEssLowR.reset(); deEssLowEnvL = deEssLowEnvR = 0.0f;
}

void PremiumDynamicsDsp::process (float* chL, float* chR, int numSamples,
                                  FxType type, float p1, float p2, float p3, float p4, float mixNorm,
                                  float p5, float p6)
{
    const float wet = mixNorm;
    const float dry = 1.0f - wet;

    // 1. Teletronix LA-2A Electro-Optical Leveling Amplifier
    if (type == FxType::StereoLeisureCompressor || type == FxType::DualLeisureCompressor
        || type == FxType::LeisureCompressor)
    {
        // p1 = Peak Reduction (0..100), p2 = Output Gain (0..100), p3 = Limit/Comp mode (0 = Comp, >0.5 = Limit)
        const float thresholdL = -42.0f + (100.0f - clampf (p1, 0.0f, 100.0f)) * 0.42f;
        const float makeupL = dbToGain ((clampf (p2, 0.0f, 100.0f) - 40.0f) * 0.4f);
        const bool isDual = (type == FxType::DualLeisureCompressor);
        const float p1R = isDual ? p4 : p1;
        const float p2R = isDual ? p5 : p2;
        const float thresholdR = -42.0f + (100.0f - clampf (p1R, 0.0f, 100.0f)) * 0.42f;
        const float makeupR = dbToGain ((clampf (p2R, 0.0f, 100.0f) - 40.0f) * 0.4f);
        const float compSlope = (p3 > 0.5f) ? 0.92f : 0.72f; // Limit (10:1) vs Compress (3.5:1)
        const auto cOptoHf = calcHighShelf (1000.0f, 4.0f, 0.707f, sr); // R37 HF emphasis

        const float attCoeff = std::exp (-1.0f / (0.010f * sr));
        const float relCoeff1 = std::exp (-1.0f / (0.060f * sr));
        const float relCoeff2 = std::exp (-1.0f / (1.800f * sr));

        for (int i = 0; i < numSamples; ++i)
        {
            const float detL = optoHpfL.process (chL[i], cOptoHf);
            const float detR = optoHpfR.process (chR[i], cOptoHf);

            if (isDual)
            {
                const float overL = std::max (0.0f, gainToDb (std::abs (detL)) - thresholdL);
                const float overR = std::max (0.0f, gainToDb (std::abs (detR)) - thresholdR);

                if (overL > la2aEnv) la2aEnv = overL + attCoeff * (la2aEnv - overL);
                else la2aEnv = overL + relCoeff1 * (la2aEnv - overL);
                la2aTail = la2aEnv + relCoeff2 * (la2aTail - la2aEnv);

                if (overR > la2aEnvR) la2aEnvR = overR + attCoeff * (la2aEnvR - overR);
                else la2aEnvR = overR + relCoeff1 * (la2aEnvR - overR);
                la2aTailR = la2aEnvR + relCoeff2 * (la2aTailR - la2aEnvR);

                const float gainL = dbToGain (-(la2aEnv * 0.55f + la2aTail * 0.45f) * compSlope) * makeupL;
                const float gainR = dbToGain (-(la2aEnvR * 0.55f + la2aTailR * 0.45f) * compSlope) * makeupR;

                const float linL = chL[i] * gainL;
                const float linR = chR[i] * gainR;
                const float outL = linL + 0.08f * linL * linL;
                const float outR = linR + 0.08f * linR * linR;

                chL[i] = chL[i] * dry + outL * wet;
                chR[i] = chR[i] * dry + outR * wet;
            }
            else
            {
                const float inPeak = std::max (std::abs (detL), std::abs (detR));
                const float inDb = gainToDb (inPeak);
                const float over = std::max (0.0f, inDb - thresholdL);

                if (over > la2aEnv) la2aEnv = over + attCoeff * (la2aEnv - over);
                else la2aEnv = over + relCoeff1 * (la2aEnv - over);

                la2aTail = la2aEnv + relCoeff2 * (la2aTail - la2aEnv);
                const float compDb = -(la2aEnv * 0.55f + la2aTail * 0.45f) * compSlope;
                const float gain = dbToGain (compDb) * makeupL;

                const float linL = chL[i] * gain;
                const float linR = chR[i] * gain;
                const float outL = linL + 0.08f * linL * linL;
                const float outR = linR + 0.08f * linR * linR;

                chL[i] = chL[i] * dry + outL * wet;
                chR[i] = chR[i] * dry + outR * wet;
            }
        }
    }
    // 2. Urei 1176LN Fast FET Limiting Amplifier with "All-Buttons-In" Mode
    else if (type == FxType::UltimoCompressor || type == FxType::DualUltimoCompressor)
    {
        // p1 = Input (0..100), p2 = Output (0..100), p3 = Ratio (4, 8, 12, 20, All-In), p4 = Attack (0..100), p5 = Release (0..100)
        const float drive = dbToGain ((clampf (p1, 0.0f, 100.0f) - 30.0f) * 0.5f);
        const float makeup = dbToGain ((clampf (p2, 0.0f, 100.0f) - 20.0f) * 0.5f);
        const bool isAllButtons = (p3 > 20.0f);
        const float ratio = isAllButtons ? 12.0f : std::max (2.0f, p3);
        const bool isDual = (type == FxType::DualUltimoCompressor);

        const float attSec = 0.000020f + (1.0f - clampf (p4, 0.0f, 100.0f) * 0.01f) * 0.000780f; // 20us to 800us ultra-fast FET
        const float relSec = isAllButtons ? 0.050f : (0.050f + (1.0f - clampf (p5, 0.0f, 100.0f) * 0.01f) * 1.050f);
        const float attCoeff = std::exp (-1.0f / (attSec * sr));
        const float relCoeff = std::exp (-1.0f / (relSec * sr));
        const float thresh = isAllButtons ? -26.0f : -20.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float drivenL = chL[i] * drive;
            const float drivenR = chR[i] * drive;

            float gL = 1.0f, gR = 1.0f;
            if (isDual)
            {
                const float overL = std::max (0.0f, gainToDb (std::abs (drivenL)) - thresh);
                const float overR = std::max (0.0f, gainToDb (std::abs (drivenR)) - thresh);
                if (overL > fetEnv) fetEnv = overL + attCoeff * (fetEnv - overL);
                else fetEnv = overL + relCoeff * (fetEnv - overL);
                if (overR > fetEnvR) fetEnvR = overR + attCoeff * (fetEnvR - overR);
                else fetEnvR = overR + relCoeff * (fetEnvR - overR);

                gL = dbToGain (-fetEnv * (1.0f - 1.0f / ratio)) * makeup;
                gR = dbToGain (-fetEnvR * (1.0f - 1.0f / ratio)) * makeup;
            }
            else
            {
                const float inPeak = std::max (std::abs (drivenL), std::abs (drivenR));
                const float over = std::max (0.0f, gainToDb (inPeak) - thresh);
                if (over > fetEnv) fetEnv = over + attCoeff * (fetEnv - over);
                else fetEnv = over + relCoeff * (fetEnv - over);
                gL = gR = dbToGain (-fetEnv * (1.0f - 1.0f / ratio)) * makeup;
            }

            float outL = drivenL * gL;
            float outR = drivenR * gR;
            if (isAllButtons)
            {
                const float saturatedL = std::tanh (outL * 1.2f);
                outL = saturatedL - 0.15f * saturatedL * saturatedL * saturatedL;
                const float saturatedR = std::tanh (outR * 1.2f);
                outR = saturatedR - 0.15f * saturatedR * saturatedR * saturatedR;
            }
            else
            {
                outL = std::tanh (outL);
                outR = std::tanh (outR);
            }

            chL[i] = chL[i] * dry + outL * wet;
            chR[i] = chR[i] * dry + outR * wet;
        }
    }
    // 3. Fairchild 670 Vari-Mu Tube Compressor & Mid/Side Processing
    else if (type == FxType::FairCompressor || type == FxType::DualFairCompressor
             || type == FxType::MsFairCompressor)
    {
        const float thresholdL = -50.0f + (100.0f - clampf (p1, 0.0f, 100.0f)) * 0.45f;
        const float makeupL = dbToGain ((clampf (p2, 0.0f, 100.0f) - 50.0f) * 0.24f);
        const float dcBias = clampf (p3, 0.0f, 100.0f) * 0.01f; // DC Threshold knee control
        // 6 switchable time constants (p4 selects position 1..6)
        const int timeConstPos = (int) std::round (clampf (p4, 1.0f, 6.0f));
        // Nominal constants documented in the UA Fairchild manual. Auto
        // positions use accumulated detector energy, not a fixed slow release.
        const float attacks[] = { .0002f, .0002f, .0004f, .0008f, .0002f, .0004f };
        const float releases[] = { .3f, .8f, 2.0f, 5.0f, 2.0f, .3f };
        const float attCoeff = std::exp (-1.0f / (attacks[timeConstPos - 1] * sr));
        const float relCoeff = std::exp (-1.0f / (releases[timeConstPos - 1] * sr));
        const float memoryCoeff = std::exp (-1.0f / sr);
        const float slowCoeff = std::exp (-1.0f / ((timeConstPos == 6 ? 25.0f : 10.0f) * sr));
        const bool isMidSide = (type == FxType::MsFairCompressor);
        const auto balPair = balanceStereo (clampf ((p5 - 50.0f) / 50.0f, -1.0f, 1.0f));

        for (int i = 0; i < numSamples; ++i)
        {
            float sL = chL[i];
            float sR = chR[i];

            if (isMidSide)
            {
                const float mid = 0.5f * (sL + sR);
                const float side = 0.5f * (sL - sR);
                sL = mid;
                sR = side;
            }

            const float inDbL = gainToDb (std::abs (sL));
            const float inDbR = gainToDb (std::abs (sR));
            const float overL = std::max (0.0f, inDbL - thresholdL);
            const float overR = std::max (0.0f, inDbR - thresholdL);

            fairMemoryL = overL + memoryCoeff * (fairMemoryL - overL);
            fairMemoryR = overR + memoryCoeff * (fairMemoryR - overR);
            const float releaseL = timeConstPos >= 5 ? relCoeff + (slowCoeff - relCoeff) * clampf (fairMemoryL / 12.0f, 0.0f, 1.0f) : relCoeff;
            const float releaseR = timeConstPos >= 5 ? relCoeff + (slowCoeff - relCoeff) * clampf (fairMemoryR / 12.0f, 0.0f, 1.0f) : relCoeff;
            if (overL > fairEnvL) fairEnvL = overL + attCoeff * (fairEnvL - overL);
            else fairEnvL = overL + releaseL * (fairEnvL - overL);
            if (overR > fairEnvR) fairEnvR = overR + attCoeff * (fairEnvR - overR);
            else fairEnvR = overR + releaseR * (fairEnvR - overR);

            if (type == FxType::FairCompressor)
            {
                const float linkedEnv = std::max (fairEnvL, fairEnvR);
                fairEnvL = fairEnvR = linkedEnv;
            }

            // Vari-Mu continuous ratio curve (2:1 up to 12:1 based on depth & DC bias)
            const float ratioL = 2.0f + fairEnvL * (0.3f + 0.3f * dcBias);
            const float ratioR = 2.0f + fairEnvR * (0.3f + 0.3f * dcBias);
            const float gL = dbToGain (-fairEnvL * (1.0f - 1.0f / ratioL)) * makeupL * balPair.left;
            const float gR = dbToGain (-fairEnvR * (1.0f - 1.0f / ratioR)) * makeupL * balPair.right;

            float compL = sL * gL;
            float compR = sR * gR;

            if (isMidSide)
            {
                const float l = compL + compR;
                const float r = compL - compR;
                compL = l;
                compR = r;
            }

            compL = std::tanh (compL * 1.1f);
            compR = std::tanh (compR * 1.1f);

            chL[i] = chL[i] * dry + compL * wet;
            chR[i] = chR[i] * dry + compR * wet;
        }
    }
    // 4. TC Finalizer 5-Band Broadcast Combinator Modeling
    else if (type == FxType::Combinator || type == FxType::DualCombinator)
    {
        const float xoverScale = std::pow (2.0f, clampf ((p6 - 50.0f) / 50.0f, -1.0f, 1.0f) * 0.5f);
        const auto cX1 = calcLPF (80.0f * xoverScale, 0.707f, sr);
        const auto cX2 = calcLPF (320.0f * xoverScale, 0.707f, sr);
        const auto cX3 = calcLPF (1280.0f * xoverScale, 0.707f, sr);
        const auto cX4 = calcLPF (5120.0f * xoverScale, 0.707f, sr);

        const float threshold = -42.0f + (100.0f - clampf (p1, 0.0f, 100.0f)) * 0.4f;
        const float makeup = dbToGain ((clampf (p2, 0.0f, 100.0f) - 40.0f) * 0.2f);
        const float ratioSlope = 1.0f - 1.0f / std::max (1.5f, p3);
        const float attSec = 0.002f + clampf (p4, 0.0f, 100.0f) * 0.0005f;
        const float relSec = 0.040f + clampf (p5, 0.0f, 100.0f) * 0.004f;
        const float attCoeff = std::exp (-1.0f / (attSec * sr));
        const float relCoeff = std::exp (-1.0f / (relSec * sr));

        for (int i = 0; i < numSamples; ++i)
        {
            const float b1L = crossFiltL[0].process (chL[i], cX1);
            const float b2L = crossFiltL[1].process (chL[i], cX2) - b1L;
            const float b3L = crossFiltL[2].process (chL[i], cX3) - (b1L + b2L);
            const float b4L = crossFiltL[3].process (chL[i], cX4) - (b1L + b2L + b3L);
            const float b5L = chL[i] - (b1L + b2L + b3L + b4L);

            const float b1R = crossFiltR[0].process (chR[i], cX1);
            const float b2R = crossFiltR[1].process (chR[i], cX2) - b1R;
            const float b3R = crossFiltR[2].process (chR[i], cX3) - (b1R + b2R);
            const float b4R = crossFiltR[3].process (chR[i], cX4) - (b1R + b2R + b3R);
            const float b5R = chR[i] - (b1R + b2R + b3R + b4R);

            const float bandsL[5] = { b1L, b2L, b3L, b4L, b5L };
            const float bandsR[5] = { b1R, b2R, b3R, b4R, b5R };
            float sumL = 0.0f, sumR = 0.0f;

            for (int b = 0; b < 5; ++b)
            {
                const float inDbL = gainToDb (std::abs (bandsL[b]));
                const float inDbR = gainToDb (std::abs (bandsR[b]));
                const float overL = std::max (0.0f, inDbL - threshold);
                const float overR = std::max (0.0f, inDbR - threshold);

                if (overL > bandEnvL[b]) bandEnvL[b] = overL + attCoeff * (bandEnvL[b] - overL);
                else bandEnvL[b] = overL + relCoeff * (bandEnvL[b] - overL);
                if (overR > bandEnvR[b]) bandEnvR[b] = overR + attCoeff * (bandEnvR[b] - overR);
                else bandEnvR[b] = overR + relCoeff * (bandEnvR[b] - overR);

                const float envL = type == FxType::Combinator ? std::max (bandEnvL[b], bandEnvR[b]) : bandEnvL[b];
                const float envR = type == FxType::Combinator ? envL : bandEnvR[b];
                const float gL = dbToGain (-envL * ratioSlope);
                const float gR = dbToGain (-envR * ratioSlope);
                sumL += bandsL[b] * gL;
                sumR += bandsR[b] * gR;
            }

            chL[i] = chL[i] * dry + (sumL * makeup) * wet;
            chR[i] = chR[i] * dry + (sumR * makeup) * wet;
        }
    }
    // 5. SPL Transient Designer / Wave Designer
    else if (type == FxType::WaveDesigner)
    {
        // p1 = Attack L (-100%..+100%), p2 = Sustain L (-100%..+100%), p3 = Gain L (-24..+24 dB)
        // p4 = Attack R, p5 = Sustain R, p6 = Gain R
        const float attMultL = dbToGain (p1 * 0.15f);
        const float susMultL = dbToGain (p2 * 0.15f);
        const float outGainL = dbToGain (p3);

        const float attMultR = dbToGain (p4 * 0.15f);
        const float susMultR = dbToGain (p5 * 0.15f);
        const float outGainR = dbToGain (p6);

        const float fastCoeff = std::exp (-1.0f / (0.005f * sr));
        const float slowCoeff = std::exp (-1.0f / (0.080f * sr));

        for (int i = 0; i < numSamples; ++i)
        {
            const float aL = std::abs (chL[i]);
            fastEnvL = aL + fastCoeff * (fastEnvL - aL);
            slowEnvL = aL + slowCoeff * (slowEnvL - aL);
            const float transL = std::max (0.0f, fastEnvL - slowEnvL);
            const float susL = slowEnvL;
            const float modL = ((transL * attMultL + susL * susMultL) / std::max (1.0e-20f, transL + susL)) * outGainL;

            const float aR = std::abs (chR[i]);
            fastEnvR = aR + fastCoeff * (fastEnvR - aR);
            slowEnvR = aR + slowCoeff * (slowEnvR - aR);
            const float transR = std::max (0.0f, fastEnvR - slowEnvR);
            const float susR = slowEnvR;
            const float modR = ((transR * attMultR + susR * susMultR) / std::max (1.0e-20f, transR + susR)) * outGainR;

            chL[i] = chL[i] * dry + (chL[i] * modL) * wet;
            chR[i] = chR[i] * dry + (chR[i] * modR) * wet;
        }
    }
    // 6. SPL Vocal De-Esser (Dual / Stereo)
    else if (type == FxType::StereoDeEsser || type == FxType::DualDeEsser)
    {
        // Two independently detected sibilance bands. Subtract only the
        // extracted band, never the broadband output of a peaking filter.
        const auto lowCoeffs = calcBell (3200.0f, 6.0f, 2.2f, sr);
        const auto highCoeffs = calcBell (6800.0f, 6.0f, 2.2f, sr);
        const float att = std::exp (-1.0f / (0.001f * sr));
        const float rel = std::exp (-1.0f / (0.050f * sr));
        const bool dual = type == FxType::DualDeEsser;
        const float norm = 1.0f / (dbToGain (6.0f) - 1.0f);
        const auto envelope = [att, rel] (float level, float& state) {
            state = level + (level > state ? att : rel) * (state - level);
        };
        const auto reduction = [] (float env, float amount) {
            const float threshold = -4.0f - clampf (amount, 0.0f, 100.0f) * .42f;
            return amount <= 0.0f ? 0.0f : 1.0f - dbToGain (-std::max (0.0f, gainToDb(env) - threshold) * .85f);
        };
        for (int i = 0; i < numSamples; ++i)
        {
            const float lowL = (deEssLowL.process (chL[i], lowCoeffs) - chL[i]) * norm;
            const float lowR = (deEssLowR.process (chR[i], lowCoeffs) - chR[i]) * norm;
            const float highL = (sibilanceFiltL.process (chL[i], highCoeffs) - chL[i]) * norm;
            const float highR = (sibilanceFiltR.process (chR[i], highCoeffs) - chR[i]) * norm;
            envelope (std::abs(lowL), deEssLowEnvL); envelope (std::abs(lowR), deEssLowEnvR);
            envelope (std::abs(highL), deEssEnv); envelope (std::abs(highR), deEssEnvR);
            const float loL = dual ? deEssLowEnvL : std::max (deEssLowEnvL, deEssLowEnvR);
            const float loR = dual ? deEssLowEnvR : loL;
            const float hiL = dual ? deEssEnv : std::max (deEssEnv, deEssEnvR);
            const float hiR = dual ? deEssEnvR : hiL;
            chL[i] -= wet * (lowL * reduction(loL,p1) + highL * reduction(hiL,p2));
            chR[i] -= wet * (lowR * reduction(loR,dual ? p4 : p1) + highR * reduction(hiR,dual ? p5 : p2));
        }
    }
    // 7. Lookahead Precision Mastering Limiter
    else
    {
        // p1 = Input Gain (0..18 dB), p2 = Out Ceiling (-18..0 dB), p3 = Squeeze (0..100%), p4 = Knee (0..10 dB), p5 = Attack ms, p6 = Release ms
        const float inGain = dbToGain (clampf (p1, 0.0f, 18.0f));
        const float ceilingLinear = dbToGain (clampf (p2, -24.0f, 0.0f));
        const float squeezeDrive = 1.0f + clampf (p3, 0.0f, 100.0f) * 0.01f;
        const float releaseCoeff = std::exp (-1.0f / (std::max (20.0f, p6) * 0.001f * sr));
        const float attackCoeff = std::exp (-1.0f / (std::max (0.02f, p5) * 0.001f * sr));
        const float knee = clampf (p4, 0.0f, 10.0f);
        const int lookaheadLen = (int) lookaheadBufL.size();
        for (int i = 0; i < numSamples; ++i)
        {
            const float sL = chL[i] * inGain * squeezeDrive;
            const float sR = chR[i] * inGain * squeezeDrive;
            lookaheadBufL[(size_t) lookaheadWrite] = sL;
            lookaheadBufR[(size_t) lookaheadWrite] = sR;
            const float pk = std::max (std::abs (sL), std::abs (sR));
            const float overDb = gainToDb (pk) - gainToDb (ceilingLinear);
            float reductionDb = std::max (0.0f, overDb);
            if (knee > 0.0f && overDb > -knee * 0.5f && overDb < knee * 0.5f)
                reductionDb = (overDb + knee * 0.5f) * (overDb + knee * 0.5f) / (2.0f * knee);
            const float target = dbToGain (-reductionDb);
            const float coeff = target < limiterGain ? attackCoeff : releaseCoeff;
            limiterGain = target + coeff * (limiterGain - target);
            int readIdx = lookaheadWrite - lookaheadLen / 2;
            if (readIdx < 0) readIdx += lookaheadLen;
            const float delayedL = lookaheadBufL[(size_t) readIdx];
            const float delayedR = lookaheadBufR[(size_t) readIdx];
            // A linked safety stage catches peaks missed by the attack envelope.
            // This guarantees a sample ceiling, not an intersample true-peak ceiling.
            const float delayedPeak = std::max (std::abs (delayedL), std::abs (delayedR));
            const float safeGain = std::min (limiterGain, ceilingLinear / std::max (ceilingLinear, delayedPeak));
            if (++lookaheadWrite >= lookaheadLen) lookaheadWrite = 0;
            chL[i] = chL[i] * dry + delayedL * safeGain * wet;
            chR[i] = chR[i] * dry + delayedR * safeGain * wet;
        }
    }
}

//==============================================================================
// PremiumTubeEqDsp Implementation (Pultec EQP-1A, MEQ-5, Aphex Exciter, BBE Sonic Maximizer)
void PremiumTubeEqDsp::prepare (double sampleRate)
{
    sr = (float) sampleRate;
    const int bbeLen = (int) (sr * 0.005f); // 5ms group delay buffer
    bbeLowBufL.assign ((size_t) bbeLen, 0.0f);
    bbeLowBufR.assign ((size_t) bbeLen, 0.0f);
    reset();
}

void PremiumTubeEqDsp::reset()
{
    eqpLowBoostL.reset(); eqpLowBoostR.reset();
    eqpLowAttenL.reset(); eqpLowAttenR.reset();
    eqpHighBoostL.reset(); eqpHighBoostR.reset();
    eqpHighAttenL.reset(); eqpHighAttenR.reset();

    meqLowL.reset(); meqLowR.reset();
    meqMidL.reset(); meqMidR.reset();
    meqHighL.reset(); meqHighR.reset();

    exciterHpfL.reset(); exciterHpfR.reset();

    std::fill (bbeLowBufL.begin(), bbeLowBufL.end(), 0.0f);
    std::fill (bbeLowBufR.begin(), bbeLowBufR.end(), 0.0f);
    bbeWrite = 0;
    bbeLpfL.reset(); bbeLpfR.reset();
    bbeHpfL.reset(); bbeHpfR.reset();
}

void PremiumTubeEqDsp::processEnhancer(float* l, float* r, int n, float bassGain, float bassHz,
    float midGain, float midQ, float highGain, float highHz, float width, float outputDb, float mix)
{
    const auto bass = calcLowShelf(bassHz,bassGain,.707f,sr);
    const auto mid = calcBell(1000.0f,midGain,midQ,sr);
    const auto high = calcHighShelf(highHz,highGain,.707f,sr);
    const float gain = dbToGain(outputDb);
    for(int i=0;i<n;++i)
    {
        const float a = eqpHighBoostL.process(meqMidL.process(eqpLowBoostL.process(l[i],bass),mid),high);
        const float b = eqpHighBoostR.process(meqMidR.process(eqpLowBoostR.process(r[i],bass),mid),high);
        const float m = .5f*(a+b), side = .5f*(a-b)*width;
        l[i] = l[i]*(1-mix)+(m+side)*gain*mix;
        r[i] = r[i]*(1-mix)+(m-side)*gain*mix;
    }
}

void PremiumTubeEqDsp::process (float* chL, float* chR, int numSamples,
                                FxType type, float p1, float p2, float p3, float p4, float p5, float p6, float mixNorm, float highQ, float attenHz, float outputDb, bool transformer)
{
    const float wet = mixNorm;
    const float dry = 1.0f - wet;

    // 1. Pultec EQP-1A Passive Tube Program Equalizer (with authentic Low-End Trick)
    if (type == FxType::StereoXtecEQ1 || type == FxType::DualXtecEQ1 || type == FxType::XtecEq1)
    {
        // p1 = Low Boost (0..10), p2 = Low Freq (Hz), p3 = Low Atten (0..10),
        // p4 = High Atten (0..10), p5 = High Boost (0..10), p6 = High Freq (Hz)
        const float lowFreq = clampf (p2 > 0.0f ? p2 : 60.0f, 20.0f, 150.0f);
        const float highFreq = clampf (p6 > 0.0f ? p6 : 8000.0f, 2000.0f, 16000.0f);

        const auto cBoost = calcLowShelf (lowFreq, p1 * 1.4f, 0.707f, sr);
        const auto cAtten = calcLowShelf (lowFreq * 1.35f, -p3 * 1.35f, 0.90f, sr);
        const auto cHighBoost = calcBell (highFreq, p5 * 1.5f, highQ, sr);
        const auto cHighAtten = calcHighShelf (attenHz, -p4 * 1.35f, 0.707f, sr);

        for (int i = 0; i < numSamples; ++i)
        {
            float sL = eqpLowBoostL.process (chL[i], cBoost);
            sL = eqpLowAttenL.process (sL, cAtten);
            sL = eqpHighBoostL.process (sL, cHighBoost);
            sL = eqpHighAttenL.process (sL, cHighAtten);

            float sR = eqpLowBoostR.process (chR[i], cBoost);
            sR = eqpLowAttenR.process (sR, cAtten);
            sR = eqpHighBoostR.process (sR, cHighBoost);
            sR = eqpHighAttenR.process (sR, cHighAtten);

            // 12AX7 passive EQ tube makeup stage saturation
            if (transformer) sL = (sL + 0.06f * sL * sL) / (1.0f + 0.06f * std::abs (sL));
            sL *= dbToGain (outputDb);
            if (transformer) sR = (sR + 0.06f * sR * sR) / (1.0f + 0.06f * std::abs (sR));
            sR *= dbToGain (outputDb);

            chL[i] = chL[i] * dry + sL * wet;
            chR[i] = chR[i] * dry + sR * wet;
        }
    }
    // 2. Pultec MEQ-5 Passive Midrange Equalizer
    else if (type == FxType::StereoXtecEQ5 || type == FxType::DualXtecEQ5 || type == FxType::XtecEq5)
    {
        const float lowMidF = clampf (p2 > 0.0f ? p2 : 500.0f, 200.0f, 1000.0f);
        const float midDipF = clampf (p4 > 0.0f ? p4 : 1500.0f, 200.0f, 7000.0f);
        const float hiMidF = clampf (p6 > 0.0f ? p6 : 3000.0f, 1500.0f, 5000.0f);

        const auto cLowMid = calcBell (lowMidF, p1 * 1.25f, 1.4f, sr);
        const auto cMidDip = calcBell (midDipF, -p3 * 1.25f, 1.4f, sr);
        const auto cHiMid = calcBell (hiMidF, p5 * 1.25f, 1.4f, sr);

        for (int i = 0; i < numSamples; ++i)
        {
            float sL = meqLowL.process (chL[i], cLowMid);
            sL = meqMidL.process (sL, cMidDip);
            sL = meqHighL.process (sL, cHiMid);

            float sR = meqLowR.process (chR[i], cLowMid);
            sR = meqMidR.process (sR, cMidDip);
            sR = meqHighR.process (sR, cHiMid);

            chL[i] = chL[i] * dry + sL * dbToGain (outputDb) * wet;
            chR[i] = chR[i] * dry + sR * dbToGain (outputDb) * wet;
        }
    }
    // 3. Aphex Aural Exciter Type C Harmonic Generator
    else if (type == FxType::StereoExciter || type == FxType::DualExciter)
    {
        // p1 = Tune (1kHz..10kHz), p2 = Peak (0..100%), p3 = Zero Fill (0..100%),
        // p4 = Timbre (-50..+50 odd/even), p5 = Harmonics (0..100%), p6 = Mix (0..100%)
        const float tuneFreq = clampf (p1 > 150.0f ? p1 : (1000.0f + clampf (p1, 0.0f, 100.0f) * 90.0f), 800.0f, 10000.0f);
        const float qPeak = 0.707f + clampf (p2, 0.0f, 100.0f) * 0.025f;
        const auto cHpf = calcHPF (tuneFreq, qPeak, sr);
        const float drive = dbToGain (clampf (p5, 0.0f, 100.0f) * 0.16f);
        const float evenWeight = clampf (0.8f - p4 * 0.008f, 0.2f, 1.4f);
        const float oddWeight = clampf (0.5f + p4 * 0.008f, 0.2f, 1.4f);
        const float harmMix = wet * clampf (p6 * 0.01f, 0.0f, 1.0f);

        for (int i = 0; i < numSamples; ++i)
        {
            const float hL = exciterHpfL.process (chL[i], cHpf);
            const float hR = exciterHpfR.process (chR[i], cHpf);

            const float harmL = (hL * hL * evenWeight + std::sin (hL * 1.8f) * oddWeight) * drive;
            const float harmR = (hR * hR * evenWeight + std::sin (hR * 1.8f) * oddWeight) * drive;

            chL[i] = chL[i] + harmL * harmMix;
            chR[i] = chR[i] + harmR * harmMix;
        }
    }
    // 4. BBE Sonic Maximizer 882i / SPL Vitalizer Stereo Enhancer
    else if (type == FxType::SoundMaxer || type == FxType::StereoEnhancer || type == FxType::DualEnhancer)
    {
        const float bassFreq = (type == FxType::SoundMaxer) ? 150.0f : clampf (p3 > 10.0f ? p3 : 120.0f, 40.0f, 400.0f);
        const float highFreq = (type == FxType::SoundMaxer) ? 2500.0f : clampf (p5 > 100.0f ? p5 : 2500.0f, 1000.0f, 8000.0f);
        const auto cLowX = calcLPF (bassFreq, 0.707f, sr);
        const auto cHighX = calcHPF (highFreq, 0.707f, sr);

        const int bbeDelaySamples = (int) clampf (0.0025f * sr, 16.0f, (float) bbeLowBufL.size() - 1);
        const float lowDrive = dbToGain (clampf (p1, -50.0f, 100.0f) * 0.12f);
        const float highDrive = dbToGain (clampf (p2, -50.0f, 100.0f) * 0.12f);
        const float outGain = (type == FxType::SoundMaxer) ? dbToGain (clampf (p3, -12.0f, 12.0f)) : 1.0f;
        const int bbeMax = (int) bbeLowBufL.size();

        for (int i = 0; i < numSamples; ++i)
        {
            const float lowL = bbeLpfL.process (chL[i], cLowX);
            const float lowR = bbeLpfR.process (chR[i], cLowX);
            const float highL = bbeHpfL.process (chL[i], cHighX);
            const float highR = bbeHpfR.process (chR[i], cHighX);
            const float midL = chL[i] - (lowL + highL);
            const float midR = chR[i] - (lowR + highR);

            bbeLowBufL[(size_t) bbeWrite] = lowL;
            bbeLowBufR[(size_t) bbeWrite] = lowR;

            int rIdx = bbeWrite - bbeDelaySamples;
            if (rIdx < 0) rIdx += bbeMax;

            const float delayedLowL = bbeLowBufL[(size_t) rIdx] * lowDrive;
            const float delayedLowR = bbeLowBufR[(size_t) rIdx] * lowDrive;

            if (++bbeWrite >= bbeMax) bbeWrite = 0;

            const float contourHighL = (highL + 0.15f * highL * std::abs (highL)) * highDrive;
            const float contourHighR = (highR + 0.15f * highR * std::abs (highR)) * highDrive;

            const float outBbeL = (delayedLowL + midL + contourHighL) * outGain;
            const float outBbeR = (delayedLowR + midR + contourHighR) * outGain;

            chL[i] = chL[i] * dry + outBbeL * wet;
            chR[i] = chR[i] * dry + outBbeR * wet;
        }
    }
}

//==============================================================================
// PremiumUtilityAmpDsp Implementation (Guitar Amp, Celestion Cab, Tube Stage, Sub Octaver)
void PremiumUtilityAmpDsp::prepare (double sampleRate)
{
    sr = (float) sampleRate;
    reset();
}

void PremiumUtilityAmpDsp::reset()
{
    outputHpfL.reset(); outputHpfR.reset(); outputLpfL.reset(); outputLpfR.reset();
    toneBassL.reset(); toneBassR.reset();
    toneMidL.reset(); toneMidR.reset();
    toneTrebleL.reset(); toneTrebleR.reset();
    tonePresenceL.reset(); tonePresenceR.reset();

    cabResoL.reset(); cabResoR.reset();
    cabScoopL.reset(); cabScoopR.reset();
    cabRollOffL.reset(); cabRollOffR.reset();

    lastInSignL = 1.0f;
    subCycleCountL = 0;
    subSquareL = 1.0f;
    subCycleCount2L = 0;
    subSquare2L = 1.0f;

    lastInSignR = 1.0f;
    subCycleCountR = 0;
    subSquareR = 1.0f;
    subCycleCount2R = 0;
    subSquare2R = 1.0f;

    subLpfL.reset(); subLpfR.reset();
    subLpf2L.reset(); subLpf2R.reset();
}

void PremiumUtilityAmpDsp::process (float* chL, float* chR, int numSamples,
                                    FxType type, float p1, float p2, float p3, float p4, float mixNorm,
                                    float p5, float p6, float outputDb, float loCutHz, float hiCutHz, float bassHz, float trebleHz)
{
    const float wet = mixNorm;
    const float dry = 1.0f - wet;

    // 1. Marshall JCM800 / Fender Twin Tube Amp & Celestion 4x12 Cabinet
    if (type == FxType::GuitarAmp || type == FxType::StereoGuitarAmp || type == FxType::DualGuitarAmp)
    {
        // p1 = Preamp Drive, p2 = Bass, p3 = Mid, p4 = Treble, p5 = Presence, p6 = Master Out
        const float drive = dbToGain (clampf (p1, 0.0f, 100.0f) * 0.35f);
        const float masterGain = dbToGain ((clampf (p6, 0.0f, 100.0f) - 50.0f) * 0.25f);
        const auto cBass = calcLowShelf (100.0f, (p2 - 50.0f) * 0.25f, 0.707f, sr);
        const auto cMid = calcBell (600.0f, (p3 - 50.0f) * 0.25f, 1.0f, sr);
        const auto cTreble = calcHighShelf (3200.0f, (p4 - 50.0f) * 0.25f, 0.707f, sr);
        const auto cPresence = calcBell (4500.0f, (p5 - 50.0f) * 0.22f, 1.2f, sr);

        const auto cCabReso = calcBell (110.0f, 4.5f, 1.8f, sr);
        const auto cCabScoop = calcBell (400.0f, -3.5f, 1.2f, sr);
        const auto cCabRollOff = calcLPF (5500.0f, 0.707f, sr);

        for (int i = 0; i < numSamples; ++i)
        {
            const float xL = chL[i] * drive;
            const float distL = std::tanh (xL + 0.15f) - std::tanh (0.15f) + 0.08f * std::tanh(xL) * std::tanh(xL);
            const float xR = chR[i] * drive;
            const float distR = std::tanh (xR + 0.15f) - std::tanh (0.15f) + 0.08f * std::tanh(xR) * std::tanh(xR);

            float eqL = toneBassL.process (distL, cBass);
            eqL = toneMidL.process (eqL, cMid);
            eqL = toneTrebleL.process (eqL, cTreble);
            eqL = tonePresenceL.process (eqL, cPresence);

            float eqR = toneBassR.process (distR, cBass);
            eqR = toneMidR.process (eqR, cMid);
            eqR = toneTrebleR.process (eqR, cTreble);
            eqR = tonePresenceR.process (eqR, cPresence);

            float cabL = cabResoL.process (eqL, cCabReso);
            cabL = cabScoopL.process (cabL, cCabScoop);
            cabL = cabRollOffL.process (cabL, cCabRollOff) * masterGain;

            float cabR = cabResoR.process (eqR, cCabReso);
            cabR = cabScoopR.process (cabR, cCabScoop);
            cabR = cabRollOffR.process (cabR, cCabRollOff) * masterGain;

            chL[i] = chL[i] * dry + cabL * wet;
            chR[i] = chR[i] * dry + cabR * wet;
        }
    }
    // 2. Telefunken VF14 Analog Triode Tube Valve Saturation
    else if (type == FxType::TubeStage || type == FxType::StereoTubeStage || type == FxType::DualTubeStage)
    {
        // p1 = Drive (0..100), p2 = Even Harm (0..100), p3 = Odd Harm (0..100), p4 = Bias (-50..50), p5 = Bass (-12..12), p6 = Treble (-12..12)
        const float drive = dbToGain (clampf (p1, 0.0f, 100.0f) * 0.24f);
        const float evenAmt = clampf (p2, 0.0f, 100.0f) * 0.003f;
        const float oddAmt = clampf (p3, 0.0f, 100.0f) * 0.003f;
        const float biasOffset = clampf (p4, -50.0f, 50.0f) * 0.003f;
        const auto cBass = calcLowShelf (bassHz, clampf (p5, -12.0f, 12.0f), 0.707f, sr);
        const auto cTreble = calcHighShelf (trebleHz, clampf (p6, -12.0f, 12.0f), 0.707f, sr);

        const auto cLow = calcHPF (loCutHz, .707f, sr), cHigh = calcLPF (hiCutHz, .707f, sr);
        for (int i = 0; i < numSamples; ++i)
        {
            const float xL = chL[i] * drive + biasOffset;
            const float satL = std::tanh (xL * (1.0f + oddAmt)) - std::tanh (biasOffset * (1.0f + oddAmt)) + evenAmt * (std::tanh(xL) * std::tanh(xL) - std::tanh(biasOffset) * std::tanh(biasOffset));
            const float xR = chR[i] * drive + biasOffset;
            const float satR = std::tanh (xR * (1.0f + oddAmt)) - std::tanh (biasOffset * (1.0f + oddAmt)) + evenAmt * (std::tanh(xR) * std::tanh(xR) - std::tanh(biasOffset) * std::tanh(biasOffset));

            float outL = toneTrebleL.process (toneBassL.process (satL, cBass), cTreble);
            float outR = toneTrebleR.process (toneBassR.process (satR, cBass), cTreble);
            if (loCutHz > 20.0f) { outL = outputHpfL.process(outL,cLow); outR = outputHpfR.process(outR,cLow); }
            if (hiCutHz < 20000.0f) { outL = outputLpfL.process(outL,cHigh); outR = outputLpfR.process(outR,cHigh); }
            outL *= dbToGain(outputDb); outR *= dbToGain(outputDb);

            chL[i] = chL[i] * dry + outL * wet;
            chR[i] = chR[i] * dry + outR * wet;
        }
    }
    // 3. Boss OC-2 Analog Dual Flip-Flop Sub-Octaver Synthesizer
    else if (type == FxType::Suboctaver)
    {
        // p1 = Direct (0..100%), p2 = Oct -1 (0..100%), p3 = Oct -2 (0..100%), p4 = Range (0=Low,1=Mid,2=Hi)
        const float directGain = clampf (p1 * 0.01f, 0.0f, 1.0f);
        const float oct1Gain = clampf (p2 * 0.01f, 0.0f, 1.2f);
        const float oct2Gain = clampf (p3 * 0.01f, 0.0f, 1.2f);
        const float cutoffHz = (p4 >= 1.5f) ? 180.0f : ((p4 >= 0.5f) ? 120.0f : 75.0f);
        const auto cSubLpf = calcLPF (cutoffHz, 0.707f, sr);

        for (int i = 0; i < numSamples; ++i)
        {
            const float inL = chL[i];
            const float inR = chR[i];

            const float signL = inL >= 0.0f ? 1.0f : -1.0f;
            if (signL != lastInSignL && signL > 0.0f)
            {
                if (++subCycleCountL >= 1)
                {
                    subCycleCountL = 0;
                    subSquareL = -subSquareL;
                    if (++subCycleCount2L >= 2)
                    {
                        subCycleCount2L = 0;
                        subSquare2L = -subSquare2L;
                    }
                }
            }
            lastInSignL = signL;

            const float signR = inR >= 0.0f ? 1.0f : -1.0f;
            if (signR != lastInSignR && signR > 0.0f)
            {
                if (++subCycleCountR >= 1)
                {
                    subCycleCountR = 0;
                    subSquareR = -subSquareR;
                    if (++subCycleCount2R >= 2)
                    {
                        subCycleCount2R = 0;
                        subSquare2R = -subSquare2R;
                    }
                }
            }
            lastInSignR = signR;

            const float envL = std::abs (inL);
            const float envR = std::abs (inR);
            const float sub1L = subLpfL.process (subSquareL * envL, cSubLpf) * oct1Gain;
            const float sub2L = subLpf2L.process (subSquare2L * envL, cSubLpf) * oct2Gain;
            const float sub1R = subLpfR.process (subSquareR * envR, cSubLpf) * oct1Gain;
            const float sub2R = subLpf2R.process (subSquare2R * envR, cSubLpf) * oct2Gain;

            const float wetL = inL * directGain + sub1L + sub2L;
            const float wetR = inR * directGain + sub1R + sub2R;

            chL[i] = inL * dry + wetL * wet;
            chR[i] = inR * dry + wetR * wet;
        }
    }
    // 4. Edison: independent low/high stereo spread, balance and centre.
    else if (type == FxType::EdisonEX1)
    {
        const float width = clampf(p1 * .02f,0.0f,2.0f);
        const float lowWidth = clampf(p2 * .02f,0.0f,2.0f);
        const float center = dbToGain((p5 - 50.0f) * .15f);
        const auto balance = balanceStereo((p3 - 50.0f) * .02f);
        const auto coeff = calcLPF(120.0f,.707f,sr);
        const float gain = dbToGain(p4);
        for (int i = 0; i < numSamples; ++i)
        {
            const float mid = .5f * (chL[i]+chR[i]) * center;
            const float side = .5f * (chL[i]-chR[i]);
            const float low = subLpfL.process(side,coeff);
            const float spread = (side-low) * width + low * lowWidth;
            const float l = (mid+spread) * balance.left * gain;
            const float r = (mid-spread) * balance.right * gain;
            chL[i] = chL[i]*dry + l*wet; chR[i] = chR[i]*dry + r*wet;
        }
    }
    // Stereo Imager exposes panning and a side-channel shelf, not Edison spread.
    else if (type == FxType::StereoImager)
    {
        const auto balance = balanceStereo((p1-50.0f)*.02f);
        const auto monoPan = panMonoConstantPower((p2-50.0f)*.02f);
        const auto stereoPan = balanceStereo((p3-50.0f)*.02f);
        const auto shelf = calcHighShelf(p5,p4,p6,sr);
        const float gain = dbToGain(outputDb);
        for (int i = 0; i < numSamples; ++i)
        {
            const float mid = .5f*(chL[i]+chR[i]);
            const float side = toneTrebleL.process(.5f*(chL[i]-chR[i]),shelf);
            const float l = (mid*monoPan.left*1.41421356f + side*stereoPan.left)*balance.left*gain;
            const float r = (mid*monoPan.right*1.41421356f - side*stereoPan.right)*balance.right*gain;
            chL[i] = chL[i]*dry + l*wet; chR[i] = chR[i]*dry + r*wet;
        }
    }
}

/* SSL G-channel implementation removed. */
/*
    const float hpfHz = 20.0f * std::pow (17.5f, clampf(p[0],0,1));
    const float lpfHz = 3000.0f * std::pow (4.0f, clampf(p[1],0,1));
    const auto hp = calcHPF(hpfHz, .707f, sr), lp = calcLPF(lpfHz, .707f, sr);
    const float hfHz = 1500.0f * std::pow (16000.0f/1500.0f, clampf(p[4],0,1));
    const float hmHz = (800.0f * std::pow(10.0f, clampf(p[7],0,1))) * (p[9] > .5f ? .333333f : 1.0f);
    const float lmHz = (200.0f * std::pow(12.5f, clampf(p[11],0,1))) * (p[13] > .5f ? 3.0f : 1.0f);
    const float lfHz = 30.0f * std::pow(15.0f, clampf(p[15],0,1));
    const float hfGain = (p[3]-.5f)*24.0f, hmGain=(p[6]-.5f)*24.0f;
    const float lmGain = (p[10]-.5f)*24.0f, lfGain=(p[14]-.5f)*24.0f;
    const auto hfc = p[5] > .5f ? calcBell(hfHz,hfGain,.9f,sr) : calcHighShelf(hfHz,hfGain,.707f,sr);
    const auto hmc = calcBell(hmHz,hmGain,0.5f+clampf(p[8],0,1)*9.5f,sr);
    const auto lmc = calcBell(lmHz,lmGain,0.5f+clampf(p[12],0,1)*9.5f,sr);
    const auto lfc = p[16] > .5f ? calcBell(lfHz,lfGain,.9f,sr) : calcLowShelf(lfHz,lfGain,.707f,sr);
    const float wet = clampf(mix,0,1), dry=1.0f-wet;
    for (int i=0;i<n;++i)
    {
        const float inL=l[i], inR=r[i];
        float aL=hpL.process(inL,hp), aR=hpR.process(inR,hp);
        aL=lpL.process(aL,lp); aR=lpR.process(aR,lp);
        float dL = p[2]>.5f ? aL : inL, dR=p[2]>.5f ? aR : inR;
        const float targetL=std::abs(dL), targetR=std::abs(dR);
        const float atk = std::exp(-1.0f/(sr*0.001f)), rel=std::exp(-1.0f/(sr*.12f));
        envL = targetL > envL ? atk*envL+(1-atk)*targetL : rel*envL+(1-rel)*targetL;
        envR = targetR > envR ? atk*envR+(1-atk)*targetR : rel*envR+(1-rel)*targetR;
        const float compL = envL > .22f ? std::pow(.22f/std::max(.22f,envL),.75f) : 1.0f;
        const float compR = envR > .22f ? std::pow(.22f/std::max(.22f,envR),.75f) : 1.0f;
        float oL=hfL.process(aL,hfc); oL=hmfL.process(oL,hmc); oL=lmfL.process(oL,lmc); oL=lfL.process(oL,lfc);
        float oR=hfR.process(aR,hfc); oR=hmfR.process(oR,hmc); oR=lmfR.process(oR,lmc); oR=lfR.process(oR,lfc);
        oL*=compL; oR*=compR;
        if (p[17] > .5f) { oL=std::tanh(oL*1.08f)*.98f; oR=std::tanh(oR*1.08f)*.98f; }
        l[i]=inL*dry+oL*wet; r[i]=inR*dry+oR*wet;
    }
}*/

//==============================================================================
// Unified FxSlotProcessor
void FxSlotProcessor::prepare (double sampleRate, bool internalWorker)
{
    sr = sampleRate;
    workL.assign (4096, 0.0f);
    workR.assign (4096, 0.0f);
    dualInputR.assign (4096, 0.0f);
    if (! internalWorker)
    {
        rightWorker = std::make_unique<FxSlotProcessor>();
        rightWorker->prepare (sampleRate, true);
    }
    if (! internalWorker)
    {
        reverb.prepare (sampleRate); delay.prepare (sampleRate);
        modulation.prepare (sampleRate); pitchShifter.prepare (sampleRate); graphicEq.prepare (sampleRate);
    }
    dynamics.prepare (sampleRate);
    tubeEq.prepare (sampleRate);
    utilityAmp.prepare (sampleRate);
    reset();
}

void FxSlotProcessor::reset()
{
    lastType = -1;
    if (rightWorker) rightWorker->reset();
    reverb.reset();
    delay.reset();
    modulation.reset();
    pitchShifter.reset();
    graphicEq.reset();
    dynamics.reset();
    tubeEq.reset();
    utilityAmp.reset();
}

namespace
{
// Helper: detect if a parameter is stored as UI-normalized [0..1] vs physical units
inline float mapNormOrRaw (float val, float minPhys, float maxPhys, bool isNormalized)
{
    if (isNormalized)
        return minPhys + clampf (val, 0.0f, 1.0f) * (maxPhys - minPhys);
    return clampf (val, minPhys, maxPhys);
}

inline float mapExpNormOrRaw (float val, float minPhys, float maxPhys, bool isNormalized)
{
    if (isNormalized)
        return minPhys * std::pow (maxPhys / minPhys, clampf (val, 0.0f, 1.0f));
    return clampf (val, minPhys, maxPhys);
}
} // namespace

void FxSlotProcessor::process (const float* inL, const float* inR,
                              float* outL, float* outR,
                              int numSamples, FxSlotParams& params)
{
    if (inL == nullptr || outL == nullptr || numSamples <= 0) return;
    const float* srcR = (inR != nullptr) ? inR : inL;

    const auto requestedType = (FxType) params.fxType.getInt();
    if (lastType != (int) requestedType) { reset(); lastType = (int) requestedType; }
    if (! monoPass && rightWorker && ! params.bypass.getBool()
        && (usesIndependentFxParameters (requestedType) || requestedType == FxType::SoundMaxer
            || requestedType == FxType::Suboctaver))
    {
        FxSlotParams left, right;
        left.fxType.set (params.fxType.get()); right.fxType.set (params.fxType.get());
        left.mix.set (params.mix.get()); right.mix.set (params.mix.get());
        left.parameterEncoding.set(params.parameterEncoding.get()); right.parameterEncoding.set(params.parameterEncoding.get());
        const bool inlineBanks = requestedType == FxType::SoundMaxer || requestedType == FxType::Suboctaver;
        bool normalized = true;
        for (int j = 0; j < 6; ++j) normalized &= std::abs (params.parameter(j).get()) <= 1.0f;
        if (params.parameterEncoding.getInt() != 0) normalized = params.parameterEncoding.getInt() == 2;
        for (int j = 0; j < FxSlotParams::kNumParameters; ++j)
        {
            left.parameter(j).set (params.parameter(j).get());
            right.parameter(j).set (params.channelParameter(1, j).get());
        }
        if (inlineBanks && normalized)
        {
            for (int j = 0; j < 3; ++j) right.parameter(j).set (params.parameter(j + 3).get());
            if (requestedType == FxType::Suboctaver) { left.p4.set (.5f); right.p4.set (.5f); }
        }
        float ipL = 0, ipR = 0;
        for (int i = 0; i < numSamples; ++i) { ipL = std::max (ipL, std::abs(inL[i])); ipR = std::max (ipR, std::abs(srcR[i])); }
        if ((int) dualInputR.size() < numSamples) dualInputR.resize ((size_t)numSamples);
        std::copy_n (srcR, numSamples, dualInputR.data());
        monoPass = true;
        process (inL, inL, outL, outL, numSamples, left);
        monoPass = false;
        rightWorker->monoPass = true;
        rightWorker->process (dualInputR.data(), dualInputR.data(), outR, outR, numSamples, right);
        rightWorker->monoPass = false;
        float opL = 0, opR = 0;
        for (int i = 0; i < numSamples; ++i) { opL = std::max (opL, std::abs(outL[i])); opR = std::max (opR, std::abs(outR[i])); }
        inPeakL.update(ipL); inPeakR.update(ipR); outPeakL.update(opL); outPeakR.update(opR);
        return;
    }

    // Track input peak meters
    float inPkL = 0.0f, inPkR = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        inPkL = std::max (inPkL, std::abs (inL[i]));
        inPkR = std::max (inPkR, std::abs (srcR[i]));
    }
    if (! monoPass) { inPeakL.update (inPkL); inPeakR.update (inPkR); }

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
    auto dspType = type;
    if (monoPass && type == FxType::DualLeisureCompressor) dspType = FxType::StereoLeisureCompressor;
    switch (type)
    {
        case FxType::DualXtecEQ1: dspType = FxType::StereoXtecEQ1; break;
        case FxType::DualXtecEQ5: dspType = FxType::StereoXtecEQ5; break;
        case FxType::DualExciter: dspType = FxType::StereoExciter; break;
        case FxType::DualEnhancer: dspType = FxType::StereoEnhancer; break;
        case FxType::DualGuitarAmp: dspType = FxType::StereoGuitarAmp; break;
        case FxType::DualTubeStage: dspType = FxType::StereoTubeStage; break;

        default: break;
    }
    const auto meta = getFxMetadata (type);
    const float mixVal = params.mix.get();

    const float rawP1 = params.p1.get();
    const float rawP2 = params.p2.get();
    const float rawP3 = params.p3.get();
    const float rawP4 = params.p4.get();
    const float rawP5 = params.p5.get();
    const float rawP6 = params.p6.get();
    const float rawP7 = params.p7.get();
    const float rawP8 = params.p8.get();
    const float rawP9 = params.p9.get();
    const float rawP10 = params.p10.get();
    const float rawP11 = params.p11.get();
    const float rawP12 = params.p12.get();

    // Detect if UI wrote normalized [0.0..1.0] values vs legacy raw test values (> 1.0f)
    const bool norm16 = params.parameterEncoding.getInt() == 2 || (params.parameterEncoding.getInt() == 0 && (std::abs (rawP1) <= 1.0f && std::abs (rawP2) <= 1.0f
                      && std::abs (rawP3) <= 1.0f && std::abs (rawP4) <= 1.0f
                      && std::abs (rawP5) <= 1.0f && std::abs (rawP6) <= 1.0f));

    // 1. Handle Combination Effects (Multi-Engine Chained Processing)
    if (type == FxType::DelayAndChamber || type == FxType::ChorusAndChamber || type == FxType::FlangerAndChamber)
    {
        // p1=Delay/Speed, p2=Fback/Depth, p3=LoCut/ChorusMix, p4=PreDel, p5=Decay, p6=RevMix
        const float revPreDel = mapNormOrRaw (norm16 ? rawP7 : rawP4, 0.0f, 100.0f, norm16);
        const float revDecay = mapNormOrRaw (norm16 ? rawP8 : rawP5, 0.2f, 5.0f, norm16);
        const float revMix = clampf ((norm16 ? rawP6 : rawP6 * 0.01f) * mixVal, 0.0f, 1.0f);

        if (type == FxType::DelayAndChamber)
        {
            const float dlyTime = mapNormOrRaw (rawP1, 10.0f, 1500.0f, norm16);
            const float dlyFb = mapNormOrRaw (norm16 ? rawP4 : rawP2, 0.0f, 90.0f, norm16);
            const float dlyLoCut = mapExpNormOrRaw (rawP3, 20.0f, 500.0f, norm16);
            delay.process (workL.data(), workR.data(), numSamples, FxType::StereoDelay,
                           dlyTime, dlyTime, dlyFb, 0.5f, 50.0f, mixVal * 0.5f, dlyLoCut, 12000.0f);
        }
        else if (type == FxType::ChorusAndChamber)
        {
            const float chSpeed = mapExpNormOrRaw (rawP1, 0.05f, 5.0f, norm16);
            const float chDepth = mapNormOrRaw (rawP2, 0.0f, 100.0f, norm16);
            const float chMix = clampf ((norm16 ? 1.0f - rawP6 : rawP3 * 0.01f) * mixVal, 0.0f, 1.0f);
            modulation.process (workL.data(), workR.data(), numSamples, FxType::StereoChorus,
                                chSpeed, chDepth, 5.0f, 90.0f, chMix);
        }
        else // FlangerAndChamber
        {
            const float flSpeed = mapExpNormOrRaw (rawP1, 0.05f, 5.0f, norm16);
            const float flDepth = mapNormOrRaw (rawP2, 0.0f, 100.0f, norm16);
            const float flFb = mapNormOrRaw (rawP3, -90.0f, 90.0f, norm16);
            modulation.process (workL.data(), workR.data(), numSamples, FxType::StereoFlanger,
                                flSpeed, flDepth, flFb, 90.0f, mixVal * 0.5f);
        }

        reverb.process (workL.data(), workR.data(), numSamples, FxType::ChamberReverb,
                        revDecay, revPreDel, norm16 ? rawP10 * 100.0f : 45.0f,
                        norm16 ? rawP9 * 100.0f : 65.0f, 80.0f, 0.0f, revMix,
                        norm16 ? mapExpNormOrRaw(rawP11,20,500,true) : 20.0f);
    }
    else if (type == FxType::DelayAndChorus || type == FxType::DelayAndFlanger || type == FxType::ModulationDelay)
    {
        const float dlyTime = mapNormOrRaw (rawP1, 10.0f, 2000.0f, norm16);
        const float dlyFb = mapNormOrRaw (norm16 && type != FxType::ModulationDelay ? rawP4 : rawP2, 0.0f, 95.0f, norm16);

        if (type == FxType::ModulationDelay)
        {
            const float hiCut = mapExpNormOrRaw (norm16 ? rawP4 : rawP5, 1000.0f, 20000.0f, norm16);
            const float modDepth = mapNormOrRaw (norm16 ? rawP5 : rawP3, 0.0f, 100.0f, norm16);
            const float modSpeed = mapExpNormOrRaw (norm16 ? rawP6 : rawP4, 0.1f, 8.0f, norm16);
            delay.process (workL.data(), workR.data(), numSamples, FxType::ModulationDelay,
                           dlyTime, dlyTime, dlyFb, 0.5f, 50.0f, mixVal, 20.0f, hiCut);
            modulation.process (workL.data(), workR.data(), numSamples, FxType::StereoChorus,
                                modSpeed, modDepth, 3.0f, 90.0f, mixVal * 0.4f);
        }
        else if (type == FxType::DelayAndChorus)
        {
            const float chSpeed = mapExpNormOrRaw (norm16 ? rawP7 : rawP3, 0.05f, 5.0f, norm16);
            const float chDepth = mapNormOrRaw (norm16 ? rawP8 : rawP4, 0.0f, 100.0f, norm16);
            const float modMix = clampf ((norm16 ? rawP5 : rawP5 * 0.01f) * mixVal, 0.0f, 1.0f);
            delay.process (workL.data(), workR.data(), numSamples, FxType::StereoDelay,
                           dlyTime, dlyTime, dlyFb, 0.5f, 50.0f, mixVal * 0.6f);
            modulation.process (workL.data(), workR.data(), numSamples, FxType::StereoChorus,
                                chSpeed, chDepth, 4.0f, 90.0f, modMix);
        }
        else // DelayAndFlanger
        {
            const float flSpeed = mapExpNormOrRaw (norm16 ? rawP7 : rawP3, 0.05f, 5.0f, norm16);
            const float flDepth = mapNormOrRaw (norm16 ? rawP8 : rawP4, 0.0f, 100.0f, norm16);
            const float flFb = mapNormOrRaw (norm16 ? rawP11 : rawP5, -90.0f, 90.0f, norm16);
            const float modMix = clampf ((norm16 ? rawP6 : rawP6 * 0.01f) * mixVal, 0.0f, 1.0f);
            delay.process (workL.data(), workR.data(), numSamples, FxType::StereoDelay,
                           dlyTime, dlyTime, dlyFb, 0.5f, 50.0f, mixVal * 0.6f);
            modulation.process (workL.data(), workR.data(), numSamples, FxType::StereoFlanger,
                                flSpeed, flDepth, flFb, 90.0f, modMix);
        }
    }
    // 2. Standard Category Routing with Normalized + Physical Parameter Mapping
    else switch (meta.category)
    {
        case FxCategory::Reverb:
        {
            // UI LARC Reverb order: p1=PRE DEL, p2=DECAY, p3=SIZE, p4=DAMP/F.BACK, p5=DIFFUSE/LO CUT, p6=LEVEL
            // Legacy raw order: p1=decaySec (1.8), p2=preDelayMs (24), p3=damping (0.4), p4=size (0.7), p5=diff (0.8), p6=levelDb (0)
            const bool isUiLarcOrder = norm16;

            float decaySec = 1.8f, preDelayMs = 24.0f, dampingNorm = 0.4f, sizeNorm = 0.7f, diffNorm = 0.8f, levelDb = 0.0f;
            if (isUiLarcOrder)
            {
                preDelayMs = rawP1 * 200.0f;
                decaySec = 0.2f + rawP2 * 9.8f;
                sizeNorm = rawP3 * 100.0f;
                dampingNorm = rawP4 * 100.0f;
                diffNorm = rawP5 * 100.0f;
                levelDb = (rawP6 - 0.5f) * 24.0f;
            }
            else
            {
                decaySec = rawP1;
                preDelayMs = rawP2;
                dampingNorm = rawP3;
                sizeNorm = rawP4;
                diffNorm = rawP5;
                levelDb = rawP6;
            }

            float loCutHz = mapExpNormOrRaw (rawP7, 20.0f, 500.0f, norm16);
            float hiCutHz = mapExpNormOrRaw (rawP8, 1500.0f, 20000.0f, norm16);
            float bassMult = mapNormOrRaw (rawP9, 0.5f, 2.0f, norm16);
            float spread = mapNormOrRaw (rawP10, 0.0f, 100.0f, norm16);
            float spinNorm = mapNormOrRaw (norm16 ? rawP12 : rawP11, 0.0f, 100.0f, norm16);
            float echoLMs = mapNormOrRaw (params.p13.get(), 0.0f, 500.0f, norm16);
            float echoRMs = mapNormOrRaw (params.p14.get(), 0.0f, 500.0f, norm16);
            float echoFbL = mapNormOrRaw (params.p15.get(), 0.0f, 90.0f, norm16);
            float echoFbR = mapNormOrRaw (params.p16.get(), 0.0f, 90.0f, norm16);

            if (norm16 && type == FxType::VintageReverb)
            {
                decaySec = 0.4f + rawP1 * 4.1f; preDelayMs = rawP2 * 100.0f;
                bassMult = 0.5f + rawP3 * 1.5f; dampingNorm = (1.0f - rawP4) * 90.0f;
                spinNorm = rawP5 * 100.0f; levelDb = (rawP6 - 0.5f) * 24.0f;
                sizeNorm = 70.0f; diffNorm = 70.0f; loCutHz = 20.0f; hiCutHz = 18000.0f;
                echoLMs = echoRMs = 0.0f;
            }
            if (norm16 && type == FxType::PlateReverb)
            {
                levelDb = 0.0f;
                loCutHz = mapExpNormOrRaw (rawP6, 20, 500, true);
                hiCutHz = mapExpNormOrRaw (rawP7, 1500, 20000, true);
                bassMult = 0.5f + rawP8 * 1.5f;
                spinNorm = rawP11 * 100.0f;
            }
            float reverbWet = mixVal;
            float attack = norm16 ? rawP11 * 100.0f : 0.0f;
            if (norm16 && (type == FxType::GatedReverb || type == FxType::ReverseReverb))
            {
                attack = rawP3 * 100.0f; sizeNorm = 60.0f; diffNorm = rawP4 * 100.0f;
                spread = rawP5 * 100.0f; levelDb = 0.0f;
                loCutHz = mapExpNormOrRaw(rawP6,20,500,true);
                hiCutHz = mapExpNormOrRaw(rawP7,1000,20000,true);
                reverbWet *= type == FxType::GatedReverb ? rawP10 : rawP9;
                echoLMs = echoRMs = 0.0f;
            }
            if (norm16 && type == FxType::PlateReverb) { attack = 0.0f; reverbWet *= rawP12; }
            if (type == FxType::VintageReverb) attack = 0.0f;
            reverb.process (workL.data(), workR.data(), numSamples, dspType,
                            decaySec, preDelayMs, dampingNorm, sizeNorm, diffNorm, levelDb, reverbWet,
                            loCutHz, hiCutHz, bassMult, spread, spinNorm,
                            echoLMs, echoRMs, echoFbL, echoFbR, attack);
            break;
        }

        case FxCategory::Delay:
        {
            float timeL = rawP1, timeR = rawP2, feedback = rawP3;
            float cross = rawP4, damp = rawP5, loCut = 20.0f, hiCut = 18000.0f;
            float wet = mixVal;
            if (norm16)
            {
                const bool stereo = type == FxType::StereoDelay;
                const float base = 5.0f + (stereo ? rawP2 : rawP1) * 2495.0f;
                timeL = base * (stereo ? 0.25f + rawP5 * 0.75f : 1.0f);
                timeR = base * (stereo ? 0.25f + rawP6 * 0.75f : 1.5f);
                if (stereo)
                {
                    timeR += (rawP7 - 0.5f) * 100.0f;
                    feedback = 95.0f * 0.5f * (rawP9 + rawP10);
                    cross = 0.0f;
                    loCut = mapExpNormOrRaw (rawP3, 20, 800, true);
                    hiCut = mapExpNormOrRaw (rawP4, 1000, 20000, true);
                    wet *= rawP1;
                }
                else
                {
                    feedback = 95.0f * (type == FxType::TapDelay ? rawP4 : rawP3);
                    loCut = mapExpNormOrRaw (rawP5, 20, 800, true);
                    hiCut = mapExpNormOrRaw (rawP6, 1000, 20000, true);
                    cross = 0.0f;
                }
                damp = 0.0f;
            }
            delay.process (workL.data(), workR.data(), numSamples, dspType,
                           timeL, timeR, feedback, cross, damp, wet, loCut, hiCut);
            break;
        }

        case FxCategory::Modulation:
        {
            float speed = mapExpNormOrRaw (rawP1, 0.05f, 8.0f, norm16);
            float depth = mapNormOrRaw (rawP2, 0.0f, 100.0f, norm16);
            float feedback = rawP3, custom = rawP4, wet = mixVal;
            float mode = 0.0f, widthR = -1.0f, dl = -1.0f, dr = -1.0f, phase = 90.0f, accel = 0.5f;
            if (norm16)
            {
                if (type == FxType::StereoChorus || type == FxType::StereoFlanger)
                {
                    widthR = rawP3 * 100.0f;
                    const float range = type == FxType::StereoFlanger ? 8.0f : 30.0f;
                    dl = 0.2f + rawP4 * range; dr = 0.2f + rawP5 * range;
                    feedback = type == FxType::StereoFlanger ? (rawP12 - 0.5f) * 180.0f : 0.0f;
                    phase = rawP9 * 180.0f; wet *= rawP6;
                }
                else if (type == FxType::StereoPhaser)
                {
                    feedback = rawP3 * 100.0f; custom = 100.0f * std::pow (40.0f, rawP4);
                    phase = rawP8 * 180.0f; wet *= rawP6;
                }
                else if (type == FxType::MoodFilter)
                {
                    feedback = rawP3 * 100.0f; custom = 80.0f * std::pow (200.0f, rawP4);
                    mode = rawP5; wet *= rawP6;
                }
                else if (type == FxType::TremoloPanner)
                {
                    depth = rawP4 * 100.0f; feedback = rawP2 * 180.0f; custom = rawP3 * 100.0f;
                }
                else if (type == FxType::RotarySpeakerCabinet)
                {
                    speed = 0.1f + rawP1 * 2.0f; custom = 3.0f + rawP2 * 5.0f;
                    depth = rawP5 * 100.0f; accel = rawP3;
                    mode = params.p7.get(); wet *= rawP6;
                }
                else if (type == FxType::DimensionC)
                {
                    // Mode switches combine their depth contributions; stereo
                    // switch controls the spatial cross-bleed independently.
                    depth = 15.0f * rawP2 + 30.0f * rawP3 + 50.0f * rawP4 + 75.0f * rawP5;
                    if (depth == 0.0f) wet = 0.0f;
                    custom = rawP1;
                }
            }
            modulation.process (workL.data(), workR.data(), numSamples, dspType,
                                speed, depth, feedback, custom, wet, mode, widthR, dl, dr, phase, accel);
            break;
        }

        case FxCategory::Pitch:
        {
            if (type == FxType::DualPitch)
            {
                const float semi1 = norm16 ? std::round ((rawP1 - 0.5f) * 24.0f) : rawP1;
                const float cent1 = norm16 ? ((rawP2 - 0.5f) * 100.0f) : rawP2;
                const float dly1 = norm16 ? (rawP3 * 100.0f) : rawP3;
                const float semi2 = norm16 ? std::round ((rawP6 - 0.5f) * 24.0f) : rawP4;
                const float cent2 = norm16 ? ((rawP7 - 0.5f) * 100.0f) : rawP5;
                const float dly2 = norm16 ? (rawP8 * 100.0f) : rawP6;
                pitchShifter.processDual (workL.data(), workR.data(), numSamples,
                                          semi1, cent1, dly1, norm16 ? rawP4 * 100.0f : 85.0f, norm16 ? rawP5 * 100.0f : 25.0f,
                                          semi2, cent2, dly2, norm16 ? rawP9 * 100.0f : 85.0f, norm16 ? rawP10 * 100.0f : 75.0f, mixVal);
            }
            else
            {
                const float semi = norm16 ? (std::round ((rawP1 - 0.5f) * 24.0f) + (rawP2 - 0.5f)) : rawP1;
                pitchShifter.process (workL.data(), workR.data(), numSamples, semi, mixVal);
            }
            break;
        }

        case FxCategory::GraphicEQ:
            graphicEq.process (workL.data(), workR.data(), numSamples,
                               params.geqGainsL, params.geqGainsR,
                               (type == FxType::StereoGEQ || type == FxType::StereoTruEQ),
                               (type == FxType::StereoTruEQ || type == FxType::DualTruEQ));
            for (int i = 0; i < numSamples; ++i)
            {
                workL[i] = inL[i] * (1.0f - mixVal) + workL[i] * mixVal;
                workR[i] = srcR[i] * (1.0f - mixVal) + workR[i] * mixVal;
            }
            break;

        case FxCategory::PremiumDynamics:
        {
            float p1 = rawP1, p2 = rawP2, p3 = rawP3, p4 = rawP4, p5 = rawP5, p6 = rawP6;
            if (norm16)
            {
                if (type == FxType::WaveDesigner)
                {
                    p1 = (rawP1 - 0.5f) * 200.0f; // -100..+100%
                    p2 = (rawP2 - 0.5f) * 200.0f;
                    p3 = (rawP3 - 0.5f) * 24.0f;  // -12..+12 dB
                    p4 = (rawP4 - 0.5f) * 200.0f;
                    p5 = (rawP5 - 0.5f) * 200.0f;
                    p6 = (rawP6 - 0.5f) * 24.0f;
                }
                else if (type == FxType::PrecisionLimiter)
                {
                    p1 = rawP1 * 18.0f;           // Input Gain 0..18 dB
                    p2 = -18.0f + rawP2 * 18.0f;  // Out Ceiling -18..0 dB
                    p3 = rawP3 * 100.0f;          // Squeeze %
                    p4 = rawP6 * 10.0f;           // Knee dB
                    p5 = 0.05f + rawP4 * 10.0f;   // Attack ms
                    p6 = 20.0f + rawP5 * 980.0f;  // Release ms
                }
                else if (type == FxType::UltimoCompressor || type == FxType::DualUltimoCompressor)
                {
                    p1 = rawP1 * 100.0f;
                    p2 = rawP2 * 100.0f;
                    // Ratio step: 4, 8, 12, 20, All-Buttons(24)
                    const float ratios[] = { 4.0f, 8.0f, 12.0f, 20.0f };
                    p3 = (rawP6 > 0.5f) ? 24.0f : ratios[std::clamp ((int) std::round (rawP5 * 3.0f), 0, 3)];
                    p4 = rawP3 * 100.0f; // Attack
                    p5 = rawP4 * 100.0f; // Release
                }
                else if (type == FxType::FairCompressor || type == FxType::DualFairCompressor || type == FxType::MsFairCompressor)
                {
                    p1 = rawP2 * 100.0f;
                    p2 = rawP4 * 100.0f;
                    p3 = rawP5 * 100.0f;
                    p4 = 1.0f + rawP3 * 5.0f;
                    p5 = rawP6 * 100.0f;
                    const float inputGain = dbToGain ((rawP1 - 0.5f) * 24.0f);
                    for (int i = 0; i < numSamples; ++i) { workL[i] *= inputGain; workR[i] *= inputGain; }
                }
                else if (type == FxType::Combinator || type == FxType::DualCombinator)
                {
                    p1 = rawP5 * 100.0f;
                    p2 = rawP6 * 100.0f;
                    p3 = 1.0f + rawP7 * 9.0f;
                    p4 = rawP2 * 100.0f;
                    p5 = rawP3 * 100.0f;
                    p6 = rawP4 * 100.0f;
                }
                else if (type == FxType::StereoLeisureCompressor || type == FxType::DualLeisureCompressor)
                {
                    p1 = rawP2 * 100.0f; p2 = rawP1 * 100.0f; p3 = 0.0f;
                    p4 = p1; p5 = p2; p6 = 0.0f;
                }
                else // De-Esser
                {
                    p1 = rawP1 * 100.0f;
                    p2 = rawP2 * 100.0f;
                    p3 = 0.0f;
                    p4 = rawP3 * 100.0f;
                    p5 = rawP4 * 100.0f;
                    p6 = 0.0f;
                }
            }
            dynamics.process (workL.data(), workR.data(), numSamples, dspType,
                              p1, p2, p3, p4,
                              norm16 && (type == FxType::Combinator || type == FxType::DualCombinator) ? mixVal * rawP1 : mixVal,
                              p5, p6);
            if (norm16 && (type == FxType::StereoLeisureCompressor || type == FxType::DualLeisureCompressor))
            {
                const float outputGain = dbToGain((rawP3 - .5f) * 24.0f);
                for (int i = 0; i < numSamples; ++i) { workL[i] *= outputGain; workR[i] *= outputGain; }
            }
            break;
        }

        case FxCategory::PremiumTubeEq:
        {
            if (norm16 && (type == FxType::StereoEnhancer || type == FxType::DualEnhancer))
            {
                const bool stereo = type == FxType::StereoEnhancer;
                const int offset = stereo ? 1 : 0;
                const auto v = [&params](int index) { return params.parameter(index).get(); };
                tubeEq.processEnhancer(workL.data(),workR.data(),numSamples,
                    (v(1+offset)-.5f)*24.0f, mapExpNormOrRaw(v(2+offset),40,400,true),
                    (v(3+offset)-.5f)*24.0f, .3f+v(4+offset)*3.7f,
                    (v(5+offset)-.5f)*24.0f, mapExpNormOrRaw(v(6+offset),1000,16000,true),
                    stereo ? rawP2*2.0f : 1.0f, (rawP1-.5f)*24.0f, mixVal);
                break;
            }
            float p1 = rawP1, p2 = rawP2, p3 = rawP3, p4 = rawP4, p5 = rawP5, p6 = rawP6;
            float highQ = 1.1f, attenHz = 10000.0f, outputDb = 0.0f;
            bool transformer = true;
            if (norm16)
            {
                if (type == FxType::StereoXtecEQ1 || type == FxType::DualXtecEQ1 || type == FxType::XtecEq1)
                {
                    // UI: gain, low boost/atten, high boost/atten, atten selector, low freq, bandwidth, high freq, transformer
                    const float loFreqs[4] = { 20.0f, 30.0f, 60.0f, 100.0f };
                    const float hiFreqs[7] = { 3000.0f, 4000.0f, 5000.0f, 8000.0f, 10000.0f, 12000.0f, 16000.0f };
                    p2 = loFreqs[std::clamp ((int) std::round (rawP7 * 3.0f), 0, 3)];
                    p1 = rawP2 * 10.0f;
                    p3 = rawP3 * 10.0f;
                    p4 = rawP5 * 10.0f;
                    p6 = hiFreqs[std::clamp ((int) std::round (rawP9 * 6.0f), 0, 6)];
                    p5 = rawP4 * 10.0f;
                    highQ = 0.3f + (1.0f - rawP8) * 3.7f;
                    const float attenFreqs[] = { 5000.0f, 10000.0f, 20000.0f };
                    attenHz = attenFreqs[std::clamp ((int) std::round (rawP6 * 2.0f), 0, 2)];
                    outputDb = (rawP1 - 0.5f) * 24.0f;
                    transformer = rawP10 > 0.5f;
                }
                else if (type == FxType::StereoXtecEQ5 || type == FxType::DualXtecEQ5 || type == FxType::XtecEq5)
                {
                    p2 = 200.0f + rawP2 * 800.0f;
                    p1 = rawP5 * 10.0f;
                    p4 = 200.0f + rawP3 * 6800.0f;
                    p3 = rawP6 * 10.0f;
                    p6 = 1500.0f + rawP4 * 3500.0f;
                    p5 = rawP7 * 10.0f;
                    outputDb = (rawP1 - 0.5f) * 24.0f;
                    transformer = rawP8 > 0.5f;
                }
                else if (type == FxType::StereoExciter || type == FxType::DualExciter)
                {
                    p1 = 1000.0f + rawP1 * 9000.0f;
                    p2 = rawP2 * 100.0f;
                    p3 = rawP3 * 100.0f;
                    p4 = (rawP4 - 0.5f) * 100.0f;
                    p5 = rawP5 * 100.0f;
                    p6 = rawP6 * 100.0f;
                }
                else // SoundMaxer / StereoEnhancer / DualEnhancer
                {
                    p1 = (rawP1 - 0.2f) * 80.0f;
                    p2 = (rawP2 - 0.2f) * 80.0f;
                    p3 = (type == FxType::SoundMaxer) ? ((rawP3 - 0.5f) * 24.0f) : (50.0f + rawP3 * 250.0f);
                    p4 = rawP4 * 100.0f;
                    p5 = 1500.0f + rawP5 * 6500.0f;
                    p6 = rawP6 * 100.0f;
                }
            }
            tubeEq.process (workL.data(), workR.data(), numSamples, dspType,
                            p1, p2, p3, p4, p5, p6, mixVal, highQ, attenHz, outputDb, transformer);
            break;
        }

        case FxCategory::PremiumUtilityAmp:
        {
            if (type == FxType::MoodFilter)
            {
                const float speedHz = mapExpNormOrRaw (rawP1, 0.05f, 8.0f, norm16);
                const float depthPct = mapNormOrRaw (rawP2, 0.0f, 100.0f, norm16);
                const float cutoffHz = norm16 ? (80.0f * std::pow (40.0f, rawP3)) : rawP3;
                const float envSens = mapNormOrRaw (rawP4, 0.0f, 100.0f, norm16);
                modulation.process (workL.data(), workR.data(), numSamples, FxType::MoodFilter,
                                    speedHz, depthPct, cutoffHz, envSens, mixVal);
            }
            else if (type == FxType::TremoloPanner)
            {
                const float speedHz = mapExpNormOrRaw (rawP1, 0.05f, 10.0f, norm16);
                const float depthPct = mapNormOrRaw (rawP2, 0.0f, 100.0f, norm16);
                const float phaseDeg = norm16 ? (rawP3 * 180.0f) : rawP3;
                const float wavePct = mapNormOrRaw (rawP4, 0.0f, 100.0f, norm16);
                modulation.process (workL.data(), workR.data(), numSamples, FxType::TremoloPanner,
                                    speedHz, depthPct, phaseDeg, wavePct, mixVal);
            }
            else
            {
                float p1 = rawP1, p2 = rawP2, p3 = rawP3, p4 = rawP4, p5 = rawP5, p6 = rawP6;
                float outputDb = 0.0f, loCutHz = 20.0f, hiCutHz = 20000.0f, bassHz = 120.0f, trebleHz = 4500.0f;
                if (norm16)
                {
                    if (type == FxType::TubeStage || type == FxType::StereoTubeStage || type == FxType::DualTubeStage)
                    {
                        p1 = rawP1 * 100.0f;
                        p2 = rawP2 * 100.0f;
                        p3 = rawP3 * 100.0f;
                        p4 = 0.0f;
                        outputDb = (rawP4 - .5f) * 24.0f;
                        loCutHz = mapExpNormOrRaw(rawP5,20,500,true);
                        hiCutHz = mapExpNormOrRaw(rawP6,1500,20000,true);
                        p5 = (rawP7 - .5f) * 24.0f; p6 = (rawP9 - .5f) * 24.0f;
                        bassHz = mapExpNormOrRaw(rawP8,40,800,true);
                        trebleHz = mapExpNormOrRaw(rawP10,1000,16000,true);
                    }
                    else if (type == FxType::Suboctaver)
                    {
                        p1 = rawP1 * 100.0f;
                        p2 = rawP2 * 100.0f;
                        p3 = rawP3 * 100.0f;
                        p4 = std::round (rawP4 * 2.0f);
                    }
                    else if (type == FxType::EdisonEX1)
                    {
                        p1 = rawP1*100.0f; p2 = rawP2*100.0f; p3 = rawP3*100.0f;
                        p4 = (rawP5-.5f)*24.0f; p5 = rawP4*100.0f;
                    }
                    else if (type == FxType::StereoImager)
                    {
                        p1 = rawP1*100.0f; p2 = rawP2*100.0f; p3 = rawP3*100.0f;
                        p4 = (rawP4-.5f)*24.0f; p5 = mapExpNormOrRaw(rawP5,100,16000,true);
                        p6 = .3f + rawP6*3.7f; outputDb = (rawP7-.5f)*24.0f;
                    }
                    else // GuitarAmp
                    {
                        p1 = rawP1 * 100.0f;
                        p2 = rawP2 * 100.0f;
                        p3 = rawP3 * 100.0f;
                        p4 = rawP4 * 100.0f;
                        p5 = rawP5 * 100.0f;
                        p6 = rawP6 * 100.0f;
                    }
                }
                utilityAmp.process (workL.data(), workR.data(), numSamples, dspType,
                                    p1, p2, p3, p4, mixVal, p5, p6, outputDb, loCutHz, hiCutHz, bassHz, trebleHz);
            }
            break;
        }
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
    if (! monoPass) { outPeakL.update (outPkL); outPeakR.update (outPkR); }
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
