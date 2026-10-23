// Mix Practice Studio - Audio Device & Hardware Setup Screen Implementation
#include "SetupScreen.h"
#include "../Core/AppEdition.h"

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
    backButton.addListener (this);
    backButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff202630));
    backButton.setColour (juce::TextButton::textColourOffId, ConsoleLookAndFeel::accentCyan);
    addAndMakeVisible (backButton);

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

    // --- Section 3: Pro Edition Annual Serial Key (Prepaid Card Model) ---
    licenseTitleLabel.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    licenseTitleLabel.setColour (juce::Label::textColourId, juce::Colour (0xffffd700)); // Gold
    licenseTitleLabel.setText (juce::String::fromUTF8 ("👑 Mix Practice Studio Pro (ระบบสิทธิ์รายปี / Serial Key)"), juce::dontSendNotification);
    addAndMakeVisible (licenseTitleLabel);

    licenseStatusLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    addAndMakeVisible (licenseStatusLabel);

    licenseKeyLabel.setFont (juce::FontOptions (12.0f));
    licenseKeyLabel.setColour (juce::Label::textColourId, ConsoleLookAndFeel::textMuted);
    licenseKeyLabel.setText (juce::String::fromUTF8 ("กรอกรหัสต่ออายุ 1 ปี (Serial Key):"), juce::dontSendNotification);
    addAndMakeVisible (licenseKeyLabel);

    licenseKeyEditor.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    licenseKeyEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff14171c));
    licenseKeyEditor.setColour (juce::TextEditor::outlineColourId, ConsoleLookAndFeel::borderSubtle);
    licenseKeyEditor.setColour (juce::TextEditor::textColourId, juce::Colour (0xff00e5ff));
    licenseKeyEditor.setTextToShowWhenEmpty (juce::String::fromUTF8 ("ตัวอย่าง: MPS-1Y-XXXX-XXXX-XXXX"), juce::Colour (0xff546e7a));
    addAndMakeVisible (licenseKeyEditor);

    activateKeyButton.setButtonText (juce::String::fromUTF8 ("⚡ เติมวัน Pro 1 ปี (+365 วัน)"));
    activateKeyButton.addListener (this);
    activateKeyButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2e7d32)); // Green
    addAndMakeVisible (activateKeyButton);

    buyKeyButton.setButtonText (juce::String::fromUTF8 ("🛒 สั่งซื้อรหัสต่ออายุ (Shopee / FB)"));
    buyKeyButton.addListener (this);
    buyKeyButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xffe65100)); // Shopee Orange
    addAndMakeVisible (buyKeyButton);

    refreshLicenseStatus();

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
    if (b == &backButton)
    {
        if (onBack) onBack();
        return;
    }

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
    else if (b == &activateKeyButton)
    {
        const juce::String inputKey = licenseKeyEditor.getText().trim();
        if (inputKey.isEmpty())
        {
            juce::AlertWindow::showMessageBoxAsync (
                juce::AlertWindow::WarningIcon,
                juce::String::fromUTF8 ("กรุณากรอกรหัส"),
                juce::String::fromUTF8 ("กรุณากรอกรหัสต่ออายุ Pro (Serial Key)")
            );
            return;
        }

        auto res = AppEdition::activateYearlyKey (inputKey);
        if (res.success)
        {
            licenseKeyEditor.clear();
            refreshLicenseStatus();
            juce::AlertWindow::showMessageBoxAsync (
                juce::AlertWindow::InfoIcon,
                juce::String::fromUTF8 ("🎉 เติมวันสำเร็จ!"),
                res.message
            );
        }
        else
        {
            juce::AlertWindow::showMessageBoxAsync (
                juce::AlertWindow::WarningIcon,
                juce::String::fromUTF8 ("รหัสไม่ถูกต้อง"),
                res.message
            );
        }
    }
    else if (b == &buyKeyButton)
    {
        juce::URL ("https://s.shopee.co.th/80DYoxalGQ").launchInDefaultBrowser();
    }
}

