// Mix Practice Studio - Hardware Simulated FX Faceplate Component Implementation
#include "FxFaceplateComponent.h"

namespace mps
{

FxFaceplateComponent::FxFaceplateComponent (AudioEngine& eng)
    : engine (eng)
{
    // 1. Setup 12 dynamic console dials
    for (int i = 0; i < kMaxDials; ++i)
    {
        dials[(size_t) i].setTitle ("PARAM");
        dials[(size_t) i].onValueChanged = [this, i] (double val) {
            if (ignoreCallbacks) return;
            auto& p = engine.getMixerParams().fxSlots[(size_t) currentSlot];
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

            if (isMix)
            {
                p.mix.set ((float) (val * 0.01));
                return;
            }

            switch (i)
            {
                case 0: p.p1.set ((float) val); break;
                case 1: p.p2.set ((float) val); break;
                case 2: p.p3.set ((float) val); break;
                case 3: p.p4.set ((float) val); break;
                case 4: p.p5.set ((float) val); break;
                case 5: p.p6.set ((float) val); break;
                case 6: p.p7.set ((float) val); break;
                case 7: p.p8.set ((float) val); break;
            }
        };
        addChildComponent (dials[(size_t) i]);
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
        l.setColour (juce::Label::textColourId, juce::Colour (0xffe63946)); // Red title
        addChildComponent (l);

        auto& vl = faderValLabels[(size_t) i];
        vl.setFont (juce::FontOptions (9.5f));
        vl.setJustificationType (juce::Justification::centred);
        vl.setColour (juce::Label::textColourId, juce::Colour (0xff20242b));
        addChildComponent (vl);
    }

    // 3. Setup Reverb Preset Ribbon Buttons (Hall, Ambience, Plate, Room, Chamber, Concert)
    const char* ribbonNames[6] = { "HALL", "AMBIENCE", "PLATE", "ROOM", "CHAMBER", "CONCERT" };
    for (int i = 0; i < 6; ++i)
    {
        reverbRibbonButtons[(size_t) i].setButtonText (ribbonNames[i]);
        reverbRibbonButtons[(size_t) i].addListener (this);
        addChildComponent (reverbRibbonButtons[(size_t) i]);
    }

    // 4. Setup 1176 Ratio buttons
    const char* ratioLabels[5] = { "4:1", "8:1", "12:1", "20:1", "ALL" };
    for (int i = 0; i < 5; ++i)
    {
        ratioButtons[i].setButtonText (ratioLabels[i]);
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
    powerSwitch.addListener (this);
    addAndMakeVisible (powerSwitch);

    bypassSwitch.setClickingTogglesState (true);
    bypassSwitch.addListener (this);
    addAndMakeVisible (bypassSwitch);

    stereoLinkSwitch.setClickingTogglesState (true);
    stereoLinkSwitch.addListener (this);
    addChildComponent (stereoLinkSwitch);

    rotarySpeedSwitch.setClickingTogglesState (true);
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
        refreshFromSlot();
        resized();
        repaint();
    }
}

void FxFaceplateComponent::refreshFromSlot()
{
    ignoreCallbacks = true;
    auto& p = engine.getMixerParams().fxSlots[(size_t) currentSlot];
    const auto type = (FxType) (int) p.fxType.getInt();
    const auto meta = getFxMetadata (type);

    powerSwitch.setToggleState (! p.bypass.getBool(), juce::dontSendNotification);
    powerSwitch.setButtonText (p.bypass.getBool() ? "OFF" : "ON");
    bypassSwitch.setToggleState (p.bypass.getBool(), juce::dontSendNotification);

    // Hide all dynamic components by default
    for (auto& d : dials) d.setVisible (false);
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

    // 1. Cream Fader Reverb (Hall, Ambience, Vintage Room, Chamber)
    const bool isCreamReverb = (type == FxType::HallReverb || type == FxType::Ambience || type == FxType::VintageRoom);
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
        const double fVals[6]  = { p.p1.get(), p.p2.get(), 50.0, 5.0, p.p3.get(), p.p4.get() };

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
    else if (type == FxType::PlateReverb || type == FxType::GateReverb || type == FxType::ReverseReverb)
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
    else if (type == FxType::XtecEq1 || type == FxType::XtecEq5)
    {
        dials[0].setVisible (true); dials[0].setTitle ("GAIN");       dials[0].setRange (-12.0, 12.0, 0.5); dials[0].setValue (0.0, juce::dontSendNotification);
        dials[1].setVisible (true); dials[1].setTitle ("LO BOOST");   dials[1].setRange (0.0, 100.0, 1.0);  dials[1].setValue (p.p1.get(), juce::dontSendNotification);
        dials[2].setVisible (true); dials[2].setTitle ("LO ATTEN");   dials[2].setRange (0.0, 100.0, 1.0);  dials[2].setValue (p.p2.get(), juce::dontSendNotification);
        dials[3].setVisible (true); dials[3].setTitle ("BANDWIDTH");  dials[3].setRange (0.0, 100.0, 1.0);  dials[3].setValue (p.p3.get(), juce::dontSendNotification);
        dials[4].setVisible (true); dials[4].setTitle ("HI BOOST");   dials[4].setRange (0.0, 100.0, 1.0);  dials[4].setValue (p.p4.get(), juce::dontSendNotification);
        dials[5].setVisible (true); dials[5].setTitle ("HI ATTEN");   dials[5].setRange (0.0, 100.0, 1.0);  dials[5].setValue (p.p5.get(), juce::dontSendNotification);
    }
    // 13. Teletronix LA-2A
    else if (type == FxType::LeisureCompressor)
    {
        dials[0].setVisible (true); dials[0].setTitle ("GAIN");        dials[0].setRange (0.0, 100.0, 1.0); dials[0].setValue (p.p2.get(), juce::dontSendNotification);
        dials[1].setVisible (true); dials[1].setTitle ("PEAK REDUCT"); dials[1].setRange (0.0, 100.0, 1.0); dials[1].setValue (p.p1.get(), juce::dontSendNotification);
    }
    // 14. Fairchild 670
    else if (type == FxType::FairCompressor || type == FxType::MsFairCompressor)
    {
        dials[0].setVisible (true); dials[0].setTitle ("THRESHOLD");  dials[0].setRange (0.0, 100.0, 1.0); dials[0].setValue (p.p1.get(), juce::dontSendNotification);
        dials[1].setVisible (true); dials[1].setTitle ("INPUT GAIN"); dials[1].setRange (0.0, 100.0, 1.0); dials[1].setValue (p.p2.get(), juce::dontSendNotification);
        dials[2].setVisible (true); dials[2].setTitle ("TIME CONST"); dials[2].setRange (1.0, 6.0, 1.0);   dials[2].setValue (p.p3.get(), juce::dontSendNotification);
        dials[3].setVisible (true); dials[3].setTitle ("OUTPUT");     dials[3].setRange (0.0, 100.0, 1.0); dials[3].setValue (p.p4.get(), juce::dontSendNotification);
    }
    // 15. Lead 100 Guitar Amp
    else if (type == FxType::GuitarAmp)
    {
        dials[0].setVisible (true); dials[0].setTitle ("DRIVE");    dials[0].setRange (0.0, 100.0, 1.0); dials[0].setValue (p.p1.get(), juce::dontSendNotification);
        dials[1].setVisible (true); dials[1].setTitle ("BASS");     dials[1].setRange (0.0, 100.0, 1.0); dials[1].setValue (p.p2.get(), juce::dontSendNotification);
        dials[2].setVisible (true); dials[2].setTitle ("MIDDLE");   dials[2].setRange (0.0, 100.0, 1.0); dials[2].setValue (p.p3.get(), juce::dontSendNotification);
        dials[3].setVisible (true); dials[3].setTitle ("TREBLE");   dials[3].setRange (0.0, 100.0, 1.0); dials[3].setValue (p.p4.get(), juce::dontSendNotification);
        dials[4].setVisible (true); dials[4].setTitle ("PRESENCE"); dials[4].setRange (0.0, 100.0, 1.0); dials[4].setValue (p.p5.get(), juce::dontSendNotification);
        dials[5].setVisible (true); dials[5].setTitle ("MASTER");   dials[5].setRange (0.0, 100.0, 1.0); dials[5].setValue (p.p6.get(), juce::dontSendNotification);
    }
    // 16. Tube Stage (12AX7 Valve Warmth)
    else if (type == FxType::TubeStage)
    {
        dials[0].setVisible (true); dials[0].setTitle ("DRIVE");      dials[0].setRange (0.0, 100.0, 1.0); dials[0].setValue (p.p1.get(), juce::dontSendNotification);
        dials[1].setVisible (true); dials[1].setTitle ("BIAS");       dials[1].setRange (0.0, 100.0, 1.0); dials[1].setValue (p.p3.get(), juce::dontSendNotification);
        dials[2].setVisible (true); dials[2].setTitle ("SATURATION"); dials[2].setRange (0.0, 100.0, 1.0); dials[2].setValue (p.p4.get(), juce::dontSendNotification);
        dials[3].setVisible (true); dials[3].setTitle ("OUTPUT");     dials[3].setRange (-20.0, 20.0, 0.5);dials[3].setValue (p.p2.get(), juce::dontSendNotification);
    }
    // 17. Default Studio Rack (Other Reverbs, Delays, De-Esser, Sub Octaver, etc.)
    else
    {
        for (int i = 0; i < 6; ++i)
        {
            dials[(size_t) i].setVisible (true);
            dials[(size_t) i].setTitle ("PARAM " + juce::String (i + 1));
            dials[(size_t) i].setRange (0.0, 100.0, 1.0);
            dials[(size_t) i].setValue (50.0, juce::dontSendNotification);
        }
        dials[5].setTitle ("MIX (%)");
        dials[5].setValue (p.mix.get() * 100.0, juce::dontSendNotification);
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

    if (b == &powerSwitch || b == &bypassSwitch)
    {
        p.bypass.toggle();
        refreshFromSlot();
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
            return;
        }
    }

    // Dimension D / C Buttons
    for (int i = 0; i < 5; ++i)
    {
        if (b == &dimButtons[i])
        {
            p.p1.set ((float) i); // 0=OFF, 1=1, 2=2, 3=3, 4=4
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
                    case 4: p.p3.set (val); break; // Release
                    case 5: p.p4.set (val); break; // Knee
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
                    case 5: p.mix.set (clampf ((val + 40.0f) / 50.0f, 0.0f, 1.0f)); break; // Level/Mix
                }
            }
            // Update value label
            if (i == 0) faderValLabels[(size_t) i].setText (juce::String (val, 0) + " ms", juce::dontSendNotification);
            else if (i == 1) faderValLabels[(size_t) i].setText (juce::String (val, 1) + " s", juce::dontSendNotification);
            else faderValLabels[(size_t) i].setText (juce::String (val, 0) + " %", juce::dontSendNotification);
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
    else if (type == FxType::HallReverb || type == FxType::Ambience || type == FxType::VintageRoom)
        layoutCreamReverb (area);
    else if (type == FxType::PlateReverb || type == FxType::GateReverb || type == FxType::ReverseReverb)
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
    else if (type == FxType::XtecEq1 || type == FxType::XtecEq5)
        layoutXtecEq (area);
    else if (type == FxType::LeisureCompressor)
        layoutLA2A (area);
    else if (type == FxType::UltimoCompressor)
        layout1176 (area);
    else if (type == FxType::FairCompressor || type == FxType::MsFairCompressor)
        layoutFairchild (area);
    else if (type == FxType::GuitarAmp)
        layoutGuitarAmp (area);
    else if (type == FxType::TubeStage)
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
    const int startX = area.getX() + 30;
    for (int i = 0; i < 6; ++i)
        dials[(size_t) i].setBounds (startX + i * 125, area.getY() + 85, 105, 110);
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
    else if (type == FxType::HallReverb || type == FxType::Ambience || type == FxType::VintageRoom)
        paintCreamReverb (g, area, meta.name);
    else if (type == FxType::PlateReverb)
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
    else if (type == FxType::XtecEq1 || type == FxType::XtecEq5)
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
    else if (type == FxType::FairCompressor || type == FxType::MsFairCompressor)
        paintFairchild (g, area);
    else if (type == FxType::GuitarAmp)
        paintGuitarAmp (g, area);
    else if (type == FxType::TubeStage)
        paintTubeStage (g, area);
    else
        paintGenericStudioRack (g, area, meta.name, meta.hardwareEmulation);

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
    juce::ColourGradient rackGrad (juce::Colour (0xff222730), area.getX(), area.getY(),
                                   juce::Colour (0xff14171d), area.getX(), area.getBottom(), false);
    g.setGradientFill (rackGrad);
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colour (0xff384252));
    g.drawRoundedRectangle (area, 4.0f, 1.2f);

    const juce::Rectangle<float> lcdBox (area.getX() + 25.0f, area.getY() + 16.0f, 380.0f, 54.0f);
    drawDigitalLcdDisplay (g, lcdBox, title, subTitle, juce::Colour (0xff0e2838));

    const juce::Rectangle<float> meterBox (area.getRight() - 220.0f, area.getY() + 16.0f, 130.0f, 60.0f);
    drawAnalogVuMeter (g, meterBox, vuNeedleL, "OUT PEAK");
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
