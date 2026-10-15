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
    homeScreen.onOpenDetail = [this] (int ch) {
        detailScreen.setSelectedChannel (ch);
        navButtons[NavDetail].setToggleState (true, juce::dontSendNotification);
        showScreen (NavDetail);
    };
    homeScreen.onOpenDetailStrip = [this] (ChannelStripComponent::StripType type, int idx) {
        detailScreen.setSelectedStrip (type, idx);
        navButtons[NavDetail].setToggleState (true, juce::dontSendNotification);
        showScreen (NavDetail);
    };

    project.onTracksChanged = [this] {
        homeScreen.rebuildStrips();
    };

    detailScreen.onBack = [this] { showScreen (NavHome); };
    tracksScreen.onBack = [this] { showScreen (NavHome); };
    effectsScreen.onBack = [this] { showScreen (NavHome); };
    scenesScreen.onBack = [this] { showScreen (NavHome); };
    setupScreen.onBack = [this] { showScreen (NavHome); };

    // Top-right Settings Gear Button
    settingsButton.setName ("settings_button");
    settingsButton.addListener (this);
    addAndMakeVisible (settingsButton);

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
        const int homeBank = homeScreen.getCurrentBank();
        switch (homeBank)
        {
            case BankSelectorBar::BankCh1_8:
            case BankSelectorBar::BankCh9_16:
            case BankSelectorBar::BankCh17_24:
            case BankSelectorBar::BankCh25_32:
                if (detailScreen.getSelectedStripType() != ChannelStripComponent::StripType::InputChannel)
                    detailScreen.setSelectedStrip (ChannelStripComponent::StripType::InputChannel, homeScreen.getSelectedChannel());
                break;
            case BankSelectorBar::BankAux1_8:
                if (detailScreen.getSelectedStripType() != ChannelStripComponent::StripType::Aux)
                    detailScreen.setSelectedStrip (ChannelStripComponent::StripType::Aux, 0);
                break;
            case BankSelectorBar::BankFx1_4:
                if (detailScreen.getSelectedStripType() != ChannelStripComponent::StripType::FxReturn)
                    detailScreen.setSelectedStrip (ChannelStripComponent::StripType::FxReturn, 0);
                break;
            case BankSelectorBar::BankBus1_8:
                if (detailScreen.getSelectedStripType() != ChannelStripComponent::StripType::Bus || detailScreen.getSelectedStripIndex() >= 8)
                    detailScreen.setSelectedStrip (ChannelStripComponent::StripType::Bus, 0);
                break;
            case BankSelectorBar::BankBus9_16:
                if (detailScreen.getSelectedStripType() != ChannelStripComponent::StripType::Bus || detailScreen.getSelectedStripIndex() < 8)
                    detailScreen.setSelectedStrip (ChannelStripComponent::StripType::Bus, 8);
                break;
            case BankSelectorBar::BankMtxMain:
                if (detailScreen.getSelectedStripType() != ChannelStripComponent::StripType::Matrix)
                    detailScreen.setSelectedStrip (ChannelStripComponent::StripType::Matrix, 0);
                break;
            default:
                break;
        }
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

MainComponent::SettingsButton::SettingsButton()
    : juce::Button ("settings")
{
    setTooltip ("Settings & Navigation Menu");
}

void MainComponent::SettingsButton::paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = getLocalBounds().toFloat().reduced (2.0f);
    if (shouldDrawButtonAsDown)
        bounds = bounds.reduced (1.0f);

    // Dark circular background
    g.setColour (shouldDrawButtonAsDown ? juce::Colour (0xff252c38)
                : shouldDrawButtonAsHighlighted ? juce::Colour (0xff1f2530)
                : juce::Colour (0xff15181f));
    g.fillEllipse (bounds);

    g.setColour (shouldDrawButtonAsHighlighted ? ConsoleLookAndFeel::accentCyan : ConsoleLookAndFeel::borderSubtle);
    g.drawEllipse (bounds, 1.2f);

    // Vector Gear Icon with 8 teeth and central circular cutout
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float rOuter = bounds.getWidth() * 0.36f;
    const float rInner = bounds.getWidth() * 0.25f;
    const float rHole  = bounds.getWidth() * 0.12f;

    juce::Path p;
    const int numTeeth = 8;
    for (int i = 0; i < numTeeth; ++i)
    {
        const float angle = (float) i * juce::MathConstants<float>::twoPi / (float) numTeeth;
        const float toothAngle = juce::MathConstants<float>::twoPi / (float) (numTeeth * 4);
        const float slope = toothAngle * 0.45f;

        const float a0 = angle - toothAngle;
        const float a1 = angle - slope;
        const float a2 = angle + slope;
        const float a3 = angle + toothAngle;

        const float p0x = cx + rInner * std::cos (a0);
        const float p0y = cy + rInner * std::sin (a0);
        const float p1x = cx + rOuter * std::cos (a1);
        const float p1y = cy + rOuter * std::sin (a1);
        const float p2x = cx + rOuter * std::cos (a2);
        const float p2y = cy + rOuter * std::sin (a2);
        const float p3x = cx + rInner * std::cos (a3);
        const float p3y = cy + rInner * std::sin (a3);

        if (i == 0)
            p.startNewSubPath (p0x, p0y);
        else
            p.lineTo (p0x, p0y);

        p.lineTo (p1x, p1y);
        p.lineTo (p2x, p2y);
        p.lineTo (p3x, p3y);
    }
    p.closeSubPath();

    juce::Path hole;
    hole.addEllipse (cx - rHole, cy - rHole, rHole * 2.0f, rHole * 2.0f);

    p.setUsingNonZeroWinding (false);
    p.addPath (hole);

    g.setColour (shouldDrawButtonAsHighlighted ? ConsoleLookAndFeel::accentCyan : juce::Colour (0xffcfd8dc));
    g.fillPath (p);
}

