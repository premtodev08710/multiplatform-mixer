// Mix Practice Studio - Application Edition & Annual License Manager
#pragma once

#include <juce_core/juce_core.h>

namespace mps
{

/** Central manager for Free vs Pro edition and Annual Serial Key license validation. */
class AppEdition
{
public:
    static constexpr int kFreeMaxTracks = 8;
    static constexpr int kProMaxTracks = 32;

    static constexpr int kFreeMaxFxSlots = 2;
    static constexpr int kProMaxFxSlots = 8;

    // Secret salt used for offline cryptographic verification
    inline static const juce::String kSecretSalt = "mps_pro_annual_x32_salt_2026_mixer";

    struct ActivationResult
    {
        bool success = false;
        juce::String message;
        int daysRemaining = 0;
    };

    /** Returns true if running as Pro Edition (compiled binary OR active yearly subscription / trial). */
    static bool isPro() noexcept
    {
#if defined(MPS_PRO_VERSION) && MPS_PRO_VERSION == 1
        return true;
#else
        ensureTrialInitialized();
        return isYearlySubscriptionActive();
#endif
    }

    /** Maximum allowed multitrack audio tracks and channel faders. */
    static int getMaxTracks() noexcept
    {
        return isPro() ? kProMaxTracks : kFreeMaxTracks;
    }

    /** Maximum available virtual FX rack slots. */
    static int getMaxFxSlots() noexcept
    {
        return isPro() ? kProMaxFxSlots : kFreeMaxFxSlots;
    }

    /** Whether saving project files is permitted. */
    static bool canSaveProject() noexcept
    {
        return isPro();
    }

    /** Whether Shopee affiliate sponsor ads should be displayed. */
    static bool hasShopeeAds() noexcept
    {
        if (isLifetimePro())
            return false;
        if (isYearlySubscriptionActive() && ! isTrial())
            return false;
        return true;
    }

    static bool isLifetimePro() noexcept
    {
#if defined(MPS_PRO_VERSION) && MPS_PRO_VERSION == 1
        return true;
#else
        return false;
#endif
    }

    /** Checks whether the stored yearly subscription is currently active. */
    static bool isYearlySubscriptionActive() noexcept
    {
        const int64_t expiry = getExpiryTimestamp();
        if (expiry <= 0)
            return false;

        const int64_t now = juce::Time::getCurrentTime().toMilliseconds() / 1000;
        return (now < expiry);
    }

    /** Returns remaining active days for Pro subscription. */
    static int getDaysRemaining() noexcept
    {
        if (isLifetimePro())
            return 9999;

        const int64_t expiry = getExpiryTimestamp();
        if (expiry <= 0)
            return 0;

        const int64_t now = juce::Time::getCurrentTime().toMilliseconds() / 1000;
        if (now >= expiry)
            return 0;

        const int64_t diffSec = expiry - now;
        return (int) (diffSec / 86400) + 1;
    }

    /** Returns formatted expiry date string (e.g. "22 Oct 2027"). */
    static juce::String getExpiryDateFormatted()
    {
        if (isLifetimePro())
            return juce::String::fromUTF8 ("Lifetime (ถาวร)");

        const int64_t expiry = getExpiryTimestamp();
        if (expiry <= 0)
            return juce::String::fromUTF8 ("ยังไม่เปิดใช้งาน (Free)");

        const int64_t now = juce::Time::getCurrentTime().toMilliseconds() / 1000;
        if (now >= expiry)
        {
            if (isTrial())
                return juce::String::fromUTF8 ("หมดช่วงทดลองใช้ 14 วันแล้ว (Expired Trial)");
            return juce::String::fromUTF8 ("หมดอายุแล้ว (Expired)");
        }

        juce::Time t ((juce::int64) expiry * 1000);
        const juce::String dateStr = t.formatted ("%d %b %Y");
        if (isTrial())
            return dateStr + juce::String::fromUTF8 (" (ทดลองใช้ฟรี 14 วัน)");
        return dateStr;
    }

    static juce::String getEditionName() noexcept
    {
        if (isLifetimePro())
            return "PRO LIFETIME";
        if (isTrialActive())
            return "PRO TRIAL";
        if (isYearlySubscriptionActive())
            return "PRO ANNUAL";
        return "FREE EDITION";
    }

    static juce::String getEditionBadge() noexcept
    {
        if (isLifetimePro())
            return "PRO";
        if (isTrialActive())
            return "TRIAL";
        if (isPro())
            return "PRO";
        return "FREE";
    }

    static juce::String getPlan() noexcept
    {
        auto file = getLicenseFile();
        if (! file.existsAsFile())
            return "";

        const auto jsonStr = file.loadFileAsString();
        auto parsed = juce::JSON::parse (jsonStr);
        if (parsed.isObject() && parsed.getDynamicObject() != nullptr)
            return parsed.getDynamicObject()->getProperty ("plan").toString();

        return "";
    }

