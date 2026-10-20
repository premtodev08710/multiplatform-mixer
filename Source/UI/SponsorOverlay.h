// Mix Practice Studio - Shopee Affiliate Sponsor Overlay
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace mps
{

/**
 * Cross-platform full-display sponsor banner displayed once upon app startup.
 * Enforces visiting the Shopee sponsor link before unlocking the app interface.
 * Strictly no skip button, auto-timeout bypass, or outside-click dismiss.
 */
class SponsorOverlay : public juce::Component
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
    void openShopeeAndDismiss();
    void dismissDirectly();

    std::function<void()> onDismissCallback;
    bool hasRedirected = false;

    juce::TextButton shopeeButton;

    static inline bool hasShownThisSession = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SponsorOverlay)
};

} // namespace mps