void MainComponent::showSettingsMenu()
{
    juce::PopupMenu menu;
    menu.addSectionHeader ("SETTINGS & NAVIGATION");
    menu.addItem (1001, "Audio & System Setup (การตั้งค่า)", true, currentScreen == NavSetup);
    menu.addSeparator();
    menu.addItem (1002, "Mixer Console (Home)", true, currentScreen == NavHome);
    menu.addItem (1003, "Channel Detail (Preamp / EQ / Dyn)", true, currentScreen == NavDetail);
    menu.addItem (1004, "Multitrack Player (Tracks)", true, currentScreen == NavTracks);
    menu.addItem (1005, "Virtual Effects Rack", true, currentScreen == NavEffects);
    menu.addItem (1006, "Scenes & Snapshots", true, currentScreen == NavScenes);
    menu.addSeparator();
    menu.addItem (1007, showTopNavBar ? "Hide Top Menu Bar (ซ่อนแถบเมนู)" : "Show Top Menu Bar (แสดงแถบเมนู)", true, showTopNavBar);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&settingsButton),
        [this] (int res) {
            switch (res)
            {
                case 1001: showScreen (NavSetup); break;
                case 1002: showScreen (NavHome); break;
                case 1003: showScreen (NavDetail); break;
                case 1004: showScreen (NavTracks); break;
                case 1005: showScreen (NavEffects); break;
                case 1006: showScreen (NavScenes); break;
                case 1007:
                    showTopNavBar = !showTopNavBar;
                    resized();
                    repaint();
                    break;
                default: break;
            }
        });
}

void MainComponent::buttonClicked (juce::Button* b)
{
    if (b == &settingsButton)
    {
        showSettingsMenu();
        return;
    }

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

    if (showTopNavBar)
    {
        const int navH = (getWidth() < 600) ? 64 : 40;
        // Subtle header separator line
        g.setColour (ConsoleLookAndFeel::borderSubtle);
        g.drawHorizontalLine (navH - 2, 0.0f, (float) getWidth());
    }
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    if (showTopNavBar)
    {
        for (int i = 0; i < NumNavTabs; ++i)
            navButtons[i].setVisible (true);

        const bool isNarrowPortrait = getWidth() < 600;
        const int navHeight = isNarrowPortrait ? 64 : 40;
        auto navArea = area.removeFromTop (navHeight).reduced (4, 2);
        navArea.removeFromRight (40); // Leave room on right for settings button

        if (isNarrowPortrait)
        {
            const int columns = NumNavTabs / 2;
            const int tabW = navArea.getWidth() / columns;
            const int rowH = navArea.getHeight() / 2;
            for (int i = 0; i < NumNavTabs; ++i)
                navButtons[i].setBounds (navArea.getX() + (i % columns) * tabW,
                                         navArea.getY() + (i / columns) * rowH,
                                         tabW - 3, rowH - 2);
        }
        else
        {
            const int tabW = navArea.getWidth() / NumNavTabs;
            for (int i = 0; i < NumNavTabs; ++i)
                navButtons[i].setBounds (navArea.removeFromLeft (tabW).reduced (2, 0));
        }
    }
    else
    {
        for (int i = 0; i < NumNavTabs; ++i)
            navButtons[i].setVisible (false);
    }

    // Top-right Settings Gear Button (Always docked in top-right corner)
    const int gearSize = 34;
    settingsButton.setBounds (getWidth() - gearSize - 5, 3, gearSize, gearSize);
    settingsButton.toFront (false);

    // Bottom Transport bar (52px for timeline + controls)
    transportBar.setBounds (area.removeFromBottom (52));

    // Center Active Screen (takes maximum vertical height with top menu hidden)
    homeScreen.setBounds (area);
    detailScreen.setBounds (area);
    tracksScreen.setBounds (area);
    effectsScreen.setBounds (area);
    scenesScreen.setBounds (area);
    setupScreen.setBounds (area);
    placeholderLabel.setBounds (area);
}

} // namespace mps
