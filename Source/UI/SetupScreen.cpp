// Mix Practice Studio - Audio Device & Hardware Setup Screen Implementation
#include "SetupScreen.h"

namespace mps
{

SetupScreen::SetupScreen (AudioEngine& eng, X32OscClient& osc)
    : engine (eng), oscClient (osc)
{
    // --- Section 1: Audio Device Setup ---
    titleLabel.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, ConsoleLookAndFeel::textPrimary);
    titleLabel.setText ("Audio Hardware & Output Device Setup", juce::dontSendNotification);
    addAndMakeVisible (titleLabel);

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

    // --- Section 2: Behringer X32 / X32 Rack OSC Remote Network ---
    x32TitleLabel.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    x32TitleLabel.setColour (juce::Label::textColourId, ConsoleLookAndFeel::accentYellow);
    x32TitleLabel.setText ("Behringer X32 / X32 Rack Hardware OSC Remote (Wi-Fi / LAN)", juce::dontSendNotification);
    addAndMakeVisible (x32TitleLabel);

    x32IpLabel.setFont (juce::FontOptions (12.0f));
    x32IpLabel.setColour (juce::Label::textColourId, ConsoleLookAndFeel::textMuted);
    x32IpLabel.setText ("Console IP Address:", juce::dontSendNotification);
    addAndMakeVisible (x32IpLabel);

    x32IpEditor.setText ("192.168.1.100");
    x32IpEditor.setFont (juce::FontOptions (13.0f));
    x32IpEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff14171c));
    x32IpEditor.setColour (juce::TextEditor::outlineColourId, ConsoleLookAndFeel::borderSubtle);
    addAndMakeVisible (x32IpEditor);

    x32PortLabel.setFont (juce::FontOptions (12.0f));
    x32PortLabel.setColour (juce::Label::textColourId, ConsoleLookAndFeel::textMuted);
    x32PortLabel.setText ("Port (UDP):", juce::dontSendNotification);
    addAndMakeVisible (x32PortLabel);

    x32PortEditor.setText ("10023");
    x32PortEditor.setFont (juce::FontOptions (13.0f));
    x32PortEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff14171c));
    x32PortEditor.setColour (juce::TextEditor::outlineColourId, ConsoleLookAndFeel::borderSubtle);
    addAndMakeVisible (x32PortEditor);

    x32ConnectButton.addListener (this);
    x32ConnectButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff00838f)); // Cyan
    addAndMakeVisible (x32ConnectButton);

    x32AutoDiscoverButton.addListener (this);
    x32AutoDiscoverButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff37474f));
    addAndMakeVisible (x32AutoDiscoverButton);

    x32StatusLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    x32StatusLabel.setColour (juce::Label::textColourId, juce::Colour (0xffe53935)); // Red disconnected
    x32StatusLabel.setText ("Status: OFFLINE (Virtual Practice Mode Active)", juce::dontSendNotification);
    addAndMakeVisible (x32StatusLabel);

    startTimerHz (4);
}

SetupScreen::~SetupScreen()
{
    stopTimer();
}

void SetupScreen::timerCallback()
{
    // Audio device info
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

    // X32 Network Status
    if (oscClient.isConnected())
    {
        x32ConnectButton.setButtonText ("DISCONNECT");
        x32ConnectButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xffd32f2f));
        x32StatusLabel.setColour (juce::Label::textColourId, juce::Colour (0xff00e676)); // Green
        x32StatusLabel.setText ("Status: CONNECTED -> " + oscClient.getConsoleModel()
                                + " @ " + oscClient.getConsoleIp()
                                + " (v" + oscClient.getFirmwareVersion() + ")",
                                juce::dontSendNotification);
    }
    else
    {
        x32ConnectButton.setButtonText ("CONNECT TO X32");
        x32ConnectButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff00838f));
        x32StatusLabel.setColour (juce::Label::textColourId, juce::Colour (0xffe53935)); // Red
        x32StatusLabel.setText ("Status: OFFLINE (Virtual Practice Mode Active)", juce::dontSendNotification);
    }
}

