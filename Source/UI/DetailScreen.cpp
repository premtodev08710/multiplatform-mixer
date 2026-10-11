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
        if (bank <= 3) // CH banks 1-8, 9-16, 17-24, 25-32
            setSelectedChannel (bank * 8);
    };

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
    startTimerHz (20); // Fast metering refresh
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
        project.setChannelName (selectedChannel, newName);
        nameEditor.setText (newName, false);
    };
    addAndMakeVisible (scribbleName);

    addAndMakeVisible (panIndicator);
    panIndicator.onPanChanged = [this] (float p) {
        if (ignoreCallbacks) return;
        auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];
        cp.pan.set (p);
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

    auto& cm = engine.getMixerMeters().channels[(size_t) selectedChannel];
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
    addChildComponent (linkButton);

    phaseButton.setClickingTogglesState (true);
    phaseButton.addListener (this);
    addChildComponent (phaseButton);

    gainDial.setTickLabels ({
        { -12.0, "-12" }, { -3.0, "-3" }, { 6.0, "6" }, { 15.0, "15" },
        { 24.0, "24" }, { 33.0, "33" }, { 42.0, "42" }, { 51.0, "51" }, { 60.0, "60" }
    });
    gainDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];
        cp.trimDb.set ((float) val);
    };
    addChildComponent (gainDial);

    auto& cm = engine.getMixerMeters().channels[(size_t) selectedChannel];
    preampMeter.setMeters (&cm.preL, &cm.preR, &cm.clip);
    addChildComponent (preampMeter);

    phantom48vButton.setClickingTogglesState (true);
    phantom48vButton.setTooltip ("Audio file simulation only - does not affect digital audio");
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

    delayButton.setClickingTogglesState (true);
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

    lowCutButton.setClickingTogglesState (true);
    lowCutButton.addListener (this);
    addChildComponent (lowCutButton);

    // Insert
    insertButton.addListener (this);
    addChildComponent (insertButton);
    addChildComponent (copyButton);
    addChildComponent (pasteButton);

    // DCA 1..8
    for (int i = 0; i < 8; ++i)
    {
        dcaButtons[i].setButtonText (juce::String (i + 1));
        dcaButtons[i].setClickingTogglesState (true);
        addChildComponent (dcaButtons[i]);
    }

    // Mute Groups 1..6
    for (int i = 0; i < 6; ++i)
    {
        muteGroupButtons[i].setButtonText (juce::String (i + 1));
        muteGroupButtons[i].setClickingTogglesState (true);
        addChildComponent (muteGroupButtons[i]);
    }

    // Automix
    automixOffButton.setClickingTogglesState (true);
    automixOffButton.setToggleState (true, juce::dontSendNotification);
    automixOffButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xffe53935));
    addChildComponent (automixOffButton);

    automixXButton.setClickingTogglesState (true);
    addChildComponent (automixXButton);

    automixYButton.setClickingTogglesState (true);
    addChildComponent (automixYButton);

    weightingDial.setTickLabels ({ { -12.0, "-12" }, { 0.0, "0" }, { 12.0, "+12" } });
    addChildComponent (weightingDial);

    // Main LR section
    mainLrButton.setClickingTogglesState (true);
    mainLrButton.setToggleState (true, juce::dontSendNotification);
    mainLrButton.addListener (this);
    addChildComponent (mainLrButton);

    monoCenterButton.setClickingTogglesState (true);
    addChildComponent (monoCenterButton);

    mainFader.setSliderStyle (juce::Slider::LinearVertical);
    mainFader.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    mainFader.setRange (0.0, 1.0, 0.001);
    mainFader.setValue (faderDbToPos (0.0f));
    mainFader.addListener (this);
    addChildComponent (mainFader);

    mainMuteButton.setClickingTogglesState (true);
    mainMuteButton.addListener (this);
    addChildComponent (mainMuteButton);

    mainSoloButton.setClickingTogglesState (true);
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
        auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];
        cp.dsp.compThreshDb.set ((float) val);
        compGraph.setCompParams ((float) val, (float) compRatioDial.getValue(), (float) compKneeDial.getValue());
    };
    addChildComponent (compThreshDial);

    compRatioDial.setTickLabels ({
        { 1.1, "1.1" }, { 1.3, "1.3" }, { 1.5, "1.5" }, { 2.0, "2" },
        { 2.5, "2.5" }, { 3.0, "3" }, { 4.0, "4" }, { 5.0, "5" },
        { 7.0, "7" }, { 10.0, "10" }, { 20.0, "20" }, { 100.0, "100" }
    });
    compRatioDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];
        cp.dsp.compRatio.set ((float) val);
        compGraph.setCompParams ((float) compThreshDial.getValue(), (float) val, (float) compKneeDial.getValue());
    };
    addChildComponent (compRatioDial);

    compGainDial.setTickLabels ({
        { 0.0, "0" }, { 3.0, "3" }, { 6.0, "6" }, { 9.0, "9" },
        { 12.0, "12" }, { 15.0, "15" }, { 18.0, "18" }, { 21.0, "21" }, { 24.0, "24" }
    });
    compGainDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];
        cp.dsp.compMakeupDb.set ((float) val);
    };
    addChildComponent (compGainDial);

    compKneeDial.setTickLabels ({
        { 0.0, "0" }, { 1.0, "1" }, { 2.0, "2" }, { 3.0, "3" }, { 4.0, "4" }, { 5.0, "5" }
    });
    compKneeDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];
        cp.dsp.compKneeDb.set ((float) val);
        compGraph.setCompParams ((float) compThreshDial.getValue(), (float) compRatioDial.getValue(), (float) val);
    };
    addChildComponent (compKneeDial);

    // Box 2
    compAttackDial.setTickLabels ({ { 0.0, "0" }, { 30.0, "30" }, { 60.0, "60" }, { 90.0, "90" }, { 120.0, "120" } });
    compAttackDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];
        cp.dsp.compAttackMs.set ((float) val);
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
        auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];
        cp.dsp.compReleaseMs.set ((float) val);
    };
    addChildComponent (compReleaseDial);

    compMixDial.setTickLabels ({
        { 0.0, "0" }, { 10.0, "10" }, { 20.0, "20" }, { 30.0, "30" },
        { 40.0, "40" }, { 50.0, "50" }, { 60.0, "60" }, { 70.0, "70" },
        { 80.0, "80" }, { 90.0, "90" }, { 100.0, "100" }
    });
    compMixDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];
        cp.dsp.compMix.set ((float) (val * 0.01));
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
        auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];
        if (selectedEqBand == 1) cp.dsp.eqLowGainDb.set ((float) val);
        else if (selectedEqBand == 2) cp.dsp.eqMid1GainDb.set ((float) val);
        else if (selectedEqBand == 3) cp.dsp.eqMid2GainDb.set ((float) val);
        else if (selectedEqBand == 4) cp.dsp.eqHighGainDb.set ((float) val);
        cp.dsp.eqBypass.setBool (false);
        eqActiveButton.setToggleState (true, juce::dontSendNotification);
        eqActiveButton.setButtonText ("EQ IN");
        eqGraph.repaint();
    };
    addChildComponent (eqGainDial);

    eqFreqDial.setTickLabels ({
        { 20.0, "20" }, { 47.0, "47" }, { 112.0, "112" }, { 266.0, "266" },
        { 632.0, "632" }, { 1400.0, "1k4" }, { 3500.0, "3k5" }, { 8400.0, "8k4" }, { 20000.0, "20k" }
    });
    eqFreqDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];
        if (selectedEqBand == 0)
        {
            cp.dsp.hpfFreq.set ((float) val);
            cp.dsp.hpfBypass.setBool (false);
            lowCutButton.setToggleState (true, juce::dontSendNotification);
            eqLoCutButton.setToggleState (true, juce::dontSendNotification);
            eqLoCutBottomButton.setToggleState (true, juce::dontSendNotification);
        }
        else
        {
            if (selectedEqBand == 1) cp.dsp.eqLowFreq.set ((float) val);
            else if (selectedEqBand == 2) cp.dsp.eqMid1Freq.set ((float) val);
            else if (selectedEqBand == 3) cp.dsp.eqMid2Freq.set ((float) val);
            else if (selectedEqBand == 4) cp.dsp.eqHighFreq.set ((float) val);
            cp.dsp.eqBypass.setBool (false);
            eqActiveButton.setToggleState (true, juce::dontSendNotification);
            eqActiveButton.setButtonText ("EQ IN");
        }
        eqGraph.repaint();
    };
    addChildComponent (eqFreqDial);

    eqQDial.setTickLabels ({
        { 0.3, "0.3" }, { 0.5, "0.5" }, { 1.0, "1" }, { 2.0, "2" }, { 5.0, "5" }, { 10.0, "10" }
    });
    eqQDial.onValueChanged = [this] (double val) {
        if (ignoreCallbacks) return;
        auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];
        if (selectedEqBand == 1) cp.dsp.eqLowQ.set ((float) val);
        else if (selectedEqBand == 2) cp.dsp.eqMid1Q.set ((float) val);
        else if (selectedEqBand == 3) cp.dsp.eqMid2Q.set ((float) val);
        else if (selectedEqBand == 4) cp.dsp.eqHighQ.set ((float) val);
        cp.dsp.eqBypass.setBool (false);
        eqActiveButton.setToggleState (true, juce::dontSendNotification);
        eqActiveButton.setButtonText ("EQ IN");
        eqGraph.repaint();
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
    for (int i = 0; i < 8; ++i)
    {
        if (i < 4)
        {
            const auto type = (FxType) (int) engine.getMixerParams().fxSlots[(size_t) i].fxType.getInt();
            const auto meta = getFxMetadata (type);
            sendDials[i].setTitle ("FX " + juce::String (i + 1) + " (" + meta.shortName + ")");
        }
        else
        {
            sendDials[i].setTitle ("BUS " + juce::String (i + 1));
        }

        sendDials[i].setRange (-60.0, 10.0, 0.5);
        sendDials[i].setDefaultValue (-60.0);
        sendDials[i].setValue (-60.0, juce::dontSendNotification);

        if (i < 4)
        {
            sendDials[i].onValueChanged = [this, i] (double val) {
                if (ignoreCallbacks) return;
                auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];
                cp.sends[(size_t) i].set ((float) val);
            };
        }

        addChildComponent (sendDials[i]);
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
    auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];

    const bool eqOn = ! cp.dsp.eqBypass.getBool();
    eqActiveButton.setToggleState (eqOn, juce::dontSendNotification);
    eqActiveButton.setButtonText (eqOn ? "EQ IN" : "EQ BYP");

    const bool hpfOn = ! cp.dsp.hpfBypass.getBool();
    lowCutButton.setToggleState (hpfOn, juce::dontSendNotification);
    eqLoCutButton.setToggleState (hpfOn, juce::dontSendNotification);
    eqLoCutBottomButton.setToggleState (hpfOn, juce::dontSendNotification);

    if (selectedEqBand == 0) // HPF
    {
        eqGainDial.setEnabled (false);
        eqGainDial.setValue (0.0, juce::dontSendNotification);
        eqFreqDial.setValue (cp.dsp.hpfFreq.get(), juce::dontSendNotification);
        eqQDial.setEnabled (false);
        eqQDial.setValue (0.707, juce::dontSendNotification);
        eqModeButton.setButtonText ("HPF");
        eqModeButton.setEnabled (false);
    }
    else if (selectedEqBand == 1) // Low
    {
        eqGainDial.setEnabled (true);
        eqGainDial.setValue (cp.dsp.eqLowGainDb.get(), juce::dontSendNotification);
        eqFreqDial.setValue (cp.dsp.eqLowFreq.get(), juce::dontSendNotification);
        eqQDial.setEnabled (true);
        eqQDial.setValue (cp.dsp.eqLowQ.get(), juce::dontSendNotification);
        eqModeButton.setEnabled (true);
        eqModeButton.setButtonText (cp.dsp.eqLowType.getInt() == 1 ? "LShv" : "PEQ");
    }
    else if (selectedEqBand == 2) // Mid 1
    {
        eqGainDial.setEnabled (true);
        eqGainDial.setValue (cp.dsp.eqMid1GainDb.get(), juce::dontSendNotification);
        eqFreqDial.setValue (cp.dsp.eqMid1Freq.get(), juce::dontSendNotification);
        eqQDial.setEnabled (true);
        eqQDial.setValue (cp.dsp.eqMid1Q.get(), juce::dontSendNotification);
        eqModeButton.setEnabled (false);
        eqModeButton.setButtonText ("PEQ");
    }
    else if (selectedEqBand == 3) // Mid 2
    {
        eqGainDial.setEnabled (true);
        eqGainDial.setValue (cp.dsp.eqMid2GainDb.get(), juce::dontSendNotification);
        eqFreqDial.setValue (cp.dsp.eqMid2Freq.get(), juce::dontSendNotification);
        eqQDial.setEnabled (true);
        eqQDial.setValue (cp.dsp.eqMid2Q.get(), juce::dontSendNotification);
        eqModeButton.setEnabled (false);
        eqModeButton.setButtonText ("PEQ");
    }
    else if (selectedEqBand == 4) // High
    {
        eqGainDial.setEnabled (true);
        eqGainDial.setValue (cp.dsp.eqHighGainDb.get(), juce::dontSendNotification);
        eqFreqDial.setValue (cp.dsp.eqHighFreq.get(), juce::dontSendNotification);
        eqQDial.setEnabled (true);
        eqQDial.setValue (cp.dsp.eqHighQ.get(), juce::dontSendNotification);
        eqModeButton.setEnabled (true);
        eqModeButton.setButtonText (cp.dsp.eqHighType.getInt() == 2 ? "HShv" : "PEQ");
    }

    ignoreCallbacks = false;
}

