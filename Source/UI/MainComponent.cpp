#include "MainComponent.h"
#include "../Core/AppEdition.h"

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
    effectsScreen.onStripNavigated = [this] (ChannelStripComponent::StripType type, int idx) {
        detailScreen.setSelectedStrip (type, idx);
        if (type == ChannelStripComponent::StripType::InputChannel)
            homeScreen.setSelectedChannel (idx);
    };
    effectsScreen.onFxSlotChanged = [this] (int slot) { oscClient.sendFxSlot (slot); };
    oscClient.onFxStateChanged = [this] { effectsScreen.refreshFromModel(); };
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
    transportBar.onQuickSave = [this] {
        saveCurrentSession (true);
    };
    addAndMakeVisible (transportBar);

    showScreen (NavHome);

    // Auto-restore previous session (audio tracks + mixer configuration)
    restorePreviousSession();

    // Auto-save periodically (every 30 seconds if modified)
    startTimer (30000);

    // Report anonymous installation & active usage metrics to Firebase in background
    telemetry.reportAppLaunchAsync();

    // Show terms of service dialog on first launch
    juce::MessageManager::callAsync ([this] {
        checkAndShowTermsOfService();
    });

    setWantsKeyboardFocus (true);
    setSize (1100, 720);
}

MainComponent::~MainComponent()
{
    stopTimer();
    saveCurrentSession (false);
    setLookAndFeel (nullptr);
}

void MainComponent::timerCallback()
{
    if (project.isModified())
    {
        project.autoSaveSession();
        engine.saveAudioDeviceSettings();
    }
}

void MainComponent::saveCurrentSession (bool showNotification)
{
    auto res = project.autoSaveSession();
    engine.saveAudioDeviceSettings();

    if (showNotification)
    {
        if (res.wasOk())
        {
            juce::AlertWindow::showMessageBoxAsync (
                juce::AlertWindow::InfoIcon,
                juce::CharPointer_UTF8 ("\x53\x61\x76\x65\x64\x20\x28\xe0\xb8\x9a\xe0\xb8\xb1\xe0\xb8\x99\xe0\xb8\x97\xe0\xb8\xb6\xe0\xb8\x81\xe0\xb9\x80\xe0\xb8\xa3\xe0\xb8\xb5\xe0\xb8\xa2\xe0\xb8\x9a\xe0\xb8\xa3\xe0\xb9\x89\xe0\xb8\xad\xe0\xb8\xa2\x29"),
                juce::CharPointer_UTF8 ("\x53\x65\x73\x73\x69\x6f\x6e\x2c\x20\x74\x72\x61\x63\x6b\x73\x20\x61\x6e\x64\x20\x6d\x69\x78\x65\x72\x20\x73\x65\x74\x74\x69\x6e\x67\x73\x20\x73\x61\x76\x65\x64\x20\x73\x75\x63\x63\x65\x73\x73\x66\x75\x6c\x6c\x79\x2e\x0a\x28\xe0\xb8\x9a\xe0\xb8\xb1\xe0\xb8\x99\xe0\xb8\x97\xe0\xb8\xb6\xe0\xb8\x81\xe0\xb9\x84\xe0\xb8\x9f\xe0\xb8\xa5\xe0\xb9\x8c\xe0\xb9\x80\xe0\xb8\xaa\xe0\xb8\xb5\xe0\xb8\xa2\xe0\xb8\x87\xe0\xb9\x81\xe0\xb8\xa5\xe0\xb8\xb0\xe0\xb8\x81\xe0\xb8\xb2\xe0\xb8\xa3\xe0\xb8\x95\xe0\xb8\xb1\xe0\xb9\x89\xe0\xb8\x87\xe0\xb8\x84\xe0\xb9\x88\xe0\xb8\xb2\xe0\xb8\xa1\xe0\xb8\xb4\xe0\xb8\x81\xe0\xb9\x80\xe0\xb8\x8b\xe0\xb8\xad\xe0\xb8\xa3\xe0\xb9\x8c\xe0\xb9\x80\xe0\xb8\xa3\xe0\xb8\xb5\xe0\xb8\xa2\xe0\xb8\x9a\xe0\xb8\xa3\xe0\xb9\x89\xe0\xb8\xad\xe0\xb8\xa2\xe0\xb9\x81\xe0\xb8\xa5\xe0\xb9\x89\xe0\xb8\xa7\x29"));
        }
        else
        {
            juce::AlertWindow::showMessageBoxAsync (
                juce::AlertWindow::WarningIcon,
                "Save Failed",
                "Failed to auto-save session: " + res.getErrorMessage());
        }
    }
}

