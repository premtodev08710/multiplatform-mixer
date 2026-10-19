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
    { FxType::SoundMaxer,              "Sound Maxer",              "MAXER",     "Utility",    FxCategory::PremiumUtilityAmp, "BBE Sonic Maximizer dynamic contourer and high-frequency clarifier", true, true },
    { FxType::DualGuitarAmp,           "Dual GuitarAmp",           "DUAL AMP",  "Guitar Amp", FxCategory::PremiumUtilityAmp, "Marshall JCM800 / Fender Twin dual mono tube amp & 4x12 cab simulator", true, true },
    { FxType::StereoGuitarAmp,         "Stereo GuitarAmp",         "ST AMP",    "Guitar Amp", FxCategory::PremiumUtilityAmp, "Marshall JCM800 / Fender Twin stereo tube amp & 4x12 cab simulator", true, true },
    { FxType::DualTubeStage,           "Dual Tube Stage",          "DUAL TUBE", "Utility",    FxCategory::PremiumUtilityAmp, "Telefunken VF14 dual mono triode valve preamp saturation & warmth", true, true },
    { FxType::StereoTubeStage,         "Stereo Tube Stage",        "ST TUBE",   "Utility",    FxCategory::PremiumUtilityAmp, "Telefunken VF14 stereo triode valve preamp saturation & warmth", true, true },
    { FxType::DualPitch,               "Dual Pitch",               "DUAL PITCH","Pitch",      FxCategory::Pitch, "Eventide H3000 dual independent semitone pitch transposition harmonizer", true, true },
    { FxType::StereoPitch,             "Stereo Pitch",             "ST PITCH", "Pitch",      FxCategory::Pitch, "Eventide H3000 stereo micro-pitch detuner and semitone harmonizer", true, true }
};

const FxMetadata& getFxMetadata (FxType type) noexcept
{
    const int idx = (int) type;
    if (idx >= 0 && idx < 61) return kFxMetadataTable[idx];
    return kFxMetadataTable[0];
}

int getNumFxTypes() noexcept { return 61; }

//==============================================================================
// FxSlotParams
FxSlotParams::FxSlotParams()
{
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
    fxType.set ((float) (int) type);
    mix.set (1.0f);
    for (int i = 0; i < kNumParameters; ++i)
        parameter (i).resetToDefault();
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
            mix.set (1.0f); p1.set (40.0f); p2.set (40.0f); break; // Peak Reduction 40, Gain 40
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
            mix.set (1.0f); p1.set (0.0f);  p2.set (0.0f);  break; // Attack 0%, Sustain 0%
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
            mix.set (1.0f); p1.set (20.0f); p2.set (20.0f); break;
        case FxType::StereoGuitarAmp:
        case FxType::DualGuitarAmp:
            mix.set (1.0f); p1.set (45.0f); p2.set (50.0f); p3.set (50.0f); p4.set (50.0f); break; // Drive, Bass, Mid, Treble
        case FxType::StereoTubeStage:
        case FxType::DualTubeStage:
            mix.set (1.0f); p1.set (30.0f); p2.set (0.0f);  break; // Drive, Output
        case FxType::Suboctaver:
            mix.set (0.4f); p1.set (80.0f); break; // Lowpass 80Hz
        case FxType::EdisonEX1:
        case FxType::StereoImager:
            mix.set (1.0f); p1.set (50.0f); break; // 100% normal width
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

    const int revLen = (int) (sr * 2.5f);
    reverseBufL.assign ((size_t) revLen, 0.0f);
    reverseBufR.assign ((size_t) revLen, 0.0f);

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

    std::fill (reverseBufL.begin(), reverseBufL.end(), 0.0f);
    std::fill (reverseBufR.begin(), reverseBufR.end(), 0.0f);
    reverseWrite = 0;
    emtLpfL.reset(); emtLpfR.reset();
    plateHpfL.reset(); plateHpfR.reset();

    lfoPhase = 0.0f;
    gateCounter = 0;
}

