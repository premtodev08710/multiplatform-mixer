// Mix Practice Studio - Channel Detail Screen Implementation
#include "DetailScreen.h"

namespace mps
{

DetailScreen::DetailScreen (AudioEngine& eng, ProjectModel& proj)
    : engine (eng), project (proj)
{
    // Top Bank Selector
    bankSelectorBar.setMeters (&engine.getMixerMeters());
    addAndMakeVisible (bankSelectorBar);
    bankSelectorBar.onBankSelected = [this] (int bank) {
        switch (bank)
        {
            case BankSelectorBar::BankCh1_8:   setSelectedStrip (ChannelStripComponent::StripType::InputChannel, 0); break;
            case BankSelectorBar::BankCh9_16:  setSelectedStrip (ChannelStripComponent::StripType::InputChannel, 8); break;
            case BankSelectorBar::BankCh17_24: setSelectedStrip (ChannelStripComponent::StripType::InputChannel, 16); break;
            case BankSelectorBar::BankCh25_32: setSelectedStrip (ChannelStripComponent::StripType::InputChannel, 24); break;
            case BankSelectorBar::BankAux1_8:  setSelectedStrip (ChannelStripComponent::StripType::Aux, 0); break;
            case BankSelectorBar::BankFx1_4:   setSelectedStrip (ChannelStripComponent::StripType::FxReturn, 0); break;
            case BankSelectorBar::BankBus1_8:  setSelectedStrip (ChannelStripComponent::StripType::Bus, 0); break;
            case BankSelectorBar::BankBus9_16: setSelectedStrip (ChannelStripComponent::StripType::Bus, 8); break;
            case BankSelectorBar::BankMtxMain: setSelectedStrip (ChannelStripComponent::StripType::Matrix, 0); break;
            case BankSelectorBar::BankDca1_8:  setSelectedStrip (ChannelStripComponent::StripType::Dca, 0); break;
            default: break;
        }
    };

    // Back to Mixer button (occupies logo area in Detail view)
    backToMixerButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff202630));
    backToMixerButton.setColour (juce::TextButton::textColourOffId, ConsoleLookAndFeel::accentCyan);
    backToMixerButton.addListener (this);
    addAndMakeVisible (backToMixerButton);

    // Left Preview Strip Setup
    setupLeftStrip();

    // Sub-Tab Navigation Buttons
    const char* tabNames[NumTabs] = { "CONFIG/PREAMP", "GATE", "DYN", "EQ", "SENDS", "NAMING", "PRESET" };
    for (int i = 0; i < NumTabs; ++i)
    {
        tabButtons[i].setButtonText (tabNames[i]);
        tabButtons[i].setClickingTogglesState (true);
        tabButtons[i].setRadioGroupId (2001);
        tabButtons[i].addListener (this);
        addAndMakeVisible (tabButtons[i]);
    }
    tabButtons[0].setToggleState (true, juce::dontSendNotification);

    // Setup All Tabs
    setupConfigTab();
    setupGateTab();
    setupDynTab();
    setupEqTab();
    setupSendsTab();
    setupNamingTab();

    updateTabVisibility();
    refreshFromModel();
    engine.getMixEngine().setRtaChannel (selectedChannel);
}

DetailScreen::~DetailScreen()
{
    stopTimer();
}

void DetailScreen::setupLeftStrip()
{
    scribbleName.setJustificationType (juce::Justification::centred);
    scribbleName.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    scribbleName.setColour (juce::Label::backgroundColourId, juce::Colour (0xffeed202));
    scribbleName.setColour (juce::Label::textColourId, juce::Colours::black);
    scribbleName.setEditable (true, false, false);
    scribbleName.setTooltip ("Click to rename channel");
    scribbleName.onTextChange = [this] {
        const auto newName = scribbleName.getText();
        if (currentStripType == ChannelStripComponent::StripType::InputChannel)
        {
            project.setChannelName (currentStripIndex, newName);
            nameEditor.setText (newName, false);
        }
    };
    addAndMakeVisible (scribbleName);

    addAndMakeVisible (panIndicator);
    panIndicator.onPanChanged = [this] (float p) {
        if (ignoreCallbacks) return;
        setPanForCurrentStrip (p);
    };

    channelNumLabel.setJustificationType (juce::Justification::centredLeft);
    channelNumLabel.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    channelNumLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (channelNumLabel);

    faderSlider.setSliderStyle (juce::Slider::LinearVertical);
    faderSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    faderSlider.setRange (0.0, 1.0, 0.001);
    faderSlider.addListener (this);
    addAndMakeVisible (faderSlider);

    faderDbLabel.setJustificationType (juce::Justification::centred);
    faderDbLabel.setFont (juce::FontOptions (10.0f));
    faderDbLabel.setColour (juce::Label::backgroundColourId, juce::Colour (0xff14171c));
    faderDbLabel.setColour (juce::Label::textColourId, ConsoleLookAndFeel::textPrimary);
    addAndMakeVisible (faderDbLabel);

    auto& cm = engine.getMixerMeters().channels[(size_t) currentStripIndex];
    channelMeter.setMeters (&cm.preL, &cm.preR, &cm.clip);
    addAndMakeVisible (channelMeter);

    prevChannelButton.addListener (this);
    addAndMakeVisible (prevChannelButton);

    nextChannelButton.addListener (this);
    addAndMakeVisible (nextChannelButton);
}

void DetailScreen::setupConfigTab()
{
    addChildComponent (signalFlow);

    // Source button
    sourceButton.addListener (this);
    addChildComponent (sourceButton);

    // Preamp dials & buttons
    linkButton.setClickingTogglesState (true);
    linkButton.setTooltip ("Stereo link adjacent channel");
    linkButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff00e676));
    linkButton.addListener (this);
    addChildComponent (linkButton);

    phaseButton.setClickingTogglesState (true);
    phaseButton.setTooltip ("Invert phase polarity");
    phaseButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffffa000));
    phaseButton.addListener (this);
    addChildComponent (phaseButton);

    gainDial.setTickLabels ({
        { -12.0, "-12" }, { -3.0, "-3" }, { 6.0, "6" }, { 15.0, "15" },
        { 24.0, "24" }, { 33.0, "33" }, { 42.0, "42" }, { 51.0, "51" }, { 60.0, "60" }
    });
    gainDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        if (currentStripType == ChannelStripComponent::StripType::InputChannel)
            engine.getMixerParams().channels[(size_t) currentStripIndex].trimDb.set ((float) val);
        else if (currentStripType == ChannelStripComponent::StripType::Aux)
            engine.getMixerParams().auxes[(size_t) currentStripIndex].trimDb.set ((float) val);
    };
    addChildComponent (gainDial);

    auto& cm = engine.getMixerMeters().channels[(size_t) currentStripIndex];
    preampMeter.setMeters (&cm.preL, &cm.preR, &cm.clip);
    addChildComponent (preampMeter);

    phantom48vButton.setClickingTogglesState (true);
    phantom48vButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffe53935));
    phantom48vButton.setTooltip ("Audio file simulation only - does not affect digital audio");
    phantom48vButton.onClick = [this] {
        if (ignoreCallbacks) return;
        const bool on = phantom48vButton.getToggleState();
        if (currentStripType == ChannelStripComponent::StripType::InputChannel)
            engine.getMixerParams().channels[(size_t) currentStripIndex].phantom48v.setBool (on);
        else if (currentStripType == ChannelStripComponent::StripType::Aux)
            engine.getMixerParams().auxes[(size_t) currentStripIndex].phantom48v.setBool (on);
    };
    addChildComponent (phantom48vButton);

    delayDial.setTickLabels ({
        { 0.3, "0.3" }, { 63.0, "63" }, { 126.0, "126" }, { 187.0, "187" },
        { 250.0, "250" }, { 313.0, "313" }, { 376.0, "376" }, { 437.0, "437" }, { 500.0, "500" }
    });
    delayDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];
        cp.dsp.delayMs.set ((float) val);
    };
    addChildComponent (delayDial);

    delayButton.setButtonText ("DELAY");
    delayButton.setClickingTogglesState (true);
    delayButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffffa000));
    delayButton.addListener (this);
    addChildComponent (delayButton);

    lowCutDial.setTickLabels ({
        { 20.0, "20" }, { 29.0, "29" }, { 43.0, "43" }, { 64.0, "64" },
        { 90.0, "90" }, { 132.0, "132" }, { 192.0, "192" }, { 280.0, "280" }, { 400.0, "400" }
    });
    lowCutDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];
        cp.dsp.hpfFreq.set ((float) val);
        eqGraph.repaint();
    };
    addChildComponent (lowCutDial);

    lowCutButton.setButtonText ("LOW CUT");
    lowCutButton.setClickingTogglesState (true);
    lowCutButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffffa000));
    lowCutButton.addListener (this);
    addChildComponent (lowCutButton);

    // Insert
    insertButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffffa000));
    insertButton.addListener (this);
    addChildComponent (insertButton);

    copyButton.setButtonText ("COPY CH");
    copyButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff282f3a));
    addChildComponent (copyButton);

    pasteButton.setButtonText ("PASTE CH");
    pasteButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff282f3a));
    addChildComponent (pasteButton);

    // DCA 1..8
    for (int i = 0; i < 8; ++i)
    {
        dcaButtons[i].setButtonText (juce::String (i + 1));
        dcaButtons[i].setClickingTogglesState (true);
        dcaButtons[i].setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff00b0ff));
        dcaButtons[i].addListener (this);
        addChildComponent (dcaButtons[i]);
    }

    // Mute Groups 1..6
    for (int i = 0; i < 6; ++i)
    {
        muteGroupButtons[i].setButtonText (juce::String (i + 1));
        muteGroupButtons[i].setClickingTogglesState (true);
        muteGroupButtons[i].setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffe53935));
        muteGroupButtons[i].addListener (this);
        addChildComponent (muteGroupButtons[i]);
    }

    // Automix
    automixOffButton.setButtonText ("OFF");
    automixOffButton.setClickingTogglesState (true);
    automixOffButton.setToggleState (true, juce::dontSendNotification);
    automixOffButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xffe53935));
    addChildComponent (automixOffButton);

    automixXButton.setButtonText ("GRP X");
    automixXButton.setClickingTogglesState (true);
    automixXButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff00b0ff));
    addChildComponent (automixXButton);

    automixYButton.setButtonText ("GRP Y");
    automixYButton.setClickingTogglesState (true);
    automixYButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffffa000));
    addChildComponent (automixYButton);

    weightingDial.setTickLabels ({ { -12.0, "-12" }, { 0.0, "0" }, { 12.0, "+12" } });
    addChildComponent (weightingDial);

    // Main LR section
    mainBusHeaderButton.setButtonText ("MAIN BUS");
    mainBusHeaderButton.setClickingTogglesState (false);
    mainBusHeaderButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff252c38));
    mainBusHeaderButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffb0bec5));
    mainBusHeaderButton.setTooltip ("Main Bus assignment: Click to toggle Main LR routing");
    mainBusHeaderButton.addListener (this);
    addChildComponent (mainBusHeaderButton);

    mainLrButton.setButtonText ("MAIN LR");
    mainLrButton.setClickingTogglesState (true);
    mainLrButton.setToggleState (true, juce::dontSendNotification);
    mainLrButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffffa000));
    mainLrButton.setTooltip ("Route channel/bus audio to Main LR");
    mainLrButton.addListener (this);
    addChildComponent (mainLrButton);

    monoCenterButton.setButtonText ("MONO / C");
    monoCenterButton.setClickingTogglesState (true);
    monoCenterButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff00b0ff));
    addChildComponent (monoCenterButton);

    mainFader.setSliderStyle (juce::Slider::LinearVertical);
    mainFader.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    mainFader.setRange (0.0, 1.0, 0.001);
    mainFader.setValue (faderDbToPos (0.0f));
    mainFader.addListener (this);
    addChildComponent (mainFader);

    mainMuteButton.setClickingTogglesState (true);
    mainMuteButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffe53935));
    mainMuteButton.addListener (this);
    addChildComponent (mainMuteButton);

    mainSoloButton.setClickingTogglesState (true);
    mainSoloButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffffea00));
    mainSoloButton.addListener (this);
    addChildComponent (mainSoloButton);
}

void DetailScreen::setupGateTab()
{
    addChildComponent (gateGraph);

    gateTypeButton.addListener (this);
    addChildComponent (gateTypeButton);

    gateActiveButton.setClickingTogglesState (true);
    gateActiveButton.addListener (this);
    addChildComponent (gateActiveButton);

    addChildComponent (gateGrMeter);

    // Box 1
    gateThreshDial.setTickLabels ({
        { -80.0, "-80" }, { -70.0, "-70" }, { -60.0, "-60" }, { -50.0, "-50" },
        { -40.0, "-40" }, { -30.0, "-30" }, { -20.0, "-20" }, { -10.0, "-10" }, { 0.0, "0" }
    });
    gateThreshDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];
        cp.dsp.gateThreshDb.set ((float) val);
        gateGraph.setGateParams ((float) val, (float) gateRangeDial.getValue());
    };
    addChildComponent (gateThreshDial);

    gateRangeDial.setTickLabels ({
        { 3.0, "3" }, { 12.0, "12" }, { 23.0, "23" }, { 32.0, "32" },
        { 41.0, "41" }, { 51.0, "51" }, { 60.0, "60" }
    });
    gateRangeDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];
        cp.dsp.gateRangeDb.set ((float) -val);
        gateGraph.setGateParams ((float) gateThreshDial.getValue(), (float) val);
    };
    addChildComponent (gateRangeDial);

    // Box 2
    gateAttackDial.setTickLabels ({ { 0.0, "0" }, { 30.0, "30" }, { 60.0, "60" }, { 90.0, "90" }, { 120.0, "120" } });
    gateAttackDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];
        cp.dsp.gateAttackMs.set ((float) val);
    };
    addChildComponent (gateAttackDial);

    gateHoldDial.setTickLabels ({
        { 0.0, "0" }, { 0.1, "0.1" }, { 0.4, "0.4" }, { 1.4, "1.4" },
        { 6.3, "6.3" }, { 25.2, "25.2" }, { 112.0, "112" }, { 447.0, "447" }, { 2000.0, "2k" }
    });
    gateHoldDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];
        cp.dsp.gateHoldMs.set ((float) val);
    };
    addChildComponent (gateHoldDial);

    gateReleaseDial.setTickLabels ({
        { 5.0, "5" }, { 6.0, "6" }, { 28.0, "28" }, { 59.0, "59" },
        { 141.0, "141" }, { 315.0, "315" }, { 752.0, "752" }, { 1800.0, "1k8" }, { 4000.0, "4k" }
    });
    gateReleaseDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];
        cp.dsp.gateReleaseMs.set ((float) val);
    };
    addChildComponent (gateReleaseDial);

    // Box 3
    gateFiltFreqDial.setTickLabels ({
        { 20.0, "20" }, { 47.0, "47" }, { 112.0, "112" }, { 266.0, "266" },
        { 632.0, "632" }, { 1400.0, "1k4" }, { 3500.0, "3k5" }, { 8400.0, "8k4" }, { 20000.0, "20k" }
    });
    addChildComponent (gateFiltFreqDial);

    gateSlopeDial.setTickLabels ({ { 1.0, "LC6" }, { 2.0, "LC12" }, { 3.0, "1" }, { 5.0, "3" }, { 10.0, "10" } });
    addChildComponent (gateSlopeDial);

    gateFilterButton.setClickingTogglesState (true);
    addChildComponent (gateFilterButton);

    gateSoloButton.setClickingTogglesState (true);
    addChildComponent (gateSoloButton);

    addChildComponent (gateKeySourceButton);
}