void SetupScreen::buttonClicked (juce::Button* b)
{
    if (b == &x32ConnectButton)
    {
        if (oscClient.isConnected())
        {
            oscClient.disconnect();
        }
        else
        {
            const juce::String ip = x32IpEditor.getText().trim();
            const int port = x32PortEditor.getText().getIntValue();
            oscClient.connectToConsole (ip, port > 0 ? port : 10023);
        }
    }
    else if (b == &x32AutoDiscoverButton)
    {
        oscClient.autoDiscover();
        juce::AlertWindow::showMessageBoxAsync (
            juce::AlertWindow::InfoIcon, "X32 Network Discovery",
            "Broadcasting discovery packet (/xinfo) across local network.\nIf X32 is connected to the same Wi-Fi/LAN, it will respond.");
    }
    else if (b == &useSpeakersButton || b == &useHeadphonesButton)
    {
        auto& dm = engine.getDeviceManager();
        auto* type = dm.getCurrentDeviceTypeObject();
        if (type == nullptr) return;

        type->scanForDevices();
        auto outNames = type->getDeviceNames (false);

        const juce::String keyword = (b == &useSpeakersButton) ? "speaker" : "headphone";
        for (const auto& name : outNames)
        {
            if (name.containsIgnoreCase (keyword))
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

    auto bounds = getLocalBounds().reduced (16).toFloat();
    const float halfH = (bounds.getHeight() - 12.0f) * 0.5f;

    auto topCard = bounds.removeFromTop (halfH);
    bounds.removeFromTop (12.0f);
    auto bottomCard = bounds;

    // Top Card (Audio Device)
    g.setColour (ConsoleLookAndFeel::bgPanel);
    g.fillRoundedRectangle (topCard, 4.0f);
    g.setColour (ConsoleLookAndFeel::borderSubtle);
    g.drawRoundedRectangle (topCard, 4.0f, 1.0f);

    // Bottom Card (X32 Network Remote)
    g.setColour (ConsoleLookAndFeel::bgPanel);
    g.fillRoundedRectangle (bottomCard, 4.0f);
    g.setColour (ConsoleLookAndFeel::borderSubtle);
    g.drawRoundedRectangle (bottomCard, 4.0f, 1.0f);
}

void SetupScreen::resized()
{
    auto area = getLocalBounds().reduced (24);
    const int halfH = (area.getHeight() - 16) / 2;

    // Top: Audio Device Setup
    auto topArea = area.removeFromTop (halfH);
    titleLabel.setBounds (topArea.removeFromTop (26));
    statusLabel.setBounds (topArea.removeFromTop (20));
    topArea.removeFromTop (6);

    auto quickRow = topArea.removeFromTop (28);
    useSpeakersButton.setBounds (quickRow.removeFromLeft (180).reduced (2));
    quickRow.removeFromLeft (8);
    useHeadphonesButton.setBounds (quickRow.removeFromLeft (180).reduced (2));
    topArea.removeFromTop (8);

    if (deviceSelector != nullptr)
        deviceSelector->setBounds (topArea);

    area.removeFromTop (16);

    // Bottom: X32 OSC Remote Network Setup
    auto bottomArea = area;
    x32TitleLabel.setBounds (bottomArea.removeFromTop (26));
    x32StatusLabel.setBounds (bottomArea.removeFromTop (20));
    bottomArea.removeFromTop (10);

    auto ipRow = bottomArea.removeFromTop (32);
    x32IpLabel.setBounds (ipRow.removeFromLeft (140));
    x32IpEditor.setBounds (ipRow.removeFromLeft (180).reduced (0, 2));
    ipRow.removeFromLeft (16);
    x32PortLabel.setBounds (ipRow.removeFromLeft (90));
    x32PortEditor.setBounds (ipRow.removeFromLeft (90).reduced (0, 2));

    bottomArea.removeFromTop (12);
    auto btnRow = bottomArea.removeFromTop (34);
    x32ConnectButton.setBounds (btnRow.removeFromLeft (180));
    btnRow.removeFromLeft (12);
    x32AutoDiscoverButton.setBounds (btnRow.removeFromLeft (160));
}

} // namespace mps
