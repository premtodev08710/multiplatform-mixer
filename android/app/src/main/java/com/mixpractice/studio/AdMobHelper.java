package com.mixpractice.studio;

import android.app.Activity;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import com.google.android.gms.ads.AdError;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.FullScreenContentCallback;
import com.google.android.gms.ads.LoadAdError;
import com.google.android.gms.ads.MobileAds;
import com.google.android.gms.ads.initialization.InitializationStatus;
import com.google.android.gms.ads.initialization.OnInitializationCompleteListener;
import com.google.android.gms.ads.interstitial.InterstitialAd;
import com.google.android.gms.ads.interstitial.InterstitialAdLoadCallback;

/**
 * Manages Google AdMob Interstitial Ads for Mix Practice Studio.
 *
 * User requirement:
 * - Display interstitial ad ONLY ONCE upon app launch (first start).
 * - Allow user to skip after 5 seconds (standard Google interstitial countdown).
 * - No frequent or recurring ads during mixing or screen transitions.
 * - Robust fallback to test ad unit if production ad unit is pending inventory/store link.
 */
public class AdMobHelper {
    private static final String TAG = "MixPracticeAdMob";

    // Official AdMob Interstitial Ad Unit ID for Mix Practice Studio
    public static final String PRODUCTION_INTERSTITIAL_AD_UNIT_ID = "ca-app-pub-7548699861335461/5536980202";

    // Google's official Interstitial sample ad unit (guaranteed fill for testing / unlinked store)
    public static final String TEST_INTERSTITIAL_AD_UNIT_ID = "ca-app-pub-3940256099942544/1033173712";

    private static final Handler mainHandler = new Handler(Looper.getMainLooper());
    private static boolean isSdkInitialized = false;
    private static boolean isLoadingAd = false;
    private static boolean adShownThisSession = false;

    /**
     * Called directly on app startup (e.g. from MainActivity.onCreate) to initialize
     * AdMob and immediately show the launch interstitial ad (with 5s skip).
     */
    public static void initializeAndShowLaunchAd(final Activity activity) {
        if (activity == null || adShownThisSession) {
            return;
        }

        mainHandler.post(new Runnable() {
            @Override
            public void run() {
                if (adShownThisSession) {
                    return;
                }

                if (!isSdkInitialized) {
                    try {
                        final Context appContext = activity.getApplicationContext();
                        MobileAds.initialize(appContext, new OnInitializationCompleteListener() {
                            @Override
                            public void onInitializationComplete(InitializationStatus initializationStatus) {
                                Log.d(TAG, "AdMob SDK successfully initialized.");
                                isSdkInitialized = true;
                                loadAndShowLaunchAd(activity, true);
                            }
                        });
                    } catch (Throwable t) {
                        Log.e(TAG, "Error initializing AdMob SDK: " + t.getMessage(), t);
                    }
                } else {
                    loadAndShowLaunchAd(activity, true);
                }
            }
        });
    }

    /**
     * Loads and immediately displays the launch interstitial ad.
     * Tries production ad unit first; falls back to test ad unit if production has no fill.
     */
    private static void loadAndShowLaunchAd(final Activity activity, final boolean tryProductionFirst) {
        if (adShownThisSession || isLoadingAd || activity == null || activity.isFinishing()) {
            return;
        }

        isLoadingAd = true;
        final String adUnitId = tryProductionFirst ? PRODUCTION_INTERSTITIAL_AD_UNIT_ID : TEST_INTERSTITIAL_AD_UNIT_ID;
        Log.d(TAG, "Loading launch interstitial with ad unit: " + (tryProductionFirst ? "PRODUCTION" : "TEST_FALLBACK"));

        try {
            AdRequest adRequest = new AdRequest.Builder().build();
            InterstitialAd.load(
                activity.getApplicationContext(),
                adUnitId,
                adRequest,
                new InterstitialAdLoadCallback() {
                    @Override
                    public void onAdLoaded(final InterstitialAd interstitialAd) {
                        isLoadingAd = false;
                        Log.d(TAG, "Launch interstitial loaded successfully.");

                        if (adShownThisSession || activity == null || activity.isFinishing() || activity.isDestroyed()) {
                            return;
                        }

                        interstitialAd.setFullScreenContentCallback(new FullScreenContentCallback() {
                            @Override
                            public void onAdShowedFullScreenContent() {
                                Log.d(TAG, "Launch interstitial ad showed fullscreen (5s skip enabled).");
                                adShownThisSession = true;
                            }

                            @Override
                            public void onAdDismissedFullScreenContent() {
                                Log.d(TAG, "Launch interstitial ad dismissed. Entering app.");
                                adShownThisSession = true;
                            }

                            @Override
                            public void onAdFailedToShowFullScreenContent(AdError adError) {
                                Log.w(TAG, "Launch interstitial failed to show: " + adError.getMessage());
                            }
                        });

                        interstitialAd.show(activity);
                    }

                    @Override
                    public void onAdFailedToLoad(final LoadAdError loadAdError) {
                        isLoadingAd = false;
                        Log.w(TAG, "Launch interstitial failed to load (" + adUnitId + "): " + loadAdError.getMessage()
                                + " (Code: " + loadAdError.getCode() + ")");

                        if (tryProductionFirst && !adShownThisSession && activity != null && !activity.isFinishing()) {
                            Log.i(TAG, "Retrying with official Google test interstitial fallback...");
                            loadAndShowLaunchAd(activity, false);
                        }
                    }
                }
            );
        } catch (Throwable t) {
            isLoadingAd = false;
            Log.e(TAG, "Exception during interstitial ad load: " + t.getMessage(), t);
        }
    }

    /**
     * Legacy initialization method for C++ JNI bridge if called.
     */
    public static void initialize(final Activity activity, final String adUnitId, final boolean showOnFirstLoad, final long cooldownMs) {
        if (!adShownThisSession && showOnFirstLoad && activity != null) {
            initializeAndShowLaunchAd(activity);
        }
    }

    /**
     * Screen transition ad call - disabled to honor user preference (no frequent/annoying ads).
     */
    public static void maybeShowInterstitial(final Activity activity, final long customCooldownMs) {
        // Intentionally no-op: ads are strictly limited to the first launch interstitial.
    }
}