void DetailScreen::setupDynTab()
{
    addChildComponent (compGraph);

    // Under Graph Buttons
    auto setupToggleBtn = [this] (juce::TextButton& b, int radioGroup, bool state) {
        b.setClickingTogglesState (true);
        b.setRadioGroupId (radioGroup);
        b.setToggleState (state, juce::dontSendNotification);
        addChildComponent (b);
    };

    setupToggleBtn (compTypeCompButton, 301, true);
    setupToggleBtn (compTypeExpButton, 301, false);
    setupToggleBtn (compModeManButton, 302, true);
    setupToggleBtn (compModeAutoButton, 302, false);
    setupToggleBtn (compDetPeakButton, 303, true);
    setupToggleBtn (compDetRmsButton, 303, false);
    setupToggleBtn (compCharLinButton, 304, true);
    setupToggleBtn (compCharLogButton, 304, false);

    compActiveButton.setClickingTogglesState (true);
    compActiveButton.addListener (this);
    addChildComponent (compActiveButton);

    addChildComponent (compGrMeter);

    // Box 1
    compThreshDial.setTickLabels ({
        { -60.0, "-60" }, { -50.0, "-50" }, { -40.0, "-40" }, { -30.0, "-30" },
        { -20.0, "-20" }, { -10.0, "-10" }, { 0.0, "0" }
    });
    compThreshDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        if (auto* dsp = getCurrentDspParams())
        {
            dsp->compThreshDb.set ((float) val);
            compGraph.setCompParams ((float) val, (float) compRatioDial.getValue(), (float) compKneeDial.getValue());
        }
    };
    addChildComponent (compThreshDial);

    compRatioDial.setTickLabels ({
        { 1.1, "1.1" }, { 1.3, "1.3" }, { 1.5, "1.5" }, { 2.0, "2" },
        { 2.5, "2.5" }, { 3.0, "3" }, { 4.0, "4" }, { 5.0, "5" },
        { 7.0, "7" }, { 10.0, "10" }, { 20.0, "20" }, { 100.0, "100" }
    });
    compRatioDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        if (auto* dsp = getCurrentDspParams())
        {
            dsp->compRatio.set ((float) val);
            compGraph.setCompParams ((float) compThreshDial.getValue(), (float) val, (float) compKneeDial.getValue());
        }
    };
    addChildComponent (compRatioDial);

    compGainDial.setTickLabels ({
        { 0.0, "0" }, { 3.0, "3" }, { 6.0, "6" }, { 9.0, "9" },
        { 12.0, "12" }, { 15.0, "15" }, { 18.0, "18" }, { 21.0, "21" }, { 24.0, "24" }
    });
    compGainDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        if (auto* dsp = getCurrentDspParams())
            dsp->compMakeupDb.set ((float) val);
    };
    addChildComponent (compGainDial);

    compKneeDial.setTickLabels ({
        { 0.0, "0" }, { 1.0, "1" }, { 2.0, "2" }, { 3.0, "3" }, { 4.0, "4" }, { 5.0, "5" }
    });
    compKneeDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        if (auto* dsp = getCurrentDspParams())
        {
            dsp->compKneeDb.set ((float) val);
            compGraph.setCompParams ((float) compThreshDial.getValue(), (float) compRatioDial.getValue(), (float) val);
        }
    };
    addChildComponent (compKneeDial);

    // Box 2
    compAttackDial.setTickLabels ({ { 0.0, "0" }, { 30.0, "30" }, { 60.0, "60" }, { 90.0, "90" }, { 120.0, "120" } });
    compAttackDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        if (auto* dsp = getCurrentDspParams())
            dsp->compAttackMs.set ((float) val);
    };
    addChildComponent (compAttackDial);

    compHoldDial.setTickLabels ({
        { 0.0, "0" }, { 0.1, "0.1" }, { 0.4, "0.4" }, { 1.4, "1.4" },
        { 6.3, "6.3" }, { 25.2, "25.2" }, { 112.0, "112" }, { 447.0, "447" }, { 2000.0, "2k" }
    });
    addChildComponent (compHoldDial);

    compReleaseDial.setTickLabels ({
        { 5.0, "5" }, { 6.0, "6" }, { 28.0, "28" }, { 59.0, "59" },
        { 141.0, "141" }, { 315.0, "315" }, { 752.0, "752" }, { 1800.0, "1k8" }, { 4000.0, "4k" }
    });
    compReleaseDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        if (auto* dsp = getCurrentDspParams())
            dsp->compReleaseMs.set ((float) val);
    };
    addChildComponent (compReleaseDial);

    compMixDial.setTickLabels ({
        { 0.0, "0" }, { 10.0, "10" }, { 20.0, "20" }, { 30.0, "30" },
        { 40.0, "40" }, { 50.0, "50" }, { 60.0, "60" }, { 70.0, "70" },
        { 80.0, "80" }, { 90.0, "90" }, { 100.0, "100" }
    });
    compMixDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        if (auto* dsp = getCurrentDspParams())
            dsp->compMix.set ((float) (val / 100.0));
    };
    addChildComponent (compMixDial);

    // Box 3
    compFiltFreqDial.setTickLabels ({
        { 20.0, "20" }, { 47.0, "47" }, { 112.0, "112" }, { 266.0, "266" },
        { 632.0, "632" }, { 1400.0, "1k4" }, { 3500.0, "3k5" }, { 8400.0, "8k4" }, { 20000.0, "20k" }
    });
    addChildComponent (compFiltFreqDial);

    compSlopeDial.setTickLabels ({ { 1.0, "LC6" }, { 2.0, "LC12" }, { 3.0, "1" }, { 5.0, "3" }, { 10.0, "10" } });
    addChildComponent (compSlopeDial);

    compFilterButton.setClickingTogglesState (true);
    addChildComponent (compFilterButton);

    compSoloButton.setClickingTogglesState (true);
    addChildComponent (compSoloButton);

    addChildComponent (compKeySourceButton);
}

void DetailScreen::setupEqTab()
{
    // Left Box
    eqGainDial.setTickLabels ({
        { -15.0, "-15" }, { -10.0, "-10" }, { -5.0, "-5" }, { 0.0, "0" },
        { 5.0, "5" }, { 10.0, "10" }, { 15.0, "15" }
    });
    eqGainDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        if (auto* dsp = getCurrentDspParams())
        {
            if (selectedEqBand == 1) dsp->eqLowGainDb.set ((float) val);
            else if (selectedEqBand == 2) dsp->eqMid1GainDb.set ((float) val);
            else if (selectedEqBand == 3) dsp->eqMid2GainDb.set ((float) val);
            else if (selectedEqBand == 4) dsp->eqHighGainDb.set ((float) val);
            dsp->eqBypass.setBool (false);
            eqActiveButton.setToggleState (true, juce::dontSendNotification);
            eqActiveButton.setButtonText ("EQ IN");
            eqGraph.repaint();
        }
    };
    addChildComponent (eqGainDial);

    eqFreqDial.setTickLabels ({
        { 20.0, "20" }, { 47.0, "47" }, { 112.0, "112" }, { 266.0, "266" },
        { 632.0, "632" }, { 1400.0, "1k4" }, { 3500.0, "3k5" }, { 8400.0, "8k4" }, { 20000.0, "20k" }
    });
    eqFreqDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        if (auto* dsp = getCurrentDspParams())
        {
            if (selectedEqBand == 0)
            {
                dsp->hpfFreq.set ((float) val);
                dsp->hpfBypass.setBool (false);
                lowCutButton.setToggleState (true, juce::dontSendNotification);
                eqLoCutButton.setToggleState (true, juce::dontSendNotification);
                eqLoCutBottomButton.setToggleState (true, juce::dontSendNotification);
            }
            else
            {
                if (selectedEqBand == 1) dsp->eqLowFreq.set ((float) val);
                else if (selectedEqBand == 2) dsp->eqMid1Freq.set ((float) val);
                else if (selectedEqBand == 3) dsp->eqMid2Freq.set ((float) val);
                else if (selectedEqBand == 4) dsp->eqHighFreq.set ((float) val);
                dsp->eqBypass.setBool (false);
                eqActiveButton.setToggleState (true, juce::dontSendNotification);
                eqActiveButton.setButtonText ("EQ IN");
            }
            eqGraph.repaint();
        }
    };
    addChildComponent (eqFreqDial);

    eqQDial.setTickLabels ({
        { 0.3, "0.3" }, { 0.5, "0.5" }, { 1.0, "1" }, { 2.0, "2" }, { 5.0, "5" }, { 10.0, "10" }
    });
    eqQDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        if (auto* dsp = getCurrentDspParams())
        {
            if (selectedEqBand == 1) dsp->eqLowQ.set ((float) val);
            else if (selectedEqBand == 2) dsp->eqMid1Q.set ((float) val);
            else if (selectedEqBand == 3) dsp->eqMid2Q.set ((float) val);
            else if (selectedEqBand == 4) dsp->eqHighQ.set ((float) val);
            dsp->eqBypass.setBool (false);
            eqActiveButton.setToggleState (true, juce::dontSendNotification);
            eqActiveButton.setButtonText ("EQ IN");
            eqGraph.repaint();
        }
    };
    addChildComponent (eqQDial);

    eqActiveButton.setClickingTogglesState (true);
    eqActiveButton.addListener (this);
    addChildComponent (eqActiveButton);

    eqLoCutButton.setClickingTogglesState (true);
    eqLoCutButton.addListener (this);
    addChildComponent (eqLoCutButton);

    // Center Graph
    addChildComponent (eqGraph);
    eqGraph.onParamsChanged = [this] {
        updateEqSelectedBandDialValues();
    };
    eqGraph.onBandSelected = [this] (int band) {
        selectedEqBand = band;
        updateEqSelectedBandDialValues();
        // Update bottom band buttons toggle
        if (band >= 1 && band <= 4)
        {
            const int btnIdxs[4] = { 0, 2, 3, 5 };
            eqBandButtons[btnIdxs[band - 1]].setToggleState (true, juce::dontSendNotification);
        }
    };

    // Bottom Bar
    eqResetButton.addListener (this);
    addChildComponent (eqResetButton);

    eqModeButton.addListener (this);
    addChildComponent (eqModeButton);

    eqRtaButton.setClickingTogglesState (true);
    eqRtaButton.addListener (this);
    eqRtaButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xfff59e0b)); // Amber M32 style
    eqRtaButton.setColour (juce::TextButton::textColourOnId, juce::Colours::black);
    addChildComponent (eqRtaButton);

    eqLoCutBottomButton.setClickingTogglesState (true);
    eqLoCutBottomButton.addListener (this);
    addChildComponent (eqLoCutBottomButton);

    const char* bandNames[6] = { "1 LOW", "LOW2", "2 LOMID", "3 HIMID", "HIGH2", "4 HIGH" };
    for (int i = 0; i < 6; ++i)
    {
        eqBandButtons[i].setButtonText (bandNames[i]);
        eqBandButtons[i].setClickingTogglesState (true);
        eqBandButtons[i].setRadioGroupId (4001);
        eqBandButtons[i].addListener (this);
        addChildComponent (eqBandButtons[i]);
    }
    eqBandButtons[0].setToggleState (true, juce::dontSendNotification);
}

void DetailScreen::setupSendsTab()
{
    const char* defaultTap = "POST FADER";

    for (int i = 0; i < 16; ++i)
    {
        if (i < 12)
        {
            sendBusBadges[i].setBus (i + 1, "Bus" + juce::String (i + 1), false);
        }
        else
        {
            const int fxSlot = i - 12;
            const auto type = (FxType) (int) engine.getMixerParams().fxSlots[(size_t) fxSlot].fxType.getInt();
            const auto meta = getFxMetadata (type);
            sendBusBadges[i].setBus (i + 1, "Fx" + juce::String (fxSlot + 1) + " (" + meta.shortName + ")", true);
        }
        addChildComponent (sendBusBadges[i]);

        sendValueLabels[i].setFont (juce::FontOptions (10.0f, juce::Font::bold));
        sendValueLabels[i].setJustificationType (juce::Justification::centred);
        sendValueLabels[i].setColour (juce::Label::textColourId, juce::Colour (0xffb0bec5));
        sendValueLabels[i].setText ("-oo", juce::dontSendNotification);
        addChildComponent (sendValueLabels[i]);

        sendFaders[i].setSliderStyle (juce::Slider::LinearVertical);
        sendFaders[i].setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        sendFaders[i].setRange (0.0, 1.0, 0.001);
        sendFaders[i].setValue (0.0, juce::dontSendNotification);
        sendFaders[i].addListener (this);
        addChildComponent (sendFaders[i]);

        sendMuteButtons[i].setButtonText ("M");
        sendMuteButtons[i].setName ("Mute");
        sendMuteButtons[i].setClickingTogglesState (true);
        sendMuteButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xff381618));
        sendMuteButtons[i].setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffee1822));
        sendMuteButtons[i].setColour (juce::TextButton::textColourOffId, juce::Colour (0xffe57373));
        sendMuteButtons[i].setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        sendMuteButtons[i].addListener (this);
        addChildComponent (sendMuteButtons[i]);
    }

    for (int t = 0; t < 8; ++t)
    {
        sendTapButtons[t].setButtonText (defaultTap);
        sendTapButtons[t].setColour (juce::TextButton::buttonColourId, juce::Colour (0xff242930));
        sendTapButtons[t].setColour (juce::TextButton::textColourOffId, juce::Colour (0xffd0d8e0));
        sendTapButtons[t].addListener (this);
        addChildComponent (sendTapButtons[t]);
    }
}

void DetailScreen::setupNamingTab()
{
    nameEditor.setFont (juce::FontOptions (14.0f));
    nameEditor.onTextChange = [this] {
        project.setChannelName (selectedChannel, nameEditor.getText());
        scribbleName.setText (nameEditor.getText(), juce::dontSendNotification);
    };
    addChildComponent (nameEditor);

    const juce::uint32 palette[8] = {
        0xffffc107, 0xffff9800, 0xffe91e63, 0xff2196f3,
        0xff4caf50, 0xff00bcd4, 0xff9c27b0, 0xff795548
    };

    for (int i = 0; i < 8; ++i)
    {
        colourButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (palette[i]));
        colourButtons[i].onClick = [this, col = juce::Colour (palette[i])] {
            project.setChannelColour (selectedChannel, col);
            scribbleName.setColour (juce::Label::backgroundColourId, col);
            repaint();
        };
        addChildComponent (colourButtons[i]);
    }
}

void DetailScreen::updateEqSelectedBandDialValues()
{
    ignoreCallbacks = true;
    auto* dsp = getCurrentDspParams();
    if (dsp == nullptr)
    {
        ignoreCallbacks = false;
        return;
    }

    const bool eqOn = ! dsp->eqBypass.getBool();
    eqActiveButton.setToggleState (eqOn, juce::dontSendNotification);
    eqActiveButton.setButtonText (eqOn ? "EQ IN" : "EQ BYP");

    const bool hpfOn = ! dsp->hpfBypass.getBool();
    lowCutButton.setToggleState (hpfOn, juce::dontSendNotification);
    eqLoCutButton.setToggleState (hpfOn, juce::dontSendNotification);
    eqLoCutBottomButton.setToggleState (hpfOn, juce::dontSendNotification);

    if (selectedEqBand == 0) // HPF
    {
        eqGainDial.setEnabled (false);
        eqGainDial.setValue (0.0, juce::dontSendNotification);
        eqFreqDial.setValue (dsp->hpfFreq.get(), juce::dontSendNotification);
        eqQDial.setEnabled (false);
        eqQDial.setValue (0.707, juce::dontSendNotification);
        eqModeButton.setButtonText ("HPF");
        eqModeButton.setEnabled (false);
    }
    else if (selectedEqBand == 1) // Low
    {
        eqGainDial.setEnabled (true);
        eqGainDial.setValue (dsp->eqLowGainDb.get(), juce::dontSendNotification);
        eqFreqDial.setValue (dsp->eqLowFreq.get(), juce::dontSendNotification);
        eqQDial.setEnabled (true);
        eqQDial.setValue (dsp->eqLowQ.get(), juce::dontSendNotification);
        eqModeButton.setEnabled (true);
        eqModeButton.setButtonText (dsp->eqLowType.getInt() == 1 ? "LShv" : "PEQ");
    }
    else if (selectedEqBand == 2) // Mid 1
    {
        eqGainDial.setEnabled (true);
        eqGainDial.setValue (dsp->eqMid1GainDb.get(), juce::dontSendNotification);
        eqFreqDial.setValue (dsp->eqMid1Freq.get(), juce::dontSendNotification);
        eqQDial.setEnabled (true);
        eqQDial.setValue (dsp->eqMid1Q.get(), juce::dontSendNotification);
        eqModeButton.setEnabled (false);
        eqModeButton.setButtonText ("PEQ");
    }
    else if (selectedEqBand == 3) // Mid 2
    {
        eqGainDial.setEnabled (true);
        eqGainDial.setValue (dsp->eqMid2GainDb.get(), juce::dontSendNotification);
        eqFreqDial.setValue (dsp->eqMid2Freq.get(), juce::dontSendNotification);
        eqQDial.setEnabled (true);
        eqQDial.setValue (dsp->eqMid2Q.get(), juce::dontSendNotification);
        eqModeButton.setEnabled (false);
        eqModeButton.setButtonText ("PEQ");
    }
    else if (selectedEqBand == 4) // High
    {
        eqGainDial.setEnabled (true);
        eqGainDial.setValue (dsp->eqHighGainDb.get(), juce::dontSendNotification);
        eqFreqDial.setValue (dsp->eqHighFreq.get(), juce::dontSendNotification);
        eqQDial.setEnabled (true);
        eqQDial.setValue (dsp->eqHighQ.get(), juce::dontSendNotification);
        eqModeButton.setEnabled (true);
        eqModeButton.setButtonText (dsp->eqHighType.getInt() == 2 ? "HShv" : "PEQ");
    }

    ignoreCallbacks = false;
}

