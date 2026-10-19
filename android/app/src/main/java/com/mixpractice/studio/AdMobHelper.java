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

public class AdMobHelper {
    private static final String TAG = "MixPracticeAdMob";

    // Official AdMob Interstitial Ad Unit ID for Mix Practice Studio
    public static final String DEFAULT_INTERSTITIAL_AD_UNIT_ID = "ca-app-pub-7548699861335461/5536980202";

    private static final Handler mainHandler = new Handler(Looper.getMainLooper());
    private static boolean isInitialized = false;
    private static boolean isLoading = false;
    private static boolean pendingShowOnFirstLoad = false;
    private static InterstitialAd currentInterstitialAd = null;
    private static String activeAdUnitId = DEFAULT_INTERSTITIAL_AD_UNIT_ID;
    private static long lastAdShownTimestampMs = 0L;
    private static long minCooldownMs = 180000L; // Default 3 minutes between interstitials

    /**
     * Initializes Google Mobile Ads SDK and preloads the first interstitial ad.
     * If showOnFirstLoad is true, the interstitial ad will automatically display
     * once the initial load completes.
     */
    public static void initialize(final Activity activity, final String adUnitId, final boolean showOnFirstLoad, final long cooldownMs) {
        if (activity == null) {
            return;
        }
        mainHandler.post(new Runnable() {
            @Override
            public void run() {
                if (adUnitId != null && !adUnitId.trim().isEmpty()) {
                    activeAdUnitId = adUnitId.trim();
                }
                if (cooldownMs > 0) {
                    minCooldownMs = cooldownMs;
                }
                pendingShowOnFirstLoad = showOnFirstLoad;

                if (isInitialized) {
                    if (currentInterstitialAd == null && !isLoading) {
                        loadInterstitialInternal(activity);
                    } else if (pendingShowOnFirstLoad && currentInterstitialAd != null) {
                        pendingShowOnFirstLoad = false;
                        showLoadedAdInternal(activity);
                    }
                    return;
                }

                try {
                    final Context appContext = activity.getApplicationContext();
                    MobileAds.initialize(appContext, new OnInitializationCompleteListener() {
                        @Override
                        public void onInitializationComplete(InitializationStatus initializationStatus) {
                            Log.d(TAG, "AdMob SDK initialized.");
                            isInitialized = true;
                            loadInterstitialInternal(activity);
                        }
                    });
                } catch (Throwable t) {
                    Log.e(TAG, "Failed to initialize AdMob: " + t.getMessage(), t);
                }
            }
        });
    }

    /**
     * Attempts to show an interstitial ad if one is loaded and the cooldown interval has elapsed.
     */
    public static void maybeShowInterstitial(final Activity activity, final long customCooldownMs) {
        if (activity == null) {
            return;
        }
        mainHandler.post(new Runnable() {
            @Override
            public void run() {
                final long effectiveCooldown = (customCooldownMs > 0) ? customCooldownMs : minCooldownMs;
                final long now = System.currentTimeMillis();

                if (lastAdShownTimestampMs > 0 && (now - lastAdShownTimestampMs) < effectiveCooldown) {
                    // Still within cooldown period; ensure next ad is preloaded
                    if (currentInterstitialAd == null && !isLoading && isInitialized) {
                        loadInterstitialInternal(activity);
                    }
                    return;
                }

                if (currentInterstitialAd != null) {
                    showLoadedAdInternal(activity);
                } else if (isInitialized && !isLoading) {
                    loadInterstitialInternal(activity);
                }
            }
        });
    }

    private static void loadInterstitialInternal(final Activity activity) {
        if (isLoading || currentInterstitialAd != null || activity == null) {
            return;
        }
        isLoading = true;

        try {
            AdRequest adRequest = new AdRequest.Builder().build();
            InterstitialAd.load(
                activity.getApplicationContext(),
                activeAdUnitId,
                adRequest,
                new InterstitialAdLoadCallback() {
                    @Override
                    public void onAdLoaded(InterstitialAd interstitialAd) {
                        isLoading = false;
                        currentInterstitialAd = interstitialAd;
                        Log.d(TAG, "Interstitial ad loaded.");

                        currentInterstitialAd.setFullScreenContentCallback(new FullScreenContentCallback() {
                            @Override
                            public void onAdDismissedFullScreenContent() {
                                Log.d(TAG, "Interstitial ad dismissed.");
                                currentInterstitialAd = null;
                                lastAdShownTimestampMs = System.currentTimeMillis();
                                // Preload the next interstitial ad in background
                                loadInterstitialInternal(activity);
                            }

                            @Override
                            public void onAdFailedToShowFullScreenContent(AdError adError) {
                                Log.w(TAG, "Interstitial ad failed to show: " + adError.getMessage());
                                currentInterstitialAd = null;
                            }

                            @Override
                            public void onAdShowedFullScreenContent() {
                                Log.d(TAG, "Interstitial ad showed fullscreen content.");
                                lastAdShownTimestampMs = System.currentTimeMillis();
                            }
                        });

                        if (pendingShowOnFirstLoad) {
                            pendingShowOnFirstLoad = false;
                            showLoadedAdInternal(activity);
                        }
                    }

                    @Override
                    public void onAdFailedToLoad(LoadAdError loadAdError) {
                        isLoading = false;
                        currentInterstitialAd = null;
                        pendingShowOnFirstLoad = false;
                        Log.w(TAG, "Interstitial ad failed to load: " + loadAdError.getMessage());
                    }
                }
            );
        } catch (Throwable t) {
            isLoading = false;
            Log.e(TAG, "Error loading interstitial ad: " + t.getMessage(), t);
        }
    }

    private static void showLoadedAdInternal(final Activity activity) {
        if (currentInterstitialAd == null || activity == null || activity.isFinishing() || activity.isDestroyed()) {
            return;
        }
        try {
            InterstitialAd adToShow = currentInterstitialAd;
            currentInterstitialAd = null;
            lastAdShownTimestampMs = System.currentTimeMillis();
            adToShow.show(activity);
        } catch (Throwable t) {
            Log.e(TAG, "Error showing interstitial ad: " + t.getMessage(), t);
        }
    }
}
