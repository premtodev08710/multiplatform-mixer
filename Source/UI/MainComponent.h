// Mix Practice Studio - Main Application Component & Navigation Coordinator
#pragma once

#include "../Audio/AudioEngine.h"
#include "../Model/ProjectModel.h"
#include "../Network/AdMobManager.h"
#include "../Network/InstallTelemetry.h"
#include "../Network/X32OscClient.h"
#include "ConsoleLookAndFeel.h"
#include "DetailScreen.h"
#include "EffectsScreen.h"
#include "HomeScreen.h"
#include "ScenesScreen.h"
#include "SetupScreen.h"
#include "TracksScreen.h"
#include "TransportBar.h"
#include "SponsorOverlay.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace mps
{

class MainComponent : public juce::Component,
                      public juce::Button::Listener,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    bool keyPressed (const juce::KeyPress& key) override;
    void buttonClicked (juce::Button* button) override;

    void showScreen (int screenIndex);

    void saveCurrentSession (bool showNotification = false);
    void restorePreviousSession();
    void checkAndShowTermsOfService();
    void showSponsorOverlayIfNeeded();

    EffectsScreen& getEffectsScreen() noexcept { return effectsScreen; }
    AudioEngine& getAudioEngine() noexcept { return engine; }

private:
    void timerCallback() override;
    ConsoleLookAndFeel lookAndFeel;

    AudioEngine engine;
    ProjectModel project { engine };
    X32OscClient oscClient { engine, project };
    InstallTelemetry telemetry;
    AdMobManager adMob;

    // Top Navigation Tabs
    enum NavTab
    {
        NavHome = 0, NavDetail, NavTracks, NavEffects,
        NavScenes, NavMeters, NavRecorder, NavRouting,
        NavMonitor, NavSetup, NumNavTabs
    };

    juce::TextButton navButtons[NumNavTabs];
    int currentScreen = NavHome;
    bool showTopNavBar = false;

    class SettingsButton : public juce::Button
    {
    public:
        SettingsButton();
        void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    };

    SettingsButton settingsButton;
    void showSettingsMenu();

    // Screens
    HomeScreen homeScreen { engine, project };
    DetailScreen detailScreen { engine, project };
    TracksScreen tracksScreen { engine, project };
    EffectsScreen effectsScreen { engine, project };
    ScenesScreen scenesScreen { engine, project };
    SetupScreen setupScreen { engine, oscClient };

    // Placeholder view for roadmap tabs (Effects, Meters, etc.)
    juce::Label placeholderLabel;

    // Bottom Transport
    TransportBar transportBar { engine };

    // Sponsor Shopee Overlay
    std::unique_ptr<SponsorOverlay> sponsorOverlay;
};

} // namespace mps