ChannelDspParams* DetailScreen::getCurrentDspParams()
{
    auto& mp = engine.getMixerParams();
    switch (currentStripType)
    {
        case ChannelStripComponent::StripType::InputChannel:
            return &mp.channels[(size_t) std::clamp (currentStripIndex, 0, (int) kMaxInputChannels - 1)].dsp;
        case ChannelStripComponent::StripType::Aux:
            return &mp.auxes[(size_t) std::clamp (currentStripIndex, 0, 7)].dsp;
        case ChannelStripComponent::StripType::FxReturn:
            return &mp.fxReturns[(size_t) std::clamp (currentStripIndex, 0, 7)].dsp;
        case ChannelStripComponent::StripType::Bus:
            return &mp.buses[(size_t) std::clamp (currentStripIndex, 0, 15)].dsp;
        case ChannelStripComponent::StripType::Matrix:
            return &mp.matrices[(size_t) std::clamp (currentStripIndex, 0, 5)].dsp;
        default:
            return nullptr;
    }
}

int DetailScreen::getMaxStripIndex (ChannelStripComponent::StripType type) const noexcept
{
    switch (type)
    {
        case ChannelStripComponent::StripType::InputChannel: return (int) kMaxInputChannels;
        case ChannelStripComponent::StripType::Aux:          return 8;
        case ChannelStripComponent::StripType::FxReturn:     return 8;
        case ChannelStripComponent::StripType::Bus:          return 16;
        case ChannelStripComponent::StripType::Matrix:       return 6;
        case ChannelStripComponent::StripType::Dca:          return 8;
        case ChannelStripComponent::StripType::MainC:
        case ChannelStripComponent::StripType::Master:       return 1;
        default: return 32;
    }
}

void DetailScreen::setPanForCurrentStrip (float p)
{
    auto& mp = engine.getMixerParams();
    switch (currentStripType)
    {
        case ChannelStripComponent::StripType::InputChannel: mp.channels[(size_t) currentStripIndex].pan.set (p); break;
        case ChannelStripComponent::StripType::Aux:          mp.auxes[(size_t) currentStripIndex].pan.set (p); break;
        case ChannelStripComponent::StripType::FxReturn:     mp.fxReturns[(size_t) currentStripIndex].pan.set (p); break;
        case ChannelStripComponent::StripType::Bus:          mp.buses[(size_t) currentStripIndex].pan.set (p); break;
        case ChannelStripComponent::StripType::Matrix:       mp.matrices[(size_t) currentStripIndex].pan.set (p); break;
        default: break;
    }
}

float DetailScreen::getPanForCurrentStrip() const noexcept
{
    const auto& mp = engine.getMixerParams();
    switch (currentStripType)
    {
        case ChannelStripComponent::StripType::InputChannel: return mp.channels[(size_t) currentStripIndex].pan.get();
        case ChannelStripComponent::StripType::Aux:          return mp.auxes[(size_t) currentStripIndex].pan.get();
        case ChannelStripComponent::StripType::FxReturn:     return mp.fxReturns[(size_t) currentStripIndex].pan.get();
        case ChannelStripComponent::StripType::Bus:          return mp.buses[(size_t) currentStripIndex].pan.get();
        case ChannelStripComponent::StripType::Matrix:       return mp.matrices[(size_t) currentStripIndex].pan.get();
        default: return 0.0f;
    }
}

void DetailScreen::setFaderForCurrentStrip (float db)
{
    auto& mp = engine.getMixerParams();
    switch (currentStripType)
    {
        case ChannelStripComponent::StripType::InputChannel: mp.channels[(size_t) currentStripIndex].faderDb.set (db); break;
        case ChannelStripComponent::StripType::Aux:          mp.auxes[(size_t) currentStripIndex].faderDb.set (db); break;
        case ChannelStripComponent::StripType::FxReturn:     mp.fxReturns[(size_t) currentStripIndex].faderDb.set (db); break;
        case ChannelStripComponent::StripType::Bus:          mp.buses[(size_t) currentStripIndex].faderDb.set (db); break;
        case ChannelStripComponent::StripType::Matrix:       mp.matrices[(size_t) currentStripIndex].faderDb.set (db); break;
        default: break;
    }
}

float DetailScreen::getFaderForCurrentStrip() const noexcept
{
    const auto& mp = engine.getMixerParams();
    switch (currentStripType)
    {
        case ChannelStripComponent::StripType::InputChannel: return mp.channels[(size_t) currentStripIndex].faderDb.get();
        case ChannelStripComponent::StripType::Aux:          return mp.auxes[(size_t) currentStripIndex].faderDb.get();
        case ChannelStripComponent::StripType::FxReturn:     return mp.fxReturns[(size_t) currentStripIndex].faderDb.get();
        case ChannelStripComponent::StripType::Bus:          return mp.buses[(size_t) currentStripIndex].faderDb.get();
        case ChannelStripComponent::StripType::Matrix:       return mp.matrices[(size_t) currentStripIndex].faderDb.get();
        default: return 0.0f;
    }
}

bool DetailScreen::getMuteForCurrentStrip() const noexcept
{
    const auto& mp = engine.getMixerParams();
    switch (currentStripType)
    {
        case ChannelStripComponent::StripType::InputChannel: return mp.channels[(size_t) currentStripIndex].mute.getBool();
        case ChannelStripComponent::StripType::Aux:          return mp.auxes[(size_t) currentStripIndex].mute.getBool();
        case ChannelStripComponent::StripType::FxReturn:     return mp.fxReturns[(size_t) currentStripIndex].mute.getBool();
        case ChannelStripComponent::StripType::Bus:          return mp.buses[(size_t) currentStripIndex].mute.getBool();
        case ChannelStripComponent::StripType::Matrix:       return mp.matrices[(size_t) currentStripIndex].mute.getBool();
        default: return false;
    }
}

void DetailScreen::setMuteForCurrentStrip (bool m)
{
    auto& mp = engine.getMixerParams();
    switch (currentStripType)
    {
        case ChannelStripComponent::StripType::InputChannel: mp.channels[(size_t) currentStripIndex].mute.setBool (m); break;
        case ChannelStripComponent::StripType::Aux:          mp.auxes[(size_t) currentStripIndex].mute.setBool (m); break;
        case ChannelStripComponent::StripType::FxReturn:     mp.fxReturns[(size_t) currentStripIndex].mute.setBool (m); break;
        case ChannelStripComponent::StripType::Bus:          mp.buses[(size_t) currentStripIndex].mute.setBool (m); break;
        case ChannelStripComponent::StripType::Matrix:       mp.matrices[(size_t) currentStripIndex].mute.setBool (m); break;
        default: break;
    }
}

bool DetailScreen::getSoloForCurrentStrip() const noexcept
{
    const auto& mp = engine.getMixerParams();
    switch (currentStripType)
    {
        case ChannelStripComponent::StripType::InputChannel: return mp.channels[(size_t) currentStripIndex].solo.getBool();
        case ChannelStripComponent::StripType::Aux:          return mp.auxes[(size_t) currentStripIndex].solo.getBool();
        case ChannelStripComponent::StripType::FxReturn:     return mp.fxReturns[(size_t) currentStripIndex].solo.getBool();
        case ChannelStripComponent::StripType::Bus:          return mp.buses[(size_t) currentStripIndex].solo.getBool();
        case ChannelStripComponent::StripType::Matrix:       return mp.matrices[(size_t) currentStripIndex].solo.getBool();
        default: return false;
    }
}

void DetailScreen::setSoloForCurrentStrip (bool s)
{
    auto& mp = engine.getMixerParams();
    switch (currentStripType)
    {
        case ChannelStripComponent::StripType::InputChannel: mp.channels[(size_t) currentStripIndex].solo.setBool (s); break;
        case ChannelStripComponent::StripType::Aux:          mp.auxes[(size_t) currentStripIndex].solo.setBool (s); break;
        case ChannelStripComponent::StripType::FxReturn:     mp.fxReturns[(size_t) currentStripIndex].solo.setBool (s); break;
        case ChannelStripComponent::StripType::Bus:          mp.buses[(size_t) currentStripIndex].solo.setBool (s); break;
        case ChannelStripComponent::StripType::Matrix:       mp.matrices[(size_t) currentStripIndex].solo.setBool (s); break;
        default: break;
    }
}

bool DetailScreen::getMainAssignForCurrentStrip() const noexcept
{
    const auto& mp = engine.getMixerParams();
    switch (currentStripType)
    {
        case ChannelStripComponent::StripType::InputChannel: return mp.channels[(size_t) currentStripIndex].mainAssign.getBool();
        case ChannelStripComponent::StripType::Aux:          return mp.auxes[(size_t) currentStripIndex].mainAssign.getBool();
        case ChannelStripComponent::StripType::FxReturn:     return mp.fxReturns[(size_t) currentStripIndex].mainAssign.getBool();
        case ChannelStripComponent::StripType::Bus:          return mp.buses[(size_t) currentStripIndex].mainAssign.getBool();
        default: return false;
    }
}

void DetailScreen::setMainAssignForCurrentStrip (bool a)
{
    auto& mp = engine.getMixerParams();
    switch (currentStripType)
    {
        case ChannelStripComponent::StripType::InputChannel: mp.channels[(size_t) currentStripIndex].mainAssign.setBool (a); break;
        case ChannelStripComponent::StripType::Aux:          mp.auxes[(size_t) currentStripIndex].mainAssign.setBool (a); break;
        case ChannelStripComponent::StripType::FxReturn:     mp.fxReturns[(size_t) currentStripIndex].mainAssign.setBool (a); break;
        case ChannelStripComponent::StripType::Bus:          mp.buses[(size_t) currentStripIndex].mainAssign.setBool (a); break;
        default: break;
    }
}

bool DetailScreen::getMonoAssignForCurrentStrip() const noexcept
{
    const auto& mp = engine.getMixerParams();
    switch (currentStripType)
    {
        case ChannelStripComponent::StripType::InputChannel: return mp.channels[(size_t) currentStripIndex].monoAssign.getBool();
        case ChannelStripComponent::StripType::Aux:          return mp.auxes[(size_t) currentStripIndex].monoAssign.getBool();
        case ChannelStripComponent::StripType::FxReturn:     return mp.fxReturns[(size_t) currentStripIndex].monoAssign.getBool();
        case ChannelStripComponent::StripType::Bus:          return mp.buses[(size_t) currentStripIndex].monoAssign.getBool();
        default: return false;
    }
}

void DetailScreen::setMonoAssignForCurrentStrip (bool a)
{
    auto& mp = engine.getMixerParams();
    switch (currentStripType)
    {
        case ChannelStripComponent::StripType::InputChannel: mp.channels[(size_t) currentStripIndex].monoAssign.setBool (a); break;
        case ChannelStripComponent::StripType::Aux:          mp.auxes[(size_t) currentStripIndex].monoAssign.setBool (a); break;
        case ChannelStripComponent::StripType::FxReturn:     mp.fxReturns[(size_t) currentStripIndex].monoAssign.setBool (a); break;
        case ChannelStripComponent::StripType::Bus:          mp.buses[(size_t) currentStripIndex].monoAssign.setBool (a); break;
        default: break;
    }
}

int DetailScreen::getDcaMaskForCurrentStrip() const noexcept
{
    const auto& mp = engine.getMixerParams();
    switch (currentStripType)
    {
        case ChannelStripComponent::StripType::InputChannel: return mp.channels[(size_t) currentStripIndex].dcaGroups.getInt();
        case ChannelStripComponent::StripType::Aux:          return mp.auxes[(size_t) currentStripIndex].dcaGroups.getInt();
        case ChannelStripComponent::StripType::FxReturn:     return mp.fxReturns[(size_t) currentStripIndex].dcaGroups.getInt();
        case ChannelStripComponent::StripType::Bus:          return mp.buses[(size_t) currentStripIndex].dcaGroups.getInt();
        default: return 0;
    }
}

void DetailScreen::setDcaMaskForCurrentStrip (int mask)
{
    auto& mp = engine.getMixerParams();
    switch (currentStripType)
    {
        case ChannelStripComponent::StripType::InputChannel: mp.channels[(size_t) currentStripIndex].dcaGroups.set ((float) mask); break;
        case ChannelStripComponent::StripType::Aux:          mp.auxes[(size_t) currentStripIndex].dcaGroups.set ((float) mask); break;
        case ChannelStripComponent::StripType::FxReturn:     mp.fxReturns[(size_t) currentStripIndex].dcaGroups.set ((float) mask); break;
        case ChannelStripComponent::StripType::Bus:          mp.buses[(size_t) currentStripIndex].dcaGroups.set ((float) mask); break;
        default: break;
    }
}

int DetailScreen::getMuteGroupMaskForCurrentStrip() const noexcept
{
    const auto& mp = engine.getMixerParams();
    switch (currentStripType)
    {
        case ChannelStripComponent::StripType::InputChannel: return mp.channels[(size_t) currentStripIndex].muteGroups.getInt();
        case ChannelStripComponent::StripType::Aux:          return mp.auxes[(size_t) currentStripIndex].muteGroups.getInt();
        case ChannelStripComponent::StripType::FxReturn:     return mp.fxReturns[(size_t) currentStripIndex].muteGroups.getInt();
        case ChannelStripComponent::StripType::Bus:          return mp.buses[(size_t) currentStripIndex].muteGroups.getInt();
        default: return 0;
    }
}

void DetailScreen::setMuteGroupMaskForCurrentStrip (int mask)
{
    auto& mp = engine.getMixerParams();
    switch (currentStripType)
    {
        case ChannelStripComponent::StripType::InputChannel: mp.channels[(size_t) currentStripIndex].muteGroups.set ((float) mask); break;
        case ChannelStripComponent::StripType::Aux:          mp.auxes[(size_t) currentStripIndex].muteGroups.set ((float) mask); break;
        case ChannelStripComponent::StripType::FxReturn:     mp.fxReturns[(size_t) currentStripIndex].muteGroups.set ((float) mask); break;
        case ChannelStripComponent::StripType::Bus:          mp.buses[(size_t) currentStripIndex].muteGroups.set ((float) mask); break;
        default: break;
    }
}

int DetailScreen::getInsertSlotForCurrentStrip() const noexcept
{
    const auto& mp = engine.getMixerParams();
    switch (currentStripType)
    {
        case ChannelStripComponent::StripType::InputChannel: return mp.channels[(size_t) currentStripIndex].insertFxSlot.getInt();
        case ChannelStripComponent::StripType::Bus:          return mp.buses[(size_t) currentStripIndex].insertFxSlot.getInt();
        case ChannelStripComponent::StripType::Matrix:       return mp.matrices[(size_t) currentStripIndex].insertFxSlot.getInt();
        case ChannelStripComponent::StripType::Master:       return mp.main.insertFxSlot.getInt();
        default: return 0;
    }
}

void DetailScreen::setInsertSlotForCurrentStrip (int slot)
{
    auto& mp = engine.getMixerParams();
    switch (currentStripType)
    {
        case ChannelStripComponent::StripType::InputChannel: mp.channels[(size_t) currentStripIndex].insertFxSlot.set ((float) slot); break;
        case ChannelStripComponent::StripType::Bus:          mp.buses[(size_t) currentStripIndex].insertFxSlot.set ((float) slot); break;
        case ChannelStripComponent::StripType::Matrix:       mp.matrices[(size_t) currentStripIndex].insertFxSlot.set ((float) slot); break;
        case ChannelStripComponent::StripType::Master:       mp.main.insertFxSlot.set ((float) slot); break;
        default: break;
    }
}

