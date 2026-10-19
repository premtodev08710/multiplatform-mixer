// Mix Practice Studio - Shopee Affiliate Sponsor Overlay
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace mps
{

/**
 * Cross-platform sponsor dialog displayed once upon app startup.
 * Counts down 5 seconds, then enables the Shopee affiliate sponsor button
 * which opens the sponsor link and enters the app.
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

private:
    void timerCallback() override;
    void openShopeeAndDismiss();
    void dismissDirectly();

    std::function<void()> onDismissCallback;
    int countdownSeconds = 5;

    juce::TextButton shopeeButton;
    juce::TextButton skipButton;

    static inline bool hasShownThisSession = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SponsorOverlay)
};

} // namespace mps