void SetupScreen::refreshLicenseStatus()
{
    if (AppEdition::isLifetimePro())
    {
        licenseStatusLabel.setColour (juce::Label::textColourId, juce::Colour (0xffffd700)); // Gold
        licenseStatusLabel.setText (juce::String::fromUTF8 ("สถานะ: PRO EDITION (ถาวร / Lifetime)"), juce::dontSendNotification);
    }
    else if (AppEdition::isTrialActive())
    {
        licenseStatusLabel.setColour (juce::Label::textColourId, juce::Colour (0xff00e5ff)); // Bright Cyan
        licenseStatusLabel.setText (juce::String::fromUTF8 ("สถานะ: PRO TRIAL (ทดลองใช้งานฟรี) • เหลืออีก ") 
                                    + juce::String (AppEdition::getDaysRemaining()) 
                                    + juce::String::fromUTF8 (" วัน (หมดอายุ: ") + AppEdition::getExpiryDateFormatted() + ")", 
                                    juce::dontSendNotification);
    }
    else if (AppEdition::isYearlySubscriptionActive())
    {
        licenseStatusLabel.setColour (juce::Label::textColourId, juce::Colour (0xff00e676)); // Bright Green
        licenseStatusLabel.setText (juce::String::fromUTF8 ("สถานะ: PRO EDITION (รายปี) • เหลืออีก ") 
                                    + juce::String (AppEdition::getDaysRemaining()) 
                                    + juce::String::fromUTF8 (" วัน (หมดอายุ: ") + AppEdition::getExpiryDateFormatted() + ")", 
                                    juce::dontSendNotification);
    }
    else
    {
        if (AppEdition::isTrial())
        {
            licenseStatusLabel.setColour (juce::Label::textColourId, juce::Colour (0xffffa000)); // Amber warning
            licenseStatusLabel.setText (juce::String::fromUTF8 ("สถานะ: หมดช่วงทดลองใช้ 14 วัน (ปรับเป็น Free 8 แทร็ก) • กรอกรหัสเพื่อต่ออายุ 1 ปี"), juce::dontSendNotification);
        }
        else
        {
            licenseStatusLabel.setColour (juce::Label::textColourId, juce::Colour (0xff90a4ae)); // Muted Grey
            licenseStatusLabel.setText (juce::String::fromUTF8 ("สถานะ: FREE EDITION (จำกัด 8 แทร็ก / 2 FX / บันทึกไม่ได้)"), juce::dontSendNotification);
        }
    }
}

void SetupScreen::paint (juce::Graphics& g)
{
    g.fillAll (ConsoleLookAndFeel::bgDark);

    auto bounds = getLocalBounds().reduced (14).toFloat();
    const bool sideBySide = (getWidth() >= 750);

    juce::Rectangle<float> leftCard, rightCard;
    if (sideBySide)
    {
        const float gap = 12.0f;
        const float leftW = std::floor ((bounds.getWidth() - gap) * 0.54f);
        leftCard = bounds.removeFromLeft (leftW);
        bounds.removeFromLeft (gap);
        rightCard = bounds;
    }
    else
    {
        const float gap = 10.0f;
        const float topH = std::floor ((bounds.getHeight() - gap) * 0.52f);
        leftCard = bounds.removeFromTop (topH);
        bounds.removeFromTop (gap);
        rightCard = bounds;
    }

    // Left Card (Audio Device)
    g.setColour (ConsoleLookAndFeel::bgPanel);
    g.fillRoundedRectangle (leftCard, 4.0f);
    g.setColour (ConsoleLookAndFeel::borderSubtle);
    g.drawRoundedRectangle (leftCard, 4.0f, 1.0f);

    // Right Card (X32 Network Remote)
    g.setColour (ConsoleLookAndFeel::bgPanel);
    g.fillRoundedRectangle (rightCard, 4.0f);
    g.setColour (ConsoleLookAndFeel::borderSubtle);
    g.drawRoundedRectangle (rightCard, 4.0f, 1.0f);
}

