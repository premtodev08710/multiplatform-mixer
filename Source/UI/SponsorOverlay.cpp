// Mix Practice Studio - Shopee Affiliate Sponsor Overlay
#include "SponsorOverlay.h"

namespace mps
{

static const juce::StringArray kShopeeAffiliateLinks = {
    "https://s.shopee.co.th/80DYoxalGQ",
    "https://s.shopee.co.th/9pfD0LgqtV",
    "https://s.shopee.co.th/2BFlsHjNlY",
    "https://s.shopee.co.th/3Vl9SmXBNO",
    "https://s.shopee.co.th/7pu8clzF22"
};

bool SponsorOverlay::shouldShowThisSession() noexcept
{
    return ! hasShownThisSession;
}

void SponsorOverlay::markShownThisSession() noexcept
{
    hasShownThisSession = true;
}

SponsorOverlay::SponsorOverlay (std::function<void()> onDismissed)
    : onDismissCallback (std::move (onDismissed))
{
    markShownThisSession();

    // Shopee Orange button styling
    shopeeButton.setButtonText (juce::CharPointer_UTF8 ("\xe0\xb8\xa3\xe0\xb8\xad\xe0\xb8\xaa\xe0\xb8\xb1\xe0\xb8\x81\xe0\xb8\x84\xe0\xb8\xa3\xe0\xb8\xb9\xe0\xb9\x88 (5s)..."));
    shopeeButton.setEnabled (false);
    shopeeButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2a3446));
    shopeeButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffee4d2d));
    shopeeButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    shopeeButton.onClick = [this] { openShopeeAndDismiss(); };
    addAndMakeVisible (shopeeButton);

    skipButton.setButtonText (juce::CharPointer_UTF8 ("\xe0\xb9\x80\xe0\xb8\x82\xe0\xb9\x89\xe0\xb8\xb2\xe0\xb9\x83\xe0\xb8\x8a\xe0\xb9\x89\xe0\xb8\x87\xe0\xb8\xb2\xe0\xb8\x99\xe0\xb9\x82\xe0\xb8\x94\xe0\xb8\xa2\xe0\xb8\x55\xe0\xb8\xa3\xe0\xb8\x87 (Skip)"));
    skipButton.setVisible (false);
    skipButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    skipButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff94a3b8));
    skipButton.onClick = [this] { dismissDirectly(); };
    addAndMakeVisible (skipButton);

    startTimer (1000);
}

SponsorOverlay::~SponsorOverlay()
{
    stopTimer();
}

void SponsorOverlay::timerCallback()
{
    if (countdownSeconds > 1)
    {
        --countdownSeconds;
        shopeeButton.setButtonText (juce::String (juce::CharPointer_UTF8 ("\xe0\xb8\xa3\xe0\xb8\xad\xe0\xb8\xaa\xe0\xb8\xb1\xe0\xb8\x81\xe0\xb8\x84\xe0\xb8\xa3\xe0\xb8\xb9\xe0\xb9\x88 ("))
                                    + juce::String (countdownSeconds) + "s)...");
        repaint();
    }
    else if (countdownSeconds == 1)
    {
        countdownSeconds = 0;
        stopTimer();

        // Enable Shopee Sponsor button with prominent Shopee brand orange
        shopeeButton.setEnabled (true);
        shopeeButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xffee4d2d));
        shopeeButton.setButtonText (juce::CharPointer_UTF8 ("\xe0\xb9\x81\xe0\xb8\xa7\xe0\xb8\xb0\xe0\xb8\x8a\xe0\xb8\xa1 Shopee \xe0\xb9\x80\xe0\xb8\x9e\xe0\xb8\xb7\xe0\xb9\x88\xe0\xb8\xad\xe0\xb9\x80\xe0\xb8\x82\xe0\xb9\x89\xe0\xb8\xb2\xe0\xb9\x83\xe0\xb8\x8a\xe0\xb9\x89\xe0\xb8\x87\xe0\xb8\xb2\xe0\xb8\x99 (Start App)"));
        skipButton.setVisible (true);
        repaint();
    }
}

void SponsorOverlay::openShopeeAndDismiss()
{
    if (kShopeeAffiliateLinks.size() > 0)
    {
        const int index = juce::Random::getSystemRandom().nextInt (kShopeeAffiliateLinks.size());
        const juce::String chosenUrl = kShopeeAffiliateLinks[index];
        juce::URL (chosenUrl).launchInDefaultBrowser();
    }

    dismissDirectly();
}