void DetailScreen::setSelectedChannel (int ch)
{
    if (selectedChannel != ch && ch >= 0 && ch < kMaxInputChannels)
    {
        selectedChannel = ch;
        auto& cm = engine.getMixerMeters().channels[(size_t) selectedChannel];
        channelMeter.setMeters (&cm.preL, &cm.preR, &cm.clip);
        preampMeter.setMeters (&cm.preL, &cm.preR, &cm.clip);
        refreshFromModel();
        resized();
    }
}

void DetailScreen::refreshFromModel()
{
    ignoreCallbacks = true;
    auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];
    auto& meta = project.getChannelMetadata (selectedChannel);

    // Left Strip
    scribbleName.setText (meta.name, juce::dontSendNotification);
    scribbleName.setColour (juce::Label::backgroundColourId, meta.colour);
    channelNumLabel.setText (juce::String (selectedChannel + 1), juce::dontSendNotification);
    panIndicator.setPan (cp.pan.get(), juce::dontSendNotification);

    const float db = cp.faderDb.get();
    faderSlider.setValue (faderDbToPos (db), juce::dontSendNotification);
    faderDbLabel.setText (db <= kFaderMinDb ? "-inf" : juce::String (db, 1) + " dB", juce::dontSendNotification);

    // Preamp
    const int src = cp.source.getInt();
    sourceButton.setButtonText (src >= 0 ? "IN " + juce::String (src + 1) : "OFF");
    phaseButton.setToggleState (cp.polarity.getBool(), juce::dontSendNotification);
    gainDial.setValue (cp.trimDb.get(), juce::dontSendNotification);
    delayDial.setValue (cp.dsp.delayMs.get(), juce::dontSendNotification);
    delayButton.setToggleState (! cp.dsp.delayBypass.getBool(), juce::dontSendNotification);
    lowCutDial.setValue (cp.dsp.hpfFreq.get(), juce::dontSendNotification);
    lowCutButton.setToggleState (! cp.dsp.hpfBypass.getBool(), juce::dontSendNotification);
    mainLrButton.setToggleState (cp.mainAssign.getBool(), juce::dontSendNotification);

    // Insert
    const int insSlot = cp.insertFxSlot.getInt();
    if (insSlot == 0)
        insertButton.setButtonText ("INS: OFF");
    else
        insertButton.setButtonText ("FX " + juce::String (insSlot));

    // Sends
    for (int i = 0; i < 4; ++i)
    {
        const auto type = (FxType) (int) engine.getMixerParams().fxSlots[(size_t) i].fxType.getInt();
        const auto meta = getFxMetadata (type);
        sendDials[i].setTitle ("FX " + juce::String (i + 1) + " (" + meta.shortName + ")");
        sendDials[i].setValue (cp.sends[(size_t) i].get(), juce::dontSendNotification);
    }

    // Gate
    const bool gateOn = ! cp.dsp.gateBypass.getBool();
    gateActiveButton.setToggleState (gateOn, juce::dontSendNotification);
    gateThreshDial.setValue (cp.dsp.gateThreshDb.get(), juce::dontSendNotification);
    gateRangeDial.setValue (-cp.dsp.gateRangeDb.get(), juce::dontSendNotification);
    gateAttackDial.setValue (cp.dsp.gateAttackMs.get(), juce::dontSendNotification);
    gateHoldDial.setValue (cp.dsp.gateHoldMs.get(), juce::dontSendNotification);
    gateReleaseDial.setValue (cp.dsp.gateReleaseMs.get(), juce::dontSendNotification);
    gateGraph.setGateParams (cp.dsp.gateThreshDb.get(), -cp.dsp.gateRangeDb.get());

    // Dynamics
    const bool compOn = ! cp.dsp.compBypass.getBool();
    compActiveButton.setToggleState (compOn, juce::dontSendNotification);
    compThreshDial.setValue (cp.dsp.compThreshDb.get(), juce::dontSendNotification);
    compRatioDial.setValue (cp.dsp.compRatio.get(), juce::dontSendNotification);
    compGainDial.setValue (cp.dsp.compMakeupDb.get(), juce::dontSendNotification);
    compKneeDial.setValue (cp.dsp.compKneeDb.get(), juce::dontSendNotification);
    compAttackDial.setValue (cp.dsp.compAttackMs.get(), juce::dontSendNotification);
    compReleaseDial.setValue (cp.dsp.compReleaseMs.get(), juce::dontSendNotification);
    compMixDial.setValue (cp.dsp.compMix.get() * 100.0, juce::dontSendNotification);
    compGraph.setCompParams (cp.dsp.compThreshDb.get(), cp.dsp.compRatio.get(), cp.dsp.compKneeDb.get());

    // EQ
    eqGraph.setParams (&cp.dsp, engine.getSampleRate());
    const bool eqOn = ! cp.dsp.eqBypass.getBool();
    eqActiveButton.setToggleState (eqOn, juce::dontSendNotification);
    eqLoCutButton.setToggleState (! cp.dsp.hpfBypass.getBool(), juce::dontSendNotification);
    eqLoCutBottomButton.setToggleState (! cp.dsp.hpfBypass.getBool(), juce::dontSendNotification);
    updateEqSelectedBandDialValues();

    // Naming
    nameEditor.setText (meta.name, false);

    ignoreCallbacks = false;
    repaint();
}