void SetupScreen::resized()
{
    auto area = getLocalBounds().reduced (14);
    const bool sideBySide = (getWidth() >= 750);

    juce::Rectangle<int> leftArea, rightArea;
    if (sideBySide)
    {
        const int gap = 12;
        const int leftW = (int) std::floor ((area.getWidth() - gap) * 0.54f);
        leftArea = area.removeFromLeft (leftW).reduced (12, 10);
        area.removeFromLeft (gap);
        rightArea = area.reduced (12, 10);
    }
    else
    {
        const int gap = 10;
        const int topH = (int) std::floor ((area.getHeight() - gap) * 0.52f);
        leftArea = area.removeFromTop (topH).reduced (10, 8);
        area.removeFromTop (gap);
        rightArea = area.reduced (10, 8);
    }

    // Left: Audio Device Setup
    auto titleRow = leftArea.removeFromTop (26);
    backButton.setBounds (titleRow.removeFromLeft (130));
    titleRow.removeFromLeft (8);
    titleLabel.setBounds (titleRow);

    statusLabel.setBounds (leftArea.removeFromTop (18));
    leftArea.removeFromTop (4);

    auto quickRow = leftArea.removeFromTop (26);
    const int quickW = (quickRow.getWidth() - 8) / 2;
    useSpeakersButton.setBounds (quickRow.removeFromLeft (quickW));
    quickRow.removeFromLeft (8);
    useHeadphonesButton.setBounds (quickRow);
    leftArea.removeFromTop (6);

    if (deviceSelector != nullptr)
        deviceSelector->setBounds (leftArea);

    // Right: X32 OSC Remote Network Setup
    x32TitleLabel.setBounds (rightArea.removeFromTop (24));
    x32StatusLabel.setBounds (rightArea.removeFromTop (18));
    rightArea.removeFromTop (8);

    auto ipRow = rightArea.removeFromTop (28);
    const int ipLabelW = 40;
    const int portLabelW = 44;
    const int portEditorW = std::clamp (ipRow.getWidth() / 5, 55, 75);
    const int ipEditorW = std::max (90, ipRow.getWidth() - ipLabelW - portLabelW - portEditorW - 12);
    x32IpLabel.setBounds (ipRow.removeFromLeft (ipLabelW));
    x32IpEditor.setBounds (ipRow.removeFromLeft (ipEditorW).reduced (0, 1));
    ipRow.removeFromLeft (6);
    x32PortLabel.setBounds (ipRow.removeFromLeft (portLabelW));
    x32PortEditor.setBounds (ipRow.reduced (0, 1));

    rightArea.removeFromTop (10);
    auto btnRow = rightArea.removeFromTop (32);
    const int btnW = (btnRow.getWidth() - 8) / 2;
    x32ConnectButton.setBounds (btnRow.removeFromLeft (btnW));
    btnRow.removeFromLeft (8);
    x32AutoDiscoverButton.setBounds (btnRow);

    // --- Section 3: Pro License & Annual Serial Key ---
    rightArea.removeFromTop (20);
    licenseTitleLabel.setBounds (rightArea.removeFromTop (22));
    licenseStatusLabel.setBounds (rightArea.removeFromTop (20));
    rightArea.removeFromTop (6);

    licenseKeyLabel.setBounds (rightArea.removeFromTop (18));
    licenseKeyEditor.setBounds (rightArea.removeFromTop (28));
    rightArea.removeFromTop (8);

    auto licBtnRow = rightArea.removeFromTop (32);
    const int licBtnW = (licBtnRow.getWidth() - 8) / 2;
    activateKeyButton.setBounds (licBtnRow.removeFromLeft (licBtnW));
    licBtnRow.removeFromLeft (8);
    buyKeyButton.setBounds (licBtnRow);
}

} // namespace mps