void DetailScreen::setSelectedChannel (int ch)
{
    setSelectedStrip (ChannelStripComponent::StripType::InputChannel, ch);
}

void DetailScreen::setSelectedStrip (ChannelStripComponent::StripType type, int index)
{
    const int maxIdx = getMaxStripIndex (type);
    const int clamped = std::clamp (index, 0, maxIdx - 1);

    currentStripType = type;
    currentStripIndex = clamped;
    if (type == ChannelStripComponent::StripType::InputChannel)
        selectedChannel = clamped;

    // 1. Sync BankSelectorBar
    switch (type)
    {
        case ChannelStripComponent::StripType::InputChannel:
            bankSelectorBar.setSelectedBank (clamped / 8, false);
            break;
        case ChannelStripComponent::StripType::Aux:
            bankSelectorBar.setSelectedBank (BankSelectorBar::BankAux1_8, false);
            break;
        case ChannelStripComponent::StripType::FxReturn:
            bankSelectorBar.setSelectedBank (BankSelectorBar::BankFx1_4, false);
            break;
        case ChannelStripComponent::StripType::Bus:
            bankSelectorBar.setSelectedBank (clamped < 8 ? BankSelectorBar::BankBus1_8 : BankSelectorBar::BankBus9_16, false);
            break;
        case ChannelStripComponent::StripType::Matrix:
            bankSelectorBar.setSelectedBank (BankSelectorBar::BankMtxMain, false);
            break;
        case ChannelStripComponent::StripType::Dca:
            bankSelectorBar.setSelectedBank (BankSelectorBar::BankDca1_8, false);
            break;
        default:
            break;
    }

    // 2. Set Signal Flow Mode
    switch (type)
    {
        case ChannelStripComponent::StripType::InputChannel:
            signalFlow.setStripMode (SignalFlowDiagram::StripMode::Input);
            break;
        case ChannelStripComponent::StripType::Aux:
            signalFlow.setStripMode (SignalFlowDiagram::StripMode::Aux);
            break;
        case ChannelStripComponent::StripType::FxReturn:
            signalFlow.setStripMode (SignalFlowDiagram::StripMode::FxReturn);
            break;
        case ChannelStripComponent::StripType::Bus:
            signalFlow.setStripMode (SignalFlowDiagram::StripMode::Bus);
            break;
        case ChannelStripComponent::StripType::Matrix:
            signalFlow.setStripMode (SignalFlowDiagram::StripMode::Matrix);
            break;
        default:
            signalFlow.setStripMode (SignalFlowDiagram::StripMode::Input);
            break;
    }

    // 3. Connect Meters
    auto& mm = engine.getMixerMeters();
    if (type == ChannelStripComponent::StripType::InputChannel)
    {
        auto& cm = mm.channels[(size_t) clamped];
        channelMeter.setMeters (&cm.preL, &cm.preR, &cm.clip);
        preampMeter.setMeters (&cm.preL, &cm.preR, &cm.clip);
        engine.getMixEngine().setRtaChannel (clamped);
    }
    else if (type == ChannelStripComponent::StripType::Aux)
    {
        auto& cm = mm.auxes[(size_t) clamped];
        channelMeter.setMeters (&cm, nullptr, nullptr);
        preampMeter.setMeters (&cm, nullptr, nullptr);
    }
    else if (type == ChannelStripComponent::StripType::FxReturn)
    {
        auto& cm = mm.fxReturns[(size_t) clamped];
        channelMeter.setMeters (&cm, nullptr, nullptr);
        preampMeter.setMeters (nullptr, nullptr, nullptr);
    }
    else if (type == ChannelStripComponent::StripType::Bus)
    {
        auto& cm = mm.buses[(size_t) clamped];
        channelMeter.setMeters (&cm, nullptr, nullptr);
        preampMeter.setMeters (nullptr, nullptr, nullptr);
    }
    else if (type == ChannelStripComponent::StripType::Matrix)
    {
        auto& cm = mm.matrices[(size_t) clamped];
        channelMeter.setMeters (&cm, nullptr, nullptr);
        preampMeter.setMeters (nullptr, nullptr, nullptr);
    }
    else
    {
        channelMeter.setMeters (nullptr, nullptr, nullptr);
        preampMeter.setMeters (nullptr, nullptr, nullptr);
    }

    updateTabVisibility();
    refreshFromModel();
    resized();
    repaint();
}

void DetailScreen::refreshFromModel()
{
    ignoreCallbacks = true;

    // 1. Left Strip & Metadata per strip type
    if (currentStripType == ChannelStripComponent::StripType::InputChannel)
    {
        auto& cp = engine.getMixerParams().channels[(size_t) currentStripIndex];
        auto& meta = project.getChannelMetadata (currentStripIndex);
        scribbleName.setText (meta.name, juce::dontSendNotification);
        scribbleName.setColour (juce::Label::backgroundColourId, meta.colour);
        scribbleName.setColour (juce::Label::textColourId, juce::Colours::black);
        channelNumLabel.setText (juce::String (currentStripIndex + 1), juce::dontSendNotification);

        const int src = cp.source.getInt();
        sourceButton.setButtonText (src >= 0 ? "IN " + juce::String (src + 1) : "OFF");
        phaseButton.setToggleState (cp.polarity.getBool(), juce::dontSendNotification);
        gainDial.setValue (cp.trimDb.get(), juce::dontSendNotification);
        phantom48vButton.setToggleState (cp.phantom48v.getBool(), juce::dontSendNotification);
        delayDial.setValue (cp.dsp.delayMs.get(), juce::dontSendNotification);
        delayButton.setToggleState (! cp.dsp.delayBypass.getBool(), juce::dontSendNotification);
        lowCutDial.setValue (cp.dsp.hpfFreq.get(), juce::dontSendNotification);
        lowCutButton.setToggleState (! cp.dsp.hpfBypass.getBool(), juce::dontSendNotification);
        linkButton.setToggleState (engine.getMixerParams().isChannelLinked (currentStripIndex), juce::dontSendNotification);
        nameEditor.setText (meta.name, false);
    }
    else if (currentStripType == ChannelStripComponent::StripType::Aux)
    {
        auto& ap = engine.getMixerParams().auxes[(size_t) currentStripIndex];
        juce::String aName = currentStripIndex < 6 ? "Aux " + juce::String (currentStripIndex + 1) : (currentStripIndex == 6 ? "USB L" : "USB R");
        scribbleName.setText (aName, juce::dontSendNotification);
        scribbleName.setColour (juce::Label::backgroundColourId, juce::Colour (0xff10b981));
        scribbleName.setColour (juce::Label::textColourId, juce::Colours::white);
        channelNumLabel.setText ("Aux" + juce::String (currentStripIndex + 1), juce::dontSendNotification);

        const int src = ap.source.getInt();
        sourceButton.setButtonText (src < 6 ? "Aux " + juce::String (src + 1) : (src == 6 ? "USB L" : "USB R"));
        phaseButton.setToggleState (ap.polarity.getBool(), juce::dontSendNotification);
        gainDial.setValue (ap.trimDb.get(), juce::dontSendNotification);
        phantom48vButton.setToggleState (ap.phantom48v.getBool(), juce::dontSendNotification);
        linkButton.setToggleState (false, juce::dontSendNotification);
        nameEditor.setText (aName, false);
    }
    else if (currentStripType == ChannelStripComponent::StripType::FxReturn)
    {
        juce::String fName = (currentStripIndex % 2 == 0 ? "FX " + juce::String (currentStripIndex / 2 + 1) + "L" : "FX " + juce::String (currentStripIndex / 2 + 1) + "R");
        scribbleName.setText (fName, juce::dontSendNotification);
        scribbleName.setColour (juce::Label::backgroundColourId, juce::Colour (0xffd946ef));
        scribbleName.setColour (juce::Label::textColourId, juce::Colours::white);
        channelNumLabel.setText ((currentStripIndex % 2 == 0 ? "Fx" + juce::String (currentStripIndex / 2 + 1) + "L" : "Fx" + juce::String (currentStripIndex / 2 + 1) + "R"), juce::dontSendNotification);
        linkButton.setToggleState (false, juce::dontSendNotification);
        nameEditor.setText (fName, false);
    }
    else if (currentStripType == ChannelStripComponent::StripType::Bus)
    {
        juce::String bName = "Bus " + juce::String (currentStripIndex + 1);
        scribbleName.setText (bName, juce::dontSendNotification);
        scribbleName.setColour (juce::Label::backgroundColourId, juce::Colour (0xff00d4e5));
        scribbleName.setColour (juce::Label::textColourId, juce::Colour (0xff0a1114));
        channelNumLabel.setText ("Bus" + juce::String (currentStripIndex + 1), juce::dontSendNotification);
        linkButton.setToggleState (false, juce::dontSendNotification);
        nameEditor.setText (bName, false);
    }
    else if (currentStripType == ChannelStripComponent::StripType::Matrix)
    {
        auto& mp = engine.getMixerParams().matrices[(size_t) currentStripIndex];
        juce::String mName = "Mtx " + juce::String (currentStripIndex + 1);
        scribbleName.setText (mName, juce::dontSendNotification);
        scribbleName.setColour (juce::Label::backgroundColourId, juce::Colour (0xffa855f7));
        scribbleName.setColour (juce::Label::textColourId, juce::Colours::white);
        channelNumLabel.setText ("Mtx" + juce::String (currentStripIndex + 1), juce::dontSendNotification);
        phaseButton.setToggleState (mp.polarity.getBool(), juce::dontSendNotification);
        linkButton.setToggleState (false, juce::dontSendNotification);
        nameEditor.setText (mName, false);
    }

    panIndicator.setPan (getPanForCurrentStrip(), juce::dontSendNotification);
    const float db = getFaderForCurrentStrip();
    faderSlider.setValue (faderDbToPos (db), juce::dontSendNotification);
    faderDbLabel.setText (db <= kFaderMinDb ? "-inf" : juce::String (db, 1) + " dB", juce::dontSendNotification);

    mainLrButton.setToggleState (getMainAssignForCurrentStrip(), juce::dontSendNotification);
    monoCenterButton.setToggleState (getMonoAssignForCurrentStrip(), juce::dontSendNotification);
    mainMuteButton.setToggleState (getMuteForCurrentStrip(), juce::dontSendNotification);
    mainSoloButton.setToggleState (getSoloForCurrentStrip(), juce::dontSendNotification);
    mainFader.setValue (faderDbToPos (engine.getMixerParams().main.faderDb.get()), juce::dontSendNotification);

    // DCA & Mute Groups
    const int dcaMask = getDcaMaskForCurrentStrip();
    for (int i = 0; i < 8; ++i)
        dcaButtons[i].setToggleState ((dcaMask & (1 << i)) != 0, juce::dontSendNotification);

    const int mgMask = getMuteGroupMaskForCurrentStrip();
    for (int i = 0; i < 6; ++i)
        muteGroupButtons[i].setToggleState ((mgMask & (1 << i)) != 0, juce::dontSendNotification);

    // Insert
    const int insSlot = getInsertSlotForCurrentStrip();
    insertButton.setButtonText (insSlot == 0 ? "INS: OFF" : "FX " + juce::String (insSlot));

    // Sends 1..16 (for InputChannel)
    if (currentStripType == ChannelStripComponent::StripType::InputChannel)
    {
        auto& cp = engine.getMixerParams().channels[(size_t) currentStripIndex];
        for (int i = 0; i < 16; ++i)
        {
            if (i >= 12)
            {
                const int fxSlot = i - 12;
                const auto type = (FxType) (int) engine.getMixerParams().fxSlots[(size_t) fxSlot].fxType.getInt();
                const auto meta = getFxMetadata (type);
                sendBusBadges[i].setBus (i + 1, "Fx" + juce::String (fxSlot + 1) + " (" + meta.shortName + ")", true);
            }
            else
            {
                sendBusBadges[i].setBus (i + 1, "Bus" + juce::String (i + 1), false);
            }

            const float sdb = cp.sends[(size_t) i].get();
            sendFaders[i].setValue (faderDbToPos (sdb), juce::dontSendNotification);
            sendValueLabels[i].setText (sdb <= -89.0f ? "-oo" : juce::String (sdb, 1), juce::dontSendNotification);
            sendMuteButtons[i].setToggleState (cp.sendMutes[(size_t) i].getBool(), juce::dontSendNotification);
        }

        const char* tapNames[6] = { "IN/LOW CUT", "PRE EQ", "POST EQ", "PRE FADER", "POST FADER", "GROUP" };
        for (int t = 0; t < 8; ++t)
        {
            const int tapMode = std::clamp (cp.sendTaps[(size_t) t].getInt(), 0, 5);
            sendTapButtons[t].setButtonText (tapNames[tapMode]);
        }
    }

    // DSP (Gate, Comp, EQ)
    if (auto* dsp = getCurrentDspParams())
    {
        gateActiveButton.setToggleState (! dsp->gateBypass.getBool(), juce::dontSendNotification);
        gateThreshDial.setValue (dsp->gateThreshDb.get(), juce::dontSendNotification);
        gateRangeDial.setValue (-dsp->gateRangeDb.get(), juce::dontSendNotification);
        gateAttackDial.setValue (dsp->gateAttackMs.get(), juce::dontSendNotification);
        gateHoldDial.setValue (dsp->gateHoldMs.get(), juce::dontSendNotification);
        gateReleaseDial.setValue (dsp->gateReleaseMs.get(), juce::dontSendNotification);
        gateGraph.setGateParams (dsp->gateThreshDb.get(), -dsp->gateRangeDb.get());

        compActiveButton.setToggleState (! dsp->compBypass.getBool(), juce::dontSendNotification);
        compThreshDial.setValue (dsp->compThreshDb.get(), juce::dontSendNotification);
        compRatioDial.setValue (dsp->compRatio.get(), juce::dontSendNotification);
        compGainDial.setValue (dsp->compMakeupDb.get(), juce::dontSendNotification);
        compKneeDial.setValue (dsp->compKneeDb.get(), juce::dontSendNotification);
        compAttackDial.setValue (dsp->compAttackMs.get(), juce::dontSendNotification);
        compReleaseDial.setValue (dsp->compReleaseMs.get(), juce::dontSendNotification);
        compMixDial.setValue (dsp->compMix.get() * 100.0, juce::dontSendNotification);
        compGraph.setCompParams (dsp->compThreshDb.get(), dsp->compRatio.get(), dsp->compKneeDb.get());

        eqGraph.setParams (dsp, engine.getSampleRate());
        const bool eqOn = ! dsp->eqBypass.getBool();
        eqActiveButton.setToggleState (eqOn, juce::dontSendNotification);
        eqLoCutButton.setToggleState (! dsp->hpfBypass.getBool(), juce::dontSendNotification);
        eqLoCutBottomButton.setToggleState (! dsp->hpfBypass.getBool(), juce::dontSendNotification);
        eqRtaButton.setToggleState (eqGraph.isRtaEnabled(), juce::dontSendNotification);
        updateEqSelectedBandDialValues();
    }

    ignoreCallbacks = false;
    repaint();
}

void DetailScreen::visibilityChanged()
{
    if (isShowing())
        startTimerHz (20);
    else
        stopTimer();
}