void MainComponent::restorePreviousSession()
{
    if (ProjectModel::hasSavedSession())
    {
        juce::StringArray missingFiles;
        auto res = project.autoLoadSession (missingFiles);
        if (res.wasOk())
        {
            homeScreen.rebuildStrips();
            detailScreen.refreshFromModel();
            effectsScreen.refreshFromModel();

            if (! missingFiles.isEmpty())
            {
                juce::AlertWindow::showMessageBoxAsync (
                    juce::AlertWindow::WarningIcon,
                    "Missing Audio Files",
                    "Some audio tracks from previous session were moved or not found:\n"
                    + missingFiles.joinIntoString ("\n"));
            }
        }
    }
}

static juce::File getTermsAcceptedMarkerFile()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("MixPracticeStudio");
    if (! dir.exists())
        dir.createDirectory();

    return dir.getChildFile ("terms_accepted.txt");
}

void MainComponent::checkAndShowTermsOfService()
{
    if (AppEdition::isPro())
        return;

    auto markerFile = getTermsAcceptedMarkerFile();
    if (markerFile.existsAsFile())
    {
        // Terms already accepted; show Shopee sponsor overlay on app launch
        showSponsorOverlayIfNeeded();
        return;
    }

    const juce::String title = juce::CharPointer_UTF8 ("\xe0\xb9\x80\xe0\xb8\x87\xe0\xb8\xb7\xe0\xb9\x88\xe0\xb8\xad\xe0\xb8\x99\xe0\xb9\x84\xe0\xb8\x82\xe0\xb8\x81\xe0\xb8\xb2\xe0\xb8\xa3\xe0\xb9\x83\xe0\xb8\x8a\xe0\xb9\x89\xe0\xb8\x87\xe0\xb8\xb2\xe0\xb8\x99 (Terms of Service)");
    const juce::String message = juce::CharPointer_UTF8 (
        "\xe0\xb8\xa2\xe0\xb8\xb4\xe0\xb8\x99\xe0\xb8\x94\xe0\xb8\xb5\xe0\xb8\x95\xe0\xb9\x89\xe0\xb8\xad\xe0\xb8\x99\xe0\xb8\xa3\xe0\xb8\xb1\xe0\xb8\x9a\xe0\xb8\xaa\xe0\xb8\xb9\xe0\xb9\x88 Mix Practice Studio!\n\n"
        "\xe0\xb8\x82\xe0\xb9\x89\xe0\xb8\xad\xe0\xb8\x81\xe0\xb8\xb3\xe0\xb8\xab\xe0\xb8\x99\xe0\xb8\x94\xe0\xb9\x81\xe0\xb8\xa5\xe0\xb8\xb0\xe0\xb9\x80\xe0\xb8\x87\xe0\xb8\xb7\xe0\xb9\x88\xe0\xb8\xad\xe0\xb8\x99\xe0\xb9\x84\xe0\xb8\x82\xe0\xb8\x81\xe0\xb8\xb2\xe0\xb8\xa3\xe0\xb9\x83\xe0\xb8\x8a\xe0\xb9\x89\xe0\xb8\x87\xe0\xb8\xb2\xe0\xb8\x99:\n"
        "1. \xe0\xb9\x81\xe0\xb8\xad\xe0\xb8\x9e\xe0\xb8\x9e\xe0\xb8\xa5\xe0\xb8\xb4\xe0\xb9\x80\xe0\xb8\x84\xe0\xb8\x8a\xe0\xb8\xb1\xe0\xb8\x99\xe0\xb8\x99\xe0\xb8\xb5\xe0\xb9\x89\xe0\xb8\x9e\xe0\xb8\xb1\xe0\xb8\x92\xe0\xb8\x99\xe0\xb8\xb2\xe0\xb8\x82\xe0\xb8\xb5\xe0\xb9\x89\xe0\xb8\x99\xe0\xb9\x80\xe0\xb8\x9e\xe0\xb8\xb7\xe0\xb9\x88\xe0\xb8\xad\xe0\xb8\x81\xe0\xb8\xb2\xe0\xb8\xa3\xe0\xb8\x9d\xe0\xb8\xb7\xe0\xb8\x81\xe0\xb8\x8b\xe0\xb9\x89\xe0\xb8\xad\xe0\xb8\xa1\xe0\xb8\xa1\xe0\xb8\xb4\xe0\xb8\x81\xe0\xb8\x8b\xe0\xb9\x8c\xe0\xb9\x80\xe0\xb8\xaa\xe0\xb8\xb5\xe0\xb8\xa2\xe0\xb8\x87\xe0\xb9\x81\xe0\xb8\xa5\xe0\xb8\xb0\xe0\xb8\x88\xe0\xb8\xb3\xe0\xb8\xa5\xe0\xb8\xad\xe0\xb8\x87\xe0\xb8\x81\xe0\xb8\xb2\xe0\xb8\xa3\xe0\xb8\x97\xe0\xb8\xb3\xe0\xb8\x87\xe0\xb8\xb2\xe0\xb8\x99\xe0\xb8\x84\xe0\xb8\xad\xe0\xb8\x99\xe0\xb9\x82\xe0\xb8\x8b\xe0\xb8\xa5\xe0\xb9\x80\xe0\xb8\xaa\xe0\xb8\xb5\xe0\xb8\xa2\xe0\xb8\x87\n"
        "2. \xe0\xb8\x9c\xe0\xb8\xb9\xe0\xb9\x89\xe0\xb9\x83\xe0\xb8\x8a\xe0\xb9\x89\xe0\xb8\x87\xe0\xb8\xb2\xe0\xb8\x99\xe0\xb8\xaa\xe0\xb8\xb2\xe0\xb8\xa1\xe0\xb8\xb2\xe0\xb8\xa3\xe0\xb8\x96\xe0\xb8\x99\xe0\xb8\xb3\xe0\xb9\x80\xe0\xb8\x82\xe0\xb9\x89\xe0\xb8\xb2\xe0\xb9\x83\xe0\xb8\x9f\xe0\xb8\xa5\xe0\xb9\x8c\xe0\xb9\x80\xe0\xb8\xaa\xe0\xb8\xb5\xe0\xb8\xa2\xe0\xb8\x87 Multitrack \xe0\xb9\x81\xe0\xb8\xa5\xe0\xb8\xb0\xe0\xb8\x9b\xe0\xb8\xa3\xe0\xb8\xb1\xe0\xb8\x9a\xe0\xb9\x81\xe0\xb8\x95\xe0\xb9\x88\xe0\xb8\x87\xe0\xb8\xa1\xe0\xb8\xb4\xe0\xb8\x81\xe0\xb8\x8b\xe0\xb9\x8c\xe0\xb9\x84\xe0\xb8\x94\xe0\xb9\x89\xe0\xb8\xad\xe0\xb8\xa2\xe0\xb9\x88\xe0\xb8\xb2\xe0\xb8\x87\xe0\xb8\xad\xe0\xb8\xb4\xe0\xb8\xaa\xe0\xb8\xa3\xe0\xb8\xb0\n"
        "3. \xe0\xb8\x81\xe0\xb8\xa3\xe0\xb8\xb8\xe0\xb8\x93\xe0\xb8\xb2\xe0\xb9\x83\xe0\xb8\x8a\xe0\xb9\x89\xe0\xb9\x84\xe0\xb8\x9f\xe0\xb8\xa5\xe0\xb9\x8c\xe0\xb9\x80\xe0\xb8\xaa\xe0\xb8\xb5\xe0\xb8\xa2\xe0\xb8\x87\xe0\xb8\x97\xe0\xb8\xb5\xe0\xb9\x88\xe0\xb9\x84\xe0\xb8\x94\xe0\xb9\x89\xe0\xb8\xa3\xe0\xb8\xb1\xe0\xb8\x9a\xe0\xb8\xad\xe0\xb8\x99\xe0\xb8\xb8\xe0\xb8\x8d\xe0\xb8\xb2\xe0\xb8\x95\xe0\xb8\xab\xe0\xb8\xa3\xe0\xb8\xb7\xe0\xb8\xad\xe0\xb8\xa1\xe0\xb8\xb5\xe0\xb8\xa5\xe0\xb8\xb4\xe0\xb8\x82\xe0\xb8\xaa\xe0\xb8\xb4\xe0\xb8\x97\xe0\xb8\x98\xe0\xb8\xb4\xe0\xb9\x8c\xe0\xb8\x96\xe0\xb8\xb9\xe0\xb8\x81\xe0\xb8\x95\xe0\xb9\x89\xe0\xb8\xad\xe0\xb8\x87\n"
        "4. \xe0\xb9\x81\xe0\xb8\xad\xe0\xb8\x9e\xe0\xb8\x99\xe0\xb8\xb5\xe0\xb9\x89\xe0\xb9\x83\xe0\xb8\xab\xe0\xb9\x89\xe0\xb8\x9a\xe0\xb8\xa3\xe0\xb8\xb4\xe0\xb8\x81\xe0\xb8\xb2\xe0\xb8\xa3\xe0\xb8\x9f\xe0\xb8\xa3\xe0\xb8\xb5\xe0\xb8\xaa\xe0\xb8\xb3\xe0\xb8\xab\xe0\xb8\xa3\xe0\xb8\xb1\xe0\xb8\x9a\xe0\xb8\x81\xe0\xb8\xb2\xe0\xb8\xa3\xe0\xb8\xa8\xe0\xb8\xb6\xe0\xb8\x81\xe0\xb8\xa9\xe0\xb8\xb2\xe0\xb9\x81\xe0\xb8\xa5\xe0\xb8\xb0\xe0\xb8\x81\xe0\xb8\xb2\xe0\xb8\xa3\xe0\xb8\x9d\xe0\xb8\xb6\xe0\xb8\x81\xe0\xb8\x9d\xe0\xb8\x99\n\n"
        "-----------------------------------------\n"
        "\xe0\xb8\x9c\xe0\xb8\xb9\xe0\xb9\x89\xe0\xb8\x9e\xe0\xb8\xb1\xe0\xb8\x92\xe0\xb8\x99\xe0\xb8\xb2 (Developer): premsak srilarin\n"
        "Facebook \xe0\xb9\x81\xe0\xb8\x88\xe0\xb9\x89\xe0\xb8\x87\xe0\xb8\x9b\xe0\xb8\xb1\xe0\xb8\x8d\xe0\xb8\x87\xe0\xb8\xb2\xe0\xb8\x81\xe0\xb8\xb2\xe0\xb8\xa3\xe0\xb9\x83\xe0\xb8\x8a\xe0\xb9\x89\xe0\xb8\x87\xe0\xb8\xb2\xe0\xb8\x99 / \xe0\xb8\x95\xe0\xb8\xb4\xe0\xb8\x94\xe0\xb8\x95\xe0\xb9\x88\xe0\xb8\xad:\n"
        "https://www.facebook.com/premsak.sed\n"
        "-----------------------------------------"
    );

    juce::AlertWindow::showOkCancelBox (
        juce::AlertWindow::QuestionIcon,
        title,
        message,
        juce::CharPointer_UTF8 ("\xe0\xb8\xa2\xe0\xb8\xad\xe0\xb8\xa1\xe0\xb8\xa3\xe0\xb8\xb1\xe0\xb8\x9a (Accept)"),
        juce::CharPointer_UTF8 ("\xe0\xb8\x95\xe0\xb8\xb4\xe0\xb8\x94\xe0\xb8\x95\xe0\xb9\x88\xe0\xb8\xad Facebook"),
        nullptr,
        juce::ModalCallbackFunction::create ([this, markerFile] (int result) {
            if (result == 1) // Accept
            {
                markerFile.replaceWithText ("accepted on " + juce::Time::getCurrentTime().toISO8601 (true));
            }
            else // Clicked Facebook link
            {
                juce::URL ("https://www.facebook.com/premsak.sed").launchInDefaultBrowser();
                markerFile.replaceWithText ("accepted on " + juce::Time::getCurrentTime().toISO8601 (true));
            }
            showSponsorOverlayIfNeeded();
        })
    );
}

