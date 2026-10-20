#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>
#include <functional>
#include <cmath>
#include <algorithm>

namespace X32Fx
{
    enum class GuiFamily
    {
        LarcReverb,       // Hall, Ambience, Rich Plate, Room, Chamber
        PcmPlate,         // Plate, Gated, Reverse, Delay+Chamber, Chorus+Chamber, Flanger+Chamber, ModDelay, StereoPitch
        Emt250,           // Vintage Reverb
        QuantecRoom,      // Vintage Room
        StereoDelay,      // Stereo Delay, 3-Tap, 4-Tap Rhythm Delay
        DualGreenLcd,     // Delay+Chorus, Delay+Flanger
        ColoredMod,       // Stereo Chorus, Flanger, Phaser, Tremolo Panner
        DimensionC,       // Dimension-C
        MoodFilter,       // Mood Filter
        RotarySpeaker,    // Rotary Speaker
        SubOctaver,       // Suboctaver
        GraphicEq,        // Dual GEQ, Stereo GEQ, Dual TruEQ, Stereo TruEQ
        DeEsser,          // Dual DeEsser, Stereo DeEsser
        XtecEq1,          // Stereo Xtec EQ1, Dual Xtec EQ1 (Pultec EQP-1A)
        XtecEq5,          // Stereo Xtec EQ5, Dual Xtec EQ5 (Pultec MEQ-5)
        WaveDesigner,     // Wave Designer
        PrecisionLimiter, // Stereo Precision Limiter
        Combinator,       // Combinator, Dual Combinator
        FairComp,         // Fair Comp, M/S Fair Comp, Dual Fair Comp
        LeisureComp,      // Leisure Comp, Dual Leisure Comp (LA-2A)
        UltimoComp,       // Ultimo Comp, Dual Ultimo Comp (1176LN)
        Enhancer,         // Dual Enhancer, Stereo Enhancer
        Exciter,          // Dual Exciter, Stereo Exciter
        StereoImager,     // Stereo Imager
        EdisonEx1,        // Edison EX1+
        SoundMaxer,       // Sound Maxer
        GuitarAmp,        // Dual GuitarAmp, Stereo GuitarAmp
        TubeStage,        // Dual Tube Stage, Stereo Tube Stage
        DualPitch         // Dual Pitch Shifter
    };

    struct EffectModelInfo
    {
        int index14;              // 0..60 index in FX1-4
        juce::String id;          // Internal ID
        juce::String name;        // Full Display Name in X32-Edit
        juce::String oscCode;     // X32 OSC Enum Code
        int page14;               // Page 1..8 for FX1-FX4
        int page58;               // Page 1..5 for FX5-FX8 (0 if not available in FX5-8)
        int pos58;                // Order on FX5-8 page
        GuiFamily family;
        juce::Colour accentColour;
        bool isDual;
        juce::String subLabel;
        std::vector<juce::String> paramNames;
        std::vector<float> defaultValues; // 0..100 scale
    };

