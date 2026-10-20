// Mix Practice Studio - Firebase Install & Active User Telemetry
#include "InstallTelemetry.h"

namespace mps
{

InstallTelemetry::InstallTelemetry()
    : juce::Thread ("MPS_InstallTelemetry")
{
    loadOrInitLocalState();
}

InstallTelemetry::~InstallTelemetry()
{
    stopThread (2000);
}

juce::File InstallTelemetry::getStateFile()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("MixPracticeStudio");
    if (! dir.exists())
        dir.createDirectory();

    return dir.getChildFile ("telemetry_state.json");
}

juce::String InstallTelemetry::detectPlatformCategory()
{
   #if JUCE_WINDOWS
    return "Windows";
   #elif JUCE_MAC
    return "macOS";
   #elif JUCE_ANDROID
    return "Android";
   #elif JUCE_IOS
    return "iOS";
   #elif JUCE_LINUX
    return "Linux";
   #else
    return "Unknown";
   #endif
}

void InstallTelemetry::loadOrInitLocalState()
{
    const auto file = getStateFile();
    const auto nowIso = juce::Time::getCurrentTime().toISO8601 (true);

    if (file.existsAsFile())
    {
        auto parsed = juce::JSON::parse (file);
        if (auto* obj = parsed.getDynamicObject())
        {
            installId           = obj->getProperty ("installId").toString();
            firstInstallTimeIso = obj->getProperty ("firstInstallTime").toString();
            lastSyncTimeIso     = obj->getProperty ("lastSyncTime").toString();
            launchCount         = (int) obj->getProperty ("launchCount");
            syncedFirstInstall  = (bool) obj->getProperty ("syncedFirstInstall");
        }
    }

    if (installId.isEmpty())
    {
        // Generate a clean 32-char hex UUID
        installId = juce::Uuid().toDashedString();
    }

    if (firstInstallTimeIso.isEmpty())
        firstInstallTimeIso = nowIso;

    launchCount = std::max (1, launchCount + 1);
    saveLocalState();
}

void InstallTelemetry::saveLocalState() const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("installId", installId);
    obj->setProperty ("firstInstallTime", firstInstallTimeIso);
    obj->setProperty ("lastSyncTime", lastSyncTimeIso);
    obj->setProperty ("launchCount", launchCount);
    obj->setProperty ("syncedFirstInstall", syncedFirstInstall);

    const auto jsonText = juce::JSON::toString (juce::var (obj));
    getStateFile().replaceWithText (jsonText);
}

void InstallTelemetry::reportAppLaunchAsync()
{
    const juce::String projectId = juce::String (kFirebaseProjectId).trim();
    const juce::String dbUrl     = juce::String (kFirebaseDatabaseUrl).trim();

    if (projectId.isEmpty() && dbUrl.isEmpty())
        return; // Telemetry not configured yet

    if (! isThreadRunning())
        startThread (juce::Thread::Priority::low);
}

void InstallTelemetry::run()
{
    if (threadShouldExit())
        return;

    // Brief pause to allow network interface/DNS to become ready on mobile
    juce::Thread::sleep (1500);

    const juce::String projectId = juce::String (kFirebaseProjectId).trim();
    const juce::String dbUrl     = juce::String (kFirebaseDatabaseUrl).trim();
    const juce::String apiKey    = juce::String (kFirebaseApiKey).trim();

    bool ok = false;

    for (int attempt = 0; attempt < 3 && ! ok && ! threadShouldExit(); ++attempt)
    {
        if (attempt > 0)
            juce::Thread::sleep (3000);

        if (dbUrl.isNotEmpty())
            ok = sendToRealtimeDb (dbUrl, apiKey);
        else if (projectId.isNotEmpty())
            ok = sendToFirestore (projectId, apiKey);
    }

    if (ok && ! threadShouldExit())
    {
        lastSyncTimeIso = juce::Time::getCurrentTime().toISO8601 (true);
        syncedFirstInstall = true;
        saveLocalState();
    }
}

static juce::var makeFirestoreStringField (const juce::String& value)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("stringValue", value);
    return juce::var (obj);
}

static juce::var makeFirestoreIntField (int value)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("integerValue", juce::String (value));
    return juce::var (obj);
}

