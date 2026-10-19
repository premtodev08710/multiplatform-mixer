// Mix Practice Studio - Android AdMob Interstitial Manager
#include "AdMobManager.h"

#if JUCE_ANDROID
#define JUCE_CORE_INCLUDE_JNI_HELPERS 1
#include <juce_core/juce_core.h>
#include <jni.h>
#endif

namespace mps
{

#if JUCE_ANDROID
static jclass g_adMobHelperClass = nullptr;

static void ensureAdMobHelperClassLoaded()
{
    if (g_adMobHelperClass != nullptr)
        return;

    auto* env = juce::getEnv();
    if (env == nullptr)
        return;

    jclass localCls = env->FindClass ("com/mixpractice/studio/AdMobHelper");
    if (localCls != nullptr)
    {
        g_adMobHelperClass = static_cast<jclass> (env->NewGlobalRef (localCls));
        env->DeleteLocalRef (localCls);
    }
    else
    {
        env->ExceptionClear();
    }
}
#endif

void AdMobManager::initialize (bool showOnFirstLoad)
{
    if (initialized)
        return;

    initialized = true;

   #if JUCE_ANDROID
    ensureAdMobHelperClassLoaded();

    auto* env = juce::getEnv();
    if (env == nullptr || g_adMobHelperClass == nullptr)
        return;

    jmethodID mid = env->GetStaticMethodID (
        g_adMobHelperClass,
        "initialize",
        "(Landroid/app/Activity;Ljava/lang/String;ZJ)V");

    if (mid == nullptr)
    {
        env->ExceptionClear();
        return;
    }

    auto jActivity = juce::getCurrentActivity();
    if (jActivity.get() == nullptr)
        return;

    jstring jAdUnitId = env->NewStringUTF (kInterstitialAdUnitId);
    env->CallStaticVoidMethod (
        g_adMobHelperClass,
        mid,
        jActivity.get(),
        jAdUnitId,
        static_cast<jboolean> (showOnFirstLoad ? JNI_TRUE : JNI_FALSE),
        static_cast<jlong> (kScreenSwitchCooldownMs));

    if (env->ExceptionCheck())
        env->ExceptionClear();

    env->DeleteLocalRef (jAdUnitId);
   #else
    juce::ignoreUnused (showOnFirstLoad);
   #endif
}

void AdMobManager::notifyScreenTransition (bool isAudioPlaying)
{
    // Screen transition ads disabled per user requirement:
    // Only 1 launch interstitial ad is shown when opening the app.
    juce::ignoreUnused (isAudioPlaying);
}

} // namespace mps