    inline const std::vector<EffectModelInfo>& getCatalog()
    {
        static const std::vector<EffectModelInfo> catalog = {
            // ==================== PAGE 1 (FX 1-4) ====================
            { 0,  "hall",        "Hall Reverb",         "HALL", 1, 0, 0, GuiFamily::LarcReverb,   juce::Colour(0xffff3020), false, "HALL",      { "PRE DEL", "DECAY", "SIZE", "DAMPING", "DIFFUSE", "LEVEL", "LO CUT", "HI CUT", "BASSMULT", "SPREAD", "ATTACK", "SPIN", "ECHO L DEL", "ECHO R DEL", "ECHO L FEED", "ECHO R FEED" }, { 12, 45, 72, 40, 75, 50, 18, 68, 52, 80, 35, 50, 25, 35, 20, 20 } },
            { 1,  "ambience",    "Ambience Reverb",     "AMBI", 1, 0, 0, GuiFamily::LarcReverb,   juce::Colour(0xff22c55e), false, "AMBIENCE",  { "PRE DEL", "DECAY", "SIZE", "DAMPING", "DIFFUSE", "LEVEL", "LO CUT", "HI CUT", "BASSMULT", "SPREAD", "ATTACK", "SPIN", "ECHO L DEL", "ECHO R DEL", "ECHO L FEED", "ECHO R FEED" }, { 8,  22, 40, 55, 65, 50, 20, 72, 48, 70, 25, 40, 20, 25, 15, 15 } },
            { 2,  "rich_plate",  "Rich Plate Reverb",   "RPLT", 1, 0, 0, GuiFamily::LarcReverb,   juce::Colour(0xff22c55e), false, "PLATE",     { "PRE DEL", "DECAY", "SIZE", "DAMPING", "DIFFUSE", "LEVEL", "LO CUT", "HI CUT", "BASSMULT", "SPREAD", "ATTACK", "SPIN", "ECHO L DEL", "ECHO R DEL", "ECHO L FEED", "ECHO R FEED" }, { 15, 55, 68, 35, 85, 50, 22, 75, 50, 85, 30, 60, 30, 40, 25, 25 } },
            { 3,  "room",        "Room Reverb",         "ROOM", 1, 0, 0, GuiFamily::LarcReverb,   juce::Colour(0xfff59e0b), false, "ROOM",      { "PRE DEL", "DECAY", "SIZE", "DAMPING", "DIFFUSE", "LEVEL", "LO CUT", "HI CUT", "BASSMULT", "SPREAD", "ATTACK", "SPIN", "ECHO L DEL", "ECHO R DEL", "ECHO L FEED", "ECHO R FEED" }, { 10, 32, 48, 50, 70, 50, 24, 65, 50, 75, 28, 45, 22, 30, 18, 18 } },
            { 4,  "chamber",     "Chamber Reverb",      "CHAM", 1, 0, 0, GuiFamily::LarcReverb,   juce::Colour(0xffeab308), false, "CHAMBER",   { "PRE DEL", "DECAY", "SIZE", "DAMPING", "DIFFUSE", "LEVEL", "LO CUT", "HI CUT", "BASSMULT", "SPREAD", "ATTACK", "SPIN", "ECHO L DEL", "ECHO R DEL", "ECHO L FEED", "ECHO R FEED" }, { 18, 50, 62, 42, 78, 50, 20, 70, 54, 82, 32, 55, 28, 38, 22, 22 } },
            { 5,  "plate",       "Plate Reverb",        "PLAT", 1, 0, 0, GuiFamily::PcmPlate,     juce::Colour(0xff0088ff), false, "PLATE",     { "PRE DELAY", "DECAY", "SIZE", "DAMP", "DIFF", "LO CUT", "HI CUT", "BASS MULT", "XOVER", "MOD DEP", "MOD SPD", "MIX" }, { 20, 52, 65, 45, 80, 22, 74, 50, 48, 35, 42, 50 } },
            { 6,  "vintage_rev", "Vintage Reverb",      "VREV", 1, 0, 0, GuiFamily::Emt250,       juce::Colour(0xffef4444), false, "EMT 250",   { "DECAY", "PRE DELAY", "LO MULT", "HI MULT", "MODULATE", "LEVEL" }, { 55, 24, 60, 45, 50, 75 } },
            { 7,  "vintage_rm",  "Vintage Room Reverb", "VRM",  1, 0, 0, GuiFamily::QuantecRoom,  juce::Colour(0xffff3020), false, "QRS",       { "ER DELAY L", "ER DELAY R", "ER LEVEL", "REV DELAY", "HI CUT", "DENSITY", "LOW CUT", "DECAY", "MIX" }, { 25, 32, 65, 38, 70, 78, 20, 54, 50 } },

            // ==================== PAGE 2 (FX 1-4) ====================
            { 8,  "gated_rev",   "Gated Reverb",        "GATE", 2, 0, 0, GuiFamily::PcmPlate,     juce::Colour(0xff22c55e), false, "GATED",     { "PRE DELAY", "DECAY", "ATTACK", "DENSITY", "SPREAD", "LO CUT", "HI SV FRQ", "HI SV GN", "DIFF", "MIX" }, { 15, 38, 12, 82, 75, 25, 65, 55, 70, 50 } },
            { 9,  "reverse_rev", "Reverse Reverb",      "RVRS", 2, 0, 0, GuiFamily::PcmPlate,     juce::Colour(0xff22c55e), false, "REVERSE",   { "PRE DELAY", "DECAY", "RISE", "DIFF", "SPREAD", "LO CUT", "HI SV FRQ", "HI SV GN", "MIX" }, { 18, 60, 72, 68, 80, 22, 64, 52, 50 } },
            { 10, "stereo_dly",  "Stereo Delay",        "DLY",  2, 0, 0, GuiFamily::StereoDelay,  juce::Colour(0xff22c55e), false, "STEREO DLY",{ "MIX", "TIME", "LO CUT", "HI CUT", "FACTOR L", "FACTOR R", "OFFSET LR", "FEED LO", "FEED L", "FEED R", "FEED HI" }, { 50, 45, 20, 75, 50, 66, 30, 25, 42, 42, 68 } },
            { 11, "tap_dly",     "Tap Delay",           "_3TAP",2, 0, 0, GuiFamily::StereoDelay,  juce::Colour(0xff38bdf8), false, "3-TAP DLY", { "TIME BASE", "GAIN BASE", "PAN BASE", "FEED", "LO CUT", "HI CUT", "FACTOR A", "GAIN A", "PAN A", "FACTOR B", "GAIN B", "PAN B" }, { 48, 80, 50, 38, 22, 74, 33, 65, 20, 66, 65, 80 } },
            { 12, "rhythm_dly",  "Rhythm Delay",        "_4TAP",2, 0, 0, GuiFamily::StereoDelay,  juce::Colour(0xfff59e0b), false, "4-TAP DLY", { "TIME BASE", "GAIN BASE", "FEED", "SPREAD", "LO CUT", "HI CUT", "FACTOR A", "GAIN A", "FACTOR B", "GAIN B", "FACTOR C", "GAIN C" }, { 52, 82, 40, 75, 20, 72, 25, 70, 50, 62, 75, 55 } },
            { 13, "st_chorus",   "Stereo Chorus",       "CRS",  2, 0, 0, GuiFamily::ColoredMod,   juce::Colour(0xff0284c7), false, "Stereo Chorus",  { "SPEED", "WIDTH L", "WIDTH R", "DELAY L", "DELAY R", "MIX", "LO CUT", "HI CUT", "PHASE", "WAVE", "SPREAD" }, { 34, 62, 62, 40, 45, 50, 18, 78, 50, 35, 80 } },
            { 14, "st_flanger",  "Stereo Flanger",      "FLNG", 2, 0, 0, GuiFamily::ColoredMod,   juce::Colour(0xff9333ea), false, "Stereo Flanger", { "SPEED", "WIDTH L", "WIDTH R", "DELAY L", "DELAY R", "MIX", "LO CUT", "HI CUT", "PHASE", "FEED LO", "FEED HI", "FEED" }, { 28, 70, 70, 32, 36, 50, 15, 82, 50, 25, 75, 58 } },
            { 15, "st_phaser",   "Stereo Phaser",       "PHAS", 2, 4, 7, GuiFamily::ColoredMod,   juce::Colour(0xff16a34a), false, "Stereo Phaser",  { "SPEED", "DEPTH", "RESO", "BASE", "STAGES", "MIX", "WAVE", "PHASE", "ENV MOD", "ATTACK", "HOLD", "RELEASE" }, { 32, 68, 55, 42, 60, 50, 40, 50, 30, 25, 35, 48 } },

            // ==================== PAGE 3 (FX 1-4) ====================
            { 16, "dimension_c", "Dimension-C",         "DIMC", 3, 0, 0, GuiFamily::DimensionC,   juce::Colour(0xffef4444), false, "DIM-C",     { "STEREO", "MODE 1", "MODE 2", "MODE 3", "MODE 4" }, { 100, 0, 100, 0, 0 } },
            { 17, "mood_filter", "Mood Filter",         "FILT", 3, 4, 8, GuiFamily::MoodFilter,   juce::Colour(0xff38bdf8), false, "mood",      { "SPEED", "DEPTH", "RESO", "BASE", "MODE", "MIX", "WAVE", "PHASE", "ENV MOD", "ATTACK", "RELEASE", "DRIVE" }, { 38, 64, 58, 45, 33, 65, 50, 50, 42, 22, 52, 35 } },
            { 18, "rotary_spk",  "Rotary Speaker",      "ROTA", 3, 0, 0, GuiFamily::RotarySpeaker,juce::Colour(0xff7c2d12), false, "ROTARY",    { "LO SPEED", "HI SPEED", "ACCEL", "DISTANCE", "BALANCE", "MIX" }, { 35, 68, 45, 52, 50, 75 } },
            { 19, "tremolo_pan", "Tremolo Panner",      "PAN",  3, 5, 1, GuiFamily::ColoredMod,   juce::Colour(0xff0d9488), false, "Stereo Tremolo", { "SPEED", "PHASE", "WAVE", "DEPTH", "ENV SPD", "ENV DPTH", "ATTACK", "HOLD", "RELEASE" }, { 48, 50, 40, 72, 30, 35, 25, 30, 55 } },
            { 20, "sub_octaver", "Suboctaver",          "SUB",  3, 5, 2, GuiFamily::SubOctaver,   juce::Colour(0xff8c5248), true,  "Sub Octaver",    { "DIRECT A", "OCT1 A", "OCT2 A", "DIRECT B", "OCT1 B", "OCT2 B" }, { 80, 62, 35, 80, 62, 35 } },
            { 21, "dly_chamber", "Delay & Chamber",     "D_RV", 3, 0, 0, GuiFamily::PcmPlate,     juce::Colour(0xff0088ff), false, "DELAY AND CHAMBER",  { "TIME", "PATTERN", "FEED HC", "FEEDBACK", "XFEED", "BALANCE", "PREDELAY", "DECAY", "SIZE", "DAMPING", "LO CUT", "MIX" }, { 46, 50, 68, 38, 25, 50, 18, 52, 64, 45, 22, 50 } },
            { 22, "chr_chamber", "Chorus & Chamber",    "CR_R", 3, 0, 0, GuiFamily::PcmPlate,     juce::Colour(0xff0088ff), false, "CHORUS AND CHAMBER", { "SPEED", "DEPTH", "DELAY", "PHASE", "WAVE", "BALANCE", "PREDELAY", "DECAY", "SIZE", "DAMPING", "LO CUT", "MIX" }, { 35, 62, 40, 50, 35, 50, 18, 52, 64, 45, 22, 50 } },
            { 23, "flg_chamber", "Flanger & Chamber",   "FL_R", 3, 0, 0, GuiFamily::PcmPlate,     juce::Colour(0xff0088ff), false, "FLANGER AND CHAMBER",{ "SPEED", "DEPTH", "DELAY", "PHASE", "FEED", "BALANCE", "PREDELAY", "DECAY", "SIZE", "DAMPING", "LO CUT", "MIX" }, { 28, 68, 34, 50, 55, 50, 18, 52, 64, 45, 22, 50 } },

            // ==================== PAGE 4 (FX 1-4) / PAGE 1 (FX 5-8) ====================
            { 24, "dly_chorus",  "Delay & Chorus",      "D_CR", 4, 0, 0, GuiFamily::DualGreenLcd, juce::Colour(0xff22c55e), false, "DELAY / CHORUS",  { "TIME", "PATTERN", "FEED HC", "FEED", "XFEED", "BAL", "SPEED", "DEPTH", "DELAY", "PHASE", "WAVE", "MIX" }, { 45, 50, 70, 40, 25, 50, 36, 62, 42, 50, 35, 50 } },
            { 25, "dly_flanger", "Delay & Flanger",     "D_FL", 4, 0, 0, GuiFamily::DualGreenLcd, juce::Colour(0xff22c55e), false, "DELAY / FLANGER", { "TIME", "PATTERN", "FEED HC", "FEED", "XFEED", "BAL", "SPEED", "DEPTH", "DELAY", "PHASE", "FEEDBACK", "MIX" }, { 45, 50, 70, 40, 25, 50, 28, 68, 34, 50, 58, 50 } },
            { 26, "mod_delay",   "Modulation Delay",    "MODD", 4, 0, 0, GuiFamily::PcmPlate,     juce::Colour(0xff22c55e), false, "DLY > CHR > REV", { "TIME", "FEED", "LOW CUT", "HI CUT", "DEPTH", "RATE", "DECAY", "PRE DLY", "MIX" }, { 48, 42, 20, 74, 55, 38, 52, 18, 50 } },
            { 27, "dual_geq",    "Dual GEQ",            "GEQ2", 4, 1, 1, GuiFamily::GraphicEq,    juce::Colour(0xffa1a1aa), true,  "DUAL GEQ",    {}, {} },
            { 28, "stereo_geq",  "Stereo GEQ",          "GEQ",  4, 1, 2, GuiFamily::GraphicEq,    juce::Colour(0xffa1a1aa), false, "STEREO GEQ",  {}, {} },
            { 29, "dual_trueq",  "Dual Tru EQ",         "TEQ2", 4, 1, 3, GuiFamily::GraphicEq,    juce::Colour(0xff38bdf8), true,  "DUAL TruEQ",  {}, {} },
            { 30, "stereo_trueq","Stereo Tru EQ",       "TEQ",  4, 1, 4, GuiFamily::GraphicEq,    juce::Colour(0xff38bdf8), false, "STEREO TruEQ",{}, {} },
            { 31, "dual_deesser","Dual DeEsser",        "DES2", 4, 1, 5, GuiFamily::DeEsser,      juce::Colour(0xfff59e0b), true,  "des",         { "LO BAND A", "HI BAND A", "LO BAND B", "HI BAND B" }, { 42, 58, 42, 58 } },

            // ==================== PAGE 5 (FX 1-4) ====================
            { 32, "st_deesser",  "Stereo DeEsser",      "DES",  5, 1, 6, GuiFamily::DeEsser,      juce::Colour(0xfff59e0b), false, "sds",         { "LO BAND L", "HI BAND L", "LO BAND R", "HI BAND R" }, { 45, 62, 45, 62 } },
            { 33, "st_xtec_eq1", "Stereo Xtec EQ1",     "P1A",  5, 1, 7, GuiFamily::XtecEq1,      juce::Colour(0xff244e5c), false, "Stereo",      { "GAIN", "LO BOOST", "LO ATT", "HI BOOST", "HI ATT", "ATT SEL", "LO FREQ", "HI BW", "HI FREQ", "TRANSF" }, { 50, 42, 28, 48, 22, 60, 40, 50, 65, 100 } },
            { 34, "du_xtec_eq1", "Dual Xtec EQ1",       "P1A2", 5, 1, 8, GuiFamily::XtecEq1,      juce::Colour(0xff244e5c), true,  "Channel A/B", { "GAIN", "LO BOOST", "LO ATT", "HI BOOST", "HI ATT", "ATT SEL", "LO FREQ", "HI BW", "HI FREQ", "TRANSF" }, { 50, 42, 28, 48, 22, 60, 40, 50, 65, 100 } },
            { 35, "st_xtec_eq5", "Stereo Xtec EQ5",     "PQ5",  5, 2, 1, GuiFamily::XtecEq5,      juce::Colour(0xff244e5c), false, "Stereo",      { "GAIN", "LO FREQ", "MID FREQ", "HI FREQ", "LO BOOST", "MID CUT", "HI BOOST", "TRANSF" }, { 50, 35, 50, 68, 44, 38, 46, 100 } },
            { 36, "du_xtec_eq5", "Dual Xtec EQ5",       "PQ5S", 5, 2, 2, GuiFamily::XtecEq5,      juce::Colour(0xff244e5c), true,  "Channel A/B", { "GAIN", "LO FREQ", "MID FREQ", "HI FREQ", "LO BOOST", "MID CUT", "HI BOOST", "TRANSF" }, { 50, 35, 50, 68, 44, 38, 46, 100 } },
            { 37, "wave_design", "Wave Designer",       "WAVD", 5, 2, 3, GuiFamily::WaveDesigner, juce::Colour(0xff17256e), true,  "Wave Designer",{ "ATTACK A", "SUSTAIN A", "GAIN A", "ATTACK B", "SUSTAIN B", "GAIN B" }, { 64, 42, 50, 64, 42, 50 } },
            { 38, "prec_limiter","Stereo Precision Limiter","LIM",5,2,4, GuiFamily::PrecisionLimiter,juce::Colour(0xff9ab3cc),false,"Limiter",      { "INPUT GAIN", "OUTPUT GAIN", "SQUEEZE", "ATTACK", "RELEASE", "KNEE" }, { 55, 78, 35, 25, 48, 40 } },
            { 39, "combinator",  "Combinator",          "CMB",  5, 0, 0, GuiFamily::Combinator,   juce::Colour(0xffa855f7), false, "STEREO",      { "MIX", "ATT", "REL", "X-OVER", "THRESH", "GAIN", "BAND RATIO", "SBC" }, { 100, 35, 52, 50, 62, 55, 48, 100 } },

            // ==================== PAGE 6 (FX 1-4) ====================
            { 40, "du_combinat", "Dual Combinator",     "CMB2", 6, 0, 0, GuiFamily::Combinator,   juce::Colour(0xffa855f7), true,  "DUAL",        { "MIX", "ATT", "REL", "X-OVER", "THRESH", "GAIN", "BAND RATIO", "SBC" }, { 100, 35, 52, 50, 62, 55, 48, 100 } },
            { 41, "fair_comp",   "Fair Comp",           "FAC",  6, 2, 5, GuiFamily::FairComp,     juce::Colour(0xff15262d), false, "STEREO",      { "INPUT GAIN", "THRESHOLD", "TIME", "OUTPUT GAIN", "BIAS", "BALANCE" }, { 48, 56, 33, 62, 50, 50 } },
            { 42, "ms_fair_comp","M/S Fair Comp",       "FAC1M",6, 2, 6, GuiFamily::FairComp,     juce::Colour(0xff15262d), true,  "M/S",         { "INPUT GAIN", "THRESHOLD", "TIME", "OUTPUT GAIN", "BIAS", "BALANCE" }, { 48, 56, 33, 62, 50, 50 } },
            { 43, "du_fair_comp","Dual Fair Comp",      "FAC2", 6, 2, 7, GuiFamily::FairComp,     juce::Colour(0xff15262d), true,  "DUAL",        { "INPUT GAIN", "THRESHOLD", "TIME", "OUTPUT GAIN", "BIAS", "BALANCE" }, { 48, 56, 33, 62, 50, 50 } },
            { 44, "leisure_comp","Leisure Comp",        "LEC",  6, 2, 8, GuiFamily::LeisureComp,  juce::Colour(0xff8c8f94), false, "STEREO",      { "GAIN", "PEAK REDUCTION", "OUTPUT" }, { 52, 58, 65 } },
            { 45, "du_leisure",  "Dual Leisure Comp",   "LEC2", 6, 3, 1, GuiFamily::LeisureComp,  juce::Colour(0xff8c8f94), true,  "DUAL",        { "GAIN", "PEAK REDUCTION", "OUTPUT" }, { 52, 58, 65 } },
            { 46, "ultimo_comp", "Ultimo Comp",         "ULC",  6, 3, 2, GuiFamily::UltimoComp,   juce::Colour(0xff161618), false, "STEREO",      { "INPUT", "OUTPUT", "ATTACK", "RELEASE" }, { 55, 62, 42, 68 } },
            { 47, "du_ultimo",   "Dual Ultimo Comp",    "ULC2", 6, 3, 3, GuiFamily::UltimoComp,   juce::Colour(0xff161618), true,  "DUAL",        { "INPUT", "OUTPUT", "ATTACK", "RELEASE" }, { 55, 62, 42, 68 } },

            // ==================== PAGE 7 (FX 1-4) ====================
            { 48, "du_enhancer", "Dual Enhancer",       "ENH2", 7, 3, 4, GuiFamily::Enhancer,     juce::Colour(0xffdfbe62), true,  "Enhancer",    { "OUT GAIN", "BASS GAIN", "BASS FREQ", "MID GAIN", "MID Q", "HI GAIN", "HI FREQ" }, { 50, 58, 40, 52, 50, 62, 68 } },
            { 49, "st_enhancer", "Stereo Enhancer",     "ENH",  7, 3, 5, GuiFamily::Enhancer,     juce::Colour(0xffdfbe62), false, "Stereo Enhancer",{ "OUT GAIN", "SPREAD", "BASS GAIN", "BASS FREQ", "MID GAIN", "MID Q", "HI GAIN", "HI FREQ" }, { 50, 65, 58, 40, 52, 50, 62, 68 } },
            { 50, "du_exciter",  "Dual Exciter",        "EXC2", 7, 3, 6, GuiFamily::Exciter,      juce::Colour(0xffd4d4d8), true,  "Dual Exciter",{ "TUNE", "PEAK", "ZERO FILL", "TIMBRE", "HARMONICS", "MIX" }, { 56, 48, 42, 55, 62, 45 } },
            { 51, "st_exciter",  "Stereo Exciter",      "EXC",  7, 3, 7, GuiFamily::Exciter,      juce::Colour(0xffd4d4d8), false, "Stereo Exciter",{ "TUNE", "PEAK", "ZERO FILL", "TIMBRE", "HARMONICS", "MIX" }, { 56, 48, 42, 55, 62, 45 } },
            { 52, "st_imager",   "Stereo Imager",       "IMG",  7, 3, 8, GuiFamily::StereoImager, juce::Colour(0xffd4d4d8), false, "STEREO IMAGER",{ "BALANCE", "MONO PAN", "STEREO PAN", "SHV GAIN", "SHV FREQ", "SHV Q", "OUT GAIN" }, { 50, 50, 68, 56, 62, 50, 55 } },
            { 53, "edison_ex1",  "Edison EX1",          "EDI",  7, 4, 1, GuiFamily::EdisonEx1,    juce::Colour(0xff22c55e), false, "EDISON EX1+", { "ST SPREAD", "LMF SPREAD", "BALANCE", "CENTER DIST", "OUTPUT GAIN" }, { 68, 45, 50, 52, 55 } },
            { 54, "sound_maxer", "Sound Maxer",         "SON",  7, 4, 2, GuiFamily::SoundMaxer,   juce::Colour(0xff991b1b), true,  "SOUND MAXER", { "LO CONTOUR A", "PROCESS A", "GAIN A", "LO CONTOUR B", "PROCESS B", "GAIN B" }, { 58, 64, 50, 58, 64, 50 } },
            { 55, "du_guitaramp","Dual GuitarAmp",      "AMP2", 7, 4, 3, GuiFamily::GuitarAmp,    juce::Colour(0xff27272a), true,  "Dual GuitarAmp",{ "PREAMP", "BUZZ", "PUNCH", "CRUNCH", "DRIVE", "LEVEL", "LOW", "HIGH" }, { 58, 42, 55, 60, 65, 70, 54, 58 } },

            // ==================== PAGE 8 (FX 1-4) ====================
            { 56, "st_guitaramp","Stereo GuitarAmp",    "AMP",  8, 4, 4, GuiFamily::GuitarAmp,    juce::Colour(0xff27272a), false, "GuitarAmp",   { "PREAMP", "BUZZ", "PUNCH", "CRUNCH", "DRIVE", "LEVEL", "LOW", "HIGH" }, { 58, 42, 55, 60, 65, 70, 54, 58 } },
            { 57, "du_tubestage","Dual Tube Stage",     "DRV2", 8, 4, 5, GuiFamily::TubeStage,    juce::Colour(0xffb91c1c), true,  "Dual Tube Stage",{ "DRIVE", "EVEN", "ODD", "GAIN", "LO CUT", "HI CUT", "BASS GAIN", "BASS FREQ", "TREBLE GAIN", "TREBLE FREQ" }, { 56, 62, 48, 52, 20, 76, 55, 42, 58, 68 } },
            { 58, "st_tubestage","Stereo Tube Stage",   "DRV",  8, 4, 6, GuiFamily::TubeStage,    juce::Colour(0xffb91c1c), false, "Tube Stage",  { "DRIVE", "EVEN", "ODD", "GAIN", "LO CUT", "HI CUT", "BASS GAIN", "BASS FREQ", "TREBLE GAIN", "TREBLE FREQ" }, { 56, 62, 48, 52, 20, 76, 55, 42, 58, 68 } },
            { 59, "dual_pitch",  "Dual Pitch",          "PIT2", 8, 0, 0, GuiFamily::DualPitch,    juce::Colour(0xff27272a), true,  "DUAL PITCH",  { "SEMI 1", "CENT 1", "DELAY 1", "GAIN 1", "PAN 1", "SEMI 2", "CENT 2", "DELAY 2", "GAIN 2", "PAN 2" }, { 50, 42, 25, 75, 25, 50, 58, 30, 75, 75 } },
            { 60, "stereo_pitch","Stereo Pitch",        "PIT",  8, 0, 0, GuiFamily::PcmPlate,     juce::Colour(0xff0088ff), false, "STEREO PITCH",{ "SEMI", "CENT", "DELAY", "LO CUT", "HI CUT", "MIX" }, { 50, 58, 28, 20, 76, 50 } }
        };
        return catalog;
    }