bool InstallTelemetry::sendToFirestore (const juce::String& projectId, const juce::String& apiKey)
{
    const auto nowIso = juce::Time::getCurrentTime().toISO8601 (true);
    const auto appVersion = juce::String (JUCE_APPLICATION_VERSION_STRING);
    const auto osName = juce::SystemStats::getOperatingSystemName();
    const auto platform = detectPlatformCategory();
    const auto deviceModel = juce::SystemStats::getDeviceDescription();
    const auto cpuModel = juce::SystemStats::getCpuModel();

    auto* fields = new juce::DynamicObject();
    fields->setProperty ("installId",        makeFirestoreStringField (installId));
    fields->setProperty ("platform",         makeFirestoreStringField (platform));
    fields->setProperty ("osName",           makeFirestoreStringField (osName));
    fields->setProperty ("appVersion",       makeFirestoreStringField (appVersion));
    fields->setProperty ("deviceModel",      makeFirestoreStringField (deviceModel));
    fields->setProperty ("cpuModel",         makeFirestoreStringField (cpuModel));
    fields->setProperty ("firstInstallTime", makeFirestoreStringField (firstInstallTimeIso));
    fields->setProperty ("lastSeenTime",     makeFirestoreStringField (nowIso));
    fields->setProperty ("launchCount",      makeFirestoreIntField (launchCount));

    auto* root = new juce::DynamicObject();
    root->setProperty ("fields", juce::var (fields));
    const juce::String payload = juce::JSON::toString (juce::var (root), true);

    juce::String endpoint = "https://firestore.googleapis.com/v1/projects/"
                          + juce::URL::addEscapeChars (projectId, false)
                          + "/databases/(default)/documents/installs/"
                          + juce::URL::addEscapeChars (installId, false);

    if (apiKey.isNotEmpty())
        endpoint += "?key=" + juce::URL::addEscapeChars (apiKey, true);

    juce::URL url (endpoint);
    url = url.withPOSTData (payload);

    int statusCode = 0;
    // Use POST with X-HTTP-Method-Override: PATCH for full Android HttpURLConnection compatibility
    // (Android HttpURLConnection throws ProtocolException if setRequestMethod("PATCH") is called directly).
    auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                       .withHttpRequestCmd ("POST")
                       .withExtraHeaders ("Content-Type: application/json\r\nX-HTTP-Method-Override: PATCH\r\n")
                       .withConnectionTimeoutMs (10000)
                       .withStatusCode (&statusCode);

    try
    {
        auto stream = url.createInputStream (options);
        if (stream != nullptr && statusCode >= 200 && statusCode < 300)
        {
            stream->readEntireStreamAsString();
            return true;
        }
    }
    catch (...)
    {
        return false;
    }

    return false;
}

bool InstallTelemetry::sendToRealtimeDb (const juce::String& dbUrl, const juce::String& apiKey)
{
    const auto nowIso = juce::Time::getCurrentTime().toISO8601 (true);
    const auto appVersion = juce::String (JUCE_APPLICATION_VERSION_STRING);
    const auto osName = juce::SystemStats::getOperatingSystemName();
    const auto platform = detectPlatformCategory();
    const auto deviceModel = juce::SystemStats::getDeviceDescription();
    const auto cpuModel = juce::SystemStats::getCpuModel();

    auto* root = new juce::DynamicObject();
    root->setProperty ("installId",        installId);
    root->setProperty ("platform",         platform);
    root->setProperty ("osName",           osName);
    root->setProperty ("appVersion",       appVersion);
    root->setProperty ("deviceModel",      deviceModel);
    root->setProperty ("cpuModel",         cpuModel);
    root->setProperty ("firstInstallTime", firstInstallTimeIso);
    root->setProperty ("lastSeenTime",     nowIso);
    root->setProperty ("launchCount",      launchCount);

    const juce::String payload = juce::JSON::toString (juce::var (root), true);

    juce::String baseUrl = dbUrl.trimCharactersAtEnd ("/");
    juce::String endpoint = baseUrl + "/installs/" + juce::URL::addEscapeChars (installId, false) + ".json";
    if (apiKey.isNotEmpty())
        endpoint += "?auth=" + juce::URL::addEscapeChars (apiKey, true);

    juce::URL url (endpoint);
    url = url.withPOSTData (payload);

    int statusCode = 0;
    auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                       .withHttpRequestCmd ("PUT")
                       .withExtraHeaders ("Content-Type: application/json\r\n")
                       .withConnectionTimeoutMs (8000)
                       .withStatusCode (&statusCode);

    try
    {
        auto stream = url.createInputStream (options);
        if (stream != nullptr && statusCode >= 200 && statusCode < 300)
        {
            stream->readEntireStreamAsString();
            return true;
        }
    }
    catch (...)
    {
        return false;
    }

    return false;
}

} // namespace mps
