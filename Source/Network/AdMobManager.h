// Mix Practice Studio - Android AdMob Interstitial Manager
#pragma once

#include <juce_core/juce_core.h>

namespace mps
{

/**
 * Manages Google AdMob Interstitial Ads on Android via JNI.
 *
 * - Displays an interstitial ad on app startup (once loaded).
 * - Displays an interstitial ad on screen navigation transitions when the
 *   cooldown timer (default: 3 minutes) has elapsed AND audio is not actively playing.
 */
class AdMobManager
{
public:
    //==========================================================================
    // ADMOB CONFIGURATION
    //==========================================================================
    static constexpr const char* kAdMobAppId           = "ca-app-pub-7548699861335461~9574543547";
    static constexpr const char* kInterstitialAdUnitId = "ca-app-pub-7548699861335461/5536980202";

    /** Show interstitial ad automatically when the app first launches. */
    static constexpr bool kShowOnAppLaunch = true;

    /** Minimum cooldown interval (in minutes) between interstitial ads on screen switches. */
    static constexpr int kScreenSwitchCooldownMinutes = 3;
    static constexpr juce::int64 kScreenSwitchCooldownMs =
        static_cast<juce::int64> (kScreenSwitchCooldownMinutes) * 60LL * 1000LL;

    AdMobManager() = default;
    ~AdMobManager() = default;

    /**
     * Initializes the AdMob SDK on Android and preloads the first interstitial ad.
     * Should be called from the main UI thread.
     */
    void initialize (bool showOnFirstLoad = kShowOnAppLaunch);

    /**
     * Called when the user switches screens or performs a major navigation action.
     * Will only trigger an interstitial if audio playback is stopped and the
     * cooldown interval (kScreenSwitchCooldownMinutes) has elapsed.
     */
    void notifyScreenTransition (bool isAudioPlaying);

private:
    bool initialized = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdMobManager)
};

} // namespace mps