    inline juce::String formatParamValue (const EffectModelInfo& model, int paramIdx, float normVal)
    {
        normVal = juce::jlimit (0.0f, 1.0f, normVal);
        const auto& id = model.id;

        // Vintage Reverb (EMT 250)
        if (id == "vintage_rev")
        {
            switch (paramIdx)
            {
                case 0: return juce::String (0.4f + normVal * 4.1f, 1) + " s"; // Decay 0.4..4.5s
                case 1: return juce::String (juce::roundToInt (normVal * 100.0f)) + " ms"; // PreDelay 0..100ms
                case 2: return normVal < 0.25f ? "Off" : (normVal < 0.7f ? "Mid" : "High"); // Lo Mult / Diff
                case 3: return juce::String (juce::roundToInt (1.0f + normVal * 15.0f)) + " kHz"; // Hi Damp
                case 4: return juce::String (0.5f + normVal * 1.5f, 2); // Bass Mult
                case 5: return normVal < 0.33f ? "Slow" : (normVal < 0.66f ? "Mid" : "Fast"); // Mod Speed
                case 6: return juce::String (juce::roundToInt (-24.0f + normVal * 24.0f)) + " dB"; // Tail Gain
                case 7: return juce::String (juce::roundToInt (2.0f + normVal * 14.0f)) + " kHz"; // HiSvFr
                case 8: return juce::String (juce::roundToInt (-12.0f + normVal * 24.0f)) + " dB"; // HiSvGn
                case 9: return juce::String (juce::roundToInt (normVal * 100.0f)) + "%"; // Mix
                default: break;
            }
        }
        // Stage Reverb, Plate Reverb, Rich Plate, Room Reverb, Vintage Room, Ambience, Hall Reverb, Chamber Reverb
        if (id == "stage_rev" || id == "plate" || id == "rich_plate" || id == "room" ||
            id == "vintage_rm" || id == "ambience" || id == "hall" || id == "chamber")
        {
            switch (paramIdx)
            {
                // --- Page 1 ---
                case 0: return juce::String (juce::roundToInt (normVal * 200.0f)) + " ms"; // PRE DEL
                case 1: return juce::String (0.2f + normVal * 7.8f, 1) + " s";             // DECAY
                case 2: return juce::String (juce::roundToInt (normVal * 100.0f)) + "%";   // SIZE
                case 3: return juce::String (juce::roundToInt (1.0f + normVal * 15.0f)) + " kHz"; // DAMPING
                case 4: return juce::String (0.5f + normVal * 1.5f, 2);                    // DIFFUSE
                case 5: return juce::String (juce::roundToInt (-24.0f + normVal * 24.0f)) + " dB"; // LEVEL

                // --- Page 2 ---
                case 6: return juce::String (juce::roundToInt (20.0f + normVal * 480.0f)) + " Hz";  // LO CUT
                case 7: return juce::String (juce::roundToInt (1.0f + normVal * 19.0f)) + " kHz";  // HI CUT
                case 8: return juce::String (0.5f + normVal * 1.5f, 2);                    // BASSMULT
                case 9: return juce::String (juce::roundToInt (normVal * 100.0f)) + "%";   // SPREAD
                case 10: return juce::String (juce::roundToInt (normVal * 100.0f)) + "%";  // ATTACK
                case 11: return juce::String (juce::roundToInt (normVal * 100.0f)) + "%";  // SPIN

                // --- Page 3 ---
                case 12: return juce::String (juce::roundToInt (normVal * 500.0f)) + " ms"; // ECHO L DEL
                case 13: return juce::String (juce::roundToInt (normVal * 500.0f)) + " ms"; // ECHO R DEL
                case 14: return juce::String (juce::roundToInt (normVal * 100.0f)) + "%";  // ECHO L FEED
                case 15: return juce::String (juce::roundToInt (normVal * 100.0f)) + "%";  // ECHO R FEED
                default: break;
            }
        }
        // Gated Reverb
        if (id == "gated_rev")
        {
            switch (paramIdx)
            {
                case 0: return juce::String (juce::roundToInt (normVal * 50.0f)) + " ms"; // Pre Delay
                case 1: return juce::String (0.1f + normVal * 1.9f, 2) + " s"; // Decay / Time
                case 2: return juce::String (juce::roundToInt (-60.0f + normVal * 60.0f)) + " dB"; // Thresh / Attack
                case 3: return juce::String (juce::roundToInt (1.0f + normVal * 15.0f)) + " kHz"; // High Cut
                case 4: return juce::String (juce::roundToInt (10.0f + normVal * 490.0f)) + " ms"; // Hold
                case 5: return juce::String (juce::roundToInt (10.0f + normVal * 490.0f)) + " ms"; // Release
                case 9: return juce::String (juce::roundToInt (normVal * 100.0f)) + "%"; // Mix
                default: break;
            }
        }
        // Reverse Reverb
        if (id == "reverse_rev")
        {
            switch (paramIdx)
            {
                case 0: return juce::String (juce::roundToInt (normVal * 200.0f)) + " ms"; // Pre Delay
                case 1: return juce::String (0.2f + normVal * 2.8f, 1) + " s"; // Decay / Time
                case 2: return juce::String (juce::roundToInt (normVal * 100.0f)) + "%"; // Rise / Diff
                case 3: return juce::String (juce::roundToInt (1.0f + normVal * 15.0f)) + " kHz"; // High Cut
                case 4: return juce::String (juce::roundToInt (normVal * 100.0f)) + "%"; // Feedback / Spread
                case 8: return juce::String (juce::roundToInt (normVal * 100.0f)) + "%"; // Mix
                default: break;
            }
        }
        // Delays: Stereo Delay, Tap Delay, Rhythm Delay, Mod Delay
        if (id == "stereo_dly" || id == "tap_dly" || id == "rhythm_dly" || id == "mod_delay" ||
            id == "dly_chamber" || id == "dly_chorus" || id == "dly_flanger")
        {
            if (paramIdx == 0 || paramIdx == 1) // Time / Time L
            {
                return juce::String (juce::roundToInt (10.0f + normVal * 990.0f)) + " ms";
            }
            if (paramIdx >= 2 && paramIdx <= 3) // Feedback / High Cut
            {
                return (model.paramNames.size() > (size_t) paramIdx && model.paramNames[(size_t) paramIdx].containsIgnoreCase ("CUT"))
                           ? (juce::String (juce::roundToInt (1.0f + normVal * 15.0f)) + " kHz")
                           : (juce::String (juce::roundToInt (normVal * 100.0f)) + "%");
            }
        }
        // Dynamics: Fair Comp, Leisure Comp, Ultimo Comp, Combinator, Limiter
        if (id == "fair_comp" || id == "ms_fair_comp" || id == "du_fair_comp")
        {
            switch (paramIdx)
            {
                case 0: return juce::String (juce::roundToInt (-20.0f + normVal * 40.0f)) + " dB"; // Input Gain
                case 1: return juce::String (juce::roundToInt (-40.0f + normVal * 40.0f)) + " dB"; // Threshold
                case 2: return "Pos " + juce::String (1 + juce::roundToInt (normVal * 5.0f)); // Time constant 1..6
                case 3: return juce::String (juce::roundToInt (-20.0f + normVal * 40.0f)) + " dB"; // Output Gain
                case 4: return juce::String (juce::roundToInt (normVal * 100.0f)) + "%"; // Bias
                case 5: return juce::String (juce::roundToInt (-50.0f + normVal * 100.0f)); // Balance
                default: break;
            }
        }
        if (id == "leisure_comp" || id == "du_leisure")
        {
            switch (paramIdx)
            {
                case 0: return juce::String (juce::roundToInt (normVal * 100.0f)) + "%"; // Gain
                case 1: return juce::String (juce::roundToInt (normVal * 100.0f)) + "%"; // Peak Reduction
                case 2: return juce::String (juce::roundToInt (-18.0f + normVal * 36.0f)) + " dB"; // Output
                default: break;
            }
        }
        if (id == "ultimo_comp" || id == "du_ultimo")
        {
            switch (paramIdx)
            {
                case 0: return juce::String (juce::roundToInt (-20.0f + normVal * 40.0f)) + " dB"; // Input
                case 1: return juce::String (juce::roundToInt (-20.0f + normVal * 40.0f)) + " dB"; // Output
                case 2: return normVal < 0.33f ? "Slow" : (normVal < 0.66f ? "Med" : "Fast"); // Attack
                case 3: return normVal < 0.33f ? "Slow" : (normVal < 0.66f ? "Med" : "Fast"); // Release
                default: break;
            }
        }
        if (id == "prec_limiter")
        {
            switch (paramIdx)
            {
                case 0: return juce::String (juce::roundToInt (-12.0f + normVal * 24.0f)) + " dB"; // Input Gain
                case 1: return juce::String (juce::roundToInt (-24.0f + normVal * 24.0f)) + " dB"; // Output Gain
                case 2: return juce::String (juce::roundToInt (normVal * 100.0f)) + "%"; // Squeeze
                case 3: return juce::String (0.1f + normVal * 9.9f, 1) + " ms"; // Attack
                case 4: return juce::String (juce::roundToInt (10.0f + normVal * 990.0f)) + " ms"; // Release
                case 5: return juce::String (juce::roundToInt (normVal * 10.0f)) + " dB"; // Knee
                default: break;
            }
        }
        if (id == "combinator" || id == "du_combinat")
        {
            switch (paramIdx)
            {
                case 0: return juce::String (juce::roundToInt (normVal * 100.0f)) + "%"; // Mix
                case 1: return juce::String (1.0f + normVal * 49.0f, 1) + " ms"; // Att
                case 2: return juce::String (juce::roundToInt (10.0f + normVal * 990.0f)) + " ms"; // Rel
                case 3: return juce::String (juce::roundToInt (100.0f + normVal * 900.0f)) + " Hz"; // X-Over
                case 4: return juce::String (juce::roundToInt (-40.0f + normVal * 40.0f)) + " dB"; // Thresh
                case 5: return juce::String (juce::roundToInt (-12.0f + normVal * 24.0f)) + " dB"; // Gain
                case 6: return juce::String (1.0f + normVal * 9.0f, 1) + ":1"; // Ratio
                case 7: return normVal < 0.5f ? "Off" : "On"; // SBC
                default: break;
            }
        }

        // Generic fallback by parameter name keywords
        if (paramIdx >= 0 && paramIdx < (int) model.paramNames.size())
        {
            const auto& pName = model.paramNames[(size_t) paramIdx].toUpperCase();
            if (pName.contains ("TIME") || pName.contains ("DELAY") || pName.contains ("PRE DEL"))
                return juce::String (juce::roundToInt (normVal * 500.0f)) + " ms";
            if (pName.contains ("DECAY"))
                return juce::String (0.2f + normVal * 7.8f, 1) + " s";
            if (pName.contains ("FREQ") || pName.contains ("CUT"))
                return (normVal > 0.4f) ? (juce::String (1.0f + (normVal - 0.4f) * 25.0f, 1) + " kHz")
                                        : (juce::String (juce::roundToInt (20.0f + normVal * 2400.0f)) + " Hz");
            if (pName.contains ("GAIN") || pName.contains ("LEVEL") || pName.contains ("THRESH"))
                return juce::String (juce::roundToInt (-24.0f + normVal * 36.0f)) + " dB";
            if (pName.contains ("RATE") || pName.contains ("SPEED"))
                return juce::String (0.1f + normVal * 9.9f, 1) + " Hz";
        }

        // Default: percentage
        return juce::String (juce::roundToInt (normVal * 100.0f)) + "%";
    }
}