    static bool isTrial() noexcept
    {
        return getPlan() == "trial";
    }

    static bool isTrialActive() noexcept
    {
        return isTrial() && isYearlySubscriptionActive();
    }

    /** Ensures that new installations receive a 14-day full Pro Trial on first launch. */
    static void ensureTrialInitialized()
    {
        auto file = getLicenseFile();
        auto markerFile = file.getParentDirectory().getChildFile (".trial_started");

        if (! file.existsAsFile() && ! markerFile.existsAsFile())
        {
            const int64_t now = juce::Time::getCurrentTime().toMilliseconds() / 1000;
            const int64_t trialExpiry = now + (14LL * 86400LL); // 14 Days Free Trial

            markerFile.getParentDirectory().createDirectory();
            markerFile.replaceWithText (juce::String (now));
            saveLicenseData (trialExpiry, "TRIAL-14D", { "TRIAL-14D" }, "trial");
        }
    }

    /** Verifies key signature offline: MPS-1Y-XXXX-XXXX-XXXX */
    static bool verifyKeyFormatAndChecksum (const juce::String& inputKey, juce::String& outCleanKey)
    {
        juce::String clean = inputKey.trim().toUpperCase().removeCharacters (" -_");
        // Expected format after removing separators: MPS1Y + 8 hex chars + 4 hex chars = 17 chars
        // Or with prefix MPS: MPS + 8 hex + 4 hex = 15 chars
        if (clean.startsWith ("MPS1Y"))
            clean = clean.substring (5);
        else if (clean.startsWith ("MPS"))
            clean = clean.substring (3);
        else
            return false;

        if (clean.length() != 12)
            return false;

        const juce::String randPayload = clean.substring (0, 8);
        const juce::String givenChecksum = clean.substring (8, 12);

        const juce::String expectedChecksum = calculateChecksum (randPayload);
        if (expectedChecksum == givenChecksum)
        {
            outCleanKey = "MPS-1Y-" + randPayload.substring (0, 4) + "-" + randPayload.substring (4, 8) + "-" + givenChecksum;
            return true;
        }

        return false;
    }

    /** Activates or extends the yearly Pro subscription by 365 days. */
    static ActivationResult activateYearlyKey (const juce::String& keyInput)
    {
        ActivationResult res;
        juce::String cleanKey;

        // Special developer/testing override key
        if (keyInput.trim() == "MPS-PRO-TEST-VIP" || keyInput.trim() == "PRO-LIFETIME-ACCESS")
        {
            const int64_t now = juce::Time::getCurrentTime().toMilliseconds() / 1000;
            const int64_t newExpiry = now + (365LL * 86400LL);
            saveLicenseData (newExpiry, "TEST-VIP-KEY", {}, "annual");
            res.success = true;
            res.daysRemaining = 365;
            res.message = juce::String::fromUTF8 ("เปิดใช้งานสิทธิ์ Pro สำเร็จ! (365 วัน)");
            return res;
        }

        // Special manual trial code (in case user wants to redeem a 14-day trial code)
        if (keyInput.trim().equalsIgnoreCase ("MPS-TRIAL-14DAYS") || keyInput.trim().equalsIgnoreCase ("FREE-TRIAL-14D"))
        {
            auto usedKeys = getUsedKeys();
            if (usedKeys.contains ("TRIAL-14D"))
            {
                res.success = false;
                res.message = juce::String::fromUTF8 ("เครื่องนี้เคยรับสิทธิ์ทดลองใช้งานฟรี 14 วันไปแล้ว");
                return res;
            }
            const int64_t now = juce::Time::getCurrentTime().toMilliseconds() / 1000;
            const int64_t newExpiry = now + (14LL * 86400LL);
            usedKeys.add ("TRIAL-14D");
            saveLicenseData (newExpiry, "TRIAL-14D", usedKeys, "trial");
            res.success = true;
            res.daysRemaining = 14;
            res.message = juce::String::fromUTF8 ("เปิดใช้งานสิทธิ์ทดลองใช้งาน Pro ฟรี 14 วันสำเร็จ!");
            return res;
        }

        if (! verifyKeyFormatAndChecksum (keyInput, cleanKey))
        {
            res.success = false;
            res.message = juce::String::fromUTF8 ("รหัสต่ออายุไม่ถูกต้อง กรุณาตรวจสอบรหัสใหม่อีกครั้ง");
            return res;
        }

        // Check if this key was already used on this machine
        auto usedKeys = getUsedKeys();
        if (usedKeys.contains (cleanKey))
        {
            res.success = false;
            res.message = juce::String::fromUTF8 ("รหัสนี้ถูกใช้งานบนเครื่องนี้ไปแล้ว ไม่สามารถใช้ซ้ำได้");
            return res;
        }

        const int64_t now = juce::Time::getCurrentTime().toMilliseconds() / 1000;
        int64_t currentExpiry = getExpiryTimestamp();

        int64_t baseTime = (currentExpiry > now) ? currentExpiry : now;
        int64_t newExpiry = baseTime + (365LL * 86400LL); // Add 365 days

        usedKeys.add (cleanKey);
        saveLicenseData (newExpiry, cleanKey, usedKeys, "annual");

        res.success = true;
        res.daysRemaining = (int) ((newExpiry - now) / 86400LL) + 1;
        res.message = juce::String::fromUTF8 ("เติมวันสำเร็จ! ได้รับสิทธิ์ Pro เพิ่มอีก 365 วัน\n(หมดอายุ: ") + getExpiryDateFormatted() + ")";
        return res;
    }

