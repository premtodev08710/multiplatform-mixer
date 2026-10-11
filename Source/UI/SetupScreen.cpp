// Mix Practice Studio - Audio Device & Hardware Setup Screen Implementation
#include "SetupScreen.h"

namespace mps
{

SetupScreen::SetupScreen (AudioEngine& eng) : engine (eng)
{
    titleLabel.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, ConsoleLookAndFeel::textPrimary);
    titleLabel.setText ("Audio Hardware & Output Device Setup", juce::dontSendNotification);
    addAndMakeVisible (titleLabel);

    // Audio Device Selector (No inputs needed, up to 2 outputs)
    deviceSelector = std::make_unique<juce::AudioDeviceSelectorComponent> (
        engine.getDeviceManager(),
        0, 0,   // Min/Max Input channels
        1, 2,   // Min/Max Output channels
        false,  // Show MIDI inputs
        false,  // Show MIDI outputs
        true,   // Stereo pairs
        false   // Hide advanced
    );
    addAndMakeVisible (deviceSelector.get());

    useSpeakersButton.addListener (this);
    useSpeakersButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff252e38));
    addAndMakeVisible (useSpeakersButton);

    useHeadphonesButton.addListener (this);
    useHeadphonesButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff252e38));
    addAndMakeVisible (useHeadphonesButton);

    statusLabel.setFont (juce::FontOptions (12.0f));
    statusLabel.setColour (juce::Label::textColourId, ConsoleLookAndFeel::accentCyan);
    addAndMakeVisible (statusLabel);

    startTimerHz (4); // Refresh device info
}

SetupScreen::~SetupScreen()
{
    stopTimer();
}

void SetupScreen::timerCallback()
{
    auto* dev = engine.getDeviceManager().getCurrentAudioDevice();
    if (dev != nullptr)
    {
        const juce::String name = dev->getName();
        const double sr = dev->getCurrentSampleRate();
        const int bs = dev->getCurrentBufferSizeSamples();
        const double latencyMs = (double) bs / (sr > 0.0 ? sr : 48000.0) * 1000.0;

        statusLabel.setText ("Active Device: " + name
                             + " | Sample Rate: " + juce::String (sr * 0.001, 1) + " kHz"
                             + " | Buffer: " + juce::String (bs) + " samples ("
                             + juce::String (latencyMs, 1) + " ms latency)",
                             juce::dontSendNotification);
    }
    else
    {
        statusLabel.setText ("No Audio Device Selected or Initialized!", juce::dontSendNotification);
    }
}

void SetupScreen::buttonClicked (juce::Button* b)
{
    auto& dm = engine.getDeviceManager();
    auto* type = dm.getCurrentDeviceTypeObject();
    if (type == nullptr) return;

    type->scanForDevices();
    auto outNames = type->getDeviceNames (false);

    if (b == &useSpeakersButton)
    {
        for (const auto& name : outNames)
        {
            if (name.containsIgnoreCase ("speaker") || name.containsIgnoreCase ("ลำโพง"))
            {
                juce::AudioDeviceManager::AudioDeviceSetup setup;
                dm.getAudioDeviceSetup (setup);
                setup.outputDeviceName = name;
                setup.outputChannels.setRange (0, 2, true);
                dm.setAudioDeviceSetup (setup, true);
                break;
            }
        }
    }
    else if (b == &useHeadphonesButton)
    {
        for (const auto& name : outNames)
        {
            if (name.containsIgnoreCase ("headphone") || name.containsIgnoreCase ("หูฟัง"))
            {
                juce::AudioDeviceManager::AudioDeviceSetup setup;
                dm.getAudioDeviceSetup (setup);
                setup.outputDeviceName = name;
                setup.outputChannels.setRange (0, 2, true);
                dm.setAudioDeviceSetup (setup, true);
                break;
            }
        }
    }
}

void SetupScreen::paint (juce::Graphics& g)
{
    g.fillAll (ConsoleLookAndFeel::bgDark);

    auto cardRect = getLocalBounds().reduced (16).toFloat();
    g.setColour (ConsoleLookAndFeel::bgPanel);
    g.fillRoundedRectangle (cardRect, 4.0f);
    g.setColour (ConsoleLookAndFeel::borderSubtle);
    g.drawRoundedRectangle (cardRect, 4.0f, 1.0f);
}

void SetupScreen::resized()
{
    auto area = getLocalBounds().reduced (24);

    titleLabel.setBounds (area.removeFromTop (28));
    statusLabel.setBounds (area.removeFromTop (22));
    area.removeFromTop (8);

    auto quickRow = area.removeFromTop (32);
    useSpeakersButton.setBounds (quickRow.removeFromLeft (200).reduced (2));
    quickRow.removeFromLeft (10);
    useHeadphonesButton.setBounds (quickRow.removeFromLeft (200).reduced (2));
    area.removeFromTop (12);

    if (deviceSelector != nullptr)
        deviceSelector->setBounds (area.removeFromTop (340));
}

} // namespace mps