class X32FxLookAndFeel : public juce::LookAndFeel_V4
{
public:
    X32FxLookAndFeel() = default;

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override
    {
        const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (3.0f);
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 4.0f;
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY();
        const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        const bool isSilver = slider.getProperties().getWithDefault ("silverKnob", false);
        const bool hasAmberRing = slider.getProperties().getWithDefault ("amberRing", false);

        // 1. Level Scale Ticks around the knob (ขีดบอกระดับ)
        const int numTicks = 11;
        const float tickInnerR = radius + 2.5f;
        const float tickOuterR = radius + 5.0f;
        for (int i = 0; i < numTicks; ++i)
        {
            const float tNorm = (float) i / (float) (numTicks - 1);
            const float tAngle = rotaryStartAngle + tNorm * (rotaryEndAngle - rotaryStartAngle);
            const bool isPast = (tNorm <= sliderPosProportional + 0.02f);

            juce::Colour tickCol = isPast ? (hasAmberRing ? juce::Colour (0xfff59e0b) : juce::Colour (0xff38bdf8))
                                          : juce::Colour (0xff3f3f46);
            g.setColour (tickCol);
            const float cosA = std::cos (tAngle - juce::MathConstants<float>::halfPi);
            const float sinA = std::sin (tAngle - juce::MathConstants<float>::halfPi);
            g.drawLine (cx + tickInnerR * cosA, cy + tickInnerR * sinA,
                        cx + tickOuterR * cosA, cy + tickOuterR * sinA, 1.4f);
        }

        // 2. Continuous Active Level Arc Track (ขีดบอกระดับแบบเส้นโค้ง)
        {
            juce::Path bgTrack;
            bgTrack.addCentredArc (cx, cy, radius + 2.0f, radius + 2.0f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
            g.setColour (juce::Colour (0xff1c1917).withAlpha (0.8f));
            g.strokePath (bgTrack, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            if (sliderPosProportional > 0.005f)
            {
                juce::Path activeTrack;
                activeTrack.addCentredArc (cx, cy, radius + 2.0f, radius + 2.0f, 0.0f, rotaryStartAngle, angle, true);
                g.setColour (hasAmberRing ? juce::Colour (0xfff59e0b) : juce::Colour (0xff38bdf8));
                g.strokePath (activeTrack, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
        }

        // 3. Drop Shadow & Knob Body
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillEllipse (cx - radius, cy - radius + 2.5f, radius * 2.0f, radius * 2.0f);

        juce::Rectangle<float> knobArea (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
        if (isSilver)
        {
            juce::ColourGradient grad (juce::Colour (0xfff4f4f5), cx - radius * 0.5f, cy - radius * 0.5f,
                                      juce::Colour (0xff71717a), cx + radius * 0.5f, cy + radius * 0.5f, true);
            g.setGradientFill (grad);
            g.fillEllipse (knobArea);
            g.setColour (juce::Colour (0xff18181b));
            g.drawEllipse (knobArea, 1.5f);
        }
        else
        {
            juce::ColourGradient grad (juce::Colour (0xff52525b), cx - radius * 0.4f, cy - radius * 0.4f,
                                      juce::Colour (0xff141416), cx + radius * 0.6f, cy + radius * 0.6f, true);
            g.setGradientFill (grad);
            g.fillEllipse (knobArea);
            g.setColour (juce::Colour (0xff09090b));
            g.drawEllipse (knobArea, 1.2f);
        }

        // 4. Knob Pointer
        juce::Path pointer;
        const float pointerLen = radius * 0.65f;
        const float pointerThick = 2.2f;
        pointer.addRoundedRectangle (-pointerThick * 0.5f, -radius + 2.5f, pointerThick, pointerLen, 1.0f);

        g.setColour (isSilver ? juce::Colour (0xff18181b) : juce::Colours::white);
        g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (cx, cy));

        // 5. Live Value Badge Popup when adjusting (ตัวเลขเล็กๆ ขึ้นมาโชว์สถานะ)
        const bool isAdjusting = slider.isMouseButtonDown();
        const juce::String liveValText = slider.getProperties().getWithDefault ("liveValueText", juce::String());

        if (isAdjusting && liveValText.isNotEmpty())
        {
            const float badgeW = juce::jmax (42.0f, (float) liveValText.length() * 7.5f + 12.0f);
            const float badgeH = 16.0f;
            const float badgeX = cx - badgeW * 0.5f;
            const float badgeY = cy - radius - 14.0f;

            juce::Rectangle<float> badgeRect (badgeX, badgeY, badgeW, badgeH);

            g.setColour (juce::Colour (0xff09090b).withAlpha (0.92f));
            g.fillRoundedRectangle (badgeRect, 3.0f);
            g.setColour (juce::Colour (0xff38bdf8));
            g.drawRoundedRectangle (badgeRect, 3.0f, 1.0f);

            g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 10.5f, juce::Font::bold));
            g.setColour (juce::Colour (0xfff0fdf4));
            g.drawText (liveValText, badgeRect, juce::Justification::centred, false);
        }
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                           const juce::Slider::SliderStyle /*style*/, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
        const float cx = bounds.getCentreX();
        const bool isLarc = slider.getProperties().getWithDefault ("larcStyle", false);
        const bool isSilver = slider.getProperties().getWithDefault ("silverCap", false);

        const float slotTop = bounds.getY() + 8.0f;
        const float slotBot = bounds.getBottom() - 8.0f;
        const float trackW = isLarc ? 5.0f : 3.5f;

        // Fader slot track
        g.setColour (juce::Colour (0xff0b0b0c));
        g.fillRoundedRectangle (cx - trackW * 0.5f, slotTop, trackW, slotBot - slotTop, 1.5f);

        // Tick marks along the track for non-LARC linear sliders (LARC draws its own 13-tick scale between columns)
        if (! isLarc)
        {
            const int numTicks = 9;
            for (int i = 0; i < numTicks; ++i)
            {
                const float tNorm = (float) i / (float) (numTicks - 1);
                const float ty = slotTop + tNorm * (slotBot - slotTop);
                g.setColour (juce::Colour (0xff3f3f46).withAlpha (0.6f));
                g.drawHorizontalLine (juce::roundToInt (ty), cx - trackW - 3.0f, cx - trackW - 1.0f);
                g.drawHorizontalLine (juce::roundToInt (ty), cx + trackW + 1.0f, cx + trackW + 3.0f);
            }
        }

        const float capW = isLarc ? juce::jmin (bounds.getWidth() * 0.82f, 48.0f)
                                  : juce::jmin (bounds.getWidth() * 0.85f, 22.0f);
        const float capH = isLarc ? 16.0f : 11.0f;
        const float cy = juce::jlimit (slotTop, slotBot, sliderPos);

        juce::Rectangle<float> capRect (cx - capW * 0.5f, cy - capH * 0.5f, capW, capH);

        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (capRect.translated (0, 2.5f), 2.0f);

        juce::Colour topCol = isLarc ? juce::Colour (0xffdfd7c2) : (isSilver ? juce::Colour (0xfff4f4f6) : juce::Colour (0xffe4e4e7));
        juce::Colour botCol = isLarc ? juce::Colour (0xffada288) : (isSilver ? juce::Colour (0xffa1a1aa) : juce::Colour (0xff71717a));

        juce::ColourGradient grad (topCol, capRect.getX(), capRect.getY(), botCol, capRect.getX(), capRect.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (capRect, 2.0f);

        g.setColour (juce::Colour (0xff27272a));
        g.drawRoundedRectangle (capRect, 2.0f, 1.0f);

        g.setColour (juce::Colour (0xff18181b));
        g.drawHorizontalLine ((int) cy, capRect.getX() + 2.0f, capRect.getRight() - 2.0f);

        // Live Value Tooltip Badge when active / dragging
        const bool isAdjusting = slider.isMouseButtonDown();
        const juce::String liveValText = slider.getProperties().getWithDefault ("liveValueText", juce::String());

        if (isAdjusting && liveValText.isNotEmpty())
        {
            const float badgeW = juce::jmax (38.0f, (float) liveValText.length() * 7.0f + 10.0f);
            const float badgeH = 15.0f;
            const float badgeX = cx - badgeW * 0.5f;
            const float badgeY = cy - capH * 0.5f - 14.0f;

            juce::Rectangle<float> badgeRect (badgeX, badgeY, badgeW, badgeH);

            g.setColour (juce::Colour (0xff09090b).withAlpha (0.92f));
            g.fillRoundedRectangle (badgeRect, 3.0f);
            g.setColour (juce::Colour (0xff38bdf8));
            g.drawRoundedRectangle (badgeRect, 3.0f, 1.0f);

            g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 10.0f, juce::Font::bold));
            g.setColour (juce::Colour (0xfff0fdf4));
            g.drawText (liveValText, badgeRect, juce::Justification::centred, false);
        }
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& /*backgroundColour*/,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override
    {
        auto b = button.getLocalBounds().toFloat().reduced (0.5f);
        const bool isLarcPreset = button.getProperties().getWithDefault ("larcPresetBtn", false);
        const bool isEngineBtn  = button.getProperties().getWithDefault ("engineBtn", false);
        const bool isMuteBtn    = button.getProperties().getWithDefault ("muteBtn", false);
        const bool isRackBtn    = button.getProperties().getWithDefault ("rackBtn", false);
        const bool toggled      = button.getToggleState();

        if (isLarcPreset)
        {
            juce::Colour topCol = toggled ? juce::Colour (0xffa89f8d) : juce::Colour (0xffded8c8);
            juce::Colour botCol = toggled ? juce::Colour (0xff787060) : juce::Colour (0xffb5ae9e);
            if (shouldDrawButtonAsHighlighted && ! toggled)
                topCol = topCol.brighter (0.08f);

            juce::ColourGradient grad (topCol, b.getX(), b.getY(), botCol, b.getX(), b.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (b, 2.0f);

            g.setColour (juce::Colour (0xff27241d));
            g.drawRoundedRectangle (b, 2.0f, 1.0f);
            return;
        }

        if (isEngineBtn)
        {
            juce::Colour topCol = toggled ? juce::Colour (0xff0ea5e9) : juce::Colour (0xff3f3f46);
            juce::Colour botCol = toggled ? juce::Colour (0xff0284c7) : juce::Colour (0xff1f1f22);
            juce::ColourGradient grad (topCol, b.getX(), b.getY(), botCol, b.getX(), b.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (b, 2.5f);

            g.setColour (toggled ? juce::Colour (0xff7dd3fc) : juce::Colour (0xff141416));
            g.drawRoundedRectangle (b, 2.5f, 1.2f);
            return;
        }

        if (isMuteBtn)
        {
            juce::Colour topCol = toggled ? juce::Colour (0xffdc2626) : juce::Colour (0xff3a3a3e);
            juce::Colour botCol = toggled ? juce::Colour (0xff991b1b) : juce::Colour (0xff202022);
            juce::ColourGradient grad (topCol, b.getX(), b.getY(), botCol, b.getX(), b.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (b, 2.0f);

            g.setColour (toggled ? juce::Colour (0xfff87171) : juce::Colour (0xff141416));
            g.drawRoundedRectangle (b, 2.0f, 1.0f);
            return;
        }

        if (isRackBtn)
        {
            juce::Colour topCol = shouldDrawButtonAsDown ? juce::Colour (0xff27272a) : juce::Colour (0xff3a3a3e);
            juce::Colour botCol = shouldDrawButtonAsDown ? juce::Colour (0xff18181b) : juce::Colour (0xff202022);
            if (shouldDrawButtonAsHighlighted)
                topCol = topCol.brighter (0.08f);

            juce::ColourGradient grad (topCol, b.getX(), b.getY(), botCol, b.getX(), b.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (b, 2.0f);

            g.setColour (juce::Colour (0xff141416));
            g.drawRoundedRectangle (b, 2.0f, 1.0f);
            return;
        }

        LookAndFeel_V4::drawButtonBackground (g, button, button.findColour (juce::TextButton::buttonColourId),
                                              shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button,
                         bool /*shouldDrawButtonAsHighlighted*/, bool /*shouldDrawButtonAsDown*/) override
    {
        const bool isLarcPreset = button.getProperties().getWithDefault ("larcPresetBtn", false);
        const bool isEngineBtn  = button.getProperties().getWithDefault ("engineBtn", false);
        const bool isMuteBtn    = button.getProperties().getWithDefault ("muteBtn", false);
        const bool isRackBtn    = button.getProperties().getWithDefault ("rackBtn", false);

        if (isLarcPreset)
        {
            g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
            g.setColour (button.getToggleState() ? juce::Colours::black : juce::Colour (0xff18181b));
            g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
            return;
        }

        if (isEngineBtn)
        {
            g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
            g.setColour (button.getToggleState() ? juce::Colours::white : juce::Colour (0xffd4d4d8));
            g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
            return;
        }

        if (isMuteBtn || isRackBtn)
        {
            g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
            g.setColour (button.getToggleState() ? juce::Colours::white : juce::Colour (0xffe4e4e7));
            g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
            return;
        }

        LookAndFeel_V4::drawButtonText (g, button, false, false);
    }
};

class X32EffectCarouselOverlay : public juce::Component
{
public:
    std::function<void(const X32Fx::EffectModelInfo&)> onEffectChosen;
    std::function<void()> onClose;

    X32EffectCarouselOverlay()
    {
        prevBtn.setButtonText ("<");
        nextBtn.setButtonText (">");
        closeBtn.setButtonText ("CLOSE");

        prevBtn.onClick = [this] { setPage (currentPage - 1); };
        nextBtn.onClick = [this] { setPage (currentPage + 1); };
        closeBtn.onClick = [this] { if (onClose) onClose(); };

        addAndMakeVisible (prevBtn);
        addAndMakeVisible (nextBtn);
        addAndMakeVisible (closeBtn);
    }

    void openForSlot (int slotNumber1To8, const juce::String& activeId)
    {
        currentSlot = slotNumber1To8;
        isFx58Mode = (slotNumber1To8 >= 5);
        activeModelId = activeId;
        setPage (1);
        setVisible (true);
        toFront (true);
    }

    int getMaxPages() const { return isFx58Mode ? 5 : 8; }

    void setPage (int p)
    {
        int maxP = getMaxPages();
        if (p < 1) p = maxP;
        if (p > maxP) p = 1;
        currentPage = p;

        pageItems.clear();
        for (const auto& item : X32Fx::getCatalog())
        {
            if (! isFx58Mode && item.page14 == currentPage)
                pageItems.push_back (item);
            else if (isFx58Mode && item.page58 == currentPage)
                pageItems.push_back (item);
        }

        if (isFx58Mode)
        {
            std::sort (pageItems.begin(), pageItems.end(),
                       [] (const X32Fx::EffectModelInfo& a, const X32Fx::EffectModelInfo& b) { return a.pos58 < b.pos58; });
        }

        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black.withAlpha (0.92f));
        auto box = getLocalBounds().reduced (16).toFloat();

        g.setColour (juce::Colour (0xff0b0b0d));
        g.fillRoundedRectangle (box, 8.0f);
        g.setColour (juce::Colour (0xff27272a));
        g.drawRoundedRectangle (box, 8.0f, 1.5f);

        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        juce::String hdr = "FX" + juce::String (currentSlot) + (isFx58Mode ? " INSERT LIBRARY (5 PAGES)" : " SIDECHAIN/INSERT LIBRARY (8 PAGES)")
                         + "   •   PAGE " + juce::String (currentPage) + " / " + juce::String (getMaxPages());
        g.drawText (hdr, (int) (box.getX() + 18), (int) (box.getY() + 8), 550, 26, juce::Justification::centredLeft, false);

        cardRects.clear();
        const float gridX = box.getX() + 20.0f;
        const float gridW = box.getWidth() - 40.0f;
        const float cardW = (gridW - 36.0f) / 4.0f;
        const float cardH = (box.getHeight() - 120.0f) * 0.45f;

        const float row1Y = box.getY() + 42.0f;
        const float row2Y = row1Y + cardH + 42.0f;

        for (size_t i = 0; i < pageItems.size(); ++i)
        {
            int col = (int) (i % 4);
            float cx = gridX + col * (cardW + 12.0f);
            float cy = (i < 4) ? row1Y : row2Y;

            juce::Rectangle<float> cRect (cx, cy, cardW, cardH);
            cardRects.push_back (cRect);

            const auto& fx = pageItems[i];
            const bool isSelected = (fx.id == activeModelId);

            g.setColour (juce::Colour (0xff141417));
            g.fillRoundedRectangle (cRect, 4.0f);

            auto thumb = cRect.reduced (6.0f).withTrimmedBottom (22.0f);
            g.setColour (fx.accentColour);
            g.fillRoundedRectangle (thumb, 3.0f);
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillRect (thumb.removeFromBottom (thumb.getHeight() * 0.45f));

            g.setColour (juce::Colours::white);
            g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::bold));
            g.drawText (fx.oscCode, thumb, juce::Justification::centred, false);

            g.setColour (juce::Colour (0xfff4f4f5));
            g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            g.drawText (fx.name, cRect.withTop (cRect.getBottom() - 24.0f), juce::Justification::centred, false);

            g.setColour (isSelected ? juce::Colour (0xff38bdf8) : juce::Colour (0xff27272a));
            g.drawRoundedRectangle (cRect, 4.0f, isSelected ? 2.2f : 1.0f);
        }

        const int maxP = getMaxPages();
        const float dotY = row1Y + cardH + 17.0f;
        const float totalDotsW = maxP * 16.0f;
        float startDotX = box.getCentreX() - totalDotsW * 0.5f;

        for (int d = 1; d <= maxP; ++d)
        {
            g.setColour (d == currentPage ? juce::Colours::white : juce::Colour (0xff52525b));
            g.fillEllipse (startDotX + (d - 1) * 16.0f, dotY, 8.0f, 8.0f);
        }
    }

    void resized() override
    {
        if (getWidth() <= 100 || getHeight() <= 100)
            return;
        auto box = getLocalBounds().reduced (16);
        closeBtn.setBounds (box.getRight() - 86, box.getY() + 8, 74, 24);

        const int midY = box.getCentreY() + 4;
        prevBtn.setBounds (box.getCentreX() - 120, midY - 12, 32, 24);
        nextBtn.setBounds (box.getCentreX() + 88,  midY - 12, 32, 24);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        for (size_t i = 0; i < cardRects.size(); ++i)
        {
            if (cardRects[i].contains (e.position))
            {
                if (onEffectChosen)
                    onEffectChosen (pageItems[i]);
                setVisible (false);
                return;
            }
        }
    }

private:
    int currentSlot = 1;
    int currentPage = 1;
    bool isFx58Mode = false;
    juce::String activeModelId;
    std::vector<X32Fx::EffectModelInfo> pageItems;
    std::vector<juce::Rectangle<float>> cardRects;
    juce::TextButton prevBtn, nextBtn, closeBtn;
};

class X32HardwareUnitCanvas : public juce::Component,
                              private juce::Slider::Listener,
                              private juce::Timer
{
public:
    std::function<void(int paramIndex, float normalizedValue)> onParameterChanged;
    std::function<void(int modelIndex)> onSubProgramSelected;
    std::function<void(int pageIndex)> onPageChanged;

    X32HardwareUnitCanvas()
    {
        setLookAndFeel (&lnf);
        for (int i = 0; i < 31; ++i)
        {
            auto s = std::make_unique<juce::Slider>();
            s->addListener (this);
            s->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            addChildComponent (s.get());
            controls.push_back (std::move (s));

            auto l = std::make_unique<juce::Label>();
            l->setJustificationType (juce::Justification::centred);
            l->setFont (juce::FontOptions (10.0f, juce::Font::bold));
            l->setInterceptsMouseClicks (false, false);
            addChildComponent (l.get());
            labels.push_back (std::move (l));
        }

        // Sub-buttons for LARC presets (Hall, Ambience, Plate, Room, Chamber, Concert)
        for (int i = 0; i < 6; ++i)
        {
            auto b = std::make_unique<juce::TextButton>();
            b->onClick = [this, i] { handleSubButton (i); };
            addChildComponent (b.get());
            subButtons.push_back (std::move (b));
        }

        // LARC bottom-left page buttons: 1, 2, 3
        for (int i = 0; i < 3; ++i)
        {
            auto pb = std::make_unique<juce::TextButton> (juce::String (i + 1));
            pb->onClick = [this, i] {
                setLarcPage (i);
            };
            addChildComponent (pb.get());
            pageButtons.push_back (std::move (pb));
        }

        loadEffectModel (X32Fx::getCatalog()[0]);
    }

    ~X32HardwareUnitCanvas() override
    {
        stopTimer();
        setLookAndFeel (nullptr);
    }

    void loadEffectModel (const X32Fx::EffectModelInfo& info)
    {
        currentModel = info;
        larcPage = 0;

        for (auto& c : controls) c->setVisible (false);
        for (auto& l : labels)   l->setVisible (false);
        for (auto& b : subButtons) b->setVisible (false);
        for (auto& pb : pageButtons) pb->setVisible (false);

        configureControlsForModel();
        resized();
        repaint();
    }

    const X32Fx::EffectModelInfo& getCurrentModel() const { return currentModel; }

    void setLarcPage (int page, juce::NotificationType notify = juce::sendNotification)
    {
        larcPage = std::max (0, std::min (2, page));
        for (size_t i = 0; i < pageButtons.size(); ++i)
            pageButtons[i]->setToggleState ((int) i == larcPage, juce::dontSendNotification);

        if (notify != juce::dontSendNotification && onPageChanged)
            onPageChanged (larcPage);
        configureControlsForModel();
        repaint();
    }

    int getLarcPage() const noexcept { return larcPage; }

    juce::Slider* getControl (int index)
    {
        if (index >= 0 && index < (int) controls.size())
            return controls[(size_t) index].get();
        return nullptr;
    }

    void updateControlStatus (int controlIdx)
    {
        if (controlIdx < 0 || controlIdx >= (int) controls.size())
            return;

        auto* s = controls[(size_t) controlIdx].get();
        if (! s) return;

        using GF = X32Fx::GuiFamily;
        if (currentModel.family == GF::LarcReverb)
        {
            const int pIdx = larcPage * 6 + controlIdx;
            const float normVal = (float) (s->getValue() / 100.0);
            const juce::String valStr = X32Fx::formatParamValue (currentModel, pIdx, normVal);
            s->getProperties().set ("liveValueText", valStr);
            return;
        }

        if (currentModel.family == GF::GraphicEq)
        {
            const float db = (float) s->getValue();
            const juce::String valStr = (db > 0.0f ? "+" : "") + juce::String (db, 1) + " dB";
            s->getProperties().set ("liveValueText", valStr);
            return;
        }

        const float normVal = (float) (s->getValue() / 100.0);
        const juce::String valStr = X32Fx::formatParamValue (currentModel, controlIdx, normVal);
        s->getProperties().set ("liveValueText", valStr);

        if (controlIdx < (int) labels.size() && controlIdx < (int) currentModel.paramNames.size())
        {
            const auto& name = currentModel.paramNames[(size_t) controlIdx];
            labels[(size_t) controlIdx]->setText (name + "\n" + valStr, juce::dontSendNotification);
        }
    }

    void updateAllControlStatuses()
    {
        for (size_t i = 0; i < controls.size(); ++i)
        {
            if (controls[i]->isVisible())
                updateControlStatus ((int) i);
        }
    }

    void setMeterPeaks (float l, float r)
    {
        meterL = l;
        meterR = r;
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        using GF = X32Fx::GuiFamily;

        switch (currentModel.family)
        {
            case GF::LarcReverb:       paintLarcFaceplate (g, b); break;
            case GF::GraphicEq:        paintGeqFaceplate (g, b); break;
            case GF::DeEsser:          paintDeEsserFaceplate (g, b); break;
            case GF::XtecEq1:
            case GF::XtecEq5:          paintXtecFaceplate (g, b); break;
            case GF::WaveDesigner:     paintWaveDesignerFaceplate (g, b); break;
            case GF::PrecisionLimiter: paintPrecisionLimiterFaceplate (g, b); break;
            case GF::FairComp:         paintFairCompFaceplate (g, b); break;
            case GF::LeisureComp:      paintLeisureCompFaceplate (g, b); break;
            case GF::UltimoComp:       paintUltimoCompFaceplate (g, b); break;
            case GF::Enhancer:         paintEnhancerFaceplate (g, b); break;
            case GF::Exciter:          paintExciterFaceplate (g, b); break;
            case GF::StereoImager:
            case GF::EdisonEx1:        paintSilverBezelDarkBox (g, b); break;
            case GF::SoundMaxer:       paintSoundMaxerFaceplate (g, b); break;
            case GF::GuitarAmp:        paintGuitarAmpFaceplate (g, b); break;
            case GF::TubeStage:        paintTubeStageFaceplate (g, b); break;
            case GF::ColoredMod:
            case GF::SubOctaver:       paintColoredModFaceplate (g, b); break;
            case GF::MoodFilter:       paintMoodFilterFaceplate (g, b); break;
            case GF::Combinator:       paintCombinatorFaceplate (g, b); break;
            case GF::Emt250:           paintEmt250Faceplate (g, b); break;
            case GF::QuantecRoom:      paintQuantecFaceplate (g, b); break;
            default:                   paintPcmPlateFaceplate (g, b); break;
        }
    }

    void resized() override
    {
        if (getWidth() <= 100 || getHeight() <= 80)
            return;
        auto b = getLocalBounds().reduced (10, 6);
        using GF = X32Fx::GuiFamily;

        if (currentModel.family == GF::LarcReverb)
        {
            const int vfdH = juce::jlimit (40, 50, juce::roundToInt ((float) b.getHeight() * 0.28f));
            vfdBounds = juce::Rectangle<int> (b.getX(), b.getY(), b.getWidth(), vfdH);
            const int btnH = juce::jlimit (22, 28, juce::roundToInt ((float) b.getHeight() * 0.16f));
            auto btnRow = b.removeFromBottom (btnH);

            // Place page buttons 1, 2, 3 in the gap between the 5th and 6th columns so they never cover LEVEL's dB readout
            const int colW = b.getWidth() / 6;
            const int pageGroupX = vfdBounds.getX() + 5 * colW - 34;
            for (int i = 0; i < 3; ++i)
                pageButtons[(size_t) i]->setBounds (pageGroupX + i * 22, vfdBounds.getBottom() - 16, 20, 14);

            auto faderArea = b.withTop (vfdBounds.getBottom() + 4).withBottom (btnRow.getY() - 4);

            for (int i = 0; i < 6; ++i)
            {
                controls[(size_t) i]->setBounds (b.getX() + i * colW + (colW - 54) / 2, faderArea.getY(), 54, faderArea.getHeight());
                subButtons[(size_t) i]->setBounds (b.getX() + i * colW + 3, btnRow.getY(), colW - 6, btnH);
            }
            return;
        }

        if (currentModel.family == GF::GraphicEq)
        {
            auto faderZone = b.withTrimmedTop (26);
            const int numBands = 31;
            const float colW = (float) faderZone.getWidth() / (float) numBands;
            for (int i = 0; i < numBands; ++i)
            {
                int cx = juce::roundToInt (faderZone.getX() + i * colW);
                controls[(size_t) i]->setBounds (cx, faderZone.getY() + 12, juce::jmax (14, juce::roundToInt (colW)), faderZone.getHeight() - 14);
                labels[(size_t) i]->setBounds (cx - 4, faderZone.getY(), juce::roundToInt (colW) + 8, 12);
            }
            return;
        }

        if (currentModel.family == GF::PrecisionLimiter)
        {
            auto rightArea = b.withTrimmedLeft (145);
            const int colW = rightArea.getWidth() / 6;
            for (int i = 0; i < 6; ++i)
            {
                auto col = juce::Rectangle<int> (rightArea.getX() + i * colW, rightArea.getY() + 4, colW, rightArea.getHeight() - 8);
                controls[(size_t) i]->setBounds (col.withTrimmedBottom (20).reduced (colW / 5, 0));
                labels[(size_t) i]->setBounds (col.removeFromBottom (20));
            }
            return;
        }

        int activeCount = (int) currentModel.paramNames.size();
        if (activeCount == 0) activeCount = 6;
        activeCount = juce::jmin (activeCount, 12);

        const bool hasVuRight = (currentModel.family == GF::FairComp ||
                                 currentModel.family == GF::LeisureComp ||
                                 currentModel.family == GF::UltimoComp);
        const bool hasLeftBanner = (currentModel.family == GF::ColoredMod ||
                                    currentModel.family == GF::SubOctaver);
        const bool hasRightMood = (currentModel.family == GF::MoodFilter);

        const int rows = (activeCount > 8) ? 2 : 1;
        const int cols = (rows == 2) ? ((activeCount + 1) / 2) : activeCount;

        const int topHeaderTrim = (rows == 2) ? 22 : 26;
        auto workArea = b.withTrimmedTop (topHeaderTrim).reduced (12, 0);
        if (hasVuRight)
            workArea.removeFromRight (juce::jmin (142, workArea.getWidth() / 5));
        if (hasLeftBanner)
            workArea.removeFromLeft (118);
        if (hasRightMood)
            workArea.removeFromRight (88);

        const int cellW = workArea.getWidth() / juce::jmax (1, cols);
        const int cellH = workArea.getHeight() / rows;
        const int labelH = (rows == 2) ? 21 : 25;

        for (int i = 0; i < activeCount; ++i)
        {
            int r = i / cols;
            int c = i % cols;
            juce::Rectangle<int> cell (workArea.getX() + c * cellW, workArea.getY() + r * cellH, cellW, cellH);
            int maxKnob = (rows == 2) ? 48 : 64;
            int minKnob = (rows == 2) ? 32 : 42;
            int knobSize = juce::jlimit (minKnob, maxKnob, juce::jmin (cell.getWidth() - 8, cell.getHeight() - labelH));
            const int blockH = knobSize + labelH;
            const int startKnobY = cell.getY() + std::max (0, (cell.getHeight() - blockH) / 2);
            controls[(size_t) i]->setBounds (cell.getCentreX() - knobSize / 2, startKnobY, knobSize, knobSize);
            labels[(size_t) i]->setFont (juce::FontOptions ((rows == 2) ? 9.2f : 10.0f, juce::Font::bold));
            labels[(size_t) i]->setBounds (cell.getX() - 2, startKnobY + knobSize, cell.getWidth() + 4, labelH);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (currentModel.family == X32Fx::GuiFamily::LarcReverb && vfdBounds.contains (e.getPosition()))
        {
            setLarcPage ((larcPage + 1) % 3);
        }
    }

private:
    X32FxLookAndFeel lnf;
    X32Fx::EffectModelInfo currentModel;
    std::vector<std::unique_ptr<juce::Slider>> controls;
    std::vector<std::unique_ptr<juce::Label>> labels;
    std::vector<std::unique_ptr<juce::TextButton>> subButtons;
    std::vector<std::unique_ptr<juce::TextButton>> pageButtons;
    juce::Rectangle<int> vfdBounds;
    int larcPage = 0;
    float meterPhase = 0.0f;
    float meterL = 0.0f, meterR = 0.0f;

    void visibilityChanged() override
    {
        if (isShowing())
            startTimerHz (15);
        else
            stopTimer();
    }

    void timerCallback() override
    {
        if (! isShowing())
        {
            stopTimer();
            return;
        }

        meterPhase += 0.18f;
        if (currentModel.family == X32Fx::GuiFamily::FairComp ||
            currentModel.family == X32Fx::GuiFamily::LeisureComp ||
            currentModel.family == X32Fx::GuiFamily::UltimoComp ||
            currentModel.family == X32Fx::GuiFamily::Combinator)
        {
            repaint();
        }
    }

    void sliderValueChanged (juce::Slider* s) override
    {
        for (size_t i = 0; i < controls.size(); ++i)
        {
            if (controls[i].get() == s)
            {
                updateControlStatus ((int) i);

                if (onParameterChanged)
                {
                    const int paramIdx = (currentModel.family == X32Fx::GuiFamily::LarcReverb)
                                             ? (larcPage * 6 + (int) i)
                                             : (int) i;
                    onParameterChanged (paramIdx, (float) (s->getValue() / 100.0));
                }
                repaint();
                break;
            }
        }
    }

    void handleSubButton (int idx)
    {
        for (int i = 0; i < 6; ++i)
            subButtons[(size_t) i]->setToggleState (i == idx, juce::dontSendNotification);

        if (onSubProgramSelected)
            onSubProgramSelected (idx);
        repaint();
    }

    void configureControlsForModel()
    {
        using GF = X32Fx::GuiFamily;

        if (currentModel.family == GF::LarcReverb)
        {
            const juce::StringArray presets = { "HALL", "AMBIENCE", "PLATE", "ROOM", "CHAMBER", "CONCERT" };
            for (int i = 0; i < 6; ++i)
            {
                const int pIdx = larcPage * 6 + i;
                const bool hasParam = (pIdx < (int) currentModel.paramNames.size());

                controls[(size_t) i]->setSliderStyle (juce::Slider::LinearVertical);
                controls[(size_t) i]->getProperties().set ("larcStyle", true);
                controls[(size_t) i]->setRange (0.0, 100.0, 1.0);
                controls[(size_t) i]->setVisible (hasParam);
                if (hasParam)
                    updateControlStatus (i);

                subButtons[(size_t) i]->setButtonText (presets[i]);
                subButtons[(size_t) i]->getProperties().set ("larcPresetBtn", true);
                subButtons[(size_t) i]->setClickingTogglesState (true);
                subButtons[(size_t) i]->setToggleState (i == currentModel.index14, juce::dontSendNotification);
                subButtons[(size_t) i]->setVisible (true);
            }

            for (size_t i = 0; i < pageButtons.size(); ++i)
            {
                pageButtons[i]->getProperties().set ("larcPresetBtn", true);
                pageButtons[i]->setClickingTogglesState (true);
                pageButtons[i]->setToggleState ((int) i == larcPage, juce::dontSendNotification);
                pageButtons[i]->setVisible (true);
            }
            return;
        }

        if (currentModel.family == GF::GraphicEq)
        {
            static const char* freqs[31] = {
                "20","25","31.5","40","50","63","80","100","125","160","200","250","315","400","500","630","800",
                "1k","1k25","1k6","2k","2k5","3k15","4k","5k","6k3","8k","10k","12k5","16k","20k"
            };
            for (int i = 0; i < 31; ++i)
            {
                controls[(size_t) i]->setSliderStyle (juce::Slider::LinearVertical);
                controls[(size_t) i]->getProperties().set ("larcStyle", false);
                controls[(size_t) i]->getProperties().set ("silverCap", true);
                controls[(size_t) i]->setRange (-15.0, 15.0, 0.5);
                controls[(size_t) i]->setVisible (true);
                updateControlStatus (i);

                labels[(size_t) i]->setText (freqs[i], juce::dontSendNotification);
                labels[(size_t) i]->setColour (juce::Label::textColourId, juce::Colour (0xffa1a1aa));
                labels[(size_t) i]->setVisible (true);
            }
            return;
        }

        if (currentModel.family == GF::PrecisionLimiter)
        {
            for (int i = 0; i < 6; ++i)
            {
                controls[(size_t) i]->setSliderStyle (juce::Slider::LinearVertical);
                controls[(size_t) i]->getProperties().set ("larcStyle", false);
                controls[(size_t) i]->getProperties().set ("silverCap", true);
                controls[(size_t) i]->setRange (0.0, 100.0, 1.0);
                controls[(size_t) i]->setVisible (true);

                labels[(size_t) i]->setColour (juce::Label::textColourId, juce::Colour (0xff18181b));
                labels[(size_t) i]->setVisible (true);
                updateControlStatus (i);
            }
            return;
        }

        int count = juce::jmin ((int) currentModel.paramNames.size(), 12);
        const bool isSilverKnob = (currentModel.family == GF::UltimoComp ||
                                   currentModel.family == GF::Exciter ||
                                   currentModel.family == GF::DeEsser ||
                                   currentModel.family == GF::MoodFilter ||
                                   currentModel.family == GF::SoundMaxer);

        const bool isDarkText = (currentModel.family == GF::LeisureComp ||
                                 currentModel.family == GF::Enhancer ||
                                 currentModel.family == GF::Exciter ||
                                 currentModel.family == GF::TubeStage);

        for (int i = 0; i < count; ++i)
        {
            controls[(size_t) i]->setSliderStyle (juce::Slider::RotaryVerticalDrag);
            controls[(size_t) i]->getProperties().set ("silverKnob", isSilverKnob);
            controls[(size_t) i]->getProperties().set ("amberRing", currentModel.family == GF::DeEsser);
            controls[(size_t) i]->setRange (0.0, 100.0, 1.0);
            controls[(size_t) i]->setVisible (true);

            labels[(size_t) i]->setColour (juce::Label::textColourId, isDarkText ? juce::Colour (0xff18181b) : juce::Colour (0xffe4e4e7));
            labels[(size_t) i]->setVisible (true);
            updateControlStatus (i);
        }
    }

    void paintVuMeter (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& title)
    {
        juce::ColourGradient vuGrad (juce::Colour (0xfff59e0b), area.getX(), area.getY(),
                                    juce::Colour (0xfffef08a), area.getX(), area.getBottom(), false);
        g.setGradientFill (vuGrad);
        g.fillRoundedRectangle (area, 3.0f);

        g.setColour (juce::Colour (0xff18181b));
        g.drawRoundedRectangle (area, 3.0f, 2.5f);

        g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        g.drawText (title, area.withHeight (16.0f), juce::Justification::centred, false);

        const float needleTarget = -0.35f + 0.55f * std::max (meterL, meterR);
        const float angle = needleTarget + 0.05f * std::sin (meterPhase);
        const float px = area.getCentreX();
        const float py = area.getBottom() - 4.0f;
        const float len = area.getHeight() * 0.72f;
        g.setColour (juce::Colour (0xffb91c1c));
        g.drawLine (px, py, px + len * std::sin (angle), py - len * std::cos (angle), 1.8f);
    }

    void paintLarcFaceplate (juce::Graphics& g, juce::Rectangle<float> b)
    {
        juce::ColourGradient bg (juce::Colour (0xffdcd5c1), 0, 0, juce::Colour (0xffada084), 0, b.getHeight(), false);
        g.setGradientFill (bg);
        g.fillRect (b);

        // Recessed VFD Matrix Screen
        auto vfd = vfdBounds.toFloat();
        g.setColour (juce::Colour (0xff070707));
        g.fillRoundedRectangle (vfd, 5.0f);

        // Subtle VFD scanlines
        g.setColour (juce::Colours::white.withAlpha (0.04f));
        for (float sy = vfd.getY() + 2.0f; sy < vfd.getBottom() - 2.0f; sy += 2.0f)
            g.drawHorizontalLine (juce::roundToInt (sy), vfd.getX() + 4.0f, vfd.getRight() - 4.0f);

        g.setColour (juce::Colour (0xff27241d));
        g.drawRoundedRectangle (vfd, 5.0f, 2.5f);

        const float slotW = vfd.getWidth() / 6.0f;
        g.setColour (currentModel.accentColour);
        g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 15.5f, juce::Font::bold));

        for (int i = 0; i < 6; ++i)
        {
            int paramIdx = larcPage * 6 + i;
            if (paramIdx < (int) currentModel.paramNames.size())
            {
                auto r = juce::Rectangle<float> (vfd.getX() + i * slotW, vfd.getY() + 5, slotW, 22);
                g.drawText (currentModel.paramNames[(size_t) paramIdx], r, juce::Justification::centred, false);

                auto vr = juce::Rectangle<float> (vfd.getX() + i * slotW, vfd.getY() + 26, slotW, 16);
                g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 10.5f, juce::Font::plain));
                const float normVal = (float) (controls[(size_t) i]->getValue() / 100.0);
                const juce::String valFormatted = X32Fx::formatParamValue (currentModel, paramIdx, normVal);
                g.drawText (valFormatted, vr, juce::Justification::centred, false);
                g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 15.5f, juce::Font::bold));
            }
        }

