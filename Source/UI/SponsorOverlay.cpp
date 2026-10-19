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
    setInterceptsMouseClicks (true, true);
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
        repaint();
    }
    else
    {
        countdownSeconds = 0;
        stopTimer();

        // 5 seconds elapsed: automatically redirect to Shopee affiliate link
        openShopeeAndDismiss();
    }
}

void SponsorOverlay::mouseDown (const juce::MouseEvent& e)
{
    juce::ignoreUnused (e);
    // Tapping anywhere on the banner immediately opens Shopee
    openShopeeAndDismiss();
}

void SponsorOverlay::openShopeeAndDismiss()
{
    if (hasRedirected)
        return;

    hasRedirected = true;
    stopTimer();

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
    // Semi-transparent dark backdrop
    g.fillAll (juce::Colours::black.withAlpha (0.80f));

    const auto bounds = getLocalBounds();
    const int dialogW = juce::jmin (560, bounds.getWidth() - 24);
    const int dialogH = juce::jmin (360, bounds.getHeight() - 20);
    const auto dialogRect = juce::Rectangle<int> ((bounds.getWidth() - dialogW) / 2,
                                                  (bounds.getHeight() - dialogH) / 2,
                                                  dialogW, dialogH).toFloat();

    // Dialog outer container
    g.setColour (juce::Colour (0xff121620));
    g.fillRoundedRectangle (dialogRect, 16.0f);
    g.setColour (juce::Colour (0xffee4d2d).withAlpha (0.50f));
    g.drawRoundedRectangle (dialogRect, 16.0f, 2.0f);

    //==========================================================================
    // PROMOTIONAL BANNER SECTION (Top 68%)
    //==========================================================================
    const float bannerH = dialogRect.getHeight() * 0.68f;
    const auto bannerRect = juce::Rectangle<float> (dialogRect.getX(), dialogRect.getY(),
                                                    dialogRect.getWidth(), bannerH);

    // Save state for clipping rounded top corners
    g.saveState();
    juce::Path bannerClip;
    bannerClip.addRoundedRectangle (dialogRect.getX(), dialogRect.getY(),
                                    dialogRect.getWidth(), dialogRect.getHeight(),
                                    16.0f);
    g.reduceClipRegion (bannerClip);

    // Vibrant Shopee Gradient Background
    juce::ColourGradient grad (juce::Colour (0xffee4d2d), bannerRect.getX(), bannerRect.getY(),
                               juce::Colour (0xffc42306), bannerRect.getRight(), bannerRect.getBottom(), false);
    grad.addColour (0.5, juce::Colour (0xffff5722));
    g.setGradientFill (grad);
    g.fillRect (bannerRect);

    // Decorative geometric patterns & visualizer bars in banner
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.fillEllipse (bannerRect.getRight() - 120.0f, bannerRect.getY() - 40.0f, 200.0f, 200.0f);
    g.fillEllipse (bannerRect.getRight() - 60.0f, bannerRect.getBottom() - 80.0f, 140.0f, 140.0f);

    // Draw stylized audio graphic (headphone / equalizer waveforms) on the right
    const float eqX = bannerRect.getRight() - 110.0f;
    const float eqY = bannerRect.getY() + 45.0f;
    static const float kBarHeights[] = { 22.0f, 44.0f, 65.0f, 38.0f, 52.0f, 30.0f, 48.0f, 25.0f };
    for (int i = 0; i < 8; ++i)
    {
        const float bx = eqX + (float) i * 11.0f;
        const float bh = kBarHeights[i];
        g.setColour (juce::Colour (0xfffef08a).withAlpha (0.85f));
        g.fillRoundedRectangle (bx, eqY + (65.0f - bh), 7.0f, bh, 3.5f);
    }

    // Shopee Mall / Official Sponsor Pill
    auto pillRect = juce::Rectangle<float> (bannerRect.getX() + 22.0f, bannerRect.getY() + 18.0f, 148.0f, 24.0f);
    g.setColour (juce::Colours::white);
    g.fillRoundedRectangle (pillRect, 12.0f);
    g.setColour (juce::Colour (0xffee4d2d));
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText (juce::CharPointer_UTF8 ("\xf0\x9f\x9b\x92 SHOPEE SPONSOR"), pillRect, juce::Justification::centred);

    // Big Bold Thai Promo Title
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText (juce::CharPointer_UTF8 ("\xe0\xb8\xa1\xe0\xb8\xab\xe0\xb8\x81\xe0\xb8\xa3\xe0\xb8\xa3\xe0\xb8\xa1\xe0\xb9\x80\xe0\xb8\x84\xe0\xb8\xa3\xe0\xb8\xb7\xe0\xb9\x88\xe0\xb8\xad\xe0\xb8\x87\xe0\xb9\x80\xe0\xb8\xaa\xe0\xb8\xb5\xe0\xb8\xa2\xe0\xb8\x87 & \xe0\xb8\xaa\xe0\xb8\x95\xe0\xb8\xb9\xe0\xb8\x94\xe0\xb8\xb4\xe0\xb9\x82\xe0\xb8\xad"),
                (int) bannerRect.getX() + 22, (int) bannerRect.getY() + 50,
                (int) bannerRect.getWidth() - 140, 30,
                juce::Justification::centredLeft);

    // Subtitle / Equipment categories
    g.setColour (juce::Colour (0xfffff1ee));
    g.setFont (juce::FontOptions (13.5f, juce::Font::plain));
    g.drawText (juce::CharPointer_UTF8 ("\xe0\xb8\xab\xe0\xb8\xb9\xe0\xb8\x9f\xe0\xb8\xb1\xe0\xb8\x87\xe0\xb8\xa1\xe0\xb8\xad\xe0\xb8\x99\xe0\xb8\xb4\xe0\xb9\x80\xe0\xb8\x95\xe0\xb8\xad\xe0\xb8\xa3\xe0\xb9\x8c \xe2\x80\xa2 Audio Interface \xe2\x80\xa2 \xe0\xb9\x84\xe0\xb8\xa1\xe0\xb9\x82\xe0\xb8\x84\xe0\xb8\xa3\xe0\xb9\x82\xe0\xb8\x9f\xe0\xb8\x99 \xe2\x80\xa2 \xe0\xb8\xa5\xe0\xb8\xb3\xe0\xb9\x82\xe0\xb8\x9e\xe0\xb8\x87"),
                (int) bannerRect.getX() + 22, (int) bannerRect.getY() + 82,
                (int) bannerRect.getWidth() - 140, 22,
                juce::Justification::centredLeft);

    // Promo Badges (Discount + Free Shipping)
    const float badgeY = bannerRect.getY() + 112.0f;
    auto discBadge = juce::Rectangle<float> (bannerRect.getX() + 22.0f, badgeY, 115.0f, 26.0f);
    g.setColour (juce::Colour (0xfffef08a)); // Light yellow
    g.fillRoundedRectangle (discBadge, 5.0f);
    g.setColour (juce::Colour (0xff78350f)); // Deep brown
    g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    g.drawText (juce::CharPointer_UTF8 ("\xe2\x9a\xa1 \xe0\xb8\xa5\xe0\xb8\x94\xe0\xb8\xaa\xe0\xb8\xb9\xe0\xb8\x87\xe0\xb8\xaa\xe0\xb8\xb8\xe0\xb8\x94 70%"), discBadge, juce::Justification::centred);

    auto shipBadge = juce::Rectangle<float> (bannerRect.getX() + 145.0f, badgeY, 130.0f, 26.0f);
    g.setColour (juce::Colour (0xffdbeafe)); // Light blue
    g.fillRoundedRectangle (shipBadge, 5.0f);
    g.setColour (juce::Colour (0xff1e3a8a)); // Deep blue
    g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    g.drawText (juce::CharPointer_UTF8 ("\xf0\x9f\x9a\x9a \xe0\xb9\x82\xe0\xb8\x84\xe0\xb9\x89\xe0\xb8\x94\xe0\xb8\xaa\xe0\xb9\x88\xe0\xb8\x87\xe0\xb8\x9f\xe0\xb8\xa3\xe0\xb8\xb5\xe0\xb8\x9e\xe0\xb8\xb4\xe0\xb9\x80\xe0\xb8\xa8\xe0\xb8\xa9"), shipBadge, juce::Justification::centred);

    // Tap to visit notice
    g.setColour (juce::Colours::white.withAlpha (0.90f));
    g.setFont (juce::FontOptions (11.5f, juce::Font::italic));
    g.drawText (juce::CharPointer_UTF8 ("(\xe0\xb9\x81\xe0\xb8\x95\xe0\xb8\xb8\xe0\xb8\xb0\xe0\xb8\x97\xe0\xb8\xb5\xe0\xb9\x88\xe0\xb8\xa3\xe0\xb8\xb9\xe0\xb8\x9b\xe0\xb9\x80\xe0\xb8\x9e\xe0\xb8\xb7\xe0\xb9\x88\xe0\xb8\xad\xe0\xb9\x80\xe0\xb8\x9b\xe0\xb8\xb4\xe0\xb8\x94 Shopee \xe0\xb9\x84\xe0\xb8\x94\xe0\xb9\x89\xe0\xb8\x97\xe0\xb8\xb1\xe0\xb8\x99\xe0\xb8\x97\xe0\xb8\xb5)"),
                (int) bannerRect.getX() + 22, (int) bannerRect.getY() + 148,
                (int) bannerRect.getWidth() - 44, 18,
                juce::Justification::centredLeft);

    g.restoreState();

    //==========================================================================
    // COUNTDOWN & AUTO-REDIRECT FOOTER (Bottom 32%)
    //==========================================================================
    const float footerY = bannerRect.getBottom();
    const float footerH = dialogRect.getBottom() - footerY;
    const auto footerRect = juce::Rectangle<float> (dialogRect.getX(), footerY,
                                                    dialogRect.getWidth(), footerH);

    g.setColour (juce::Colour (0xff0f131a));
    g.fillRect (footerRect.reduced (1.0f));

    // Progress bar showing elapsed time
    const float progressFraction = (float) (5 - countdownSeconds) / 5.0f;
    const auto progBarRect = juce::Rectangle<float> (footerRect.getX() + 24.0f, footerY + 16.0f,
                                                     footerRect.getWidth() - 48.0f, 6.0f);
    g.setColour (juce::Colour (0xff1e2638));
    g.fillRoundedRectangle (progBarRect, 3.0f);

    auto fillRect = progBarRect;
    fillRect.setWidth (progBarRect.getWidth() * progressFraction);
    g.setColour (juce::Colour (0xffee4d2d));
    g.fillRoundedRectangle (fillRect, 3.0f);

    // Auto-redirect countdown notice
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));

    if (countdownSeconds > 0)
    {
        const juce::String redirectMsg = juce::String (juce::CharPointer_UTF8 ("\xe2\x8f\xb3 \xe0\xb8\x81\xe0\xb8\xb3\xe0\xb8\xa5\xe0\xb8\xb1\xe0\xb8\x87\xe0\xb9\x80\xe0\xb8\x9b\xe0\xb8\xb4\xe0\xb8\x94 Shopee \xe0\xb8\xaa\xe0\xb8\x99\xe0\xb8\xb1\xe0\xb8\x9a\xe0\xb8\xaa\xe0\xb8\x99\xe0\xb8\xb8\xe0\xb8\x99\xe0\xb8\x9c\xe0\xb8\xb9\xe0\xb9\x89\xe0\xb8\x9e\xe0\xb8\xb1\xe0\xb8\x92\xe0\xb8\x99\xe0\xb8\xb2\xe0\xb9\x83\xe0\xb8\x99 "))
                                       + juce::String (countdownSeconds)
                                       + juce::String (juce::CharPointer_UTF8 (" \xe0\xb8\xa7\xe0\xb8\xb4\xe0\xb8\x99\xe0\xb8\xb2\xe0\xb8\x97\xe0\xb8\xb5..."));
        g.drawText (redirectMsg,
                    (int) footerRect.getX() + 24, (int) footerY + 30,
                    (int) footerRect.getWidth() - 48, 26,
                    juce::Justification::centred);
    }
    else
    {
        g.setColour (juce::Colour (0xff10b981));
        g.drawText (juce::CharPointer_UTF8 ("\xf0\x9f\x9a\x80 \xe0\xb8\x81\xe0\xb8\xb3\xe0\xb8\xa5\xe0\xb8\xb1\xe0\xb8\x87\xe0\xb8\x9e\xe0\xb8\xb2\xe0\xb9\x84\xe0\xb8\x9b\xe0\xb8\x97\xe0\xb8\xb5\xe0\xb9\x88 Shopee..."),
                    (int) footerRect.getX() + 24, (int) footerY + 30,
                    (int) footerRect.getWidth() - 48, 26,
                    juce::Justification::centred);
    }

    g.setColour (juce::Colour (0xff94a3b8));
    g.setFont (juce::FontOptions (12.0f, juce::Font::plain));
    g.drawText (juce::CharPointer_UTF8 ("(\xe0\xb9\x80\xe0\xb8\x9b\xe0\xb8\xb4\xe0\xb8\x94\xe0\xb8\xad\xe0\xb8\xb1\xe0\xb8\x95\xe0\xb9\x82\xe0\xb8\x99\xe0\xb8\xa1\xe0\xb8\xb1\xe0\xb8\x95\xe0\xb8\xb4 \xe0\xb9\x84\xe0\xb8\xa1\xe0\xb9\x88\xe0\xb8\x95\xe0\xb9\x89\xe0\xb8\xad\xe0\xb8\x87\xe0\xb8\x81\xe0\xb8\x94\xe0\xb8\x9b\xe0\xb8\xb8\xe0\xb9\x88\xe0\xb8\xa1\xe0\xb9\x83\xe0\xb8\x94\xe0\xb9\x86)"),
                (int) footerRect.getX() + 24, (int) footerY + 56,
                (int) footerRect.getWidth() - 48, 20,
                juce::Justification::centred);
}

void SponsorOverlay::resized()
{
}

} // namespace mps
