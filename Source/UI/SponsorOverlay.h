// Mix Practice Studio - Shopee Affiliate Sponsor Overlay
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace mps
{

/**
 * Cross-platform full-display sponsor banner displayed once upon app startup.
 * Displays an eye-catching Shopee Audio Equipment promotional banner,
 * counts down 5 seconds, and AUTOMATICALLY opens Shopee to drop the affiliate
 * cookie and enters the app (no skip button needed).
 */
class SponsorOverlay : public juce::Component,
                       private juce::Timer
{
public:
    SponsorOverlay (std::function<void()> onDismissed = nullptr);
    ~SponsorOverlay() override;

    static bool shouldShowThisSession() noexcept;
    static void markShownThisSession() noexcept;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;

private:
    void timerCallback() override;
    void openShopeeAndDismiss();
    void dismissDirectly();

    std::function<void()> onDismissCallback;
    int countdownSeconds = 5;
    bool hasRedirected = false;

    juce::TextButton shopeeButton;
    juce::TextButton enterAppButton;

    static inline bool hasShownThisSession = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SponsorOverlay)
};

} // namespace mps
