#include "MainComponent.h"

namespace mps
{

MainComponent::MainComponent()
{
    setLookAndFeel (&lookAndFeel);

    // Initialize audio device
    engine.initDevice();

    // Top Navigation Tabs
    const char* tabNames[NumNavTabs] = {
        "Home", "Detail", "Tracks", "Effects", "Scenes",
        "Meters", "Recorder", "Routing", "Monitor", "Setup"
    };

    for (int i = 0; i < NumNavTabs; ++i)
    {
        navButtons[i].setName ("nav_" + juce::String (tabNames[i]).toLowerCase());
        navButtons[i].setButtonText (tabNames[i]);
        navButtons[i].setClickingTogglesState (true);
        navButtons[i].setRadioGroupId (3001);
        navButtons[i].addListener (this);

        // Active screens: Home, Detail, Tracks, Effects, Scenes, Setup
        // Roadmap screens: Meters, Recorder, Routing, Monitor
        if (i != NavHome && i != NavDetail && i != NavTracks && i != NavEffects && i != NavScenes && i != NavSetup)
        {
            navButtons[i].setColour (juce::TextButton::textColourOffId, ConsoleLookAndFeel::textMuted);
            navButtons[i].setTooltip ("Phase 4 Roadmap module");
        }

        addAndMakeVisible (navButtons[i]);
    }
    navButtons[0].setToggleState (true, juce::dontSendNotification);

    // Wire selection between Home and Detail
    homeScreen.onChannelSelected = [this] (int ch) {
        detailScreen.setSelectedChannel (ch);
    };

    project.onTracksChanged = [this] {
        homeScreen.rebuildStrips();
    };

    // Add screens
    addChildComponent (homeScreen);
    addChildComponent (detailScreen);
    addChildComponent (tracksScreen);
    addChildComponent (effectsScreen);
    addChildComponent (scenesScreen);
    addChildComponent (setupScreen);

    // Placeholder
    placeholderLabel.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    placeholderLabel.setJustificationType (juce::Justification::centred);
    placeholderLabel.setColour (juce::Label::textColourId, ConsoleLookAndFeel::textMuted);
    addChildComponent (placeholderLabel);

    // Bottom Transport
    addAndMakeVisible (transportBar);

    showScreen (NavHome);

    setWantsKeyboardFocus (true);
    setSize (1100, 720);
}

MainComponent::~MainComponent()
{
    setLookAndFeel (nullptr);
}

void MainComponent::showScreen (int screenIdx)
{
    currentScreen = screenIdx;

    homeScreen.setVisible (screenIdx == NavHome);
    detailScreen.setVisible (screenIdx == NavDetail);
    tracksScreen.setVisible (screenIdx == NavTracks);
    effectsScreen.setVisible (screenIdx == NavEffects);
    scenesScreen.setVisible (screenIdx == NavScenes);
    setupScreen.setVisible (screenIdx == NavSetup);

    const bool isPlaceholder = (screenIdx != NavHome && screenIdx != NavDetail
                                && screenIdx != NavTracks && screenIdx != NavEffects
                                && screenIdx != NavScenes && screenIdx != NavSetup);
    placeholderLabel.setVisible (isPlaceholder);

    if (isPlaceholder)
    {
        const char* names[NumNavTabs] = {
            "Home", "Detail", "Tracks", "Effects Rack", "Scenes",
            "Bridge Meters", "Multitrack Recorder", "Signal Routing Patchbay",
            "Studio Monitor & Solo Bus", "System & Audio Hardware Setup"
        };
        placeholderLabel.setText (juce::String (names[screenIdx])
                                  + "\n\n(Scheduled for Phase 4 Expansion as specified in architecture roadmap)",
                                  juce::dontSendNotification);
    }
    else if (screenIdx == NavDetail)
    {
        detailScreen.refreshFromModel();
    }
    else if (screenIdx == NavEffects)
    {
        effectsScreen.refreshFromModel();
    }

    resized();
}

bool MainComponent::keyPressed (const juce::KeyPress& key)
{
    // Space bar toggles Play/Pause when not editing text
    if (key.getKeyCode() == juce::KeyPress::spaceKey)
    {
        if (engine.getStreamer().isPlaying())
            engine.pause();
        else
            engine.play();
        return true;
    }
    return false;
}

void MainComponent::buttonClicked (juce::Button* b)
{
    for (int i = 0; i < NumNavTabs; ++i)
    {
        if (b == &navButtons[i])
        {
            showScreen (i);
            break;
        }
    }
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (ConsoleLookAndFeel::bgDark);

    // Subtle header separator line
    g.setColour (ConsoleLookAndFeel::borderSubtle);
    g.drawHorizontalLine (38, 0.0f, (float) getWidth());
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    // Top Navigation bar
    auto navArea = area.removeFromTop (44).reduced (6, 3);
    const int tabW = navArea.getWidth() / NumNavTabs;
    for (int i = 0; i < NumNavTabs; ++i)
        navButtons[i].setBounds (navArea.removeFromLeft (tabW).reduced (2, 0));

    // Bottom Transport bar (54px for timeline + controls)
    transportBar.setBounds (area.removeFromBottom (54));

    // Center Active Screen
    homeScreen.setBounds (area);
    detailScreen.setBounds (area);
    tracksScreen.setBounds (area);
    effectsScreen.setBounds (area);
    scenesScreen.setBounds (area);
    setupScreen.setBounds (area);
    placeholderLabel.setBounds (area);
}

} // namespace mps