void DetailScreen::timerCallback()
{
    if (! isShowing())
    {
        stopTimer();
        return;
    }

    bankSelectorBar.repaint();

    // Keep channel name & colour in sync with project model (only for InputChannel)
    if (currentStripType == ChannelStripComponent::StripType::InputChannel)
    {
        const auto& meta = project.getChannelMetadata (currentStripIndex);
        if (! scribbleName.isBeingEdited() && scribbleName.getText() != meta.name)
        {
            scribbleName.setText (meta.name, juce::dontSendNotification);
            scribbleName.setColour (juce::Label::backgroundColourId, meta.colour);
            if (! nameEditor.hasKeyboardFocus (true))
                nameEditor.setText (meta.name, false);
        }
    }

    auto& mm = engine.getMixerMeters();

    if (currentTab == TabGate)
    {
        if (currentStripType == ChannelStripComponent::StripType::InputChannel)
        {
            const float gr = mm.channels[(size_t) currentStripIndex].dsp.gateGainReductionDb.load (std::memory_order_relaxed);
            gateGrMeter.setReductionDb (gr);
        }
    }
    else if (currentTab == TabDyn)
    {
        if (currentStripType == ChannelStripComponent::StripType::InputChannel)
        {
            const float gr = mm.channels[(size_t) currentStripIndex].dsp.compGainReductionDb.load (std::memory_order_relaxed);
            compGrMeter.setReductionDb (gr);
        }
    }
    else if (currentTab == TabEq && eqGraph.isRtaEnabled())
    {
        if (currentStripType == ChannelStripComponent::StripType::InputChannel)
        {
            auto& cm = mm.channels[(size_t) currentStripIndex];
            float rtaBands[InteractiveEqGraph::kNumRtaBands];
            const float peak = std::max (cm.preL.read(), cm.preR.read());

            if (peak > 0.0001f)
            {
                for (int i = 0; i < InteractiveEqGraph::kNumRtaBands; ++i)
                {
                    const float bandPeak = cm.rtaBands[(size_t) i].read();
                    const float db = gainToDb (bandPeak);
                    rtaBands[i] = std::clamp (db, -60.0f, 15.0f);
                }
            }
            else
            {
                for (int i = 0; i < InteractiveEqGraph::kNumRtaBands; ++i)
                    rtaBands[i] = -60.0f;
            }

            eqGraph.updateRtaSpectrum (rtaBands, InteractiveEqGraph::kNumRtaBands);
        }
    }
}

void DetailScreen::updateTabVisibility()
{
    const bool isInput = (currentStripType == ChannelStripComponent::StripType::InputChannel);
    const bool isAux   = (currentStripType == ChannelStripComponent::StripType::Aux);
    const bool isFx    = (currentStripType == ChannelStripComponent::StripType::FxReturn);
    const bool isBus   = (currentStripType == ChannelStripComponent::StripType::Bus);
    const bool isMtx   = (currentStripType == ChannelStripComponent::StripType::Matrix);

    const bool gateAvail = isInput;
    const bool dynAvail  = (isInput || isBus || isMtx);
    const bool sendsAvail = ! isMtx;

    tabButtons[TabGate].setEnabled (gateAvail);
    tabButtons[TabDyn].setEnabled (dynAvail);
    tabButtons[TabSends].setEnabled (sendsAvail);

    if ((currentTab == TabGate && ! gateAvail) ||
        (currentTab == TabDyn && ! dynAvail) ||
        (currentTab == TabSends && ! sendsAvail))
    {
        currentTab = TabConfig;
        tabButtons[TabConfig].setToggleState (true, juce::dontSendNotification);
    }

    const bool isCfg = (currentTab == TabConfig);
    signalFlow.setVisible (isCfg);

    // Source button: only visible for InputChannel and Aux!
    sourceButton.setVisible (isCfg && (isInput || isAux));

    // Preamp controls:
    linkButton.setVisible (isCfg);
    phaseButton.setVisible (isCfg && (isInput || isAux || isMtx));
    gainDial.setVisible (isCfg && (isInput || isAux));
    preampMeter.setVisible (isCfg && (isInput || isAux));
    phantom48vButton.setVisible (isCfg && (isInput || isAux));
    delayDial.setVisible (isCfg && isInput);
    delayButton.setVisible (isCfg && isInput);
    lowCutDial.setVisible (isCfg && isInput);
    lowCutButton.setVisible (isCfg && isInput);

    // Insert & utils:
    insertButton.setVisible (isCfg && (isInput || isBus || isMtx));
    copyButton.setVisible (isCfg);
    pasteButton.setVisible (isCfg);

    // DCA & Mute groups: visible for all except Matrix
    const bool showDca = isCfg && ! isMtx;
    for (auto& b : dcaButtons) b.setVisible (showDca);
    for (auto& b : muteGroupButtons) b.setVisible (showDca);

    // Automix: only for Input channels
    automixOffButton.setVisible (isCfg && isInput);
    automixXButton.setVisible (isCfg && isInput);
    automixYButton.setVisible (isCfg && isInput);
    weightingDial.setVisible (isCfg && isInput);

    // Main LR section:
    // For Matrix: NO main LR, NO mono/c, NO main fader. Only Mute & Solo!
    mainBusHeaderButton.setVisible (isCfg && ! isMtx);
    mainLrButton.setVisible (isCfg && ! isMtx);
    monoCenterButton.setVisible (isCfg && ! isMtx);
    mainFader.setVisible (isCfg && ! isMtx);
    mainMuteButton.setVisible (isCfg);
    mainSoloButton.setVisible (isCfg);

    const bool isGate = (currentTab == TabGate);
    gateGraph.setVisible (isGate);
    gateTypeButton.setVisible (isGate);
    gateActiveButton.setVisible (isGate);
    gateGrMeter.setVisible (isGate);
    gateThreshDial.setVisible (isGate);
    gateRangeDial.setVisible (isGate);
    gateAttackDial.setVisible (isGate);
    gateHoldDial.setVisible (isGate);
    gateReleaseDial.setVisible (isGate);
    gateFiltFreqDial.setVisible (isGate);
    gateSlopeDial.setVisible (isGate);
    gateFilterButton.setVisible (isGate);
    gateSoloButton.setVisible (isGate);
    gateKeySourceButton.setVisible (isGate);

    const bool isDyn = (currentTab == TabDyn);
    compGraph.setVisible (isDyn);
    compTypeCompButton.setVisible (isDyn);
    compTypeExpButton.setVisible (isDyn);
    compModeManButton.setVisible (isDyn);
    compModeAutoButton.setVisible (isDyn);
    compDetPeakButton.setVisible (isDyn);
    compDetRmsButton.setVisible (isDyn);
    compCharLinButton.setVisible (isDyn);
    compCharLogButton.setVisible (isDyn);
    compActiveButton.setVisible (isDyn);
    compGrMeter.setVisible (isDyn);
    compThreshDial.setVisible (isDyn);
    compRatioDial.setVisible (isDyn);
    compGainDial.setVisible (isDyn);
    compKneeDial.setVisible (isDyn);
    compAttackDial.setVisible (isDyn);
    compHoldDial.setVisible (isDyn);
    compReleaseDial.setVisible (isDyn);
    compMixDial.setVisible (isDyn);
    compFiltFreqDial.setVisible (isDyn);
    compSlopeDial.setVisible (isDyn);
    compFilterButton.setVisible (isDyn);
    compSoloButton.setVisible (isDyn);
    compKeySourceButton.setVisible (isDyn);

    const bool isEq = (currentTab == TabEq);
    eqGainDial.setVisible (isEq);
    eqFreqDial.setVisible (isEq);
    eqQDial.setVisible (isEq);
    eqActiveButton.setVisible (isEq);
    eqLoCutButton.setVisible (isEq);
    eqGraph.setVisible (isEq);
    eqResetButton.setVisible (isEq);
    eqModeButton.setVisible (isEq);
    eqRtaButton.setVisible (isEq);
    eqLoCutBottomButton.setVisible (isEq);
    for (auto& b : eqBandButtons) b.setVisible (isEq);

    const bool isSends = (currentTab == TabSends);
    for (auto& b : sendBusBadges)   b.setVisible (isSends);
    for (auto& l : sendValueLabels) l.setVisible (isSends);
    for (auto& f : sendFaders)      f.setVisible (isSends);
    for (auto& m : sendMuteButtons) m.setVisible (isSends);
    for (auto& t : sendTapButtons)  t.setVisible (isSends);

    const bool isNaming = (currentTab == TabNaming);
    nameEditor.setVisible (isNaming);
    for (auto& b : colourButtons) b.setVisible (isNaming);

    resized();
    repaint();
}

void DetailScreen::buttonClicked (juce::Button* b)
{
    if (b == &backToMixerButton)
    {
        if (onBack) onBack();
        return;
    }

    for (int i = 0; i < NumTabs; ++i)
    {
        if (b == &tabButtons[i])
        {
            currentTab = i;
            updateTabVisibility();
            return;
        }
    }

    if (b == &prevChannelButton)
    {
        if (currentStripIndex > 0)
            setSelectedStrip (currentStripType, currentStripIndex - 1);
        return;
    }
    if (b == &nextChannelButton)
    {
        if (currentStripIndex < getMaxStripIndex (currentStripType) - 1)
            setSelectedStrip (currentStripType, currentStripIndex + 1);
        return;
    }

    if (ignoreCallbacks) return;

    if (b == &phaseButton)
    {
        const bool on = phaseButton.getToggleState();
        if (currentStripType == ChannelStripComponent::StripType::InputChannel)
            engine.getMixerParams().channels[(size_t) currentStripIndex].polarity.setBool (on);
        else if (currentStripType == ChannelStripComponent::StripType::Aux)
            engine.getMixerParams().auxes[(size_t) currentStripIndex].polarity.setBool (on);
        else if (currentStripType == ChannelStripComponent::StripType::Matrix)
            engine.getMixerParams().matrices[(size_t) currentStripIndex].polarity.setBool (on);
    }
    else if (b == &linkButton)
    {
        if (currentStripType == ChannelStripComponent::StripType::InputChannel)
        {
            const int pairIdx = currentStripIndex / 2;
            const bool willLink = linkButton.getToggleState();
            engine.getMixerParams().channelPairLinked[(size_t) pairIdx].store (willLink, std::memory_order_relaxed);
            if (willLink)
            {
                const int chA = pairIdx * 2;
                const int chB = pairIdx * 2 + 1;
                auto& pA = engine.getMixerParams().channels[(size_t) chA];
                auto& pB = engine.getMixerParams().channels[(size_t) chB];
                pB.faderDb.set (pA.faderDb.get());
                pB.mute.set (pA.mute.get());
                pB.solo.set (pA.solo.get());
                pA.pan.set (-1.0f);
                pB.pan.set (1.0f);
                pB.trimDb.set (pA.trimDb.get());
                pB.dsp.hpfFreq.set (pA.dsp.hpfFreq.get());
                pB.dsp.hpfBypass.set (pA.dsp.hpfBypass.get());
                pB.dsp.eqLowFreq.set (pA.dsp.eqLowFreq.get());
                pB.dsp.eqLowGainDb.set (pA.dsp.eqLowGainDb.get());
                pB.dsp.eqLowQ.set (pA.dsp.eqLowQ.get());
                pB.dsp.eqMid1Freq.set (pA.dsp.eqMid1Freq.get());
                pB.dsp.eqMid1GainDb.set (pA.dsp.eqMid1GainDb.get());
                pB.dsp.eqMid1Q.set (pA.dsp.eqMid1Q.get());
                pB.dsp.eqMid2Freq.set (pA.dsp.eqMid2Freq.get());
                pB.dsp.eqMid2GainDb.set (pA.dsp.eqMid2GainDb.get());
                pB.dsp.eqMid2Q.set (pA.dsp.eqMid2Q.get());
                pB.dsp.eqHighFreq.set (pA.dsp.eqHighFreq.get());
                pB.dsp.eqHighGainDb.set (pA.dsp.eqHighGainDb.get());
                pB.dsp.eqHighQ.set (pA.dsp.eqHighQ.get());
                pB.dsp.gateThreshDb.set (pA.dsp.gateThreshDb.get());
                pB.dsp.gateRangeDb.set (pA.dsp.gateRangeDb.get());
                pB.dsp.compThreshDb.set (pA.dsp.compThreshDb.get());
                pB.dsp.compRatio.set (pA.dsp.compRatio.get());
            }
        }
        refreshFromModel();
    }
    else if (b == &sourceButton)
    {
        if (currentStripType == ChannelStripComponent::StripType::InputChannel)
        {
            juce::PopupMenu m;
            auto& cp = engine.getMixerParams().channels[(size_t) currentStripIndex];
            const int curSrc = cp.source.getInt();
            m.addItem (1, "OFF", true, curSrc < 0);
            for (int i = 0; i < (int) kMaxInputChannels; ++i)
                m.addItem (i + 2, "IN " + juce::String (i + 1), true, curSrc == i);
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&sourceButton),
                [this] (int res) {
                    if (res == 1) {
                        engine.getMixerParams().channels[(size_t) currentStripIndex].source.set (-1.0f);
                        refreshFromModel();
                    } else if (res > 1) {
                        engine.getMixerParams().channels[(size_t) currentStripIndex].source.set ((float) (res - 2));
                        refreshFromModel();
                    }
                });
        }
        else if (currentStripType == ChannelStripComponent::StripType::Aux)
        {
            juce::PopupMenu m;
            auto& ap = engine.getMixerParams().auxes[(size_t) currentStripIndex];
            const int curSrc = ap.source.getInt();
            for (int i = 0; i < 6; ++i)
                m.addItem (i + 1, "Aux " + juce::String (i + 1), true, curSrc == i);
            m.addItem (7, "USB L", true, curSrc == 6);
            m.addItem (8, "USB R", true, curSrc == 7);
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&sourceButton),
                [this] (int res) {
                    if (res >= 1 && res <= 8) {
                        engine.getMixerParams().auxes[(size_t) currentStripIndex].source.set ((float) (res - 1));
                        refreshFromModel();
                    }
                });
        }
    }
    else if (b == &delayButton)
    {
        if (auto* dsp = getCurrentDspParams())
            dsp->delayBypass.setBool (! delayButton.getToggleState());
    }
    else if (b == &lowCutButton || b == &eqLoCutButton || b == &eqLoCutBottomButton)
    {
        const bool active = b->getToggleState();
        if (auto* dsp = getCurrentDspParams())
            dsp->hpfBypass.setBool (! active);
        lowCutButton.setToggleState (active, juce::dontSendNotification);
        eqLoCutButton.setToggleState (active, juce::dontSendNotification);
        eqLoCutBottomButton.setToggleState (active, juce::dontSendNotification);
        eqGraph.repaint();
    }
    else if (b == &insertButton)
    {
        const int curSlot = getInsertSlotForCurrentStrip();
        juce::PopupMenu m;
        m.addItem (1, "Insert: OFF", true, curSlot == 0);
        for (int i = 1; i <= 8; ++i)
        {
            auto& fxSlot = engine.getMixerParams().fxSlots[(size_t) (i - 1)];
            const auto meta = getFxMetadata ((FxType) (int) fxSlot.fxType.getInt());
            m.addItem (i + 1, "FX " + juce::String (i) + " (" + meta.shortName + ")", true, curSlot == i);
        }
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&insertButton),
                         [this] (int res) {
                             if (res >= 1)
                             {
                                 setInsertSlotForCurrentStrip (res - 1);
                                 refreshFromModel();
                             }
                         });
    }
    else if (b == &mainBusHeaderButton)
    {
        const bool newState = ! getMainAssignForCurrentStrip();
        mainLrButton.setToggleState (newState, juce::dontSendNotification);
        setMainAssignForCurrentStrip (newState);
        if (newState && engine.getMixerParams().main.mute.getBool())
            engine.getMixerParams().main.mute.setBool (false);
    }
    else if (b == &mainLrButton)
    {
        const bool on = mainLrButton.getToggleState();
        setMainAssignForCurrentStrip (on);
        if (on && engine.getMixerParams().main.mute.getBool())
            engine.getMixerParams().main.mute.setBool (false);
    }
    else if (b == &monoCenterButton)
    {
        setMonoAssignForCurrentStrip (monoCenterButton.getToggleState());
    }
    else if (b == &mainMuteButton)
    {
        setMuteForCurrentStrip (mainMuteButton.getToggleState());
    }
    else if (b == &mainSoloButton)
    {
        setSoloForCurrentStrip (mainSoloButton.getToggleState());
    }
    else if (b == &gateActiveButton)
    {
        if (auto* dsp = getCurrentDspParams())
            dsp->gateBypass.setBool (! gateActiveButton.getToggleState());
    }
    else if (b == &compActiveButton)
    {
        if (auto* dsp = getCurrentDspParams())
            dsp->compBypass.setBool (! compActiveButton.getToggleState());
    }
    else if (b == &eqActiveButton)
    {
        const bool active = eqActiveButton.getToggleState();
        if (auto* dsp = getCurrentDspParams())
            dsp->eqBypass.setBool (! active);
        eqActiveButton.setButtonText (active ? "EQ IN" : "EQ BYP");
        eqGraph.repaint();
    }
    else if (b == &eqRtaButton)
    {
        const bool active = eqRtaButton.getToggleState();
        eqGraph.setRtaEnabled (active);
    }
    else if (b == &eqModeButton)
    {
        if (auto* dsp = getCurrentDspParams())
        {
            if (selectedEqBand == 1)
            {
                const int cur = dsp->eqLowType.getInt();
                const int next = (cur == 1) ? 0 : 1; // 1 = LowShelf, 0 = Bell
                dsp->eqLowType.set ((float) next);
                eqModeButton.setButtonText (next == 1 ? "LShv" : "PEQ");
                eqGraph.repaint();
            }
            else if (selectedEqBand == 4)
            {
                const int cur = dsp->eqHighType.getInt();
                const int next = (cur == 2) ? 0 : 2; // 2 = HighShelf, 0 = Bell
                dsp->eqHighType.set ((float) next);
                eqModeButton.setButtonText (next == 2 ? "HShv" : "PEQ");
                eqGraph.repaint();
            }
        }
    }
    else if (b == &eqResetButton)
    {
        if (auto* dsp = getCurrentDspParams())
        {
            dsp->eqLowGainDb.set (0.0f);
            dsp->eqMid1GainDb.set (0.0f);
            dsp->eqMid2GainDb.set (0.0f);
            dsp->eqHighGainDb.set (0.0f);
            dsp->eqLowQ.set (0.707f);
            dsp->eqMid1Q.set (1.0f);
            dsp->eqMid2Q.set (1.0f);
            dsp->eqHighQ.set (0.707f);
            eqGraph.repaint();
            updateEqSelectedBandDialValues();
        }
    }
    else
    {
        // Band buttons: LOW, LOW2, LOMID, HIMID, HIGH2, HIGH
        const int mapBand[6] = { 1, 1, 2, 3, 4, 4 };
        for (int i = 0; i < 6; ++i)
        {
            if (b == &eqBandButtons[i])
            {
                selectedEqBand = mapBand[i];
                eqGraph.setSelectedBand (selectedEqBand);
                updateEqSelectedBandDialValues();
                break;
            }
        }
    }

    for (int i = 0; i < 6; ++i)
    {
        if (b == &muteGroupButtons[i])
        {
            int mask = getMuteGroupMaskForCurrentStrip();
            if (muteGroupButtons[i].getToggleState())
                mask |= (1 << i);
            else
                mask &= ~(1 << i);
            setMuteGroupMaskForCurrentStrip (mask);
            break;
        }
    }

    for (int i = 0; i < 8; ++i)
    {
        if (b == &dcaButtons[i])
        {
            int mask = getDcaMaskForCurrentStrip();
            if (dcaButtons[i].getToggleState())
                mask |= (1 << i);
            else
                mask &= ~(1 << i);
            setDcaMaskForCurrentStrip (mask);
            break;
        }
    }

    for (int i = 0; i < 16; ++i)
    {
        if (b == &sendMuteButtons[i])
        {
            if (currentStripType == ChannelStripComponent::StripType::InputChannel)
            {
                auto& ch = engine.getMixerParams().channels[(size_t) currentStripIndex];
                ch.sendMutes[(size_t) i].setBool (sendMuteButtons[i].getToggleState());
            }
            return;
        }
    }

    for (int t = 0; t < 8; ++t)
    {
        if (b == &sendTapButtons[t])
        {
            if (currentStripType == ChannelStripComponent::StripType::InputChannel)
            {
                auto& ch = engine.getMixerParams().channels[(size_t) currentStripIndex];
                const int currentMode = std::clamp (ch.sendTaps[(size_t) t].getInt(), 0, 5);

                juce::PopupMenu menu;
                const char* menuNames[6] = { "In/Low Cut", "Pre EQ", "Post EQ", "Pre Fader", "Post Fader", "Group" };
                for (int m = 0; m < 6; ++m)
                    menu.addItem (m + 1, menuNames[m], true, (m == currentMode));

                menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&sendTapButtons[t]),
                    [this, t] (int result) {
                        if (result >= 1 && result <= 6)
                        {
                            const int newMode = result - 1;
                            const char* btnNames[6] = { "IN/LOW CUT", "PRE EQ", "POST EQ", "PRE FADER", "POST FADER", "GROUP" };
                            auto& ch2 = engine.getMixerParams().channels[(size_t) currentStripIndex];
                            ch2.sendTaps[(size_t) t].set ((float) newMode);
                            sendTapButtons[t].setButtonText (btnNames[newMode]);
                        }
                    });
            }
            return;
        }
    }
}