void SponsorOverlay::dismissDirectly()
{
    setVisible (false);
    if (onDismissCallback != nullptr)
        onDismissCallback();
}

void SponsorOverlay::paint (juce::Graphics& g)
{
    // Semi-transparent backdrop
    g.fillAll (juce::Colours::black.withAlpha (0.75f));

    const auto bounds = getLocalBounds();
    const int dialogW = juce::jmin (520, bounds.getWidth() - 32);
    const int dialogH = juce::jmin (340, bounds.getHeight() - 32);
    const auto dialogRect = juce::Rectangle<int> ((bounds.getWidth() - dialogW) / 2,
                                                  (bounds.getHeight() - dialogH) / 2,
                                                  dialogW, dialogH).toFloat();

    // Dialog card
    g.setColour (juce::Colour (0xff141820));
    g.fillRoundedRectangle (dialogRect, 14.0f);

    // Border
    g.setColour (juce::Colour (0xff2d3748));
    g.drawRoundedRectangle (dialogRect, 14.0f, 1.5f);

    // Shopee badge pill
    auto badgeRect = juce::Rectangle<float> (dialogRect.getX() + 24.0f, dialogRect.getY() + 20.0f, 130.0f, 24.0f);
    g.setColour (juce::Colour (0xffee4d2d));
    g.fillRoundedRectangle (badgeRect, 6.0f);

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText ("SHOPEE SPONSOR", badgeRect, juce::Justification::centred);

    // Title
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.drawText (juce::CharPointer_UTF8 ("\xe0\xb8\xaa\xe0\xb8\x99\xe0\xb8\xb1\xe0\xb8\x9a\xe0\xb8\xaa\xe0\xb8\x99\xe0\xb8\xb8\xe0\xb8\x99\xe0\xb8\x9c\xe0\xb8\xb9\xe0\xb9\x89\xe0\xb8\x9e\xe0\xb8\xb1\xe0\xb8\x92\xe0\xb8\x99\xe0\xb8\xb2 (Mix Practice Studio)"),
                (int) dialogRect.getX() + 24, (int) dialogRect.getY() + 54,
                (int) dialogRect.getWidth() - 48, 28,
                juce::Justification::centredLeft);

    // Message
    g.setColour (juce::Colour (0xffcbd5e1));
    g.setFont (juce::FontOptions (13.5f));
    const juce::String msg = juce::CharPointer_UTF8 (
        "\xe0\xb8\x82\xe0\xb8\xad\xe0\xb8\x9a\xe0\xb8\x84\xe0\xb8\xb8\xe0\xb8\x93\xe0\xb8\x97\xe0\xb8\xb5\xe0\xb9\x88\xe0\xb8\xa3\xe0\xb9\x88\xe0\xb8\xa7\xe0\xb8\xa1\xe0\xb8\xaa\xe0\xb8\x99\xe0\xb8\xb1\xe0\xb8\x9a\xe0\xb8\xaa\xe0\xb8\x99\xe0\xb8\xb8\xe0\xb8\x99\xe0\xb9\x81\xe0\xb8\xad\xe0\xb8\x9b\xe0\xb8\x9d\xe0\xb8\xb6\xe0\xb8\x81\xe0\xb8\xa1\xe0\xb8\xb4\xe0\xb8\x81\xe0\xb8\x8b\xe0\xb9\x8c\xe0\xb9\x80\xe0\xb8\xaa\xe0\xb8\xb5\xe0\xb8\xa2\xe0\xb8\x87\xe0\xb8\x9f\xe0\xb8\xa3\xe0\xb8\xb5!\n"
        "\xe0\xb9\x81\xe0\xb8\xa7\xe0\xb8\xb0\xe0\xb8\x8a\xe0\xb8\xa1\xe0\xb8\xad\xe0\xb8\xb8\xe0\xb8\x9b\xe0\xb8\x81\xe0\xb8\xa3\xe0\xb8\x93\xe0\xb9\x8c\xe0\xb9\x80\xe0\xb8\x84\xe0\xb8\xa3\xe0\xb8\xb7\xe0\xb9\x88\xe0\xb8\xad\xe0\xb8\x87\xe0\xb8\x90\xe0\xb8\xb2\xe0\xb8\xa2\xe0\xb9\x81\xe0\xb8\xa5\xe0\xb8\xb0\xe0\xb8\xaa\xe0\xb8\xb4\xe0\xb8\x99\xe0\xb8\x84\xe0\xb9\x89\xe0\xb8\xb2\xe0\xb9\x81\xe0\xb8\x99\xe0\xb8\xb0\xe0\xb8\x99\xe0\xb8\xb3\xe0\xb8\x9a\xe0\xb8\x99 Shopee \xe0\xb8\xaa\xe0\xb8\xb1\xe0\xb8\x81\xe0\xb8\x84\xe0\xb8\xa3\xe0\xb8\xb9\xe0\xb9\x88\xe0\xb9\x80\xe0\xb8\x9e\xe0\xb8\xb7\xe0\xb9\x88\xe0\xb8\xad\xe0\xb9\x80\xe0\xb8\xa3\xe0\xb8\xb4\xe0\xb9\x88\xe0\xb8\xa1\xe0\xb9\x80\xe0\xb8\x82\xe0\xb9\x89\xe0\xb8\xb2\xe0\xb9\x83\xe0\xb8\x8a\xe0\xb9\x89\xe0\xb8\x87\xe0\xb8\xb2\xe0\xb8\x99"
    );
    g.drawFittedText (msg, (int) dialogRect.getX() + 24, (int) dialogRect.getY() + 88,
                      (int) dialogRect.getWidth() - 48, 48,
                      juce::Justification::centredLeft, 2);

    // Countdown / Ready status indicator
    auto statusRect = juce::Rectangle<int> ((int) dialogRect.getX() + 24, (int) dialogRect.getY() + 144,
                                           (int) dialogRect.getWidth() - 48, 32);
    if (countdownSeconds > 0)
    {
        g.setColour (juce::Colour (0xfff59e0b)); // Amber
        g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        const juce::String statusMsg = juce::String (juce::CharPointer_UTF8 ("\xe2\x8f\xb3 \xe0\xb8\x81\xe0\xb8\xa3\xe0\xb8\xb8\xe0\xb8\x83\xe0\xb8\xb2\xe0\xb8\xa3\xe0\xb8\xad\xe0\xb8\xaa\xe0\xb8\xb1\xe0\xb8\x81\xe0\xb8\x84\xe0\xb8\xa3\xe0\xb8\xb9\xe0\xb9\x88... "))
                                     + juce::String (countdownSeconds)
                                     + juce::String (juce::CharPointer_UTF8 (" \xe0\xb8\xa7\xe0\xb8\xb4\xe0\xb8\x99\xe0\xb8\xb2\xe0\xb8\x97\xe0\xb8\xb5"));
        g.drawText (statusMsg, statusRect, juce::Justification::centred);
    }
    else
    {
        g.setColour (juce::Colour (0xff10b981)); // Green
        g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        g.drawText (juce::CharPointer_UTF8 ("\xe2\x9c\x85 \xe0\xb8\x9e\xe0\xb8\xa3\xe0\xb9\x89\xe0\xb8\xad\xe0\xb8\xa1\xe0\xb9\x80\xe0\xb8\x82\xe0\xb9\x89\xe0\xb8\xb2\xe0\xb9\x83\xe0\xb8\x8a\xe0\xb9\x89\xe0\xb8\x87\xe0\xb8\xb2\xe0\xb8\x99\xe0\xb9\x81\xe0\xb8\xa5\xe0\xb9\x89\xe0\xb8\xa7!"),
                    statusRect, juce::Justification::centred);
    }
}

void SponsorOverlay::resized()
{
    const auto bounds = getLocalBounds();
    const int dialogW = juce::jmin (520, bounds.getWidth() - 32);
    const int dialogH = juce::jmin (340, bounds.getHeight() - 32);
    const auto dialogRect = juce::Rectangle<int> ((bounds.getWidth() - dialogW) / 2,
                                                  (bounds.getHeight() - dialogH) / 2,
                                                  dialogW, dialogH);

    const int btnW = dialogW - 48;
    const int btnH = 46;
    const int btnX = dialogRect.getX() + 24;
    const int btnY = dialogRect.getY() + dialogH - 100;

    shopeeButton.setBounds (btnX, btnY, btnW, btnH);
    skipButton.setBounds (btnX, btnY + btnH + 6, btnW, 28);
}

} // namespace mps