void MainComponent::showSponsorOverlayIfNeeded()
{
    if (! AppEdition::hasShopeeAds())
        return;

    if (SponsorOverlay::shouldShowThisSession())
    {
        sponsorOverlay = std::make_unique<SponsorOverlay> ([this] {
            if (sponsorOverlay != nullptr)
                sponsorOverlay->setVisible (false);
        });
        addAndMakeVisible (sponsorOverlay.get());
        sponsorOverlay->setBounds (getLocalBounds());
        sponsorOverlay->toFront (true);
    }
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
        effectsScreen.setSelectedStrip (detailScreen.getSelectedStripType(), detailScreen.getSelectedStripIndex());
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
    menu.addItem (1001, juce::CharPointer_UTF8 ("\x41\x75\x64\x69\x6f\x20\x26\x20\x53\x79\x73\x74\x65\x6d\x20\x53\x65\x74\x75\x70\x20\x28\xe0\xb8\x81\xe0\xb8\xb2\xe0\xb8\xa3\xe0\xb8\x95\xe0\xb8\xb1\xe0\xb9\x89\xe0\xb8\x87\xe0\xb8\x84\xe0\xb9\x88\xe0\xb8\xb2\x29"), true, currentScreen == NavSetup);
    menu.addSeparator();
    menu.addItem (1002, "Mixer Console (Home)", true, currentScreen == NavHome);
    menu.addItem (1003, "Channel Detail (Preamp / EQ / Dyn)", true, currentScreen == NavDetail);
    menu.addItem (1004, "Multitrack Player (Tracks)", true, currentScreen == NavTracks);
    menu.addItem (1005, "Virtual Effects Rack", true, currentScreen == NavEffects);
    menu.addItem (1006, "Scenes & Snapshots", true, currentScreen == NavScenes);
    menu.addSeparator();
    menu.addItem (1008, juce::CharPointer_UTF8 ("\x53\x61\x76\x65\x20\x43\x75\x72\x72\x65\x6e\x74\x20\x53\x65\x73\x73\x69\x6f\x6e\x20\x28\xe0\xb8\x9a\xe0\xb8\xb1\xe0\xb8\x99\xe0\xb8\x97\xe0\xb8\xb6\xe0\xb8\x81\xe0\xb8\x87\xe0\xb8\xb2\xe0\xb8\x99\xe0\xb8\x9b\xe0\xb8\xb1\xe0\xb8\x88\xe0\xb8\x88\xe0\xb8\xb8\xe0\xb8\x9a\xe0\xb8\xb1\xe0\xb8\x99\x29"), true);
    menu.addItem (1009, juce::CharPointer_UTF8 ("\x52\x65\x6c\x6f\x61\x64\x20\x53\x61\x76\x65\x64\x20\x53\x65\x73\x73\x69\x6f\x6e\x20\x28\xe0\xb9\x82\xe0\xb8\xab\xe0\xb8\xa5\xe0\xb8\x94\xe0\xb8\x87\xe0\xb8\xb2\xe0\xb8\x99\xe0\xb8\x97\xe0\xb8\xb5\xe0\xb9\x88\xe0\xb8\x9a\xe0\xb8\xb1\xe0\xb8\x99\xe0\xb8\x97\xe0\xb8\xb6\xe0\xb8\x81\xe0\xb9\x84\xe0\xb8\xa7\xe0\xb9\x89\x29"), ProjectModel::hasSavedSession());
    menu.addSeparator();
    menu.addItem (1010, juce::CharPointer_UTF8 ("\x54\x65\x72\x6d\x73\x20\x26\x20\x44\x65\x76\x65\x6c\x6f\x70\x65\x72\x20\x49\x6e\x66\x6f\x20\x28\xe0\xb9\x80\xe0\xb8\x87\xe0\xb8\xb7\xe0\xb9\x88\xe0\xb8\xad\xe0\xb8\x99\xe0\xb9\x84\xe0\xb8\x82\xe0\xb9\x81\xe0\xb8\xa5\xe0\xb8\xb0\xe0\xb8\x82\xe0\xb9\x89\xe0\xb8\xad\xe0\xb8\xa1\xe0\xb8\xb9\xe0\xb8\xa5\xe0\xb8\x9c\xe0\xb8\xb9\xe0\xb9\x89\xe0\xb8\x9e\xe0\xb8\xb1\xe0\xb8\x92\xe0\xb8\x99\xe0\xb8\xb2\x29"), true);
    menu.addItem (1007, showTopNavBar ? juce::CharPointer_UTF8 ("\x48\x69\x64\x65\x20\x54\x6f\x70\x20\x4d\x65\x6e\x75\x20\x42\x61\x72\x20\x28\xe0\xb8\x8b\xe0\xb9\x88\xe0\xb8\xad\xe0\xb8\x99\xe0\xb9\x81\xe0\xb8\x96\xe0\xb8\x9a\xe0\xb9\x80\xe0\xb8\xa1\xe0\xb8\x99\xe0\xb8\xb9\x29")
                                       : juce::CharPointer_UTF8 ("\x53\x68\x6f\x77\x20\x54\x6f\x70\x20\x4d\x65\x6e\x75\x20\x42\x61\x72\x20\x28\xe0\xb9\x81\xe0\xb8\xaa\xe0\xb8\x94\xe0\xb8\x87\xe0\xb9\x81\xe0\xb8\x96\xe0\xb8\x9a\xe0\xb9\x80\xe0\xb8\xa1\xe0\xb8\x99\xe0\xb8\xb9\x29"), true, showTopNavBar);

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
                case 1008:
                    saveCurrentSession (true);
                    break;
                case 1009:
                    restorePreviousSession();
                    break;
                case 1010:
                {
                    auto markerFile = getTermsAcceptedMarkerFile();
                    markerFile.deleteFile();
                    checkAndShowTermsOfService();
                    break;
                }
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

    // Bottom Transport bar (enlarged 72px for touch-friendly timeline & time zoom)
    const int transportH = (getHeight() < 500) ? 64 : 72;
    transportBar.setBounds (area.removeFromBottom (transportH));

    // Center Active Screen (takes maximum vertical height with top menu hidden)
    homeScreen.setBounds (area);
    detailScreen.setBounds (area);
    tracksScreen.setBounds (area);
    effectsScreen.setBounds (area);
    scenesScreen.setBounds (area);
    setupScreen.setBounds (area);
    placeholderLabel.setBounds (area);

    if (sponsorOverlay != nullptr)
        sponsorOverlay->setBounds (getLocalBounds());
}

} // namespace mps