        // 13-tick horizontal scale lines between the 6 fader columns
        g.setColour (juce::Colour (0xff191815));
        const float faderTop = vfd.getBottom() + 14.0f;
        const float faderH = b.getBottom() - 42.0f - faderTop;
        for (int i = 0; i < 5; ++i)
        {
            float colX = b.getX() + (float) i * slotW;
            float tx1 = colX + slotW * 0.5f + 25.0f;
            float tx2 = colX + slotW + (slotW * 0.5f) - 25.0f;
            for (int t = 0; t < 13; ++t)
            {
                float ty = faderTop + (float) t * (faderH / 12.0f);
                g.drawHorizontalLine (juce::roundToInt (ty), tx1, tx2);
            }
        }

        // Bottom metallic shadow strip under the 6 preset buttons
        juce::Rectangle<float> botStrip (b.getX(), b.getBottom() - 5.0f, b.getWidth(), 5.0f);
        juce::ColourGradient botGrad (juce::Colour (0xffd6d3d1), botStrip.getX(), botStrip.getY(),
                                     juce::Colour (0xff57534e), botStrip.getX(), botStrip.getBottom(), false);
        g.setGradientFill (botGrad);
        g.fillRect (botStrip);
    }

    void paintGeqFaceplate (juce::Graphics& g, juce::Rectangle<float> b)
    {
        g.fillAll (juce::Colour (0xff232326));
        g.setColour (juce::Colour (0xff161618));
        g.fillRoundedRectangle (b.reduced (10, 36), 4.0f);
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (14.0f, juce::Font::bold | juce::Font::italic));
        g.drawText (currentModel.name.toUpperCase(), b.removeFromTop (32).reduced (16, 0), juce::Justification::centredLeft, false);
    }

    void paintDeEsserFaceplate (juce::Graphics& g, juce::Rectangle<float> b)
    {
        juce::ColourGradient bg (juce::Colour (0xffe4e4e7), 0, 0, juce::Colour (0xffa1a1aa), 0, b.getHeight(), false);
        g.setGradientFill (bg);
        g.fillRect (b);

        auto inner = b.reduced (18, 22);
        g.setColour (juce::Colour (0xff18181b));
        g.fillRoundedRectangle (inner, 36.0f);

        auto centerBox = inner.withSizeKeepingCentre (64, inner.getHeight() + 6);
        g.setColour (juce::Colour (0xffd4d4d8));
        g.fillRect (centerBox);
        g.setColour (juce::Colour (0xff18181b));
        g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
        g.drawText (currentModel.subLabel, centerBox, juce::Justification::centred, false);
    }

    void paintXtecFaceplate (juce::Graphics& g, juce::Rectangle<float> b)
    {
        g.fillAll (juce::Colour (0xff244e5c));
        g.setColour (juce::Colour (0xff122b33));
        g.drawRect (b, 3.0f);
        g.setColour (juce::Colour (0xffccfbf1));
        g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        g.drawText (currentModel.name.toUpperCase() + "  •  PROGRAM EQUALIZER", b.removeFromTop (30).reduced (16, 0), juce::Justification::centredLeft, false);

        g.setColour (juce::Colour (0xffef4444));
        g.fillEllipse (24.0f, b.getBottom() - 36.0f, 12.0f, 12.0f);
    }

    void paintWaveDesignerFaceplate (juce::Graphics& g, juce::Rectangle<float> b)
    {
        g.fillAll (juce::Colour (0xff152266));
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        auto work = b.reduced (24, 34);
        g.drawRoundedRectangle (work.removeFromLeft (work.getWidth() * 0.48f), 4.0f, 2.0f);
        g.drawRoundedRectangle (work.removeFromRight (work.getWidth() * 0.92f), 4.0f, 2.0f);
        g.setFont (juce::FontOptions ("Times New Roman", 18.0f, juce::Font::bold | juce::Font::italic));
        g.drawText ("Wave Designer", b.removeFromTop (32).reduced (18, 0), juce::Justification::centredLeft, false);
    }

    void paintPrecisionLimiterFaceplate (juce::Graphics& g, juce::Rectangle<float> b)
    {
        juce::ColourGradient bg (juce::Colour (0xffd4d6da), 0, 0, juce::Colour (0xffa6a9b0), 0, b.getHeight(), false);
        g.setGradientFill (bg);
        g.fillRect (b);

        auto rightBlue = b.withTrimmedLeft (160 + (b.getWidth() - 160) * 2.0f / 6.0f).reduced (8, 12);
        g.setColour (juce::Colour (0xff9ab3cc));
        g.fillRoundedRectangle (rightBlue, 4.0f);
        g.setColour (juce::Colour (0xff52525b));
        g.drawRoundedRectangle (rightBlue, 4.0f, 1.5f);

        g.setColour (juce::Colour (0xff18181b));
        g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
        g.drawFittedText ("Stereo\nPrecision\nLimiter", juce::Rectangle<int> (20, 24, 120, 80), juce::Justification::topLeft, 3);
    }

    void paintFairCompFaceplate (juce::Graphics& g, juce::Rectangle<float> b)
    {
        g.fillAll (juce::Colour (0xff15262d));
        g.setColour (juce::Colour (0xffa5f3fc));
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText (currentModel.name.toUpperCase(), b.withHeight (26.0f).reduced (16, 0), juce::Justification::centredLeft, false);
        const float vuH = juce::jlimit (54.0f, 70.0f, b.getHeight() * 0.44f);
        paintVuMeter (g, juce::Rectangle<float> (b.getRight() - 140.0f, b.getCentreY() - vuH * 0.45f, 124.0f, vuH), "VU REDUCTION");
    }

    void paintLeisureCompFaceplate (juce::Graphics& g, juce::Rectangle<float> b)
    {
        g.fillAll (juce::Colour (0xff8c8f94));
        g.setColour (juce::Colour (0xff18181b));
        g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        g.drawText ("ELECTRONIC  •  " + currentModel.name.toUpperCase(), b.withHeight (26.0f).reduced (16, 0), juce::Justification::centredLeft, false);
        const float vuH = juce::jlimit (56.0f, 72.0f, b.getHeight() * 0.46f);
        paintVuMeter (g, juce::Rectangle<float> (b.getRight() - 142.0f, b.getCentreY() - vuH * 0.45f, 126.0f, vuH), "LEVELING AMPLIFIER");
    }

    void paintUltimoCompFaceplate (juce::Graphics& g, juce::Rectangle<float> b)
    {
        g.fillAll (juce::Colour (0xff151517));
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        g.drawText (currentModel.name.toUpperCase() + "  •  LIMITING AMPLIFIER", b.withHeight (26.0f).reduced (16, 0), juce::Justification::centredLeft, false);
        const float vuH = juce::jlimit (56.0f, 72.0f, b.getHeight() * 0.46f);
        paintVuMeter (g, juce::Rectangle<float> (b.getRight() - 142.0f, b.getCentreY() - vuH * 0.45f, 126.0f, vuH), "VU GR");
    }

    void paintEnhancerFaceplate (juce::Graphics& g, juce::Rectangle<float> b)
    {
        juce::ColourGradient bg (juce::Colour (0xfff5e19c), 0, 0, juce::Colour (0xffb5943b), 0, b.getHeight(), false);
        g.setGradientFill (bg);
        g.fillRect (b);
        g.setColour (juce::Colour (0xff18181b));
        g.setFont (juce::FontOptions ("Times New Roman", 18.0f, juce::Font::bold | juce::Font::italic));
        g.drawText (currentModel.name, b.removeFromTop (30).reduced (16, 0), juce::Justification::centredLeft, false);
    }

    void paintExciterFaceplate (juce::Graphics& g, juce::Rectangle<float> b)
    {
        juce::ColourGradient bg (juce::Colour (0xfff4f4f5), 0, 0, juce::Colour (0xffa1a1aa), 0, b.getHeight(), false);
        g.setGradientFill (bg);
        g.fillRect (b);
        g.setColour (juce::Colour (0xff18181b));
        g.fillRoundedRectangle (16.0f, 8.0f, 140.0f, 22.0f, 3.0f);
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (currentModel.name, 20, 8, 132, 22, juce::Justification::centred, false);
    }

    void paintSilverBezelDarkBox (juce::Graphics& g, juce::Rectangle<float> b)
    {
        g.fillAll (juce::Colour (0xffd4d4d8));
        auto inner = b.reduced (14, 14);
        g.setColour (juce::Colour (0xff232326));
        g.fillRoundedRectangle (inner, 4.0f);
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText (currentModel.subLabel, inner.removeFromTop (26).reduced (12, 0), juce::Justification::centredLeft, false);
    }

    void paintSoundMaxerFaceplate (juce::Graphics& g, juce::Rectangle<float> b)
    {
        g.fillAll (juce::Colour (0xffe4e4e7));
        auto inner = b.reduced (16, 16);
        g.setColour (juce::Colour (0xff991b1b));
        g.fillRoundedRectangle (inner, 10.0f);
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText ("SOUND MAXER", inner.removeFromTop (24).reduced (14, 0), juce::Justification::centredLeft, false);
    }

    void paintGuitarAmpFaceplate (juce::Graphics& g, juce::Rectangle<float> b)
    {
        g.fillAll (juce::Colour (0xff18181b));
        auto inner = b.reduced (24, 16);
        g.setColour (juce::Colour (0xff27272a));
        g.fillRoundedRectangle (inner, 4.0f);
        g.setColour (juce::Colour (0xff71717a));
        g.drawRoundedRectangle (inner, 4.0f, 2.0f);
        g.setColour (juce::Colour (0xfff59e0b));
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText (currentModel.name.toUpperCase(), inner.removeFromTop (24).reduced (12, 0), juce::Justification::centredLeft, false);
    }

    void paintTubeStageFaceplate (juce::Graphics& g, juce::Rectangle<float> b)
    {
        g.fillAll (juce::Colour (0xffe4e4e7));
        g.setColour (juce::Colour (0xffb91c1c));
        for (int r = 30; r <= 150; r += 30)
            g.drawEllipse (b.getRight() - 120.0f - (float) r, b.getCentreY() - (float) r, (float) (r * 2), (float) (r * 2), 10.0f);

        auto card = b.reduced (24, 14);
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.fillRoundedRectangle (card, 6.0f);
        g.setColour (juce::Colour (0xffb91c1c));
        g.setFont (juce::FontOptions ("Times New Roman", 18.0f, juce::Font::bold | juce::Font::italic));
        g.drawText (currentModel.name + " / Overdrive", card.removeFromTop (26).reduced (12, 0), juce::Justification::centredLeft, false);
    }

    void paintColoredModFaceplate (juce::Graphics& g, juce::Rectangle<float> b)
    {
        g.fillAll (currentModel.accentColour);
        auto inner = b.withTrimmedLeft (130).withTrimmedRight (30).reduced (0, 12);
        g.setColour (juce::Colour (0xff3f3f46));
        g.fillRoundedRectangle (inner, 6.0f);
        g.setColour (juce::Colour (0xff18181b));
        g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
        g.drawFittedText (currentModel.name, juce::Rectangle<int> (12, 20, 110, 60), juce::Justification::topLeft, 2);
    }

    void paintMoodFilterFaceplate (juce::Graphics& g, juce::Rectangle<float> b)
    {
        g.fillAll (juce::Colour (0xff1b1b1f));
        auto inner = b.withTrimmedRight (95).reduced (12, 10);
        g.setColour (juce::Colour (0xff38bdf8));
        g.drawRoundedRectangle (inner, 4.0f, 2.0f);
        g.fillRect (inner.removeFromTop (18));
        g.setColour (juce::Colour (0xff38bdf8));
        g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 22.0f, juce::Font::bold));
        g.drawText ("mood", (int) (b.getRight() - 90), 24, 80, 40, juce::Justification::centred, false);
    }

    void paintCombinatorFaceplate (juce::Graphics& g, juce::Rectangle<float> b)
    {
        g.fillAll (juce::Colour (0xff202024));
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText (currentModel.name.toUpperCase() + "  •  5-BAND MULTIBAND COMPRESSOR", b.removeFromTop (26).reduced (14, 0), juce::Justification::centredLeft, false);
    }

    void paintEmt250Faceplate (juce::Graphics& g, juce::Rectangle<float> b)
    {
        g.fillAll (juce::Colour (0xff242326));
        g.setColour (juce::Colour (0xffef4444));
        g.drawRect (b.removeFromRight (36).reduced (6), 3.0f);
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText ("VINTAGE REVERB / EMT 250 HALLGERÄT", b.removeFromTop (26).reduced (14, 0), juce::Justification::centredLeft, false);
    }

    void paintQuantecFaceplate (juce::Graphics& g, juce::Rectangle<float> b)
    {
        g.fillAll (juce::Colour (0xff161f2c));
        g.setColour (juce::Colour (0xffff3020));
        g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::bold));
        g.drawText ("VINTAGE ROOM REVERB  •  QRS", b.removeFromTop (28).reduced (14, 0), juce::Justification::centredLeft, false);
    }

    void paintPcmPlateFaceplate (juce::Graphics& g, juce::Rectangle<float> b)
    {
        g.fillAll (juce::Colour (0xff1f1e21));
        auto lcd = juce::Rectangle<float> (b.getCentreX() - 110.0f, 8.0f, 220.0f, 24.0f);
        g.setColour (currentModel.accentColour);
        g.fillRoundedRectangle (lcd, 2.0f);
        g.setColour (juce::Colours::black);
        g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::bold));
        g.drawText (currentModel.subLabel, lcd, juce::Justification::centred, false);
    }
};