void ReverbDsp::process (float* chL, float* chR, int numSamples,
                         FxType type, float decaySec, float preDelayMs,
                         float dampingNorm, float roomSizeNorm, float diffusionNorm,
                         float levelDb, float mixNorm)
{
    const int preDelaySamples = clampf ((preDelayMs * 0.001f * sr), 0.0f, (float) preDelayBufL.size() - 1);
    const float feedback = clampf (std::exp (-3.0f / (std::max (0.1f, decaySec) * (sr / 1400.0f))), 0.2f, 0.98f);
    const float dampCoeff = clampf (dampingNorm * 0.01f, 0.05f, 0.9f);
    const float diffusionCoeff = clampf (diffusionNorm * 0.0095f, 0.0f, 0.95f);
    const float roomScale = 0.35f + 0.65f * clampf (roomSizeNorm * 0.01f, 0.0f, 1.0f);
    const float wetGain = mixNorm * dbToGain (clampf (levelDb, -40.0f, 10.0f));
    const float dryGain = 1.0f - wetGain;

    const bool isGate = (type == FxType::GatedReverb);
    const bool isReverse = (type == FxType::ReverseReverb);
    const bool isVintage = (type == FxType::VintageReverb || type == FxType::VintageRoomReverb);
    const bool isPlate = (type == FxType::PlateReverb || type == FxType::RichPlateReverb);
    const int gateHoldSamples = (int) (std::max (0.04f, decaySec * 0.12f) * sr);

    // Precalculate authentic hardware filter curves
    const auto emtCoeffs = calcLPF (4500.0f, 0.707f, sr);  // EMT 250 4.5kHz brickwall
    const auto plateCoeffs = calcHPF (100.0f, 0.707f, sr); // Plate 100Hz rumble cut

    for (int i = 0; i < numSamples; ++i)
    {
        const float inL = chL[i];
        const float inR = chR[i];

        // Reverse Reverb path: circular backward buffer with quadratic swell
        if (isReverse)
        {
            reverseBufL[(size_t) reverseWrite] = inL;
            reverseBufR[(size_t) reverseWrite] = inR;

            const int revWin = std::max (512, (int) clampf (decaySec * 0.35f * sr, 1000.0f, (float) reverseBufL.size() - 1));
            const int revPos = reverseWrite % revWin;
            const float tNorm = (float) revPos / (float) revWin;
            const float revEnv = tNorm * tNorm * 1.6f; // quadratic swell curve

            int revRead = reverseWrite - (revWin - revPos);
            if (revRead < 0) revRead += (int) reverseBufL.size();

            const float revL = reverseBufL[(size_t) revRead] * revEnv;
            const float revR = reverseBufR[(size_t) revRead] * revEnv;

            if (++reverseWrite >= (int) reverseBufL.size()) reverseWrite = 0;

            chL[i] = inL * dryGain + revL * wetGain;
            chR[i] = inR * dryGain + revR * wetGain;
            continue;
        }

        // 1. Pre-delay
        preDelayBufL[(size_t) preDelayWritePos] = inL;
        preDelayBufR[(size_t) preDelayWritePos] = inR;

        int readIdx = preDelayWritePos - preDelaySamples;
        if (readIdx < 0) readIdx += (int) preDelayBufL.size();

        const float pdInL = preDelayBufL[(size_t) readIdx];
        const float pdInR = preDelayBufR[(size_t) readIdx];

        if (++preDelayWritePos >= (int) preDelayBufL.size())
            preDelayWritePos = 0;

        // Vintage EMT 250 low-rate delay modulation to prevent flutter
        if (isVintage)
        {
            lfoPhase += (0.6f * 2.0f * kPi) / sr;
            if (lfoPhase >= 2.0f * kPi) lfoPhase -= 2.0f * kPi;
        }

        // 2. Parallel Comb Filters
        float sumCombL = 0.0f;
        float sumCombR = 0.0f;

        for (int c = 0; c < kNumCombs; ++c)
        {
            // Left Comb
            auto& bufL = combBufL[c];
            int readL = (combPosL[c] - std::max (1, (int) (bufL.size() * roomScale)) + (int) bufL.size()) % (int) bufL.size();
            if (isVintage && (c % 2 == 0))
            {
                readL = (readL + (int)(std::sin (lfoPhase + (float) c) * 3.0f) + (int) bufL.size()) % (int) bufL.size();
            }
            const float outCombL = bufL[(size_t) readL];
            combFilterStateL[c] = outCombL * (1.0f - dampCoeff) + combFilterStateL[c] * dampCoeff;
            bufL[(size_t) combPosL[c]] = pdInL + combFilterStateL[c] * feedback;
            if (++combPosL[c] >= (int) bufL.size()) combPosL[c] = 0;
            sumCombL += outCombL;

            // Right Comb
            auto& bufR = combBufR[c];
            int readR = (combPosR[c] - std::max (1, (int) (bufR.size() * roomScale)) + (int) bufR.size()) % (int) bufR.size();
            if (isVintage && (c % 2 != 0))
            {
                readR = (readR + (int)(std::cos (lfoPhase + (float) c) * 3.0f) + (int) bufR.size()) % (int) bufR.size();
            }
            const float outCombR = bufR[(size_t) readR];
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

        // Apply hardware brand filters
        if (isVintage)
        {
            // EMT 250 vintage converter brickwall filtering
            apL = emtLpfL.process (apL, emtCoeffs);
            apR = emtLpfR.process (apR, emtCoeffs);
        }
        else if (isPlate)
        {
            // Steel plate rumble highpass cut
            apL = plateHpfL.process (apL, plateCoeffs);
            apR = plateHpfR.process (apR, plateCoeffs);
        }

        // 4. Gate / Envelope check (Phil Collins gated snare)
        if (isGate)
        {
            const float level = std::max (std::abs (inL), std::abs (inR));
            if (level > 0.04f) gateCounter = gateHoldSamples;
            if (gateCounter > 0)
            {
                --gateCounter;
                const int releaseSamples = (int) (0.012f * sr); // 12ms quick release
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

    const int dimLen = (int) (sr * 0.05f);
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
    for (int i = 0; i < 4; ++i) moogStageL[i] = moogStageR[i] = 0.0f;
    moodEnv = 0.0f;

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
    const float wet = mixNorm;
    const float dry = 1.0f - wet;

    // 1. Roland SDD-320 Dimension D Modeling
    if (type == FxType::DimensionC)
    {
        // Dimension D has authentic fixed 0.25 Hz dual LFO with antiphase BBD modulation
        const float dimLfoInc = (0.25f * 2.0f * kPi) / sr;
        const auto cPre = calcHighShelf (2500.0f, 3.5f, 0.707f, sr);
        const auto cPost = calcHighShelf (2500.0f, -3.5f, 0.707f, sr);
        const float depthMs = 0.4f + clampf (depthNorm * 0.01f, 0.0f, 1.0f) * 1.2f; // Mode 1..4 depth
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
            const float dSamplesL = clampf ((2.5f + depthMs * modL) * 0.001f * sr, 1.0f, (float) dimMax - 1);
            const float dSamplesR = clampf ((2.5f + depthMs * modR) * 0.001f * sr, 1.0f, (float) dimMax - 1);

            int rIdxL = dimWrite - (int) dSamplesL;
            if (rIdxL < 0) rIdxL += dimMax;
            int rIdxR = dimWrite - (int) dSamplesR;
            if (rIdxR < 0) rIdxR += dimMax;

            const float bbdL = dimBufL[(size_t) rIdxL];
            const float bbdR = dimBufR[(size_t) rIdxR];

            if (++dimWrite >= dimMax) dimWrite = 0;

            // Antiphase cross-bleed creates legendary spatial widening without vibrato pitch flutter
            const float crossL = bbdL - 0.35f * bbdR;
            const float crossR = bbdR - 0.35f * bbdL;

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
        const bool isFast = (speedHz > 3.0f || paramCustom > 0.5f);
        const float targetHornHz = isFast ? 7.0f : 0.8f;
        const float targetDrumHz = isFast ? 5.8f : 0.6f;
        const float hornInc = (targetHornHz * 2.0f * kPi) / sr;
        const float drumInc = (targetDrumHz * 2.0f * kPi) / sr;
        const auto cHorn = calcHPF (800.0f, 0.707f, sr); // Horn gets highs
        const auto cDrum = calcLPF (800.0f, 0.707f, sr); // Drum gets lows

        for (int i = 0; i < numSamples; ++i)
        {
            rotorPhaseHorn += hornInc;
            if (rotorPhaseHorn >= 2.0f * kPi) rotorPhaseHorn -= 2.0f * kPi;
            rotorPhaseDrum += drumInc;
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

            const int dHornL = (int) clampf (hornDelayMsL * 0.001f * sr, 1.0f, (float) maxLen - 1);
            const int dHornR = (int) clampf (hornDelayMsR * 0.001f * sr, 1.0f, (float) maxLen - 1);

            int rHornL = modWritePos - dHornL;
            if (rHornL < 0) rHornL += maxLen;
            int rHornR = modWritePos - dHornR;
            if (rHornR < 0) rHornR += maxLen;

            const float delayedHornL = modDelayBufL[(size_t) rHornL];
            const float delayedHornR = modDelayBufR[(size_t) rHornR];

            if (++modWritePos >= maxLen) modWritePos = 0;

            // Split into horn and drum bands
            const float hL = moodBiquadL.process (delayedHornL, cHorn) * hornAmL;
            const float hR = moodBiquadR.process (delayedHornR, cHorn) * hornAmR;
            const float dL = (tubeInL - moodBiquadL.process (tubeInL, cHorn)) * drumAmL;
            const float dR = (tubeInR - moodBiquadR.process (tubeInR, cHorn)) * drumAmR;

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
        const float baseCutoff = clampf (speedHz * 400.0f, 60.0f, 16000.0f);
        const float resAmount = clampf (feedbackNorm * 0.038f, 0.0f, 3.85f); // self-oscillates near 4.0
        const float envSensitivity = clampf (depthNorm * 0.01f, 0.0f, 1.0f);
        const float envAttackCoeff = std::exp (-1.0f / (0.005f * sr));
        const float envReleaseCoeff = std::exp (-1.0f / (0.120f * sr));

        for (int i = 0; i < numSamples; ++i)
        {
            const float inPk = std::max (std::abs (chL[i]), std::abs (chR[i]));
            if (inPk > moodEnv) moodEnv = inPk + envAttackCoeff * (moodEnv - inPk);
            else moodEnv = inPk + envReleaseCoeff * (moodEnv - inPk);

            const float modCutoff = clampf (baseCutoff + moodEnv * envSensitivity * 10000.0f, 40.0f, 18000.0f);
            const float g = clampf (1.0f - std::exp (-(2.0f * kPi * modCutoff) / sr), 0.001f, 0.99f);

            // Left 4-Pole Ladder
            const float feedL = chL[i] - resAmount * moogStageL[3];
            moogStageL[0] += g * (std::tanh (feedL) - std::tanh (moogStageL[0]));
            moogStageL[1] += g * (std::tanh (moogStageL[0]) - std::tanh (moogStageL[1]));
            moogStageL[2] += g * (std::tanh (moogStageL[1]) - std::tanh (moogStageL[2]));
            moogStageL[3] += g * (std::tanh (moogStageL[2]) - std::tanh (moogStageL[3]));

            // Right 4-Pole Ladder
            const float feedR = chR[i] - resAmount * moogStageR[3];
            moogStageR[0] += g * (std::tanh (feedR) - std::tanh (moogStageR[0]));
            moogStageR[1] += g * (std::tanh (moogStageR[0]) - std::tanh (moogStageR[1]));
            moogStageR[2] += g * (std::tanh (moogStageR[0]) - std::tanh (moogStageR[2]));
            moogStageR[3] += g * (std::tanh (moogStageR[2]) - std::tanh (moogStageR[3]));

            chL[i] = chL[i] * dry + moogStageL[3] * wet;
            chR[i] = chR[i] * dry + moogStageR[3] * wet;
        }
        return;
    }

    // 4. Standard Chorus, Flanger, Phaser, Tremolo
    const float lfoInc = (speedHz * 2.0f * kPi) / sr;
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
            // Chorus / Flanger
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
}

void PremiumDynamicsDsp::process (float* chL, float* chR, int numSamples,
                                  FxType type, float p1, float p2, float p3, float p4, float mixNorm)
{
    const float wet = mixNorm;
    const float dry = 1.0f - wet;

    // 1. Teletronix LA-2A Electro-Optical Leveling Amplifier
    if (type == FxType::StereoLeisureCompressor || type == FxType::DualLeisureCompressor
        || type == FxType::LeisureCompressor)
    {
        // p1 = Peak Reduction (0..100), p2 = Output Gain (0..100)
        const float threshold = -42.0f + (100.0f - p1) * 0.42f;
        const float makeup = dbToGain ((p2 - 40.0f) * 0.4f);
        const auto cOptoHf = calcHighShelf (1000.0f, 4.0f, 0.707f, sr); // R37 HF emphasis

        for (int i = 0; i < numSamples; ++i)
        {
            // Detector with high-frequency pre-emphasis
            const float detL = optoHpfL.process (chL[i], cOptoHf);
            const float detR = optoHpfR.process (chR[i], cOptoHf);
            const float inPeak = std::max (std::abs (detL), std::abs (detR));
            const float inDb = gainToDb (inPeak);
            const float over = std::max (0.0f, inDb - threshold);

            // T4 Optical cell: 10ms attack, dual-stage release (60ms fast 50%, 1.5s-5s memory tail)
            const float attCoeff = std::exp (-1.0f / (0.010f * sr));
            const float relCoeff1 = std::exp (-1.0f / (0.060f * sr));
            const float relCoeff2 = std::exp (-1.0f / (1.800f * sr));

            if (over > la2aEnv) la2aEnv = over + attCoeff * (la2aEnv - over);
            else la2aEnv = over + relCoeff1 * (la2aEnv - over);

            la2aTail = la2aEnv + relCoeff2 * (la2aTail - la2aEnv);
            const float compDb = -(la2aEnv * 0.55f + la2aTail * 0.45f);
            const float gain = dbToGain (compDb) * makeup;

            // 12AX7 tube 2nd-order harmonic saturation
            const float linL = chL[i] * gain;
            const float linR = chR[i] * gain;
            const float outL = linL + 0.08f * linL * std::abs (linL);
            const float outR = linR + 0.08f * linR * std::abs (linR);

            chL[i] = chL[i] * dry + outL * wet;
            chR[i] = chR[i] * dry + outR * wet;
        }
    }
    // 2. Urei 1176LN Fast FET Limiting Amplifier with "All-Buttons-In" Mode
    else if (type == FxType::UltimoCompressor || type == FxType::DualUltimoCompressor)
    {
        // p1 = Input, p2 = Output, p3 = Ratio (4, 8, 12, 20, All-In), p4 = Attack (us)
        const float drive = dbToGain ((p1 - 30.0f) * 0.5f);
        const float makeup = dbToGain ((p2 - 20.0f) * 0.5f);
        const bool isAllButtons = (p3 >= 20.0f);
        const float ratio = isAllButtons ? 12.0f : std::max (2.0f, p3);

        const float attSec = 0.000020f + (p4 * 0.01f) * 0.000800f; // 20us to 800us ultra-fast FET
        const float attCoeff = std::exp (-1.0f / (attSec * sr));
        const float relCoeff = std::exp (-1.0f / (isAllButtons ? 0.050f : 0.100f * sr));

        for (int i = 0; i < numSamples; ++i)
        {
            const float drivenL = chL[i] * drive;
            const float drivenR = chR[i] * drive;
            const float inPeak = std::max (std::abs (drivenL), std::abs (drivenR));
            const float inDb = gainToDb (inPeak);
            const float thresh = isAllButtons ? -26.0f : -20.0f;
            const float over = std::max (0.0f, inDb - thresh);

            if (over > fetEnv) fetEnv = over + attCoeff * (fetEnv - over);
            else fetEnv = over + relCoeff * (fetEnv - over);

            const float grDb = fetEnv * (1.0f - 1.0f / ratio);
            const float g = dbToGain (-grDb) * makeup;

            // In All-Buttons-In mode: aggressive 3rd harmonic saturation and punchy overdrive
            float outL = drivenL * g;
            float outR = drivenR * g;
            if (isAllButtons)
            {
                outL = std::tanh (outL * 1.2f) - 0.15f * outL * outL * outL;
                outR = std::tanh (outR * 1.2f) - 0.15f * outR * outR * outR;
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
        const float threshold = -50.0f + clampf (p1, 0.0f, 100.0f) * 0.5f;
        const float makeup = dbToGain ((p2 - 50.0f) * 0.12f);
        // 6 switchable time constants (p4 selects position 1..6)
        const int timeConstPos = clampf (p4, 1.0f, 6.0f);
        float relSec = 0.5f;
        switch (timeConstPos)
        {
            case 1: relSec = 0.200f; break;
            case 2: relSec = 0.500f; break;
            case 3: relSec = 1.000f; break;
            case 4: relSec = 2.000f; break;
            case 5: relSec = 3.500f; break; // dual automatic
            case 6: relSec = 5.000f; break; // multi automatic
            default: relSec = 0.500f; break;
        }
        const float attCoeff = std::exp (-1.0f / (0.002f * sr)); // 2ms fast Vari-Mu
        const float relCoeff = std::exp (-1.0f / (relSec * sr));
        const bool isMidSide = (type == FxType::MsFairCompressor);

        for (int i = 0; i < numSamples; ++i)
        {
            float sL = chL[i];
            float sR = chR[i];

            // Mid/Side matrix encode
            if (isMidSide)
            {
                const float mid = 0.5f * (sL + sR);
                const float side = 0.5f * (sL - sR);
                sL = mid;
                sR = side;
            }

            const float inDbL = gainToDb (std::abs (sL));
            const float inDbR = gainToDb (std::abs (sR));
            // Progressive soft knee: starts compressing 6dB below threshold
            const float overL = std::max (0.0f, inDbL - threshold);
            const float overR = std::max (0.0f, inDbR - threshold);

            if (overL > fairEnvL) fairEnvL = overL + attCoeff * (fairEnvL - overL);
            else fairEnvL = overL + relCoeff * (fairEnvL - overL);
            if (overR > fairEnvR) fairEnvR = overR + attCoeff * (fairEnvR - overR);
            else fairEnvR = overR + relCoeff * (fairEnvR - overR);

            // Vari-Mu continuous ratio curve (2:1 up to 10:1 based on depth)
            const float ratioL = 2.0f + fairEnvL * 0.4f;
            const float ratioR = 2.0f + fairEnvR * 0.4f;
            const float gL = dbToGain (-fairEnvL * (1.0f - 1.0f / ratioL)) * makeup;
            const float gR = dbToGain (-fairEnvR * (1.0f - 1.0f / ratioR)) * makeup;

            float compL = sL * gL;
            float compR = sR * gR;

            // Mid/Side matrix decode
            if (isMidSide)
            {
                const float l = compL + compR;
                const float r = compL - compR;
                compL = l;
                compR = r;
            }

            // Tube saturation warmth
            compL = std::tanh (compL * 1.1f);
            compR = std::tanh (compR * 1.1f);

            chL[i] = chL[i] * dry + compL * wet;
            chR[i] = chR[i] * dry + compR * wet;
        }
    }
    // 4. TC Finalizer 5-Band Broadcast Combinator Modeling
    else if (type == FxType::Combinator || type == FxType::DualCombinator)
    {
        // 5-band crossover at 80 Hz, 320 Hz, 1280 Hz, 5120 Hz
        const auto cX1 = calcLPF (80.0f, 0.707f, sr);
        const auto cX2 = calcLPF (320.0f, 0.707f, sr);
        const auto cX3 = calcLPF (1280.0f, 0.707f, sr);
        const auto cX4 = calcLPF (5120.0f, 0.707f, sr);

        const float threshold = -40.0f + clampf (p1, 0.0f, 100.0f) * 0.4f;
        const float makeup = dbToGain ((p2 - 50.0f) * 0.12f);
        const float attCoeff = std::exp (-1.0f / (0.008f * sr));
        const float relCoeff = std::exp (-1.0f / (0.150f * sr));

        for (int i = 0; i < numSamples; ++i)
        {
            // Split into 5 frequency bands
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

                const float gL = dbToGain (-bandEnvL[b] * 0.65f);
                const float gR = dbToGain (-bandEnvR[b] * 0.65f);
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

            const float aR = std::abs (chR[i]);
            fastEnvR = aR + fastCoeff * (fastEnvR - aR);
            slowEnvR = aR + slowCoeff * (slowEnvR - aR);
            const float transR = std::max (0.0f, fastEnvR - slowEnvR);
            const float susR = slowEnvR;
            const float modR = (transR * attMult + susR * susMult) / std::max (0.001f, transR + susR);

            chL[i] = chL[i] * dry + (chL[i] * modL) * wet;
            chR[i] = chR[i] * dry + (chR[i] * modR) * wet;
        }
    }
    // 6. SPL Vocal De-Esser (Dual / Stereo)
    else if (type == FxType::StereoDeEsser || type == FxType::DualDeEsser)
    {
        const auto cSib = calcBell (6500.0f, 6.0f, 2.5f, sr); // 6.5 kHz sibilance detector
        const float thresh = -45.0f + clampf (p1, 0.0f, 100.0f) * 0.45f;
        const float attCoeff = std::exp (-1.0f / (0.001f * sr)); // 1ms instant attack
        const float relCoeff = std::exp (-1.0f / (0.050f * sr)); // 50ms release

        for (int i = 0; i < numSamples; ++i)
        {
            const float sibL = sibilanceFiltL.process (chL[i], cSib);
            const float sibR = sibilanceFiltR.process (chR[i], cSib);
            const float sibPeak = std::max (std::abs (sibL), std::abs (sibR));
            const float sibDb = gainToDb (sibPeak);
            const float over = std::max (0.0f, sibDb - thresh);

            if (over > deEssEnv) deEssEnv = over + attCoeff * (deEssEnv - over);
            else deEssEnv = over + relCoeff * (deEssEnv - over);

            // Phase cancellation attenuation on sibilance band
            const float atten = dbToGain (-deEssEnv * 0.8f);
            const float cleanL = chL[i] - sibL * (1.0f - atten);
            const float cleanR = chR[i] - sibR * (1.0f - atten);

            chL[i] = chL[i] * dry + cleanL * wet;
            chR[i] = chR[i] * dry + cleanR * wet;
        }
    }
    // 7. Lookahead Precision Mastering Limiter
    else
    {
        const float threshLinear = dbToGain (-std::max (0.1f, p1 * 0.1f));
        const int lookaheadLen = (int) lookaheadBufL.size();
        for (int i = 0; i < numSamples; ++i)
        {
            lookaheadBufL[(size_t) lookaheadWrite] = chL[i];
            lookaheadBufR[(size_t) lookaheadWrite] = chR[i];

            const float pk = std::max (std::abs (chL[i]), std::abs (chR[i]));
            if (pk > threshLinear)
                limiterGain = threshLinear / pk;
            else
                limiterGain += (1.0f - limiterGain) * 0.005f;

            int readIdx = lookaheadWrite - (lookaheadLen / 2);
            if (readIdx < 0) readIdx += lookaheadLen;

            const float outL = lookaheadBufL[(size_t) readIdx] * limiterGain;
            const float outR = lookaheadBufR[(size_t) readIdx] * limiterGain;

            if (++lookaheadWrite >= lookaheadLen) lookaheadWrite = 0;

            chL[i] = chL[i] * dry + outL * wet;
            chR[i] = chR[i] * dry + outR * wet;
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

void PremiumTubeEqDsp::process (float* chL, float* chR, int numSamples,
                                FxType type, float p1, float p2, float p3, float p4, float p5, float p6, float mixNorm)
{
    const float wet = mixNorm;
    const float dry = 1.0f - wet;

    // 1. Pultec EQP-1A Passive Tube Program Equalizer (with authentic Low-End Trick)
    if (type == FxType::StereoXtecEQ1 || type == FxType::DualXtecEQ1 || type == FxType::XtecEq1)
    {
        // p1 = Low Boost (dB), p2 = Low Freq (Hz), p3 = Low Atten (dB),
        // p4 = High Atten (dB), p5 = High Boost (dB), p6 = High Freq (Hz)
        const float lowFreq = clampf (p2 > 0.0f ? p2 : 60.0f, 20.0f, 150.0f);
        const float highFreq = clampf (p6 > 0.0f ? p6 : 8000.0f, 2000.0f, 16000.0f);

        // Pultec Low-End Trick: Boost is a broad shelf, Atten is a steeper shelf offset by ~1.35x freq.
        // When both are dialed in, it boosts the deep sub while scooping the muddy 300-400 Hz octave!
        const auto cBoost = calcLowShelf (lowFreq, p1 * 1.2f, 0.707f, sr);
        const auto cAtten = calcLowShelf (lowFreq * 1.35f, -p3 * 1.2f, 0.90f, sr);
        const auto cHighBoost = calcBell (highFreq, p5 * 1.4f, 1.2f, sr);
        const auto cHighAtten = calcHighShelf (10000.0f, -p4 * 1.2f, 0.707f, sr);

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
            sL = (sL + 0.06f * sL * sL) / (1.0f + 0.06f * std::abs (sL));
            sR = (sR + 0.06f * sR * sR) / (1.0f + 0.06f * std::abs (sR));

            chL[i] = chL[i] * dry + sL * wet;
            chR[i] = chR[i] * dry + sR * wet;
        }
    }
    // 2. Pultec MEQ-5 Passive Midrange Equalizer
    else if (type == FxType::StereoXtecEQ5 || type == FxType::DualXtecEQ5 || type == FxType::XtecEq5)
    {
        // Band 1: Low-Mid Peak (200, 300, 500, 700, 1000 Hz)
        // Band 2: Mid Dip (200 to 7000 Hz)
        // Band 3: High-Mid Peak (1.5k, 2k, 3k, 4k, 5k Hz)
        const float lowMidF = clampf (p2 > 0.0f ? p2 : 500.0f, 200.0f, 1000.0f);
        const float midDipF = clampf (p4 > 0.0f ? p4 : 1500.0f, 200.0f, 7000.0f);
        const float hiMidF = clampf (p6 > 0.0f ? p6 : 3000.0f, 1500.0f, 5000.0f);

        const auto cLowMid = calcBell (lowMidF, p1 * 1.2f, 1.4f, sr);
        const auto cMidDip = calcBell (midDipF, -p3 * 1.2f, 1.4f, sr);
        const auto cHiMid = calcBell (hiMidF, p5 * 1.2f, 1.4f, sr);

        for (int i = 0; i < numSamples; ++i)
        {
            float sL = meqLowL.process (chL[i], cLowMid);
            sL = meqMidL.process (sL, cMidDip);
            sL = meqHighL.process (sL, cHiMid);

            float sR = meqLowR.process (chR[i], cLowMid);
            sR = meqMidR.process (sR, cMidDip);
            sR = meqHighR.process (sR, cHiMid);

            chL[i] = chL[i] * dry + sL * wet;
            chR[i] = chR[i] * dry + sR * wet;
        }
    }
    // 3. Aphex Aural Exciter Type C Harmonic Generator
    else if (type == FxType::StereoExciter || type == FxType::DualExciter)
    {
        // High-pass sidechain at 4 kHz with 2nd and 3rd harmonic generator
        const auto cHpf = calcHPF (4000.0f, 0.707f, sr);
        const float drive = dbToGain (clampf (p1, 0.0f, 100.0f) * 0.15f);

        for (int i = 0; i < numSamples; ++i)
        {
            const float hL = exciterHpfL.process (chL[i], cHpf);
            const float hR = exciterHpfR.process (chR[i], cHpf);

            // Asymmetric even and odd harmonic saturation for airy sheen
            const float harmL = (hL * hL * 0.8f + std::sin (hL * 1.6f) * 0.5f) * drive;
            const float harmR = (hR * hR * 0.8f + std::sin (hR * 1.6f) * 0.5f) * drive;

            chL[i] = chL[i] + harmL * (wet * 0.45f);
            chR[i] = chR[i] + harmR * (wet * 0.45f);
        }
    }
    // 4. BBE Sonic Maximizer 882i (3-Band Group Delay Phase Alignment & Contour)
    else if (type == FxType::SoundMaxer || type == FxType::StereoEnhancer || type == FxType::DualEnhancer)
    {
        // Split into:
        // - Low band (< 150 Hz): Delayed by ~2.5 ms (120 samples) to align speaker group delay
        // - Mid band (150 Hz - 2.5 kHz): Passed through
        // - High band (> 2.5 kHz): Dynamic harmonic contour
        const auto cLowX = calcLPF (150.0f, 0.707f, sr);
        const auto cHighX = calcHPF (2500.0f, 0.707f, sr);

        const int bbeDelaySamples = (int) clampf (0.0025f * sr, 16.0f, (float) bbeLowBufL.size() - 1);
        const float lowDrive = dbToGain (clampf (p1, -50.0f, 50.0f) * 0.12f);
        const float highDrive = dbToGain (clampf (p2, -50.0f, 50.0f) * 0.12f);
        const int bbeMax = (int) bbeLowBufL.size();

        for (int i = 0; i < numSamples; ++i)
        {
            // Low band
            const float lowL = bbeLpfL.process (chL[i], cLowX);
            const float lowR = bbeLpfR.process (chR[i], cLowX);
            // High band
            const float highL = bbeHpfL.process (chL[i], cHighX);
            const float highR = bbeHpfR.process (chR[i], cHighX);
            // Mid band
            const float midL = chL[i] - (lowL + highL);
            const float midR = chR[i] - (lowR + highR);

            // Circular delay buffer for low frequencies
            bbeLowBufL[(size_t) bbeWrite] = lowL;
            bbeLowBufR[(size_t) bbeWrite] = lowR;

            int rIdx = bbeWrite - bbeDelaySamples;
            if (rIdx < 0) rIdx += bbeMax;

            const float delayedLowL = bbeLowBufL[(size_t) rIdx] * lowDrive;
            const float delayedLowR = bbeLowBufR[(size_t) rIdx] * lowDrive;

            if (++bbeWrite >= bbeMax) bbeWrite = 0;

            // High frequency harmonic expansion
            const float contourHighL = (highL + 0.15f * highL * std::abs (highL)) * highDrive;
            const float contourHighR = (highR + 0.15f * highR * std::abs (highR)) * highDrive;

            const float outBbeL = delayedLowL + midL + contourHighL;
            const float outBbeR = delayedLowR + midR + contourHighR;

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
    toneBassL.reset(); toneBassR.reset();
    toneMidL.reset(); toneMidR.reset();
    toneTrebleL.reset(); toneTrebleR.reset();

    cabResoL.reset(); cabResoR.reset();
    cabScoopL.reset(); cabScoopR.reset();
    cabRollOffL.reset(); cabRollOffR.reset();

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

    // 1. Marshall JCM800 / Fender Twin Tube Amp & Celestion 4x12 Cabinet
    if (type == FxType::GuitarAmp || type == FxType::StereoGuitarAmp || type == FxType::DualGuitarAmp)
    {
        // p1 = Drive, p2 = Bass, p3 = Mid, p4 = Treble
        const float drive = dbToGain (clampf (p1, 0.0f, 100.0f) * 0.35f);
        // 3-Band Tone Stack
        const auto cBass = calcLowShelf (100.0f, (p2 - 50.0f) * 0.25f, 0.707f, sr);
        const auto cMid = calcBell (600.0f, (p3 - 50.0f) * 0.25f, 1.0f, sr);
        const auto cTreble = calcHighShelf (3500.0f, (p4 - 50.0f) * 0.25f, 0.707f, sr);

        // Celestion 4x12 closed-back cabinet impulse response curve:
        // 110 Hz low punch resonance, 400 Hz cone scoop, 2.8 kHz presence peak, 5.5 kHz 4th-order roll-off
        const auto cCabReso = calcBell (110.0f, 4.5f, 1.8f, sr);
        const auto cCabScoop = calcBell (400.0f, -3.5f, 1.2f, sr);
        const auto cCabRollOff = calcLPF (5500.0f, 0.707f, sr);

        for (int i = 0; i < numSamples; ++i)
        {
            // 12AX7 dual triode preamp asymmetric clipping with warm grid conduction
            const float xL = chL[i] * drive;
            const float distL = std::tanh (xL + 0.15f) - 0.15f + 0.08f * xL * xL;
            const float xR = chR[i] * drive;
            const float distR = std::tanh (xR + 0.15f) - 0.15f + 0.08f * xR * xR;

            // Tone Stack
            float eqL = toneBassL.process (distL, cBass);
            eqL = toneMidL.process (eqL, cMid);
            eqL = toneTrebleL.process (eqL, cTreble);

            float eqR = toneBassR.process (distR, cBass);
            eqR = toneMidR.process (eqR, cMid);
            eqR = toneTrebleR.process (eqR, cTreble);

            // Celestion 4x12 Cabinet resonance
            float cabL = cabResoL.process (eqL, cCabReso);
            cabL = cabScoopL.process (cabL, cCabScoop);
            cabL = cabRollOffL.process (cabL, cCabRollOff);

            float cabR = cabResoR.process (eqR, cCabReso);
            cabR = cabScoopR.process (cabR, cCabScoop);
            cabR = cabRollOffR.process (cabR, cCabRollOff);

            chL[i] = chL[i] * dry + cabL * wet;
            chR[i] = chR[i] * dry + cabR * wet;
        }
    }
    // 2. Telefunken VF14 Analog Triode Tube Valve Saturation
    else if (type == FxType::TubeStage || type == FxType::StereoTubeStage || type == FxType::DualTubeStage)
    {
        const float drive = dbToGain (clampf (p1, 0.0f, 100.0f) * 0.22f);
        const float outTrim = dbToGain (p2 * 0.1f);
        for (int i = 0; i < numSamples; ++i)
        {
            const float xL = chL[i] * drive;
            const float satL = ((xL + 0.18f * xL * xL) / (1.0f + std::abs (xL))) * outTrim;
            const float xR = chR[i] * drive;
            const float satR = ((xR + 0.18f * xR * xR) / (1.0f + std::abs (xR))) * outTrim;

            chL[i] = chL[i] * dry + satL * wet;
            chR[i] = chR[i] * dry + satR * wet;
        }
    }
    // 3. Boss OC-2 Analog Dual Flip-Flop Sub-Octaver Synthesizer
    else if (type == FxType::Suboctaver)
    {
        // 24dB Butterworth LPF smoothing at 80 Hz
        const auto cSubLpf = calcLPF (80.0f, 0.707f, sr);
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
            chL[i] = chL[i] + subSignal * (wet * 0.75f);
            chR[i] = chR[i] + subSignal * (wet * 0.75f);
        }
    }
    // 4. Edison EX1 / Stereo Imager (Mid/Side width & Mono-Bass Centering)
    else if (type == FxType::EdisonEX1 || type == FxType::StereoImager)
    {
        const float width = clampf (p1 * 0.02f, 0.0f, 2.5f); // 0 = Mono, 1 = Normal, 2 = Ultra-wide
        const auto cMonoBass = calcLPF (120.0f, 0.707f, sr);

        for (int i = 0; i < numSamples; ++i)
        {
            const float mid = 0.5f * (chL[i] + chR[i]);
            float side = 0.5f * (chL[i] - chR[i]) * width;

            // Mono bass centering: eliminate out-of-phase rumble below 120 Hz
            const float lowSide = subLpfL.process (side, cMonoBass);
            side -= lowSide;

            chL[i] = mid + side;
            chR[i] = mid - side;
        }
    }
    else
    {
        // Sound Maxer
        const float drive = dbToGain (clampf (p1, 0.0f, 100.0f) * 0.05f);
        const float ceiling = dbToGain (-clampf (p2, 0.0f, 100.0f) * 0.08f);
        for (int i = 0; i < numSamples; ++i)
        {
            const float maxL = std::tanh (chL[i] * drive) * ceiling;
            const float maxR = std::tanh (chR[i] * drive) * ceiling;
            chL[i] = chL[i] * dry + maxL * wet;
            chR[i] = chR[i] * dry + maxR * wet;
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
    auto dspType = type;
    switch (type)
    {
        case FxType::DualXtecEQ1: dspType = FxType::StereoXtecEQ1; break;
        case FxType::DualXtecEQ5: dspType = FxType::StereoXtecEQ5; break;
        case FxType::DualExciter: dspType = FxType::StereoExciter; break;
        case FxType::DualEnhancer: dspType = FxType::StereoEnhancer; break;
        case FxType::DualGuitarAmp: dspType = FxType::StereoGuitarAmp; break;
        case FxType::DualTubeStage: dspType = FxType::StereoTubeStage; break;
        case FxType::DualPitch: dspType = FxType::StereoPitch; break;
        case FxType::DualUltimoCompressor: dspType = FxType::UltimoCompressor; break;
        case FxType::DualLeisureCompressor: dspType = FxType::StereoLeisureCompressor; break;
        case FxType::DualFairCompressor: dspType = FxType::FairCompressor; break;
        case FxType::DualCombinator: dspType = FxType::Combinator; break;
        default: break;
    }
    const auto meta = getFxMetadata (type);
    const float mixVal = params.mix.get();

    // Route to specialized DSP processor
    switch (meta.category)
    {
        case FxCategory::Reverb:
            reverb.process (workL.data(), workR.data(), numSamples, dspType,
                            params.p1.get(), params.p2.get(), params.p3.get(), params.p4.get(),
                            params.p5.get(), params.p6.get(), mixVal);
            break;

        case FxCategory::Delay:
            delay.process (workL.data(), workR.data(), numSamples, dspType,
                           params.p1.get(), params.p2.get(), params.p3.get(), params.p4.get(), params.p5.get(), mixVal);
            break;

        case FxCategory::Modulation:
            modulation.process (workL.data(), workR.data(), numSamples, dspType,
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
            dynamics.process (workL.data(), workR.data(), numSamples, dspType,
                              params.p1.get(), params.p2.get(), params.p3.get(), params.p4.get(), mixVal);
            break;

        case FxCategory::PremiumTubeEq:
            tubeEq.process (workL.data(), workR.data(), numSamples, dspType,
                            params.p1.get(), params.p2.get(), params.p3.get(), params.p4.get(),
                            params.p5.get(), params.p6.get(), mixVal);
            break;

        case FxCategory::PremiumUtilityAmp:
            utilityAmp.process (workL.data(), workR.data(), numSamples, dspType,
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
