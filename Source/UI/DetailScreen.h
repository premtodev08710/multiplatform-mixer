// Mix Practice Studio - Channel Detail Screen (X32/M32 Console Layout & Workflow)
#pragma once

#include "BankSelectorBar.h"
#include "ChannelStripComponent.h"
#include "ConsoleDial.h"
#include "ConsoleLookAndFeel.h"
#include "GainReductionMeter.h"
#include "InteractiveEqGraph.h"
#include "LedMeter.h"
#include "SignalFlowDiagram.h"
#include "TransferCurveGraph.h"
#include "TrapezoidPanIndicator.h"
#include "../Audio/AudioEngine.h"
#include "../Model/ProjectModel.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

namespace mps
{

class DetailScreen : public juce::Component,
                     public juce::Button::Listener,
                     public juce::Slider::Listener,
                     private juce::Timer
{
public:
    DetailScreen (AudioEngine& engine, ProjectModel& project);
    ~DetailScreen() override;

    void setSelectedStrip (ChannelStripComponent::StripType type, int index);
    void setSelectedChannel (int channel);
    int getSelectedChannel() const noexcept { return currentStripIndex; }
    ChannelStripComponent::StripType getSelectedStripType() const noexcept { return currentStripType; }
    int getSelectedStripIndex() const noexcept { return currentStripIndex; }

    void paint (juce::Graphics& g) override;
    void resized() override;

    void buttonClicked (juce::Button* button) override;
    void sliderValueChanged (juce::Slider* slider) override;

    void refreshFromModel();

private:
    void timerCallback() override;

    AudioEngine& engine;
    ProjectModel& project;
    ChannelStripComponent::StripType currentStripType = ChannelStripComponent::StripType::InputChannel;
    int currentStripIndex = 0;
    int selectedChannel = 0;
    int selectedEqBand = 1; // 0=HPF, 1=Low, 2=Mid1, 3=Mid2, 4=High

    // Top Bank Selector Bar
    BankSelectorBar bankSelectorBar;

    // Sub-Tabs
    enum Tab { TabConfig = 0, TabGate, TabDyn, TabEq, TabSends, TabNaming, TabPreset, NumTabs };
    juce::TextButton tabButtons[NumTabs];
    int currentTab = TabConfig;

    //==========================================================================
    // Left Channel Preview Strip
    juce::Label scribbleName;
    TrapezoidPanIndicator panIndicator;
    juce::Label channelNumLabel;
    juce::Slider faderSlider;
    juce::Label faderDbLabel;
    LedMeter channelMeter { LedMeter::Mode::Mono };
    juce::TextButton prevChannelButton { "<" };
    juce::TextButton nextChannelButton { ">" };

    //==========================================================================
    // Tab 1: CONFIG / PREAMP Controls
    SignalFlowDiagram signalFlow;

    // Source box
    juce::TextButton sourceButton { "OFF" };

    // Preamp Panel
    juce::TextButton linkButton { "LINK" };
    juce::TextButton phaseButton { "PHASE" };
    ConsoleDial gainDial { "GAIN", -12.0, 60.0, 0.0, 0.5 };
    LedMeter preampMeter;
    juce::TextButton phantom48vButton { "48V" };
    ConsoleDial delayDial { "DELAY", 0.3, 500.0, 0.3, 0.1 };
    juce::TextButton delayButton { "DELAY" };
    ConsoleDial lowCutDial { "LOW CUT", 20.0, 400.0, 80.0, 1.0, true };
    juce::TextButton lowCutButton { "LOWCUT" };

    // Insert Box
    juce::TextButton insertButton { "OFF" };
    juce::TextButton copyButton { "Copy" };
    juce::TextButton pasteButton { "Paste" };

    // DCA & Mute Groups
    juce::TextButton dcaButtons[8];
    juce::TextButton muteGroupButtons[6];

    // Automix Group & Weighting
    juce::TextButton automixOffButton { "OFF" };
    juce::TextButton automixXButton { "X" };
    juce::TextButton automixYButton { "Y" };
    ConsoleDial weightingDial { "WEIGHTING", -12.0, 12.0, 0.0, 0.5 };

    // Main LR Section (Right anchored)
    juce::TextButton mainBusHeaderButton { "MAIN BUS" };
    juce::TextButton mainLrButton { "MAIN LR" };
    juce::TextButton monoCenterButton { "MONO/C" };
    juce::Slider mainFader;
    juce::TextButton mainMuteButton { "MUTE" };
    juce::TextButton mainSoloButton { "SOLO" };

    //==========================================================================
    // Tab 2: GATE Controls
    TransferCurveGraph gateGraph { TransferCurveGraph::Mode::Gate };
    juce::TextButton gateTypeButton { "EXP2" };
    juce::TextButton gateActiveButton { "GATE" };
    GainReductionMeter gateGrMeter { GainReductionMeter::ScaleType::Gate };

    // Box 1: Threshold & Range
    ConsoleDial gateThreshDial { "THRESHOLD", -80.0, 0.0, -40.0, 0.5 };
    ConsoleDial gateRangeDial { "RANGE", 3.0, 60.0, 30.0, 0.5 };

    // Box 2: Envelopes
    ConsoleDial gateAttackDial { "ATTACK", 0.0, 120.0, 10.0, 0.5 };
    ConsoleDial gateHoldDial { "HOLD", 0.0, 2000.0, 20.0, 1.0 };
    ConsoleDial gateReleaseDial { "RELEASE", 5.0, 4000.0, 150.0, 1.0 };

    // Box 3: Sidechain Filter
    ConsoleDial gateFiltFreqDial { "FILT. FREQ", 20.0, 20000.0, 1000.0, 1.0, true };
    ConsoleDial gateSlopeDial { "SLOPE", 1.0, 10.0, 2.0, 1.0 };
    juce::TextButton gateFilterButton { "FILTER" };
    juce::TextButton gateSoloButton { "SOLO" };
    juce::TextButton gateKeySourceButton { "Self" };

    //==========================================================================
    // Tab 3: DYNAMICS Controls
    TransferCurveGraph compGraph { TransferCurveGraph::Mode::Compressor };
    juce::TextButton compTypeCompButton { "COMP" };
    juce::TextButton compTypeExpButton { "EXP" };
    juce::TextButton compModeManButton { "MAN" };
    juce::TextButton compModeAutoButton { "AUTO" };
    juce::TextButton compDetPeakButton { "PEAK" };
    juce::TextButton compDetRmsButton { "RMS" };
    juce::TextButton compCharLinButton { "LIN" };
    juce::TextButton compCharLogButton { "LOG" };
    juce::TextButton compActiveButton { "DYN" };
    GainReductionMeter compGrMeter { GainReductionMeter::ScaleType::Compressor };

    // Box 1: Dynamics Main
    ConsoleDial compThreshDial { "THRESHOLD", -60.0, 0.0, -20.0, 0.5 };
    ConsoleDial compRatioDial { "RATIO", 1.1, 100.0, 4.0, 0.1 };
    ConsoleDial compGainDial { "GAIN", 0.0, 24.0, 0.0, 0.1 };
    ConsoleDial compKneeDial { "KNEE", 0.0, 5.0, 2.0, 0.1 };

    // Box 2: Envelopes & Mix
    ConsoleDial compAttackDial { "ATTACK", 0.0, 120.0, 20.0, 0.5 };
    ConsoleDial compHoldDial { "HOLD", 0.0, 2000.0, 0.0, 1.0 };
    ConsoleDial compReleaseDial { "RELEASE", 5.0, 4000.0, 200.0, 1.0 };
    ConsoleDial compMixDial { "MIX", 0.0, 100.0, 100.0, 1.0 };

    // Box 3: Sidechain Filter
    ConsoleDial compFiltFreqDial { "FILT. FREQ", 20.0, 20000.0, 1000.0, 1.0, true };
    ConsoleDial compSlopeDial { "SLOPE", 1.0, 10.0, 2.0, 1.0 };
    juce::TextButton compFilterButton { "FILTER" };
    juce::TextButton compSoloButton { "SOLO" };
    juce::TextButton compKeySourceButton { "Self" };

    //==========================================================================
    // Tab 4: EQ Controls
    ConsoleDial eqGainDial { "GAIN", -15.0, 15.0, 0.0, 0.1 };
    ConsoleDial eqFreqDial { "FREQ", 20.0, 20000.0, 1000.0, 1.0, true };
    ConsoleDial eqQDial { "Q", 0.3, 10.0, 1.0, 0.05 };
    juce::TextButton eqActiveButton { "EQ IN" };
    juce::TextButton eqLoCutButton { "LO CUT" };

    InteractiveEqGraph eqGraph;
    juce::TextButton eqResetButton { "RESET" };
    juce::TextButton eqModeButton { "LShv" };
    juce::TextButton eqRtaButton { "RTA" };
    juce::TextButton eqLoCutBottomButton { "LO CUT" };

    juce::TextButton eqBandButtons[6]; // LOW, LOW2, LOMID, HIMID, HIGH2, HIGH

    //==========================================================================
    // Tab 5: SENDS Controls (16 MixBus Sends matching M32/X32-Mix layout)
    struct BusBadgeComponent : public juce::Component
    {
        int busNumber = 1;
        juce::String busName { "Bus1" };
        bool isFx = false;

        void setBus (int num, const juce::String& name, bool fx = false)
        {
            busNumber = num;
            busName = name;
            isFx = fx;
            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat().reduced (1.0f);
            const float cr = 5.0f;

            juce::Colour topCol = isFx ? juce::Colour (0xffd946ef) : juce::Colour (0xff00d4e5);
            juce::Colour botCol = isFx ? juce::Colour (0xffa21caf) : juce::Colour (0xff009fb0);
            juce::Colour textCol = isFx ? juce::Colours::white : juce::Colour (0xff0a1114);

            juce::ColourGradient grad (topCol, bounds.getCentreX(), bounds.getY(),
                                      botCol, bounds.getCentreX(), bounds.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (bounds, cr);

            g.setColour (juce::Colours::white.withAlpha (0.4f));
            g.drawRoundedRectangle (bounds, cr, 1.0f);

            g.setColour (textCol);
            g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            g.drawText (juce::String (busNumber), bounds.removeFromTop (bounds.getHeight() * 0.52f),
                        juce::Justification::centred, false);

            g.setFont (juce::FontOptions (9.5f, juce::Font::plain));
            g.drawText (busName, bounds, juce::Justification::centred, false);
        }
    };

    BusBadgeComponent sendBusBadges[16];
    juce::Label       sendValueLabels[16];
    juce::Slider      sendFaders[16];
    juce::TextButton  sendMuteButtons[16];
    juce::TextButton  sendTapButtons[8]; // 8 pairs: Bus 1-2, 3-4, 5-6, 7-8, 9-10, 11-12, 13-14, 15-16

    // Tab 6: NAMING Controls
    juce::TextEditor nameEditor;
    juce::TextButton colourButtons[8];

    // Helper setup
    void setupLeftStrip();
    void setupConfigTab();
    void setupGateTab();
    void setupDynTab();
    void setupEqTab();
    void setupSendsTab();
    void setupNamingTab();

    void updateTabVisibility();
    void updateEqSelectedBandDialValues();

    ChannelDspParams* getCurrentDspParams();
    int getMaxStripIndex (ChannelStripComponent::StripType type) const noexcept;
    void setPanForCurrentStrip (float p);
    void setFaderForCurrentStrip (float db);
    float getFaderForCurrentStrip() const noexcept;
    float getPanForCurrentStrip() const noexcept;
    bool getMuteForCurrentStrip() const noexcept;
    void setMuteForCurrentStrip (bool m);
    bool getSoloForCurrentStrip() const noexcept;
    void setSoloForCurrentStrip (bool s);
    bool getMainAssignForCurrentStrip() const noexcept;
    void setMainAssignForCurrentStrip (bool a);
    bool getMonoAssignForCurrentStrip() const noexcept;
    void setMonoAssignForCurrentStrip (bool a);
    int getDcaMaskForCurrentStrip() const noexcept;
    void setDcaMaskForCurrentStrip (int mask);
    int getMuteGroupMaskForCurrentStrip() const noexcept;
    void setMuteGroupMaskForCurrentStrip (int mask);
    int getInsertSlotForCurrentStrip() const noexcept;
    void setInsertSlotForCurrentStrip (int slot);

    bool ignoreCallbacks = false;
};

} // namespace mps