void DetailScreen::sliderValueChanged (juce::Slider* s)
{
    if (ignoreCallbacks) return;

    for (int i = 0; i < 16; ++i)
    {
        if (s == &sendFaders[i])
        {
            if (currentStripType == ChannelStripComponent::StripType::InputChannel)
            {
                const float db = faderPosToDb ((float) sendFaders[i].getValue());
                auto& ch = engine.getMixerParams().channels[(size_t) currentStripIndex];
                ch.sends[(size_t) i].set (db);
                sendValueLabels[i].setText (db <= -89.0f ? "-oo" : juce::String (db, 1), juce::dontSendNotification);
            }
            return;
        }
    }

    if (s == &faderSlider)
    {
        const float db = faderPosToDb ((float) faderSlider.getValue());
        setFaderForCurrentStrip (db);
        faderDbLabel.setText (db <= kFaderMinDb ? "-inf" : juce::String (db, 1) + " dB", juce::dontSendNotification);
    }
    else if (s == &mainFader)
    {
        const float db = faderPosToDb ((float) mainFader.getValue());
        auto& mp = engine.getMixerParams().main;
        mp.faderDb.set (db);
    }
}

DetailScreen::ConfigLayout DetailScreen::computeConfigLayout() const
{
    ConfigLayout layout;
    const int totalW = getWidth();
    const int totalH = getHeight();
    const bool compactH = (totalH < 560);
    const int bankH = compactH ? 40 : 48;
    const int subtabH = compactH ? 24 : 28;
    const int tabStartX = 94;
    const int subtabY = bankH + 2;

    const int contentX = tabStartX;
    const int contentY = subtabY + subtabH + 3;
    const int contentW = totalW - contentX - 6;
    const int contentH = totalH - contentY - 4;

    layout.cardRect = juce::Rectangle<int> (contentX, contentY, contentW, contentH).reduced (3);

    const bool isInput = (currentStripType == ChannelStripComponent::StripType::InputChannel);
    const bool isMtx   = (currentStripType == ChannelStripComponent::StripType::Matrix);

    const int pad = 8;
    const int cardX = layout.cardRect.getX() + pad;
    const int cardY = layout.cardRect.getY() + pad;
    const int availW = layout.cardRect.getWidth() - 2 * pad;
    const int availH = layout.cardRect.getHeight() - 2 * pad;

    const int mainBoxW = isMtx ? 0 : std::clamp ((int) (availW * 0.155f), 125, 155);
    const int leftAreaW = isMtx ? availW : (availW - mainBoxW - pad);

    // Row 1: Flow Box
    const int flowH = std::clamp ((int) (availH * 0.23f), 52, 64);
    layout.flowBox = juce::Rectangle<int> (cardX, cardY, leftAreaW, flowH);

    // Remaining parameters area below Flow Box
    const int paramY = cardY + flowH + pad;
    const int paramH = availH - flowH - pad;

    // Widths inside leftAreaW:
    const int utilsW = std::clamp ((int) (leftAreaW * 0.15f), 98, 122);
    const int remainingW = leftAreaW - utilsW - pad;
    const int preampW = isInput ? std::clamp ((int) (remainingW * 0.58f), 310, 430)
                                : std::clamp ((int) (remainingW * 0.44f), 220, 320);
    const int dcaAutoW = remainingW - preampW - pad;

    layout.preampBox = juce::Rectangle<int> (cardX, paramY, preampW, paramH);

    if (! isMtx)
    {
        if (isInput)
        {
            const int dcaH = (paramH - pad) / 2;
            layout.dcaBox = juce::Rectangle<int> (cardX + preampW + pad, paramY, dcaAutoW, dcaH);
            layout.autoBox = juce::Rectangle<int> (cardX + preampW + pad, paramY + dcaH + pad, dcaAutoW, paramH - dcaH - pad);
        }
        else
        {
            layout.dcaBox = juce::Rectangle<int> (cardX + preampW + pad, paramY, dcaAutoW, paramH);
            layout.autoBox = {};
        }
        layout.dcaColW = (int) (dcaAutoW * 0.54f);
    }
    else
    {
        layout.dcaBox = {};
        layout.autoBox = {};
        layout.dcaColW = 0;
    }

    const int utilsX = cardX + preampW + pad + (layout.dcaBox.isEmpty() ? 0 : (dcaAutoW + pad));
    const int actualUtilsW = isMtx ? (leftAreaW - (utilsX - cardX)) : utilsW;
    layout.utilsBox = juce::Rectangle<int> (utilsX, paramY, actualUtilsW, paramH);

    if (! isMtx)
        layout.mainBox = juce::Rectangle<int> (layout.cardRect.getRight() - pad - mainBoxW, cardY, mainBoxW, availH);
    else
        layout.mainBox = {};

    return layout;
}

