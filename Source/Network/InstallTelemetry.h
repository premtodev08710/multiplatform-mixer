// Mix Practice Studio - Firebase Install & Active User Telemetry
#pragma once

#include <juce_core/juce_core.h>

namespace mps
{

/**
 * Reports anonymous installation & active usage metrics to Firebase.
 *
 * Supports both:
 * 1. Firebase Cloud Firestore (just set kFirebaseProjectId)
 * 2. Firebase Realtime Database (set kFirebaseDatabaseUrl)
 *
 * Each device generates a persistent anonymous UUID stored in:
 * <userApplicationDataDirectory>/MixPracticeStudio/telemetry_state.json
 */
class InstallTelemetry : private juce::Thread
{
public:
    //==========================================================================
    // FIREBASE CONFIGURATION
    // Set your Firebase Project ID (for Cloud Firestore) OR Realtime DB URL below:
    //==========================================================================
    static constexpr const char* kFirebaseProjectId   = "pad-mix";
    static constexpr const char* kFirebaseDatabaseUrl = "";
    static constexpr const char* kFirebaseApiKey      = "AIzaSyDrnDzAK1WLPgdKRUq__zYpHpag6vOFUQ8";

    InstallTelemetry();
    ~InstallTelemetry() override;

    /** Starts an asynchronous background check-in to report install / active status. */
    void reportAppLaunchAsync();

    /** Returns this device's persistent anonymous install ID. */
    juce::String getInstallId() const { return installId; }

    /** Returns how many times the app has been launched on this device. */
    int getLaunchCount() const { return launchCount; }

private:
    void run() override;

    void loadOrInitLocalState();
    void saveLocalState() const;
    static juce::File getStateFile();
    static juce::String detectPlatformCategory();

    bool sendToFirestore (const juce::String& projectId, const juce::String& apiKey);
    bool sendToRealtimeDb (const juce::String& dbUrl, const juce::String& apiKey);

    juce::String installId;
    juce::String firstInstallTimeIso;
    juce::String lastSyncTimeIso;
    int launchCount = 0;
    bool syncedFirstInstall = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InstallTelemetry)
};

} // namespace mps