void DetailScreen::timerCallback()
{
    bankSelectorBar.repaint();

    // Keep channel name & colour in sync with project model
    const auto& meta = project.getChannelMetadata (selectedChannel);
    if (! scribbleName.isBeingEdited() && scribbleName.getText() != meta.name)
    {
        scribbleName.setText (meta.name, juce::dontSendNotification);
        scribbleName.setColour (juce::Label::backgroundColourId, meta.colour);
        if (! nameEditor.hasKeyboardFocus (true))
            nameEditor.setText (meta.name, false);
    }

    auto& cm = engine.getMixerMeters().channels[(size_t) selectedChannel];

    if (currentTab == TabGate)
    {
        const float gr = cm.dsp.gateGainReductionDb.load (std::memory_order_relaxed);
        gateGrMeter.setReductionDb (gr);
    }
    else if (currentTab == TabDyn)
    {
        const float gr = cm.dsp.compGainReductionDb.load (std::memory_order_relaxed);
        compGrMeter.setReductionDb (gr);
    }
}

void DetailScreen::updateTabVisibility()
{
    const bool isCfg = (currentTab == TabConfig);
    signalFlow.setVisible (isCfg);
    sourceButton.setVisible (isCfg);
    linkButton.setVisible (isCfg);
    phaseButton.setVisible (isCfg);
    gainDial.setVisible (isCfg);
    preampMeter.setVisible (isCfg);
    phantom48vButton.setVisible (isCfg);
    delayDial.setVisible (isCfg);
    delayButton.setVisible (isCfg);
    lowCutDial.setVisible (isCfg);
    lowCutButton.setVisible (isCfg);
    insertButton.setVisible (isCfg);
    copyButton.setVisible (isCfg);
    pasteButton.setVisible (isCfg);
    for (auto& b : dcaButtons) b.setVisible (isCfg);
    for (auto& b : muteGroupButtons) b.setVisible (isCfg);
    automixOffButton.setVisible (isCfg);
    automixXButton.setVisible (isCfg);
    automixYButton.setVisible (isCfg);
    weightingDial.setVisible (isCfg);
    mainLrButton.setVisible (isCfg);
    monoCenterButton.setVisible (isCfg);
    mainFader.setVisible (isCfg);
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
    for (auto& d : sendDials) d.setVisible (isSends);

    const bool isNaming = (currentTab == TabNaming);
    nameEditor.setVisible (isNaming);
    for (auto& b : colourButtons) b.setVisible (isNaming);

    resized();
    repaint();
}