void DetailScreen::paint (juce::Graphics& g)
{
    g.fillAll (ConsoleLookAndFeel::bgDark);

    const auto cfgLayout = computeConfigLayout();
    const auto cardRect = cfgLayout.cardRect.toFloat();

    // 1. Draw Section Card Background (dark slate charcoal with 1px border)
    g.setColour (juce::Colour (0xff1e2229));
    g.fillRoundedRectangle (cardRect, 4.0f);
    g.setColour (juce::Colour (0xff323a46));
    g.drawRoundedRectangle (cardRect, 4.0f, 1.0f);

    // 2. Draw Left Strip Fader Scale Markings
    const float faderTop = (float) faderSlider.getY() + 8.0f;
    const float faderBottom = (float) faderSlider.getBottom() - 8.0f;
    const float faderSpan = std::max (10.0f, faderBottom - faderTop);

    const struct { float db; const char* txt; } marks[] = {
        { 10.0f, "10" }, { 5.0f, "5" }, { 0.0f, "0" }, { -5.0f, "-5" },
        { -10.0f, "-10" }, { -20.0f, "-20" }, { -30.0f, "-30" }, { -40.0f, "-40" },
        { -50.0f, "-50" }, { -70.0f, "-70" }, { -90.0f, "-oo" }
    };

    g.setFont (juce::FontOptions (7.5f));
    g.setColour (juce::Colour (0xff8a94a2));
    for (const auto& m : marks)
    {
        const float pos = faderDbToPos (m.db);
        const float y = faderBottom - pos * faderSpan;
        g.drawText (m.txt, 4, (int) y - 5, 18, 10, juce::Justification::centredRight);
        g.drawHorizontalLine ((int) y, 24.0f, 27.0f);
    }

    // 3. Tab-Specific Card Dividers / Box Outlines
    if (currentTab == TabConfig)
    {
        const bool isInput = (currentStripType == ChannelStripComponent::StripType::InputChannel);
        const bool isAux   = (currentStripType == ChannelStripComponent::StripType::Aux);
        const bool isFx    = (currentStripType == ChannelStripComponent::StripType::FxReturn);
        const bool isBus   = (currentStripType == ChannelStripComponent::StripType::Bus);
        const bool isMtx   = (currentStripType == ChannelStripComponent::StripType::Matrix);

        const auto l = cfgLayout;

        // 1. Flow Box (Top)
        g.setColour (juce::Colour (0xff242932));
        g.fillRoundedRectangle (l.flowBox.toFloat(), 4.0f);
        g.setColour (juce::Colour (0xff3d4756));
        g.drawRoundedRectangle (l.flowBox.toFloat(), 4.0f, 1.0f);

        g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
        g.setColour (juce::Colour (0xff8a94a2));
        const char* flowTitle = isInput ? "SIGNAL FLOW & INPUT PATCH" :
                                (isAux ? "AUX SIGNAL FLOW" :
                                (isFx ? "FX RETURN SIGNAL FLOW" :
                                (isBus ? "MIX BUS SIGNAL FLOW" : "MATRIX SIGNAL FLOW")));
        g.drawText (flowTitle, l.flowBox.getX() + 10, l.flowBox.getY() + 4, 200, 14, juce::Justification::centredLeft);

        if (isInput)
        {
            const float srcLineY = (float) sourceButton.getY() + (float) sourceButton.getHeight() * 0.5f;
            g.setColour (juce::Colour (0xff00b0ff));
            g.drawLine ((float) sourceButton.getRight(), srcLineY, (float) signalFlow.getX() + 8.0f, srcLineY, 2.0f);
        }

        // 2. Preamp Box
        g.setColour (juce::Colour (0xff242932));
        g.fillRoundedRectangle (l.preampBox.toFloat(), 4.0f);
        g.setColour (juce::Colour (0xff3d4756));
        g.drawRoundedRectangle (l.preampBox.toFloat(), 4.0f, 1.0f);

        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.setColour (juce::Colour (0xffb0bec5));
        const char* preampTitle = isInput ? "PREAMP / GAIN & FILTERS" :
                                  (isAux ? "PREAMP / GAIN" :
                                  (isFx ? "FX RETURN CONFIG" :
                                  (isBus ? "MIX BUS CONFIG" : "MATRIX CONFIG")));
        g.drawText (preampTitle, l.preampBox.getX() + 10, l.preampBox.getY() + 6, 200, 14, juce::Justification::centredLeft);

        // 3. DCA Box
        if (! l.dcaBox.isEmpty())
        {
            g.setColour (juce::Colour (0xff242932));
            g.fillRoundedRectangle (l.dcaBox.toFloat(), 4.0f);
            g.setColour (juce::Colour (0xff3d4756));
            g.drawRoundedRectangle (l.dcaBox.toFloat(), 4.0f, 1.0f);

            g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
            g.setColour (juce::Colour (0xffb0bec5));
            g.drawText ("DCA GROUPS (1-8)", l.dcaBox.getX() + 8, l.dcaBox.getY() + 5, l.dcaColW, 14, juce::Justification::centredLeft);
            g.drawText ("MUTE GROUPS (1-6)", l.dcaBox.getX() + l.dcaColW + 6, l.dcaBox.getY() + 5, l.dcaBox.getWidth() - l.dcaColW - 12, 14, juce::Justification::centredLeft);

            // Divider inside DCA box
            g.setColour (juce::Colour (0xff2d3440));
            g.drawVerticalLine ((float) (l.dcaBox.getX() + l.dcaColW), (float) l.dcaBox.getY() + 5.0f, (float) l.dcaBox.getBottom() - 5.0f);
        }

        // 4. Automix Box
        if (! l.autoBox.isEmpty())
        {
            g.setColour (juce::Colour (0xff242932));
            g.fillRoundedRectangle (l.autoBox.toFloat(), 4.0f);
            g.setColour (juce::Colour (0xff3d4756));
            g.drawRoundedRectangle (l.autoBox.toFloat(), 4.0f, 1.0f);

            g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
            g.setColour (juce::Colour (0xffb0bec5));
            g.drawText ("AUTOMIXING (DUGAN)", l.autoBox.getX() + 10, l.autoBox.getY() + 5, l.autoBox.getWidth() - 20, 14, juce::Justification::centredLeft);
        }

        // 5. Utils Box
        g.setColour (juce::Colour (0xff242932));
        g.fillRoundedRectangle (l.utilsBox.toFloat(), 4.0f);
        g.setColour (juce::Colour (0xff3d4756));
        g.drawRoundedRectangle (l.utilsBox.toFloat(), 4.0f, 1.0f);

        g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
        g.setColour (juce::Colour (0xffb0bec5));
        const char* utilsTitle = (isInput || isBus || isMtx) ? "INSERT / UTILS" : "UTILS";
        g.drawText (utilsTitle, l.utilsBox.getX(), l.utilsBox.getY() + 5, l.utilsBox.getWidth(), 14, juce::Justification::centred);

        g.setFont (juce::FontOptions (8.0f, juce::Font::bold));
        g.setColour (juce::Colour (0xff788494));
        if (isInput || isBus || isMtx)
            g.drawText ("INSERT POINT", l.utilsBox.getX() + 4, l.utilsBox.getY() + 50, l.utilsBox.getWidth() - 8, 12, juce::Justification::centred);

        // 6. Main Box on Right
        if (! l.mainBox.isEmpty())
        {
            g.setColour (juce::Colour (0xff242932));
            g.fillRoundedRectangle (l.mainBox.toFloat(), 4.0f);
            g.setColour (juce::Colour (0xff4a5465));
            g.drawRoundedRectangle (l.mainBox.toFloat(), 4.0f, 1.2f);

            // Main Header Accent Stripe
            g.setColour (juce::Colour (0xffffa000));
            g.fillRect ((float) l.mainBox.getX() + 10.0f, (float) l.mainBox.getY() + 3.0f, (float) l.mainBox.getWidth() - 20.0f, 2.5f);

            g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            g.setColour (juce::Colours::white);
            g.drawText ("MAIN BUS", l.mainBox.getX(), l.mainBox.getY() + 7, l.mainBox.getWidth(), 16, juce::Justification::centred);

            // Main Fader Scale Markings
            const float mfTop = (float) mainFader.getY() + 8.0f;
            const float mfBottom = (float) mainFader.getBottom() - 8.0f;
            const float mfSpan = std::max (10.0f, mfBottom - mfTop);
            const struct { float db; const char* txt; } fMarks[] = {
                { 10.0f, "+10" }, { 0.0f, "0" }, { -5.0f, "-5" }, { -10.0f, "-10" },
                { -20.0f, "-20" }, { -30.0f, "-30" }, { -50.0f, "-50" }, { -90.0f, "-oo" }
            };
            g.setFont (juce::FontOptions (7.5f));
            g.setColour (juce::Colour (0xff788494));
            for (const auto& m : fMarks)
            {
                const float pos = faderDbToPos (m.db);
                const float my = mfBottom - pos * mfSpan;
                const float lineX = (float) mainFader.getX() - 3.0f;
                g.drawHorizontalLine ((int) my, lineX - 4.0f, lineX);
                g.drawText (m.txt, (int) (lineX - 24.0f), (int) (my - 5.0f), 20, 10, juce::Justification::centredRight);
            }
        }
    }
    else if (currentTab == TabGate || currentTab == TabDyn)
    {
        const float leftX = cardRect.getX() + 10.0f;
        const float leftW = std::clamp (cardRect.getWidth() * 0.44f, 320.0f, 420.0f);
        const float boxH = std::floor ((cardRect.getHeight() - 26.0f) / 3.0f);

        // Box 1
        const juce::Rectangle<float> box1 (leftX, cardRect.getY() + 8.0f, leftW, boxH);
        g.setColour (juce::Colour (0xff242932));
        g.fillRoundedRectangle (box1, 3.0f);
        g.setColour (juce::Colour (0xff3d4756));
        g.drawRoundedRectangle (box1, 3.0f, 1.0f);

        // Box 2
        const juce::Rectangle<float> box2 (leftX, cardRect.getY() + 12.0f + boxH, leftW, boxH);
        g.setColour (juce::Colour (0xff242932));
        g.fillRoundedRectangle (box2, 3.0f);
        g.setColour (juce::Colour (0xff3d4756));
        g.drawRoundedRectangle (box2, 3.0f, 1.0f);

        // Box 3
        const juce::Rectangle<float> box3 (leftX, cardRect.getY() + 16.0f + 2.0f * boxH, leftW, boxH);
        g.setColour (juce::Colour (0xff242932));
        g.fillRoundedRectangle (box3, 3.0f);
        g.setColour (juce::Colour (0xff3d4756));
        g.drawRoundedRectangle (box3, 3.0f, 1.0f);

        // Section header labels on the left boxes
        g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        g.setColour (juce::Colour (0xff90a4ae));
        if (currentTab == TabGate)
        {
            g.drawText ("THRESHOLD & RANGE", (int) box1.getX() + 12, (int) box1.getY() + 3, 180, 14, juce::Justification::left);
            g.drawText ("ENVELOPE DYNAMICS", (int) box2.getX() + 12, (int) box2.getY() + 3, 180, 14, juce::Justification::left);
            g.drawText ("SIDECHAIN FILTER", (int) box3.getX() + 12, (int) box3.getY() + 3, 180, 14, juce::Justification::left);
        }
        else
        {
            g.drawText ("THRESHOLD, RATIO & GAIN", (int) box1.getX() + 12, (int) box1.getY() + 3, 200, 14, juce::Justification::left);
            g.drawText ("ENVELOPE & PARALLEL MIX", (int) box2.getX() + 12, (int) box2.getY() + 3, 200, 14, juce::Justification::left);
            g.drawText ("SIDECHAIN FILTER", (int) box3.getX() + 12, (int) box3.getY() + 3, 200, 14, juce::Justification::left);
        }

        // Display Monitor Frame on the RIGHT (Full Width to Right Margin)
        const float rightX = leftX + leftW + 10.0f;
        const float rightW = cardRect.getRight() - rightX - 8.0f;
        const juce::Rectangle<float> dispBox (rightX, cardRect.getY() + 8.0f, rightW, cardRect.getHeight() - 16.0f);
        g.setColour (juce::Colour (0xff161920));
        g.fillRoundedRectangle (dispBox, 4.0f);
        g.setColour (juce::Colour (0xff3d4756));
        g.drawRoundedRectangle (dispBox, 4.0f, 1.0f);

        g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
        g.setColour (juce::Colour (0xff78909c));
        g.drawText ("TRANSFER CURVE / MONITOR", (int) dispBox.getX(), (int) dispBox.getY() + 4, (int) dispBox.getWidth(), 14, juce::Justification::centred);
    }
    else if (currentTab == TabEq)
    {
        const bool compactEq = (cardRect.getHeight() < 340.0f);
        const float leftBoxW = compactEq ? 190.0f : 124.0f;
        const juce::Rectangle<float> eqLeftBox (cardRect.getX() + 10.0f, cardRect.getY() + 8.0f, leftBoxW, cardRect.getHeight() - 16.0f);
        g.setColour (juce::Colour (0xff242932));
        g.fillRoundedRectangle (eqLeftBox, 3.0f);
        g.setColour (juce::Colour (0xff3d4756));
        g.drawRoundedRectangle (eqLeftBox, 3.0f, 1.0f);

        // Header for band selection on the right
        const float bandHeaderX = cardRect.getRight() - 6.0f * 50.0f - 24.0f;
        g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        g.setColour (juce::Colour (0xff90a4ae));
        g.drawText ("SELECT", (int) bandHeaderX, (int) cardRect.getBottom() - 44, 6 * 50, 14, juce::Justification::centred);
    }
    else if (currentTab == TabSends)
    {
        const int numCols = 16;
        const float faderTopY = (float) sendFaders[0].getY() + 8.0f;
        const float faderBottomY = (float) sendFaders[0].getBottom() - 8.0f;
        const float faderRangeH = std::max (10.0f, faderBottomY - faderTopY);

        const struct { float db; const char* txt; } marks[] = {
            { 10.0f, "10" }, { 5.0f, "5" }, { 0.0f, "0" }, { -5.0f, "-5" },
            { -10.0f, "-10" }, { -20.0f, "-20" }, { -30.0f, "-30" }, { -40.0f, "-40" },
            { -50.0f, "-50" }, { -70.0f, "-70" }, { -90.0f, "-oo" }
        };

        // Subtle column background cards
        for (int i = 0; i < numCols; ++i)
        {
            const auto colBounds = juce::Rectangle<float> ((float) (sendBusBadges[i].getX() - 1),
                                                           (float) sendBusBadges[i].getY(),
                                                           (float) (sendBusBadges[i].getWidth() + 2),
                                                           (float) (sendMuteButtons[i].getBottom() - sendBusBadges[i].getY() + 4));

            g.setColour (juce::Colour (i % 2 == 0 ? 0x221a1f26 : 0x18141820));
            g.fillRoundedRectangle (colBounds, 4.0f);
            g.setColour (juce::Colour (0x28384252));
            g.drawRoundedRectangle (colBounds, 4.0f, 1.0f);

            // Fader track background slot
            const float trackW = 4.0f;
            const float trackX = (float) sendFaders[i].getBounds().getCentreX() - trackW * 0.5f;
            juce::Rectangle<float> trackRect (trackX, faderTopY, trackW, faderRangeH);
            g.setColour (juce::Colour (0xff0a0d10));
            g.fillRoundedRectangle (trackRect, 2.0f);
        }

        // Horizontal tick lines across each fader column
        g.setFont (juce::FontOptions (7.5f));
        for (const auto& m : marks)
        {
            const float pos = faderDbToPos (m.db);
            const float y = faderBottomY - pos * faderRangeH;
            const bool isUnity = (std::abs (m.db) < 0.01f);

            for (int i = 0; i < numCols; ++i)
            {
                const float fX = (float) sendFaders[i].getX();
                const float fW = (float) sendFaders[i].getWidth();
                const float leftTick = fX + 3.0f;
                const float rightTick = fX + fW - 3.0f;

                if (isUnity)
                {
                    g.setColour (juce::Colour (0x8000d2df)); // Cyan highlight line at 0 dB
                    g.drawHorizontalLine ((int) y, leftTick, rightTick);
                }
                else
                {
                    g.setColour (juce::Colour (0x35607080));
                    g.drawHorizontalLine ((int) y, leftTick + 3.0f, rightTick - 3.0f);
                }
            }
        }
    }
}

