package com.mixpractice.studio;

import android.os.Bundle;
import android.util.Log;
import com.rmsl.juce.JuceActivity;

/**
 * Main Android Activity for Mix Practice Studio.
 * Extends JuceActivity to integrate natively with JUCE while providing direct
 * lifecycle access for Google AdMob launch interstitial ad.
 */
public class MainActivity extends JuceActivity {
    private static final String TAG = "MixPracticeActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "MainActivity onCreate - triggering launch interstitial ad");

        // Immediately trigger AdMob initialization and launch ad
        AdMobHelper.initializeAndShowLaunchAd(this);
    }
}