void DetailScreen::buttonClicked (juce::Button* b)
{
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
        if (selectedChannel > 0) setSelectedChannel (selectedChannel - 1);
        return;
    }
    if (b == &nextChannelButton)
    {
        if (selectedChannel < kMaxInputChannels - 1) setSelectedChannel (selectedChannel + 1);
        return;
    }

    if (ignoreCallbacks) return;
    auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];

    if (b == &phaseButton)
    {
        cp.polarity.setBool (phaseButton.getToggleState());
    }
    else if (b == &delayButton)
    {
        cp.dsp.delayBypass.setBool (! delayButton.getToggleState());
    }
    else if (b == &lowCutButton || b == &eqLoCutButton || b == &eqLoCutBottomButton)
    {
        const bool active = b->getToggleState();
        cp.dsp.hpfBypass.setBool (! active);
        lowCutButton.setToggleState (active, juce::dontSendNotification);
        eqLoCutButton.setToggleState (active, juce::dontSendNotification);
        eqLoCutBottomButton.setToggleState (active, juce::dontSendNotification);
        eqGraph.repaint();
    }
    else if (b == &insertButton)
    {
        juce::PopupMenu m;
        m.addItem (1, "Insert: OFF", true, cp.insertFxSlot.getInt() == 0);
        for (int i = 1; i <= 8; ++i)
        {
            auto& fxSlot = engine.getMixerParams().fxSlots[(size_t) (i - 1)];
            const auto meta = getFxMetadata ((FxType) (int) fxSlot.fxType.getInt());
            m.addItem (i + 1, "FX " + juce::String (i) + " (" + meta.shortName + ")", true, cp.insertFxSlot.getInt() == i);
        }
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&insertButton),
                         [this] (int res) {
                             if (res >= 1)
                             {
                                 auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];
                                 cp.insertFxSlot.set ((float) (res - 1));
                                 refreshFromModel();
                             }
                         });
    }
    else if (b == &mainLrButton)
    {
        cp.mainAssign.setBool (mainLrButton.getToggleState());
    }
    else if (b == &gateActiveButton)
    {
        cp.dsp.gateBypass.setBool (! gateActiveButton.getToggleState());
    }
    else if (b == &compActiveButton)
    {
        cp.dsp.compBypass.setBool (! compActiveButton.getToggleState());
    }
    else if (b == &eqActiveButton)
    {
        const bool active = eqActiveButton.getToggleState();
        cp.dsp.eqBypass.setBool (! active);
        eqActiveButton.setButtonText (active ? "EQ IN" : "EQ BYP");
        eqGraph.repaint();
    }
    else if (b == &eqModeButton)
    {
        if (selectedEqBand == 1)
        {
            const int cur = cp.dsp.eqLowType.getInt();
            const int next = (cur == 1) ? 0 : 1; // 1 = LowShelf, 0 = Bell
            cp.dsp.eqLowType.set ((float) next);
            eqModeButton.setButtonText (next == 1 ? "LShv" : "PEQ");
            eqGraph.repaint();
        }
        else if (selectedEqBand == 4)
        {
            const int cur = cp.dsp.eqHighType.getInt();
            const int next = (cur == 2) ? 0 : 2; // 2 = HighShelf, 0 = Bell
            cp.dsp.eqHighType.set ((float) next);
            eqModeButton.setButtonText (next == 2 ? "HShv" : "PEQ");
            eqGraph.repaint();
        }
    }
    else if (b == &eqResetButton)
    {
        cp.dsp.eqLowGainDb.set (0.0f);
        cp.dsp.eqMid1GainDb.set (0.0f);
        cp.dsp.eqMid2GainDb.set (0.0f);
        cp.dsp.eqHighGainDb.set (0.0f);
        cp.dsp.eqLowQ.set (0.707f);
        cp.dsp.eqMid1Q.set (1.0f);
        cp.dsp.eqMid2Q.set (1.0f);
        cp.dsp.eqHighQ.set (0.707f);
        eqGraph.repaint();
        updateEqSelectedBandDialValues();
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
}