void DetailScreen::resized()
{
    const int totalW = getWidth();
    const int totalH = getHeight();

    const bool compactH = (totalH < 560);
    const int bankH = compactH ? 40 : 48;
    const int subtabH = compactH ? 24 : 28;

    // 1. Top Bank Selector Bar
    bankSelectorBar.setBounds (0, 0, totalW, bankH);
    backToMixerButton.setBounds (3, 3, 86, bankH - 6);
    backToMixerButton.toFront (false);

    // 2. Sub-Tab Buttons Row
    const int tabStartX = 94;
    const int tabAvailW = totalW - tabStartX - 8;
    const int tabW = std::min (105, tabAvailW / NumTabs);
    const int subtabY = bankH + 2;

    for (int i = 0; i < NumTabs; ++i)
        tabButtons[i].setBounds (tabStartX + i * tabW, subtabY, tabW - 2, subtabH);

    // 3. Left Preview Strip
    const int stripX = 4;
    const int stripW = 86;
    const int scribbleH = compactH ? 22 : 26;
    const int panH = compactH ? 22 : 26;
    const int numH = compactH ? 18 : 22;
    const int navBtnH = compactH ? 22 : 24;

    scribbleName.setBounds (stripX, subtabY, stripW, scribbleH);
    panIndicator.setBounds (stripX, scribbleName.getBottom() + 2, stripW, panH);
    channelNumLabel.setBounds (stripX + 4, panIndicator.getBottom() + 1, 30, numH);

    const int faderTop = channelNumLabel.getBottom() + 2;
    const int faderBottom = totalH - navBtnH - 6;
    faderSlider.setBounds (stripX + 22, faderTop, 36, faderBottom - faderTop);
    channelMeter.setBounds (stripX + 60, faderTop + 2, 16, faderBottom - faderTop - 4);

    prevChannelButton.setBounds (stripX, totalH - navBtnH - 2, (stripW - 4) / 2, navBtnH);
    nextChannelButton.setBounds (stripX + (stripW - 4) / 2 + 4, totalH - navBtnH - 2, (stripW - 4) / 2, navBtnH);

    // 4. Center Content Area
    const int contentX = tabStartX;
    const int contentY = subtabY + subtabH + 3;
    const int contentW = totalW - contentX - 6;
    const int contentH = totalH - contentY - 4;

    const auto cfgLayout = computeConfigLayout();

    if (currentTab == TabConfig)
    {
        const bool isInput = (currentStripType == ChannelStripComponent::StripType::InputChannel);
        const bool isAux   = (currentStripType == ChannelStripComponent::StripType::Aux);
        const bool isMtx   = (currentStripType == ChannelStripComponent::StripType::Matrix);

        const auto flowBox = cfgLayout.flowBox;
        const auto preampBox = cfgLayout.preampBox;
        const auto dcaBox = cfgLayout.dcaBox;
        const auto autoBox = cfgLayout.autoBox;
        const auto utilsBox = cfgLayout.utilsBox;
        const auto mainBox = cfgLayout.mainBox;

        // 1. Flow Box (Signal Flow & Source button)
        if (isInput || isAux)
        {
            sourceButton.setBounds (flowBox.getX() + 10, flowBox.getY() + (flowBox.getHeight() - 26) / 2, 64, 26);
            signalFlow.setBounds (flowBox.getX() + 80, flowBox.getY() + 4, flowBox.getWidth() - 88, flowBox.getHeight() - 8);
        }
        else
        {
            signalFlow.setBounds (flowBox.getX() + 8, flowBox.getY() + 4, flowBox.getWidth() - 16, flowBox.getHeight() - 8);
        }

        // 2. Preamp Box
        const int pX = preampBox.getX();
        const int pY = preampBox.getY();
        const int pW = preampBox.getWidth();
        const int pH = preampBox.getHeight();

        if (isInput)
        {
            const int btnW = std::clamp ((pW - 24) / 5, 52, 66);
            const int btnH = std::clamp ((pH - 36) / 3, 20, 26);
            const int btnGap = std::clamp ((pH - 22 - 3 * btnH) / 3, 3, 7);
            linkButton.setBounds (pX + 10, pY + 22, btnW, btnH);
            phaseButton.setBounds (pX + 10, linkButton.getBottom() + btnGap, btnW, btnH);
            phantom48vButton.setBounds (pX + 10, phaseButton.getBottom() + btnGap, btnW, btnH);

            const int dialAreaW = pW - btnW - 26;
            const int dialColW = dialAreaW / 3;
            const int dialX1 = pX + 10 + btnW + 8;
            const int dialSize = std::clamp (std::min (dialColW - 18, pH - 30), 52, 88);
            const int dialY = pY + 20 + (pH - 24 - dialSize) / 2;
            gainDial.setBounds (dialX1, dialY, dialSize, dialSize);
            preampMeter.setBounds (dialX1 + dialSize + 4, dialY + 2, 14, dialSize - 4);

            const int dialX2 = dialX1 + dialColW;
            const int subDialSize = std::clamp (std::min (dialColW - 6, pH - 52), 46, 76);
            delayDial.setBounds (dialX2 + (dialColW - subDialSize) / 2, pY + 18, subDialSize, subDialSize);
            const int tglH = std::clamp (pH - subDialSize - 26, 18, 24);
            delayButton.setBounds (dialX2 + (dialColW - 58) / 2, delayDial.getBottom() + 3, 58, tglH);

            const int dialX3 = dialX2 + dialColW;
            lowCutDial.setBounds (dialX3 + (dialColW - subDialSize) / 2, pY + 18, subDialSize, subDialSize);
            lowCutButton.setBounds (dialX3 + (dialColW - 58) / 2, lowCutDial.getBottom() + 3, 58, tglH);
        }
        else if (isAux)
        {
            const int btnW = 64;
            const int btnH = std::clamp ((pH - 36) / 3, 20, 26);
            linkButton.setBounds (pX + 12, pY + 26, btnW, btnH);
            phaseButton.setBounds (pX + 12, linkButton.getBottom() + 6, btnW, btnH);
            phantom48vButton.setBounds (pX + 12, phaseButton.getBottom() + 6, btnW, btnH);

            const int dialSize = std::clamp (pH - 32, 54, 95);
            gainDial.setBounds (pX + btnW + 24, pY + 20 + (pH - 24 - dialSize) / 2, dialSize, dialSize);
            preampMeter.setBounds (pX + btnW + 24 + dialSize + 6, pY + 24, 16, dialSize - 8);
        }
        else if (isMtx)
        {
            linkButton.setBounds (pX + 14, pY + 30, 72, 28);
            phaseButton.setBounds (pX + 94, pY + 30, 72, 28);
        }
        else // Bus, FxReturn
        {
            linkButton.setBounds (pX + 14, pY + 30, 72, 28);
        }

        // 3. DCA & Mute Groups Box
        if (! dcaBox.isEmpty())
        {
            const int dcaColW = cfgLayout.dcaColW;
            const int mgColW = dcaBox.getWidth() - dcaColW;
            const int btnH = std::clamp ((dcaBox.getHeight() - 28) / 2, 18, 24);
            const int bStartY = dcaBox.getY() + 22;

            const int dcaBtnW = std::clamp ((dcaColW - 20) / 4, 20, 36);
            for (int i = 0; i < 4; ++i)
                dcaButtons[i].setBounds (dcaBox.getX() + 8 + i * (dcaBtnW + 3), bStartY, dcaBtnW, btnH);
            for (int i = 4; i < 8; ++i)
                dcaButtons[i].setBounds (dcaBox.getX() + 8 + (i - 4) * (dcaBtnW + 3), bStartY + btnH + 3, dcaBtnW, btnH);

            const int mgBtnW = std::clamp ((mgColW - 16) / 3, 22, 40);
            const int mgStartX = dcaBox.getX() + dcaColW + 6;
            for (int i = 0; i < 3; ++i)
                muteGroupButtons[i].setBounds (mgStartX + i * (mgBtnW + 3), bStartY, mgBtnW, btnH);
            for (int i = 3; i < 6; ++i)
                muteGroupButtons[i].setBounds (mgStartX + (i - 3) * (mgBtnW + 3), bStartY + btnH + 3, mgBtnW, btnH);
        }

        // 4. Automix Box
        if (! autoBox.isEmpty())
        {
            const int autoBtnW = std::clamp ((autoBox.getWidth() - 95) / 3, 28, 44);
            const int autoBtnH = std::clamp (autoBox.getHeight() - 28, 20, 25);
            automixOffButton.setBounds (autoBox.getX() + 10, autoBox.getY() + 22, autoBtnW, autoBtnH);
            automixXButton.setBounds (automixOffButton.getRight() + 4, autoBox.getY() + 22, autoBtnW, autoBtnH);
            automixYButton.setBounds (automixXButton.getRight() + 4, autoBox.getY() + 22, autoBtnW, autoBtnH);

            const int weightSize = std::clamp (autoBox.getHeight() - 24, 44, 76);
            weightingDial.setBounds (autoBox.getRight() - weightSize - 10, autoBox.getY() + 18, weightSize, weightSize);
        }

        // 5. Utils Box
        const int uX = utilsBox.getX();
        const int uY = utilsBox.getY();
        const int uW = utilsBox.getWidth();
        insertButton.setBounds (uX + 8, uY + 22, uW - 16, 26);
        const int clipY = isInput || isMtx ? 74 : 26;
        copyButton.setBounds (uX + 8, uY + clipY, uW - 16, 24);
        pasteButton.setBounds (uX + 8, copyButton.getBottom() + 4, uW - 16, 24);

        // 6. Main Box on Right
        if (! mainBox.isEmpty())
        {
            mainBusHeaderButton.setBounds (mainBox.getX() + 10, mainBox.getY() + 6, mainBox.getWidth() - 20, 20);
            mainLrButton.setBounds (mainBox.getX() + 10, mainBox.getY() + 28, mainBox.getWidth() - 20, 24);
            monoCenterButton.setBounds (mainBox.getX() + 10, mainLrButton.getBottom() + 3, mainBox.getWidth() - 20, 22);

            const int muteW = (mainBox.getWidth() - 24) / 2;
            const int muteY = mainBox.getBottom() - 30;
            mainMuteButton.setBounds (mainBox.getX() + 10, muteY, muteW, 25);
            mainSoloButton.setBounds (mainBox.getX() + 10 + muteW + 4, muteY, muteW, 25);

            const int fTop = monoCenterButton.getBottom() + 5;
            const int fBottom = muteY - 5;
            mainFader.setBounds (mainBox.getX() + (mainBox.getWidth() - 36) / 2 + 8, fTop, 36, fBottom - fTop);
        }
        else
        {
            // Matrix: Mute and Solo at the bottom right corner
            const int btnW = 66;
            mainMuteButton.setBounds (cfgLayout.cardRect.getRight() - 150, cfgLayout.cardRect.getBottom() - 36, btnW, 28);
            mainSoloButton.setBounds (cfgLayout.cardRect.getRight() - 76, cfgLayout.cardRect.getBottom() - 36, btnW, 28);
        }
    }
    else if (currentTab == TabGate)
    {
        const int leftBoxW = std::clamp ((int) (contentW * 0.44f), 320, 420);
        const int boxH = (int) std::floor ((contentH - 42) / 3.0f);

        // Box 1 (Threshold & Range)
        const int b1Y = contentY + 10;
        const int dial1Size = std::clamp (boxH - 24, 46, 86);
        const int d1Y = b1Y + 18 + (boxH - 18 - dial1Size) / 2;
        gateThreshDial.setBounds (contentX + 24, d1Y, dial1Size, dial1Size);
        gateRangeDial.setBounds (contentX + 24 + dial1Size + 16, d1Y, dial1Size, dial1Size);

        // Box 2 (Envelopes)
        const int b2Y = b1Y + boxH + 8;
        const int dial2Size = std::clamp ((leftBoxW - 36) / 3, 44, 80);
        const int d2Y = b2Y + 18 + (boxH - 18 - dial2Size) / 2;
        gateAttackDial.setBounds (contentX + 16, d2Y, dial2Size, dial2Size);
        gateHoldDial.setBounds (contentX + 16 + dial2Size + 4, d2Y, dial2Size, dial2Size);
        gateReleaseDial.setBounds (contentX + 16 + 2 * (dial2Size + 4), d2Y, dial2Size, dial2Size);

        // Box 3 (Sidechain Filter)
        const int b3Y = b2Y + boxH + 8;
        const int dial3Size = std::clamp (boxH - 24, 44, 78);
        const int d3Y = b3Y + 18 + (boxH - 18 - dial3Size) / 2;
        gateFiltFreqDial.setBounds (contentX + 16, d3Y, dial3Size, dial3Size);
        gateSlopeDial.setBounds (contentX + 16 + dial3Size + 6, d3Y, dial3Size, dial3Size);

        const int btnColX = contentX + 16 + 2 * (dial3Size + 6);
        const int btnW = std::clamp (leftBoxW - (btnColX - contentX) - 14, 52, 70);
        const int scBtnH = std::clamp ((boxH - 26) / 2, 18, 24);
        gateFilterButton.setBounds (btnColX, b3Y + 18, btnW, scBtnH);
        gateSoloButton.setBounds (btnColX, b3Y + 18 + scBtnH + 4, btnW, scBtnH);
        gateKeySourceButton.setBounds (btnColX + btnW + 4, b3Y + 18 + (scBtnH + 4) / 2, std::min (60, leftBoxW - (btnColX + btnW + 8)), scBtnH);

        // Display Screen on the RIGHT
        const int dispX = contentX + leftBoxW + 14;
        const int dispW = contentW - leftBoxW - 22;
        const int meterW = 30;
        const int pad = 10;
        const int graphW = dispW - meterW - pad * 3;
        const int btnAreaH = 32;
        const int graphH = std::clamp (contentH - 44 - btnAreaH, 100, 360);
        const int graphY = contentY + 18;

        gateGraph.setBounds (dispX + pad, graphY, graphW, graphH);
        gateGrMeter.setBounds (dispX + pad * 2 + graphW, graphY, meterW, graphH);

        const int btnY = graphY + graphH + 8;
        gateTypeButton.setBounds (dispX + pad, btnY, 80, 24);
        gateActiveButton.setBounds (dispX + pad + 86, btnY, 70, 24);
    }
    else if (currentTab == TabDyn)
    {
        const int leftBoxW = std::clamp ((int) (contentW * 0.44f), 320, 430);
        const int boxH = (int) std::floor ((contentH - 42) / 3.0f);

        // Box 1 (Dynamics Main)
        const int b1Y = contentY + 10;
        const int dial1Size = std::clamp ((leftBoxW - 32) / 4, 44, 76);
        const int d1Y = b1Y + 18 + (boxH - 18 - dial1Size) / 2;
        compThreshDial.setBounds (contentX + 14, d1Y, dial1Size, dial1Size);
        compRatioDial.setBounds (contentX + 14 + dial1Size + 3, d1Y, dial1Size, dial1Size);
        compGainDial.setBounds (contentX + 14 + 2 * (dial1Size + 3), d1Y, dial1Size, dial1Size);
        compKneeDial.setBounds (contentX + 14 + 3 * (dial1Size + 3), d1Y, dial1Size, dial1Size);

        // Box 2 (Envelopes & Mix)
        const int b2Y = b1Y + boxH + 8;
        compAttackDial.setBounds (contentX + 14, b2Y + 18 + (boxH - 18 - dial1Size) / 2, dial1Size, dial1Size);
        compHoldDial.setBounds (contentX + 14 + dial1Size + 3, b2Y + 18 + (boxH - 18 - dial1Size) / 2, dial1Size, dial1Size);
        compReleaseDial.setBounds (contentX + 14 + 2 * (dial1Size + 3), b2Y + 18 + (boxH - 18 - dial1Size) / 2, dial1Size, dial1Size);
        compMixDial.setBounds (contentX + 14 + 3 * (dial1Size + 3), b2Y + 18 + (boxH - 18 - dial1Size) / 2, dial1Size, dial1Size);

        // Box 3 (Sidechain Filter)
        const int b3Y = b2Y + boxH + 8;
        const int dial3Size = std::clamp (boxH - 24, 44, 76);
        const int d3Y = b3Y + 18 + (boxH - 18 - dial3Size) / 2;
        compFiltFreqDial.setBounds (contentX + 16, d3Y, dial3Size, dial3Size);
        compSlopeDial.setBounds (contentX + 16 + dial3Size + 6, d3Y, dial3Size, dial3Size);

        const int btnColX = contentX + 16 + 2 * (dial3Size + 6);
        const int btnW = std::clamp (leftBoxW - (btnColX - contentX) - 14, 52, 68);
        const int scBtnH = std::clamp ((boxH - 26) / 2, 18, 24);
        compFilterButton.setBounds (btnColX, b3Y + 18, btnW, scBtnH);
        compSoloButton.setBounds (btnColX, b3Y + 18 + scBtnH + 4, btnW, scBtnH);
        compKeySourceButton.setBounds (btnColX + btnW + 4, b3Y + 18 + (scBtnH + 4) / 2, std::min (58, leftBoxW - (btnColX + btnW + 8)), scBtnH);

        // Display Screen on the RIGHT
        const int dispX = contentX + leftBoxW + 14;
        const int dispW = contentW - leftBoxW - 22;
        const int meterW = 30;
        const int pad = 10;
        const int graphW = dispW - meterW - pad * 3;
        const int modeAreaH = 54;
        const int graphH = std::clamp (contentH - 44 - modeAreaH, 100, 360);
        const int graphY = contentY + 18;

        compGraph.setBounds (dispX + pad, graphY, graphW, graphH);
        compGrMeter.setBounds (dispX + pad * 2 + graphW, graphY, meterW, graphH);

        const int btnY = graphY + graphH + 6;
        const int modeBtnW = std::clamp ((dispW - 2 * pad) / 8 - 3, 34, 50);
        const int modeBtnH = 22;
        compTypeCompButton.setBounds (dispX + pad, btnY, modeBtnW, modeBtnH);
        compTypeExpButton.setBounds (dispX + pad + (modeBtnW + 3), btnY, modeBtnW, modeBtnH);
        compModeManButton.setBounds (dispX + pad + 2 * (modeBtnW + 3), btnY, modeBtnW, modeBtnH);
        compModeAutoButton.setBounds (dispX + pad + 3 * (modeBtnW + 3), btnY, modeBtnW, modeBtnH);
        compDetPeakButton.setBounds (dispX + pad + 4 * (modeBtnW + 3), btnY, modeBtnW, modeBtnH);
        compDetRmsButton.setBounds (dispX + pad + 5 * (modeBtnW + 3), btnY, modeBtnW, modeBtnH);
        compCharLinButton.setBounds (dispX + pad + 6 * (modeBtnW + 3), btnY, modeBtnW, modeBtnH);
        compCharLogButton.setBounds (dispX + pad + 7 * (modeBtnW + 3), btnY, modeBtnW, modeBtnH);

        compActiveButton.setBounds (dispX + pad, btnY + modeBtnH + 4, 64, 24);
    }
    else if (currentTab == TabEq)
    {
        const bool compactEq = (contentH < 340);
        const int leftBoxW = compactEq ? 190 : 124;

        if (compactEq)
        {
            // 2x2 layout inside leftBox
            const int dialSize = std::clamp ((contentH - 44) / 2, 50, 78);
            const int d1Y = contentY + 16;
            const int d2Y = d1Y + dialSize + 4;
            eqGainDial.setBounds (contentX + 16, d1Y, dialSize, dialSize);
            eqFreqDial.setBounds (contentX + 16 + dialSize + 6, d1Y, dialSize, dialSize);
            eqQDial.setBounds (contentX + 16, d2Y, dialSize, dialSize);

            const int btnX = contentX + 16 + dialSize + 6;
            const int btnH = std::clamp ((dialSize - 6) / 2, 20, 26);
            eqActiveButton.setBounds (btnX, d2Y + 4, dialSize, btnH);
            eqLoCutButton.setBounds (btnX, d2Y + 4 + btnH + 4, dialSize, btnH);
        }
        else
        {
            // Vertical stack
            const int dialH = std::clamp ((contentH - 70) / 3, 60, 92);
            eqGainDial.setBounds (contentX + 16, contentY + 16, 92, dialH);
            eqFreqDial.setBounds (contentX + 16, contentY + 16 + dialH + 4, 92, dialH);
            eqQDial.setBounds (contentX + 16, contentY + 16 + 2 * (dialH + 4), 92, dialH);
            const int btnY = contentY + 16 + 3 * (dialH + 4) + 4;
            eqActiveButton.setBounds (contentX + 14, btnY, 44, 26);
            eqLoCutButton.setBounds (contentX + 62, btnY, 48, 26);
        }

        // Center/Right Graph Area
        const int graphX = contentX + leftBoxW + 12;
        const int graphW = contentW - leftBoxW - 20;
        const int botH = 28;
        const int graphH = contentH - botH - 24;
        eqGraph.setBounds (graphX, contentY + 10, graphW, graphH);

        // Bottom Bar controls
        const int botY = contentY + graphH + 16;
        eqResetButton.setBounds (graphX + 6, botY, 46, 24);
        eqModeButton.setBounds (graphX + 56, botY, 46, 24);
        eqRtaButton.setBounds (graphX + 106, botY, 46, 24);
        eqLoCutBottomButton.setBounds (graphX + 156, botY, 54, 24);

        const int bandStartX = graphX + graphW - 6 * 50 - 6;
        for (int i = 0; i < 6; ++i)
            eqBandButtons[i].setBounds (bandStartX + i * 50, botY, 46, 24);
    }
    else if (currentTab == TabSends)
    {
        const int numCols = 16;
        const int availW = contentW - 20;
        const int colW = std::clamp (availW / numCols, 44, 76);
        const int colStartX = contentX + 10 + (availW - colW * numCols) / 2;

        const int badgeH = compactH ? 26 : 34;
        const int valH   = 14;
        const int tapH   = compactH ? 18 : 22;
        const int muteH  = compactH ? 20 : 24;

        const int topY   = contentY + 8;
        const int valY   = topY + badgeH + 2;
        const int tapY   = valY + valH + 3;
        const int faderY = tapY + tapH + 5;
        const int muteY  = contentY + contentH - muteH - 8;
        const int faderH = std::max (40, muteY - faderY - 6);

        for (int i = 0; i < numCols; ++i)
        {
            const int colX = colStartX + i * colW;
            const int badgeW = colW - 4;
            sendBusBadges[i].setBounds (colX + 2, topY, badgeW, badgeH);
            sendValueLabels[i].setBounds (colX + 2, valY, badgeW, valH);

            const int muteW = std::clamp (colW - 14, 22, 28);
            sendMuteButtons[i].setBounds (colX + (colW - muteW) / 2, muteY, muteW, muteH);

            const int faderW = std::clamp (colW - 8, 26, 40);
            sendFaders[i].setBounds (colX + (colW - faderW) / 2, faderY, faderW, faderH);
        }

        // Tap buttons: 1 button per 2 columns (8 pairs)
        for (int t = 0; t < 8; ++t)
        {
            const int leftColX = colStartX + (t * 2) * colW;
            const int pairW = colW * 2 - 8;
            sendTapButtons[t].setBounds (leftColX + 4, tapY, pairW, tapH);
        }
    }
    else if (currentTab == TabNaming)
    {
        nameEditor.setBounds (contentX + 30, contentY + 30, 240, 32);
        for (int i = 0; i < 8; ++i)
            colourButtons[i].setBounds (contentX + 30 + i * 40, contentY + 80, 36, 36);
    }
}

} // namespace mps
