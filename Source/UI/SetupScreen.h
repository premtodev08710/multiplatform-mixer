// Mix Practice Studio - Audio Device & Hardware Setup Screen
#pragma once

#include "../Audio/AudioEngine.h"
#include "../Network/X32OscClient.h"
#include "ConsoleLookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>

namespace mps
{

class SetupScreen : public juce::Component,
                    public juce::Button::Listener,
                    private juce::Timer
{
public:
    SetupScreen (AudioEngine& engine, X32OscClient& oscClient);
    ~SetupScreen() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void buttonClicked (juce::Button* button) override;

    std::function<void()> onBack;

private:
    juce::TextButton backButton { "< Mixer Console" };
    void timerCallback() override;

    AudioEngine& engine;
    X32OscClient& oscClient;

    // JUCE Audio Device Selector Component (CoreAudio output selector, buffer, sample rate)
    std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;

    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::TextButton useSpeakersButton { "Switch to MacBook Speakers" };
    juce::TextButton useHeadphonesButton { "Switch to Headphones" };

    // Behringer X32 OSC Hardware Remote Network Panel
    juce::Label x32TitleLabel;
    juce::Label x32IpLabel;
    juce::TextEditor x32IpEditor;
    juce::Label x32PortLabel;
    juce::TextEditor x32PortEditor;
    juce::TextButton x32ConnectButton { "CONNECT TO X32" };
    juce::TextButton x32AutoDiscoverButton { "AUTO DISCOVER" };
    juce::Label x32StatusLabel;

    // Pro License & Annual Prepaid Key Panel
    juce::Label licenseTitleLabel;
    juce::Label licenseStatusLabel;
    juce::Label licenseKeyLabel;
    juce::TextEditor licenseKeyEditor;
    juce::TextButton activateKeyButton;
    juce::TextButton buyKeyButton;
    void refreshLicenseStatus();
};

} // namespace mps