void DetailScreen::sliderValueChanged (juce::Slider* s)
{
    if (ignoreCallbacks) return;

    if (s == &faderSlider)
    {
        const float db = faderPosToDb ((float) faderSlider.getValue());
        auto& cp = engine.getMixerParams().channels[(size_t) selectedChannel];
        cp.faderDb.set (db);
        faderDbLabel.setText (db <= kFaderMinDb ? "-inf" : juce::String (db, 1) + " dB", juce::dontSendNotification);
    }
    else if (s == &mainFader)
    {
        const float db = faderPosToDb ((float) mainFader.getValue());
        auto& mp = engine.getMixerParams().main;
        mp.faderDb.set (db);
    }
}

void DetailScreen::paint (juce::Graphics& g)
{
    g.fillAll (ConsoleLookAndFeel::bgDark);

    auto centerArea = getLocalBounds();
    centerArea.removeFromTop (86); // Bank bar (52) + subtab bar (34)
    centerArea.removeFromLeft (98); // Left strip + margin

    // 1. Draw Section Card Background (dark slate charcoal with 1px border)
    const auto cardRect = centerArea.reduced (4).toFloat();
    g.setColour (juce::Colour (0xff1e2229));
    g.fillRoundedRectangle (cardRect, 4.0f);
    g.setColour (juce::Colour (0xff323a46));
    g.drawRoundedRectangle (cardRect, 4.0f, 1.0f);

    // 2. Draw Left Strip Fader Scale Markings
    const float faderTop = (float) faderSlider.getY() + 10.0f;
    const float faderBottom = (float) faderSlider.getBottom() - 10.0f;
    const float faderSpan = faderBottom - faderTop;

    const struct { float db; const char* txt; } marks[] = {
        { 10.0f, "10" }, { 5.0f, "5" }, { 0.0f, "0" }, { -5.0f, "-5" },
        { -10.0f, "-10" }, { -20.0f, "-20" }, { -30.0f, "-30" }, { -40.0f, "-40" },
        { -50.0f, "-50" }, { -70.0f, "-70" }, { -90.0f, "-oo" }
    };

    g.setFont (juce::FontOptions (8.0f));
    g.setColour (juce::Colour (0xff8a94a2));
    for (const auto& m : marks)
    {
        const float pos = faderDbToPos (m.db);
        const float y = faderBottom - pos * faderSpan;
        g.drawText (m.txt, 6, (int) y - 5, 18, 10, juce::Justification::centredRight);
        g.drawHorizontalLine ((int) y, 26.0f, 29.0f);
    }

    // 3. Tab-Specific Card Dividers / Box Outlines
    if (currentTab == TabConfig)
    {
        // Preamp Panel Box
        const juce::Rectangle<float> preampBox (cardRect.getX() + 14.0f, cardRect.getY() + 80.0f, 410.0f, 150.0f);
        g.setColour (juce::Colour (0xff242932));
        g.fillRoundedRectangle (preampBox, 3.0f);
        g.setColour (juce::Colour (0xff3d4756));
        g.drawRoundedRectangle (preampBox, 3.0f, 1.0f);

        // DCA & Mute Groups Box
        const juce::Rectangle<float> dcaBox (cardRect.getX() + 14.0f, cardRect.getY() + 238.0f, 310.0f, 110.0f);
        g.setColour (juce::Colour (0xff242932));
        g.fillRoundedRectangle (dcaBox, 3.0f);
        g.setColour (juce::Colour (0xff3d4756));
        g.drawRoundedRectangle (dcaBox, 3.0f, 1.0f);

        g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        g.setColour (juce::Colour (0xffb0bec5));
        g.drawText ("DCA", (int) dcaBox.getX() + 10, (int) dcaBox.getY() + 4, 120, 14, juce::Justification::centred);
        g.drawText ("MUTE GROUPS", (int) dcaBox.getX() + 150, (int) dcaBox.getY() + 4, 140, 14, juce::Justification::centred);

        // Automix & Weighting Box
        const juce::Rectangle<float> autoBox (cardRect.getX() + 332.0f, cardRect.getY() + 238.0f, 170.0f, 110.0f);
        g.setColour (juce::Colour (0xff242932));
        g.fillRoundedRectangle (autoBox, 3.0f);
        g.setColour (juce::Colour (0xff3d4756));
        g.drawRoundedRectangle (autoBox, 3.0f, 1.0f);

        g.drawText ("AUTOMIX GROUP", (int) autoBox.getX() + 6, (int) autoBox.getY() + 4, 90, 14, juce::Justification::centred);
        g.drawText ("WEIGHTING", (int) autoBox.getX() + 94, (int) autoBox.getY() + 4, 70, 14, juce::Justification::centred);

        // Main Box on Right
        const juce::Rectangle<float> mainBox (cardRect.getRight() - 170.0f, cardRect.getY() + 8.0f, 155.0f, cardRect.getHeight() - 16.0f);
        g.setColour (juce::Colour (0xff242932));
        g.fillRoundedRectangle (mainBox, 3.0f);
        g.setColour (juce::Colour (0xff4a5465));
        g.drawRoundedRectangle (mainBox, 3.0f, 1.2f);

        g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        g.setColour (juce::Colours::white);
        g.drawText ("MAIN", (int) mainBox.getX(), (int) mainBox.getY() + 8, (int) mainBox.getWidth(), 20, juce::Justification::centred);
    }
    else if (currentTab == TabGate || currentTab == TabDyn)
    {
        // 3 Distinct Rounded Boxes on the Right
        const float rightX = cardRect.getX() + 250.0f;
        const float rightW = cardRect.getRight() - rightX - 16.0f;

        // Box 1
        const juce::Rectangle<float> box1 (rightX, cardRect.getY() + 12.0f, rightW, 100.0f);
        g.setColour (juce::Colour (0xff242932));
        g.fillRoundedRectangle (box1, 3.0f);
        g.setColour (juce::Colour (0xff3d4756));
        g.drawRoundedRectangle (box1, 3.0f, 1.0f);

        // Box 2
        const juce::Rectangle<float> box2 (rightX, cardRect.getY() + 118.0f, rightW, 100.0f);
        g.setColour (juce::Colour (0xff242932));
        g.fillRoundedRectangle (box2, 3.0f);
        g.setColour (juce::Colour (0xff3d4756));
        g.drawRoundedRectangle (box2, 3.0f, 1.0f);

        // Box 3
        const juce::Rectangle<float> box3 (rightX, cardRect.getY() + 224.0f, rightW, 100.0f);
        g.setColour (juce::Colour (0xff242932));
        g.fillRoundedRectangle (box3, 3.0f);
        g.setColour (juce::Colour (0xff3d4756));
        g.drawRoundedRectangle (box3, 3.0f, 1.0f);
    }
    else if (currentTab == TabEq)
    {
        // Left Box surrounding GAIN & FREQ
        const juce::Rectangle<float> eqLeftBox (cardRect.getX() + 14.0f, cardRect.getY() + 16.0f, 110.0f, cardRect.getHeight() - 70.0f);
        g.setColour (juce::Colour (0xff242932));
        g.fillRoundedRectangle (eqLeftBox, 3.0f);
        g.setColour (juce::Colour (0xff3d4756));
        g.drawRoundedRectangle (eqLeftBox, 3.0f, 1.0f);
    }
}

