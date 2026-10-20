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

    // Button 1: Shopee Sponsor Visit
    shopeeButton.setButtonText (juce::CharPointer_UTF8 ("\xf0\x9f\x9b\x92 \xe0\xb9\x81\xe0\xb8\xa7\xe0\xb8\xb0\xe0\xb8\x8a\xe0\xb8\xa1 Shopee (\xe0\xb8\xaa\xe0\xb8\x99\xe0\xb8\xb1\xe0\xb8\x9a\xe0\xb8\xaa\xe0\xb8\x99\xe0\xb8\xb8\xe0\xb8\x99\xe0\xb8\x9c\xe0\xb8\xb9\xe0\xb9\x89\xe0\xb8\x9e\xe0\xb8\xb1\xe0\xb8\x92\xe0\xb8\x99\xe0\xb8\xb2)"));
    shopeeButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xffee4d2d));
    shopeeButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    shopeeButton.onClick = [this] { openShopeeAndDismiss(); };
    addAndMakeVisible (shopeeButton);

    // Button 2: Enter App Directly (Skip)
    enterAppButton.setButtonText (juce::CharPointer_UTF8 ("\xe0\xb9\x80\xe0\xb8\x82\xe0\xb9\x89\xe0\xb8\xb2\xe0\xb9\x83\xe0\xb8\x8a\xe0\xb9\x89\xe0\xb8\x87\xe0\xb8\xb2\xe0\xb8\x99\xe0\xb8\x97\xe0\xb8\xb1\xe0\xb8\x99\xe0\xb8\x97\xe0\xb8\xb5 (\xe0\xb8\x82\xe0\xb9\x89\xe0\xb8\xb2\xe0\xb8\xa1)"));
    enterAppButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1e2638));
    enterAppButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffcbd5e1));
    enterAppButton.onClick = [this] { dismissDirectly(); };
    addAndMakeVisible (enterAppButton);

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

        // 5 seconds elapsed: seamlessly dismiss banner and enter app directly.
        // NEVER kick the user out of the app automatically!
        dismissDirectly();
    }
}

void SponsorOverlay::mouseDown (const juce::MouseEvent& e)
{
    const auto bounds = getLocalBounds();
    const int dialogW = juce::jmin (560, bounds.getWidth() - 24);
    const int dialogH = juce::jmin (390, bounds.getHeight() - 20);
    const auto dialogRect = juce::Rectangle<int> ((bounds.getWidth() - dialogW) / 2,
                                                  (bounds.getHeight() - dialogH) / 2,
                                                  dialogW, dialogH);
    const float bannerH = (float) dialogH * 0.60f;
    const auto bannerRect = juce::Rectangle<int> (dialogRect.getX(), dialogRect.getY(),
                                                  dialogRect.getWidth(), (int) bannerH);

    // If tapping inside the banner card, visit Shopee
    if (bannerRect.contains (e.getPosition()))
    {
        openShopeeAndDismiss();
    }
    else if (! dialogRect.contains (e.getPosition()))
    {
        // Tapping outside the dialog dismisses into the app
        dismissDirectly();
    }
}