    /** Generates a new 1-year key (for Admin tools / testing). */
    static juce::String generateYearlyKey()
    {
        static const char hexChars[] = "0123456789ABCDEF";
        auto& r = juce::Random::getSystemRandom();

        juce::String randPayload;
        for (int i = 0; i < 8; ++i)
            randPayload += hexChars[r.nextInt (16)];

        const juce::String checksum = calculateChecksum (randPayload);
        return "MPS-1Y-" + randPayload.substring (0, 4) + "-" + randPayload.substring (4, 8) + "-" + checksum;
    }

private:
    static uint64_t fnv1a64 (const char* str, size_t len) noexcept
    {
        uint64_t hash = 0xcbf29ce484222325ULL;
        for (size_t i = 0; i < len; ++i)
        {
            hash ^= (uint64_t) (uint8_t) str[i];
            hash *= 0x100000001b3ULL;
        }
        return hash;
    }

    static juce::String calculateChecksum (const juce::String& payload8)
    {
        const juce::String raw = "MPS-1Y-" + payload8 + "-" + kSecretSalt;
        const uint64_t h = fnv1a64 (raw.toRawUTF8(), (size_t) raw.getNumBytesAsUTF8());
        return juce::String::toHexString ((int64_t) h).toUpperCase().paddedLeft ('0', 16).substring (0, 4);
    }

    static juce::String calculateFileSignature (int64_t expiry, const juce::String& key)
    {
        const juce::String raw = juce::String (expiry) + ":" + key + ":" + kSecretSalt;
        const uint64_t h = fnv1a64 (raw.toRawUTF8(), (size_t) raw.getNumBytesAsUTF8());
        return juce::String::toHexString ((int64_t) h).toLowerCase().paddedLeft ('0', 16);
    }

    static juce::File getLicenseFile()
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
            .getChildFile ("MixPracticeStudio")
            .getChildFile ("pro_license_v2.json");
    }

    static int64_t getExpiryTimestamp() noexcept
    {
        auto file = getLicenseFile();
        if (! file.existsAsFile())
            return 0;

        const auto jsonStr = file.loadFileAsString();
        auto parsed = juce::JSON::parse (jsonStr);
        if (! parsed.isObject())
            return 0;

        auto* obj = parsed.getDynamicObject();
        if (obj == nullptr)
            return 0;

        const int64_t expiry = (int64_t) obj->getProperty ("expiresAt");
        const juce::String lastKey = obj->getProperty ("lastKey").toString();
        const juce::String sig = obj->getProperty ("signature").toString();

        // Anti-tamper check
        if (calculateFileSignature (expiry, lastKey) != sig)
        {
            return 0; // File was tampered with!
        }

        return expiry;
    }

    static juce::StringArray getUsedKeys()
    {
        juce::StringArray list;
        auto file = getLicenseFile();
        if (! file.existsAsFile())
            return list;

        auto parsed = juce::JSON::parse (file.loadFileAsString());
        if (parsed.isObject() && parsed.getDynamicObject() != nullptr)
        {
            auto usedVar = parsed.getDynamicObject()->getProperty ("usedKeys");
            if (usedVar.isArray())
            {
                for (int i = 0; i < usedVar.size(); ++i)
                    list.add (usedVar[i].toString());
            }
        }
        return list;
    }

    static void saveLicenseData (int64_t expiry, const juce::String& lastKey, juce::StringArray usedKeys = {}, const juce::String& plan = "annual")
    {
        auto file = getLicenseFile();
        file.getParentDirectory().createDirectory();

        auto* obj = new juce::DynamicObject();
        obj->setProperty ("edition", "PRO");
        obj->setProperty ("plan", plan);
        obj->setProperty ("expiresAt", (juce::int64) expiry);
        obj->setProperty ("lastKey", lastKey);
        obj->setProperty ("signature", calculateFileSignature (expiry, lastKey));

        juce::Array<juce::var> usedArr;
        for (const auto& k : usedKeys)
            usedArr.add (k);
        obj->setProperty ("usedKeys", usedArr);

        juce::var rootVar (obj);
        file.replaceWithText (juce::JSON::toString (rootVar));
    }
};

} // namespace mps