void DetailScreen::resized()
{
    const int totalW = getWidth();
    const int totalH = getHeight();

    // 1. Top Bank Selector Bar
    bankSelectorBar.setBounds (0, 0, totalW, 52);

    // 2. Left Preview Strip (x: 4, y: 56, w: 90, h: totalH - 60)
    scribbleName.setBounds (4, 56, 88, 26);
    panIndicator.setBounds (4, 84, 88, 26);
    channelNumLabel.setBounds (10, 112, 30, 24);

    const int faderTop = 138;
    const int faderBottom = totalH - 36;
    faderSlider.setBounds (28, faderTop, 36, faderBottom - faderTop);
    channelMeter.setBounds (68, faderTop + 4, 18, faderBottom - faderTop - 8);

    prevChannelButton.setBounds (4, totalH - 30, 42, 24);
    nextChannelButton.setBounds (50, totalH - 30, 42, 24);

    // 3. Sub-Tab Buttons Row
    const int tabStartX = 98;
    const int tabAvailW = totalW - tabStartX - 8;
    const int tabW = std::min (105, tabAvailW / NumTabs);

    for (int i = 0; i < NumTabs; ++i)
        tabButtons[i].setBounds (tabStartX + i * tabW, 56, tabW - 2, 26);

    // 4. Center Content Area
    const int contentX = tabStartX;
    const int contentY = 86;
    const int contentW = totalW - contentX - 8;
    const int contentH = totalH - contentY - 8;

    if (currentTab == TabConfig)
    {
        signalFlow.setBounds (contentX + 10, contentY + 10, contentW - 190, 60);

        sourceButton.setBounds (contentX + 38, contentY + 44, 54, 22);

        // Preamp Panel (x: contentX + 18, y: contentY + 84)
        linkButton.setBounds (contentX + 28, contentY + 116, 68, 26);
        phaseButton.setBounds (contentX + 28, contentY + 196, 68, 26);

        gainDial.setBounds (contentX + 104, contentY + 92, 85, 95);
        preampMeter.setBounds (contentX + 192, contentY + 92, 14, 95);
        phantom48vButton.setBounds (contentX + 114, contentY + 196, 65, 26);

        delayDial.setBounds (contentX + 218, contentY + 92, 85, 95);
        delayButton.setBounds (contentX + 228, contentY + 196, 65, 26);

        lowCutDial.setBounds (contentX + 318, contentY + 92, 85, 95);
        lowCutButton.setBounds (contentX + 328, contentY + 196, 65, 26);

        // Insert
        insertButton.setBounds (contentX + 434, contentY + 116, 68, 24);
        copyButton.setBounds (contentX + 434, contentY + 160, 68, 22);
        pasteButton.setBounds (contentX + 434, contentY + 196, 68, 22);

        // DCA & Mute Groups
        const int dcaX = contentX + 24;
        const int dcaY = contentY + 262;
        for (int i = 0; i < 4; ++i)
            dcaButtons[i].setBounds (dcaX + i * 32, dcaY, 28, 26);
        for (int i = 4; i < 8; ++i)
            dcaButtons[i].setBounds (dcaX + (i - 4) * 32, dcaY + 34, 28, 26);

        const int mgX = contentX + 164;
        for (int i = 0; i < 3; ++i)
            muteGroupButtons[i].setBounds (mgX + i * 36, dcaY, 30, 26);
        for (int i = 3; i < 6; ++i)
            muteGroupButtons[i].setBounds (mgX + (i - 3) * 36, dcaY + 34, 30, 26);

        // Automix
        automixOffButton.setBounds (contentX + 344, contentY + 262, 60, 26);
        automixXButton.setBounds (contentX + 344, contentY + 296, 28, 26);
        automixYButton.setBounds (contentX + 376, contentY + 296, 28, 26);
        weightingDial.setBounds (contentX + 420, contentY + 252, 70, 75);

        // Main section on right
        const int mainX = contentX + contentW - 160;
        mainLrButton.setBounds (mainX + 20, contentY + 38, 65, 26);
        monoCenterButton.setBounds (mainX + 20, contentY + 76, 65, 24);
        mainFader.setBounds (mainX + 90, contentY + 38, 36, contentH - 120);
        mainMuteButton.setBounds (mainX + 16, contentH + contentY - 68, 42, 26);
        mainSoloButton.setBounds (mainX + 62, contentH + contentY - 68, 42, 26);
    }
    else if (currentTab == TabGate)
    {
        gateGraph.setBounds (contentX + 18, contentY + 16, 175, 175);
        gateTypeButton.setBounds (contentX + 70, contentY + 200, 70, 24);
        gateActiveButton.setBounds (contentX + 18, contentY + 250, 48, 28);
        gateGrMeter.setBounds (contentX + 200, contentY + 16, 36, 175);

        const int rX = contentX + 256;
        // Box 1
        gateThreshDial.setBounds (rX + 20, contentY + 18, 95, 90);
        gateRangeDial.setBounds (rX + 130, contentY + 18, 95, 90);

        // Box 2
        gateAttackDial.setBounds (rX + 10, contentY + 124, 85, 90);
        gateHoldDial.setBounds (rX + 100, contentY + 124, 85, 90);
        gateReleaseDial.setBounds (rX + 190, contentY + 124, 85, 90);

        // Box 3
        gateFiltFreqDial.setBounds (rX + 10, contentY + 230, 85, 90);
        gateSlopeDial.setBounds (rX + 100, contentY + 230, 85, 90);
        gateFilterButton.setBounds (rX + 196, contentY + 242, 54, 26);
        gateSoloButton.setBounds (rX + 196, contentY + 274, 54, 26);
        gateKeySourceButton.setBounds (rX + 258, contentY + 258, 54, 26);
    }
    else if (currentTab == TabDyn)
    {
        compGraph.setBounds (contentX + 18, contentY + 16, 175, 175);

        // Under Graph Buttons
        compTypeCompButton.setBounds (contentX + 30, contentY + 200, 48, 20);
        compTypeExpButton.setBounds (contentX + 80, contentY + 200, 48, 20);
        compModeManButton.setBounds (contentX + 136, contentY + 200, 48, 20);
        compModeAutoButton.setBounds (contentX + 186, contentY + 200, 48, 20);

        compDetPeakButton.setBounds (contentX + 136, contentY + 224, 48, 20);
        compDetRmsButton.setBounds (contentX + 186, contentY + 224, 48, 20);
        compCharLinButton.setBounds (contentX + 136, contentY + 248, 48, 20);
        compCharLogButton.setBounds (contentX + 186, contentY + 248, 48, 20);

        compActiveButton.setBounds (contentX + 18, contentY + 250, 48, 28);
        compGrMeter.setBounds (contentX + 242, contentY + 16, 36, 175);

        const int rX = contentX + 290;
        // Box 1
        compThreshDial.setBounds (rX + 10, contentY + 18, 75, 90);
        compRatioDial.setBounds (rX + 85, contentY + 18, 75, 90);
        compGainDial.setBounds (rX + 160, contentY + 18, 75, 90);
        compKneeDial.setBounds (rX + 235, contentY + 18, 75, 90);

        // Box 2
        compAttackDial.setBounds (rX + 10, contentY + 124, 75, 90);
        compHoldDial.setBounds (rX + 85, contentY + 124, 75, 90);
        compReleaseDial.setBounds (rX + 160, contentY + 124, 75, 90);
        compMixDial.setBounds (rX + 235, contentY + 124, 75, 90);

        // Box 3
        compFiltFreqDial.setBounds (rX + 10, contentY + 230, 85, 90);
        compSlopeDial.setBounds (rX + 100, contentY + 230, 85, 90);
        compFilterButton.setBounds (rX + 196, contentY + 242, 54, 26);
        compSoloButton.setBounds (rX + 196, contentY + 274, 54, 26);
        compKeySourceButton.setBounds (rX + 258, contentY + 258, 54, 26);
    }
    else if (currentTab == TabEq)
    {
        // Left Box controls
        eqGainDial.setBounds (contentX + 24, contentY + 20, 90, 85);
        eqFreqDial.setBounds (contentX + 24, contentY + 110, 90, 85);
        eqQDial.setBounds (contentX + 24, contentY + 200, 90, 85);
        eqActiveButton.setBounds (contentX + 22, contentH + contentY - 80, 44, 26);
        eqLoCutButton.setBounds (contentX + 70, contentH + contentY - 80, 50, 26);

        // Center Graph
        const int graphX = contentX + 130;
        const int graphW = contentW - 140;
        const int graphH = contentH - 60;
        eqGraph.setBounds (graphX, contentY + 12, graphW, graphH);

        // Bottom Bar controls
        const int botY = contentY + graphH + 20;
        eqResetButton.setBounds (graphX + 20, botY, 44, 26);
        eqModeButton.setBounds (graphX + 70, botY, 44, 26);
        eqRtaButton.setBounds (graphX + 120, botY, 40, 26);
        eqLoCutBottomButton.setBounds (graphX + 166, botY, 52, 26);

        const int bandStartX = graphX + 240;
        for (int i = 0; i < 6; ++i)
            eqBandButtons[i].setBounds (bandStartX + i * 54, botY, 50, 26);
    }
    else if (currentTab == TabSends)
    {
        const int dialW = 85;
        const int dialH = 95;
        for (int i = 0; i < 4; ++i)
            sendDials[i].setBounds (contentX + 30 + i * 110, contentY + 30, dialW, dialH);
        for (int i = 4; i < 8; ++i)
            sendDials[i].setBounds (contentX + 30 + (i - 4) * 110, contentY + 150, dialW, dialH);
    }
    else if (currentTab == TabNaming)
    {
        nameEditor.setBounds (contentX + 30, contentY + 30, 240, 32);
        for (int i = 0; i < 8; ++i)
            colourButtons[i].setBounds (contentX + 30 + i * 40, contentY + 80, 36, 36);
    }
}

} // namespace mps