static void reportShopeeClickToFirestore (const juce::String& chosenUrl)
{
#if JUCE_ANDROID
    if (auto* env = juce::getEnv())
    {
        jclass telemetryCls = env->FindClass ("com/mixpractice/studio/TelemetryHelper");
        if (telemetryCls != nullptr)
        {
            jmethodID mid = env->GetStaticMethodID (
                telemetryCls,
                "reportShopeeClickAsync",
                "(Landroid/content/Context;Ljava/lang/String;)V");
            if (mid != nullptr)
            {
                auto jContext = juce::getAppContext();
                jstring jUrl = env->NewStringUTF (chosenUrl.toUTF8());
                env->CallStaticVoidMethod (telemetryCls, mid, jContext.get(), jUrl);
                env->DeleteLocalRef (jUrl);
            }
            env->DeleteLocalRef (telemetryCls);
        }
        if (env->ExceptionCheck())
            env->ExceptionClear();
    }
#endif

    juce::Thread::launch ([chosenUrl] {
        const auto nowIso = juce::Time::getCurrentTime().toISO8601 (true);
        const auto appVersion = juce::String (JUCE_APPLICATION_VERSION_STRING);
        const auto osName = juce::SystemStats::getOperatingSystemName();
        const auto deviceModel = juce::SystemStats::getDeviceDescription();

#if JUCE_WINDOWS
        const juce::String platform = "Windows";
#elif JUCE_MAC
        const juce::String platform = "macOS";
#elif JUCE_ANDROID
        const juce::String platform = "Android";
#elif JUCE_IOS
        const juce::String platform = "iOS";
#elif JUCE_LINUX
        const juce::String platform = "Linux";
#else
        const juce::String platform = "Other";
#endif

        auto makeStringField = [] (const juce::String& val) {
            auto* o = new juce::DynamicObject();
            o->setProperty ("stringValue", val);
            return juce::var (o);
        };

        auto* fields = new juce::DynamicObject();
        fields->setProperty ("timestamp",   makeStringField (nowIso));
        fields->setProperty ("platform",    makeStringField (platform));
        fields->setProperty ("osName",      makeStringField (osName));
        fields->setProperty ("appVersion",  makeStringField (appVersion));
        fields->setProperty ("deviceModel", makeStringField (deviceModel));
        fields->setProperty ("targetUrl",   makeStringField (chosenUrl));

        auto* root = new juce::DynamicObject();
        root->setProperty ("fields", juce::var (fields));
        const juce::String payload = juce::JSON::toString (juce::var (root), true);

        juce::String endpoint = "https://firestore.googleapis.com/v1/projects/pad-mix/databases/(default)/documents/shopee_clicks?key=AIzaSyDrnDzAK1WLPgdKRUq__zYpHpag6vOFUQ8";
        juce::URL url (endpoint);
        url = url.withPOSTData (payload);

        int statusCode = 0;
        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                           .withHttpRequestCmd ("POST")
                           .withExtraHeaders ("Content-Type: application/json\r\n")
                           .withConnectionTimeoutMs (8000)
                           .withStatusCode (&statusCode);

        if (auto stream = url.createInputStream (options))
            stream->readEntireStreamAsString();
    });
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

        reportShopeeClickToFirestore (chosenUrl);

       #if JUCE_ANDROID
        bool launched = false;
        if (auto* env = juce::getEnv())
        {
            jclass telemetryCls = env->FindClass ("com/mixpractice/studio/TelemetryHelper");
            if (telemetryCls != nullptr)
            {
                jmethodID mid = env->GetStaticMethodID (
                    telemetryCls,
                    "openBrowserSafely",
                    "(Landroid/content/Context;Ljava/lang/String;)Z");
                if (mid != nullptr)
                {
                    auto jContext = juce::getAppContext();
                    jstring jUrl = env->NewStringUTF (chosenUrl.toUTF8());
                    jboolean res = env->CallStaticBooleanMethod (telemetryCls, mid, jContext.get(), jUrl);
                    env->DeleteLocalRef (jUrl);
                    launched = (res == JNI_TRUE);
                }
                env->DeleteLocalRef (telemetryCls);
            }
            if (env->ExceptionCheck())
                env->ExceptionClear();
        }

        if (! launched)
        {
            try {
                juce::URL (chosenUrl).launchInDefaultBrowser();
            } catch (...) {}
        }
       #else
        juce::URL (chosenUrl).launchInDefaultBrowser();
       #endif
    }

    dismissDirectly();
}

void SponsorOverlay::dismissDirectly()
{
    stopTimer();
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
    const int dialogH = juce::jmin (420, bounds.getHeight() - 20);
    const auto dialogRect = juce::Rectangle<int> ((bounds.getWidth() - dialogW) / 2,
                                                  (bounds.getHeight() - dialogH) / 2,
                                                  dialogW, dialogH).toFloat();

    // Dialog outer container
    g.setColour (juce::Colour (0xff121620));
    g.fillRoundedRectangle (dialogRect, 16.0f);
    g.setColour (juce::Colour (0xffee4d2d).withAlpha (0.50f));
    g.drawRoundedRectangle (dialogRect, 16.0f, 2.0f);

    //==========================================================================
    // PROMOTIONAL BANNER SECTION (Top 58%)
    //==========================================================================
    const float bannerH = dialogRect.getHeight() * 0.58f;
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
    const float eqY = bannerRect.getY() + 35.0f;
    static const float kBarHeights[] = { 22.0f, 44.0f, 65.0f, 38.0f, 52.0f, 30.0f, 48.0f, 25.0f };
    for (int i = 0; i < 8; ++i)
    {
        const float bx = eqX + (float) i * 11.0f;
        const float bh = kBarHeights[i];
        g.setColour (juce::Colour (0xfffef08a).withAlpha (0.85f));
        g.fillRoundedRectangle (bx, eqY + (65.0f - bh), 7.0f, bh, 3.5f);
    }

    // Shopee Mall / Official Sponsor Pill
    auto pillRect = juce::Rectangle<float> (bannerRect.getX() + 20.0f, bannerRect.getY() + 16.0f, 148.0f, 24.0f);
    g.setColour (juce::Colours::white);
    g.fillRoundedRectangle (pillRect, 12.0f);
    g.setColour (juce::Colour (0xffee4d2d));
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText (juce::CharPointer_UTF8 ("\xf0\x9f\x9b\x92 SHOPEE SPONSOR"), pillRect, juce::Justification::centred);

    // Big Bold Thai Promo Title
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    g.drawText (juce::CharPointer_UTF8 ("\xe0\xb8\xa1\xe0\xb8\xab\xe0\xb8\x81\xe0\xb8\xa3\xe0\xb8\xa3\xe0\xb8\xa1\xe0\xb9\x80\xe0\xb8\x84\xe0\xb8\xa3\xe0\xb8\xb7\xe0\xb9\x88\xe0\xb8\xad\xe0\xb8\x87\xe0\xb9\x80\xe0\xb8\xaa\xe0\xb8\xb5\xe0\xb8\xa2\xe0\xb8\x87 & \xe0\xb8\xaa\xe0\xb8\x95\xe0\xb8\xb9\xe0\xb8\x94\xe0\xb8\xb4\xe0\xb9\x82\xe0\xb8\xad"),
                (int) bannerRect.getX() + 20, (int) bannerRect.getY() + 46,
                (int) bannerRect.getWidth() - 130, 28,
                juce::Justification::centredLeft);

    // Subtitle / Equipment categories
    g.setColour (juce::Colour (0xfffff1ee));
    g.setFont (juce::FontOptions (13.0f, juce::Font::plain));
    g.drawText (juce::CharPointer_UTF8 ("\xe0\xb8\xab\xe0\xb8\xb9\xe0\xb8\x9f\xe0\xb8\xb1\xe0\xb8\x87\xe0\xb8\xa1\xe0\xb8\xad\xe0\xb8\x99\xe0\xb8\xb4\xe0\xb9\x80\xe0\xb8\x95\xe0\xb8\xad\xe0\xb8\xa3\xe0\xb9\x8c \xe2\x80\xa2 Audio Interface \xe2\x80\xa2 \xe0\xb9\x84\xe0\xb8\xa1\xe0\xb9\x82\xe0\xb8\x84\xe0\xb8\xa3\xe0\xb9\x82\xe0\xb8\x9f\xe0\xb8\x99 \xe2\x80\xa2 \xe0\xb8\xa5\xe0\xb8\xb3\xe0\xb9\x82\xe0\xb8\x9e\xe0\xb8\x87"),
                (int) bannerRect.getX() + 20, (int) bannerRect.getY() + 76,
                (int) bannerRect.getWidth() - 130, 20,
                juce::Justification::centredLeft);

    // Promo Badges (Discount + Free Shipping)
    const float badgeY = bannerRect.getY() + 104.0f;
    auto discBadge = juce::Rectangle<float> (bannerRect.getX() + 20.0f, badgeY, 115.0f, 24.0f);
    g.setColour (juce::Colour (0xfffef08a)); // Light yellow
    g.fillRoundedRectangle (discBadge, 5.0f);
    g.setColour (juce::Colour (0xff78350f)); // Deep brown
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (juce::CharPointer_UTF8 ("\xe2\x9a\xa1 \xe0\xb8\xa5\xe0\xb8\x94\xe0\xb8\xaa\xe0\xb8\xb9\xe0\xb8\x87\xe0\xb8\xaa\xe0\xb8\xb8\xe0\xb8\x94 70%"), discBadge, juce::Justification::centred);

    auto shipBadge = juce::Rectangle<float> (bannerRect.getX() + 142.0f, badgeY, 130.0f, 24.0f);
    g.setColour (juce::Colour (0xffdbeafe)); // Light blue
    g.fillRoundedRectangle (shipBadge, 5.0f);
    g.setColour (juce::Colour (0xff1e3a8a)); // Deep blue
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (juce::CharPointer_UTF8 ("\xf0\x9f\x9a\x9a \xe0\xb9\x82\xe0\xb8\x84\xe0\xb9\x89\xe0\xb8\x94\xe0\xb8\xaa\xe0\xb9\x88\xe0\xb8\x87\xe0\xb8\x9f\xe0\xb8\xa3\xe0\xb8\xb5\xe0\xb8\x9e\xe0\xb8\xb4\xe0\xb9\x80\xe0\xb8\xa8\xe0\xb8\xa9"), shipBadge, juce::Justification::centred);

    // Tap notice
    g.setColour (juce::Colours::white.withAlpha (0.90f));
    g.setFont (juce::FontOptions (11.0f, juce::Font::italic));
    g.drawText (juce::CharPointer_UTF8 ("(\xe0\xb9\x81\xe0\xb8\x95\xe0\xb8\xb0\xe0\xb8\x9b\xe0\xb9\x89\xe0\xb8\xb2\xe0\xb8\xa2\xe0\xb9\x80\xe0\xb8\x9e\xe0\xb8\xb7\xe0\xb9\x88\xe0\xb8\xad\xe0\xb9\x80\xe0\xb8\x9b\xe0\xb8\xb4\xe0\xb8\x94 Shopee \xe0\xb8\xaa\xe0\xb8\x99\xe0\xb8\xb1\xe0\xb8\x9a\xe0\xb8\xaa\xe0\xb8\x99\xe0\xb8\xb8\xe0\xb8\x99\xe0\xb8\x9c\xe0\xb8\xb9\xe0\xb9\x89\xe0\xb8\x9e\xe0\xb8\xb1\xe0\xb8\x92\xe0\xb8\x99\xe0\xb8\xb2)"),
                (int) bannerRect.getX() + 20, (int) bannerRect.getY() + 136,
                (int) bannerRect.getWidth() - 40, 18,
                juce::Justification::centredLeft);

    g.restoreState();

    //==========================================================================
    // COUNTDOWN & ACTION FOOTER (Bottom 42%)
    //==========================================================================
    const float footerY = bannerRect.getBottom();
    const float footerH = dialogRect.getBottom() - footerY;
    const auto footerRect = juce::Rectangle<float> (dialogRect.getX(), footerY,
                                                    dialogRect.getWidth(), footerH);

    g.setColour (juce::Colour (0xff0f131a));
    g.fillRect (footerRect.reduced (1.0f));

    // Progress bar showing elapsed time
    const float progressFraction = (float) (5 - countdownSeconds) / 5.0f;
    const auto progBarRect = juce::Rectangle<float> (footerRect.getX() + 20.0f, footerY + 14.0f,
                                                     footerRect.getWidth() - 40.0f, 4.0f);
    g.setColour (juce::Colour (0xff1e2638));
    g.fillRoundedRectangle (progBarRect, 2.0f);

    auto fillRect = progBarRect;
    fillRect.setWidth (progBarRect.getWidth() * progressFraction);
    g.setColour (juce::Colour (0xffee4d2d));
    g.fillRoundedRectangle (fillRect, 2.0f);

    // Auto-enter countdown notice (clear explanation that it enters app, not kicks out)
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));

    if (countdownSeconds > 0)
    {
        const juce::String statusMsg = juce::String (juce::CharPointer_UTF8 ("\xe2\x8f\xb3 \xe0\xb8\x81\xe0\xb8\xb3\xe0\xb8\xa5\xe0\xb8\xb1\xe0\xb8\x87\xe0\xb9\x80\xe0\xb8\x82\xe0\xb9\x89\xe0\xb8\xb2\xe0\xb8\xaa\xe0\xb8\xb9\xe0\xb9\x88\xe0\xb9\x81\xe0\xb8\xad\xe0\xb8\x9b\xe0\xb9\x83\xe0\xb8\x99 "))
                                     + juce::String (countdownSeconds)
                                     + juce::String (juce::CharPointer_UTF8 (" \xe0\xb8\xa7\xe0\xb8\xb4\xe0\xb8\x99\xe0\xb8\xb2\xe0\xb8\x97\xe0\xb8\xb5... (\xe0\xb8\xab\xe0\xb8\xa3\xe0\xb8\xb7\xe0\xb8\xad\xe0\xb9\x80\xe0\xb8\xa5\xe0\xb8\xb7\xe0\xb8\xad\xe0\xb8\x81\xe0\xb8\x9b\xe0\xb8\xb8\xe0\xb9\x88\xe0\xb8\xa1\xe0\xb8\x94\xe0\xb9\x89\xe0\xb8\xb2\xe0\xb8\x99\xe0\xb8\xa5\xe0\xb9\x88\xe0\xb8\xb2\xe0\xb8\x87)"));
        g.drawText (statusMsg,
                    (int) footerRect.getX() + 20, (int) footerY + 24,
                    (int) footerRect.getWidth() - 40, 22,
                    juce::Justification::centred);
    }
    else
    {
        g.setColour (juce::Colour (0xff10b981));
        g.drawText (juce::CharPointer_UTF8 ("\xe2\x9c\x85 \xe0\xb8\x9e\xe0\xb8\xa3\xe0\xb9\x89\xe0\xb8\xad\xe0\xb8\xa1\xe0\xb9\x80\xe0\xb8\x82\xe0\xb9\x89\xe0\xb8\xb2\xe0\xb9\x83\xe0\xb8\x8a\xe0\xb9\x89\xe0\xb8\x87\xe0\xb8\xb2\xe0\xb8\x99\xe0\xb9\x81\xe0\xb8\xa5\xe0\xb9\x89\xe0\xb8\xa7!"),
                    (int) footerRect.getX() + 20, (int) footerY + 24,
                    (int) footerRect.getWidth() - 40, 22,
                    juce::Justification::centred);
    }
}

void SponsorOverlay::resized()
{
    const auto bounds = getLocalBounds();
    const int dialogW = juce::jmin (560, bounds.getWidth() - 24);
    const int dialogH = juce::jmin (420, bounds.getHeight() - 20);
    const auto dialogRect = juce::Rectangle<int> ((bounds.getWidth() - dialogW) / 2,
                                                  (bounds.getHeight() - dialogH) / 2,
                                                  dialogW, dialogH);

    const int footerY = dialogRect.getY() + (int) ((float) dialogH * 0.58f);
    const int footerH = dialogRect.getBottom() - footerY;

    const int btnH = 40;
    const int btnSpacing = 12;
    const int totalBtnW = dialogRect.getWidth() - 40;
    const int btnW = (totalBtnW - btnSpacing) / 2;

    const int btnY = footerY + footerH - btnH - 16;

    shopeeButton.setBounds (dialogRect.getX() + 20, btnY, btnW, btnH);
    enterAppButton.setBounds (dialogRect.getX() + 20 + btnW + btnSpacing, btnY, btnW, btnH);
}

} // namespace mps
