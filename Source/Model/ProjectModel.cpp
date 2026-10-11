#include "ProjectModel.h"

namespace mps
{

ProjectModel::ProjectModel (AudioEngine& e) : engine (e)
{
    setupDefaults();
}

void ProjectModel::setupDefaults()
{
    // Professional standard channel naming & colors (X32/M32 console style palette)
    const struct { const char* name; juce::uint32 color; } defaults[] = {
        { "Kick",    0xffffc107 }, // Yellow / Gold
        { "Snare",   0xffff9800 }, // Orange
        { "Hi-Hat",  0xffffeb3b }, // Light Yellow
        { "Toms",    0xffff5722 }, // Deep Orange
        { "Overhead",0xffe91e63 }, // Pink
        { "Bass",    0xff2196f3 }, // Blue
        { "Gtr 1",   0xff4caf50 }, // Green
        { "Gtr 2",   0xff8bc34a }, // Light Green
        { "Keys",    0xff00bcd4 }, // Cyan
        { "Lead Vox",0xff9c27b0 }, // Purple
        { "Back Vox",0xffba68c8 }, // Light Purple
        { "Acoustic",0xff795548 }  // Brown
    };

    for (int i = 0; i < kMaxInputChannels; ++i)
    {
        if (i < 12)
        {
            channelMeta[(size_t) i].name = defaults[i].name;
            channelMeta[(size_t) i].colour = juce::Colour (defaults[i].color);
        }
        else
        {
            channelMeta[(size_t) i].name = "CH " + juce::String (i + 1);
            channelMeta[(size_t) i].colour = juce::Colour (0xff2196f3);
        }
        channelMeta[(size_t) i].icon = "";
    }
}

const ChannelMetadata& ProjectModel::getChannelMetadata (int ch) const
{
    jassert (ch >= 0 && ch < kMaxInputChannels);
    return channelMeta[(size_t) std::max (0, std::min (kMaxInputChannels - 1, ch))];
}

void ProjectModel::setChannelName (int ch, const juce::String& name)
{
    if (ch >= 0 && ch < kMaxInputChannels)
    {
        channelMeta[(size_t) ch].name = name;
        modified = true;
    }
}

void ProjectModel::setChannelColour (int ch, juce::Colour col)
{
    if (ch >= 0 && ch < kMaxInputChannels)
    {
        channelMeta[(size_t) ch].colour = col;
        modified = true;
    }
}

juce::File ProjectModel::getTrackFile (int slot) const
{
    if (slot >= 0 && slot < kMaxTracks)
        return trackFiles[(size_t) slot];
    return {};
}

void ProjectModel::setTrackFile (int slot, const juce::File& file)
{
    if (slot >= 0 && slot < kMaxTracks)
    {
        trackFiles[(size_t) slot] = file;
        modified = true;
    }
}

void ProjectModel::swapTracks (int slotA, int slotB)
{
    if (slotA < 0 || slotA >= kMaxTracks || slotB < 0 || slotB >= kMaxTracks || slotA == slotB)
        return;

    std::swap (trackFiles[(size_t) slotA], trackFiles[(size_t) slotB]);
    std::swap (channelMeta[(size_t) slotA], channelMeta[(size_t) slotB]);
    engine.swapTracks (slotA, slotB);
    modified = true;

    if (onTracksChanged)
        onTracksChanged();
}

void ProjectModel::moveTrack (int fromSlot, int toSlot)
{
    if (fromSlot < 0 || fromSlot >= kMaxTracks || toSlot < 0 || toSlot >= kMaxTracks || fromSlot == toSlot)
        return;

    auto f = trackFiles[(size_t) fromSlot];
    auto m = channelMeta[(size_t) fromSlot];
    if (fromSlot < toSlot)
    {
        for (int i = fromSlot; i < toSlot; ++i)
        {
            trackFiles[(size_t) i] = trackFiles[(size_t) (i + 1)];
            channelMeta[(size_t) i] = channelMeta[(size_t) (i + 1)];
        }
    }
    else
    {
        for (int i = fromSlot; i > toSlot; --i)
        {
            trackFiles[(size_t) i] = trackFiles[(size_t) (i - 1)];
            channelMeta[(size_t) i] = channelMeta[(size_t) (i - 1)];
        }
    }
    trackFiles[(size_t) toSlot] = f;
    channelMeta[(size_t) toSlot] = m;
    engine.moveTrack (fromSlot, toSlot);
    modified = true;

    if (onTracksChanged)
        onTracksChanged();
}

juce::var ProjectModel::serializeMixerState() const
{
    auto& params = engine.getMixerParams();
    auto mixerObj = new juce::DynamicObject();

    // Channels
    juce::Array<juce::var> chArray;
    for (int i = 0; i < kMaxInputChannels; ++i)
    {
        auto& cp = params.channels[(size_t) i];
        auto& meta = channelMeta[(size_t) i];
        auto chObj = new juce::DynamicObject();

        chObj->setProperty ("index", i);
        chObj->setProperty ("name", meta.name);
        chObj->setProperty ("colour", meta.colour.toDisplayString (true));
        chObj->setProperty ("source", cp.source.getInt());
        chObj->setProperty ("sourceMode", cp.sourceMode.getInt());
        chObj->setProperty ("trimDb", cp.trimDb.get());
        chObj->setProperty ("polarity", cp.polarity.getBool());
        chObj->setProperty ("faderDb", cp.faderDb.get());
        chObj->setProperty ("pan", cp.pan.get());
        chObj->setProperty ("mute", cp.mute.getBool());
        chObj->setProperty ("solo", cp.solo.getBool());
        chObj->setProperty ("mainAssign", cp.mainAssign.getBool());

        // Sends
        juce::Array<juce::var> sendsArr;
        for (int s = 0; s < 4; ++s)
            sendsArr.add (cp.sends[(size_t) s].get());
        chObj->setProperty ("sends", sendsArr);

        // DSP
        auto dspObj = new juce::DynamicObject();
        dspObj->setProperty ("delayMs", cp.dsp.delayMs.get());
        dspObj->setProperty ("delayBypass", cp.dsp.delayBypass.getBool());
        dspObj->setProperty ("hpfFreq", cp.dsp.hpfFreq.get());
        dspObj->setProperty ("hpfBypass", cp.dsp.hpfBypass.getBool());

        dspObj->setProperty ("gateBypass", cp.dsp.gateBypass.getBool());
        dspObj->setProperty ("gateThreshDb", cp.dsp.gateThreshDb.get());
        dspObj->setProperty ("gateRangeDb", cp.dsp.gateRangeDb.get());
        dspObj->setProperty ("gateAttackMs", cp.dsp.gateAttackMs.get());
        dspObj->setProperty ("gateHoldMs", cp.dsp.gateHoldMs.get());
        dspObj->setProperty ("gateReleaseMs", cp.dsp.gateReleaseMs.get());

        dspObj->setProperty ("eqBypass", cp.dsp.eqBypass.getBool());
        dspObj->setProperty ("eqLowType", cp.dsp.eqLowType.getInt());
        dspObj->setProperty ("eqLowFreq", cp.dsp.eqLowFreq.get());
        dspObj->setProperty ("eqLowGainDb", cp.dsp.eqLowGainDb.get());
        dspObj->setProperty ("eqLowQ", cp.dsp.eqLowQ.get());

        dspObj->setProperty ("eqMid1Freq", cp.dsp.eqMid1Freq.get());
        dspObj->setProperty ("eqMid1GainDb", cp.dsp.eqMid1GainDb.get());
        dspObj->setProperty ("eqMid1Q", cp.dsp.eqMid1Q.get());

        dspObj->setProperty ("eqMid2Freq", cp.dsp.eqMid2Freq.get());
        dspObj->setProperty ("eqMid2GainDb", cp.dsp.eqMid2GainDb.get());
        dspObj->setProperty ("eqMid2Q", cp.dsp.eqMid2Q.get());

        dspObj->setProperty ("eqHighType", cp.dsp.eqHighType.getInt());
        dspObj->setProperty ("eqHighFreq", cp.dsp.eqHighFreq.get());
        dspObj->setProperty ("eqHighGainDb", cp.dsp.eqHighGainDb.get());
        dspObj->setProperty ("eqHighQ", cp.dsp.eqHighQ.get());

        dspObj->setProperty ("compBypass", cp.dsp.compBypass.getBool());
        dspObj->setProperty ("compThreshDb", cp.dsp.compThreshDb.get());
        dspObj->setProperty ("compRatio", cp.dsp.compRatio.get());
        dspObj->setProperty ("compKneeDb", cp.dsp.compKneeDb.get());
        dspObj->setProperty ("compAttackMs", cp.dsp.compAttackMs.get());
        dspObj->setProperty ("compReleaseMs", cp.dsp.compReleaseMs.get());
        dspObj->setProperty ("compMakeupDb", cp.dsp.compMakeupDb.get());
        dspObj->setProperty ("compMix", cp.dsp.compMix.get());

        chObj->setProperty ("dsp", juce::var (dspObj));
        chArray.add (juce::var (chObj));
    }
    mixerObj->setProperty ("channels", chArray);

    // Main
    auto mainObj = new juce::DynamicObject();
    mainObj->setProperty ("faderDb", params.main.faderDb.get());
    mainObj->setProperty ("mute", params.main.mute.getBool());
    mixerObj->setProperty ("main", juce::var (mainObj));

    // FX Returns
    juce::Array<juce::var> fxRetArray;
    for (int r = 0; r < 8; ++r)
    {
        auto& rp = params.fxReturns[(size_t) r];
        auto retObj = new juce::DynamicObject();
        retObj->setProperty ("index", r);
        retObj->setProperty ("faderDb", rp.faderDb.get());
        retObj->setProperty ("pan", rp.pan.get());
        retObj->setProperty ("mute", rp.mute.getBool());
        retObj->setProperty ("solo", rp.solo.getBool());
        fxRetArray.add (juce::var (retObj));
    }
    mixerObj->setProperty ("fxReturns", fxRetArray);

    return juce::var (mixerObj);
}

void ProjectModel::deserializeMixerState (const juce::var& root)
{
    if (! root.isObject()) return;
    auto* rootObj = root.getDynamicObject();
    if (rootObj == nullptr) return;

    auto& params = engine.getMixerParams();

    if (rootObj->hasProperty ("channels"))
    {
        auto* arr = rootObj->getProperty ("channels").getArray();
        if (arr != nullptr)
        {
            for (auto& chVar : *arr)
            {
                auto* chObj = chVar.getDynamicObject();
                if (chObj == nullptr) continue;
                const int i = chObj->getProperty ("index");
                if (i < 0 || i >= kMaxInputChannels) continue;

                auto& cp = params.channels[(size_t) i];
                if (chObj->hasProperty ("name"))
                    channelMeta[(size_t) i].name = chObj->getProperty ("name").toString();
                if (chObj->hasProperty ("colour"))
                    channelMeta[(size_t) i].colour = juce::Colour::fromString (chObj->getProperty ("colour").toString());

                if (chObj->hasProperty ("source")) cp.source.set ((float) (int) chObj->getProperty ("source"));
                if (chObj->hasProperty ("sourceMode")) cp.sourceMode.set ((float) (int) chObj->getProperty ("sourceMode"));
                if (chObj->hasProperty ("trimDb")) cp.trimDb.set ((float) (double) chObj->getProperty ("trimDb"));
                if (chObj->hasProperty ("polarity")) cp.polarity.setBool ((bool) chObj->getProperty ("polarity"));
                if (chObj->hasProperty ("faderDb")) cp.faderDb.set ((float) (double) chObj->getProperty ("faderDb"));
                if (chObj->hasProperty ("pan")) cp.pan.set ((float) (double) chObj->getProperty ("pan"));
                if (chObj->hasProperty ("mute")) cp.mute.setBool ((bool) chObj->getProperty ("mute"));
                if (chObj->hasProperty ("solo")) cp.solo.setBool ((bool) chObj->getProperty ("solo"));
                if (chObj->hasProperty ("mainAssign")) cp.mainAssign.setBool ((bool) chObj->getProperty ("mainAssign"));

                if (chObj->hasProperty ("sends"))
                {
                    auto* sArr = chObj->getProperty ("sends").getArray();
                    if (sArr != nullptr)
                    {
                        for (int s = 0; s < 4 && s < sArr->size(); ++s)
                            cp.sends[(size_t) s].set ((float) (double) (*sArr)[s]);
                    }
                }

                if (chObj->hasProperty ("dsp"))
                {
                    auto* dspObj = chObj->getProperty ("dsp").getDynamicObject();
                    if (dspObj != nullptr)
                    {
                        if (dspObj->hasProperty ("delayMs")) cp.dsp.delayMs.set ((float) (double) dspObj->getProperty ("delayMs"));
                        if (dspObj->hasProperty ("delayBypass")) cp.dsp.delayBypass.setBool ((bool) dspObj->getProperty ("delayBypass"));
                        if (dspObj->hasProperty ("hpfFreq")) cp.dsp.hpfFreq.set ((float) (double) dspObj->getProperty ("hpfFreq"));
                        if (dspObj->hasProperty ("hpfBypass")) cp.dsp.hpfBypass.setBool ((bool) dspObj->getProperty ("hpfBypass"));

                        if (dspObj->hasProperty ("gateBypass")) cp.dsp.gateBypass.setBool ((bool) dspObj->getProperty ("gateBypass"));
                        if (dspObj->hasProperty ("gateThreshDb")) cp.dsp.gateThreshDb.set ((float) (double) dspObj->getProperty ("gateThreshDb"));
                        if (dspObj->hasProperty ("gateRangeDb")) cp.dsp.gateRangeDb.set ((float) (double) dspObj->getProperty ("gateRangeDb"));
                        if (dspObj->hasProperty ("gateAttackMs")) cp.dsp.gateAttackMs.set ((float) (double) dspObj->getProperty ("gateAttackMs"));
                        if (dspObj->hasProperty ("gateHoldMs")) cp.dsp.gateHoldMs.set ((float) (double) dspObj->getProperty ("gateHoldMs"));
                        if (dspObj->hasProperty ("gateReleaseMs")) cp.dsp.gateReleaseMs.set ((float) (double) dspObj->getProperty ("gateReleaseMs"));

                        if (dspObj->hasProperty ("eqBypass")) cp.dsp.eqBypass.setBool ((bool) dspObj->getProperty ("eqBypass"));
                        if (dspObj->hasProperty ("eqLowType")) cp.dsp.eqLowType.set ((float) (int) dspObj->getProperty ("eqLowType"));
                        if (dspObj->hasProperty ("eqLowFreq")) cp.dsp.eqLowFreq.set ((float) (double) dspObj->getProperty ("eqLowFreq"));
                        if (dspObj->hasProperty ("eqLowGainDb")) cp.dsp.eqLowGainDb.set ((float) (double) dspObj->getProperty ("eqLowGainDb"));
                        if (dspObj->hasProperty ("eqLowQ")) cp.dsp.eqLowQ.set ((float) (double) dspObj->getProperty ("eqLowQ"));

                        if (dspObj->hasProperty ("eqMid1Freq")) cp.dsp.eqMid1Freq.set ((float) (double) dspObj->getProperty ("eqMid1Freq"));
                        if (dspObj->hasProperty ("eqMid1GainDb")) cp.dsp.eqMid1GainDb.set ((float) (double) dspObj->getProperty ("eqMid1GainDb"));
                        if (dspObj->hasProperty ("eqMid1Q")) cp.dsp.eqMid1Q.set ((float) (double) dspObj->getProperty ("eqMid1Q"));

                        if (dspObj->hasProperty ("eqMid2Freq")) cp.dsp.eqMid2Freq.set ((float) (double) dspObj->getProperty ("eqMid2Freq"));
                        if (dspObj->hasProperty ("eqMid2GainDb")) cp.dsp.eqMid2GainDb.set ((float) (double) dspObj->getProperty ("eqMid2GainDb"));
                        if (dspObj->hasProperty ("eqMid2Q")) cp.dsp.eqMid2Q.set ((float) (double) dspObj->getProperty ("eqMid2Q"));

                        if (dspObj->hasProperty ("eqHighType")) cp.dsp.eqHighType.set ((float) (int) dspObj->getProperty ("eqHighType"));
                        if (dspObj->hasProperty ("eqHighFreq")) cp.dsp.eqHighFreq.set ((float) (double) dspObj->getProperty ("eqHighFreq"));
                        if (dspObj->hasProperty ("eqHighGainDb")) cp.dsp.eqHighGainDb.set ((float) (double) dspObj->getProperty ("eqHighGainDb"));
                        if (dspObj->hasProperty ("eqHighQ")) cp.dsp.eqHighQ.set ((float) (double) dspObj->getProperty ("eqHighQ"));

                        if (dspObj->hasProperty ("compBypass")) cp.dsp.compBypass.setBool ((bool) dspObj->getProperty ("compBypass"));
                        if (dspObj->hasProperty ("compThreshDb")) cp.dsp.compThreshDb.set ((float) (double) dspObj->getProperty ("compThreshDb"));
                        if (dspObj->hasProperty ("compRatio")) cp.dsp.compRatio.set ((float) (double) dspObj->getProperty ("compRatio"));
                        if (dspObj->hasProperty ("compKneeDb")) cp.dsp.compKneeDb.set ((float) (double) dspObj->getProperty ("compKneeDb"));
                        if (dspObj->hasProperty ("compAttackMs")) cp.dsp.compAttackMs.set ((float) (double) dspObj->getProperty ("compAttackMs"));
                        if (dspObj->hasProperty ("compReleaseMs")) cp.dsp.compReleaseMs.set ((float) (double) dspObj->getProperty ("compReleaseMs"));
                        if (dspObj->hasProperty ("compMakeupDb")) cp.dsp.compMakeupDb.set ((float) (double) dspObj->getProperty ("compMakeupDb"));
                        if (dspObj->hasProperty ("compMix")) cp.dsp.compMix.set ((float) (double) dspObj->getProperty ("compMix"));
                    }
                }
            }
        }
    }

    if (rootObj->hasProperty ("main"))
    {
        auto* mainObj = rootObj->getProperty ("main").getDynamicObject();
        if (mainObj != nullptr)
        {
            if (mainObj->hasProperty ("faderDb")) params.main.faderDb.set ((float) (double) mainObj->getProperty ("faderDb"));
            if (mainObj->hasProperty ("mute")) params.main.mute.setBool ((bool) mainObj->getProperty ("mute"));
        }
    }

    if (rootObj->hasProperty ("fxReturns"))
    {
        auto* arr = rootObj->getProperty ("fxReturns").getArray();
        if (arr != nullptr)
        {
            for (auto& retVar : *arr)
            {
                auto* retObj = retVar.getDynamicObject();
                if (retObj == nullptr) continue;
                const int r = retObj->getProperty ("index");
                if (r < 0 || r >= 8) continue;
                auto& rp = params.fxReturns[(size_t) r];
                if (retObj->hasProperty ("faderDb")) rp.faderDb.set ((float) (double) retObj->getProperty ("faderDb"));
                if (retObj->hasProperty ("pan")) rp.pan.set ((float) (double) retObj->getProperty ("pan"));
                if (retObj->hasProperty ("mute")) rp.mute.setBool ((bool) retObj->getProperty ("mute"));
                if (retObj->hasProperty ("solo")) rp.solo.setBool ((bool) retObj->getProperty ("solo"));
            }
        }
    }
}

juce::Result ProjectModel::saveProject (const juce::File& file, bool copyAudio)
{
    auto parentDir = file.getParentDirectory();
    if (! parentDir.exists())
        parentDir.createDirectory();

    auto audioDir = parentDir.getChildFile ("Audio");
    if (copyAudio && ! audioDir.exists())
        audioDir.createDirectory();

    auto root = new juce::DynamicObject();
    root->setProperty ("app", "MixPracticeStudio");
    root->setProperty ("version", kProjectVersion);
    root->setProperty ("saveDate", juce::Time::getCurrentTime().toISO8601 (true));

    // Tracks array
    juce::Array<juce::var> tracksArray;
    for (int s = 0; s < kMaxTracks; ++s)
    {
        auto srcFile = trackFiles[(size_t) s];
        if (srcFile.existsAsFile())
        {
            auto trackObj = new juce::DynamicObject();
            trackObj->setProperty ("slot", s);

            juce::File savedPath = srcFile;
            if (copyAudio)
            {
                auto targetFile = audioDir.getChildFile (srcFile.getFileName());
                if (! targetFile.existsAsFile() || targetFile.getLastModificationTime() < srcFile.getLastModificationTime())
                    srcFile.copyFileTo (targetFile);
                savedPath = targetFile;
                trackFiles[(size_t) s] = targetFile;
            }

            // Save relative path if inside parent directory, otherwise full path
            if (savedPath.isAChildOf (parentDir))
                trackObj->setProperty ("relativePath", savedPath.getRelativePathFrom (parentDir));
            else
                trackObj->setProperty ("absolutePath", savedPath.getFullPathName());

            tracksArray.add (juce::var (trackObj));
        }
    }
    root->setProperty ("tracks", tracksArray);

    // Mixer state
    root->setProperty ("mixer", serializeMixerState());

    // Safe atomic save: write to temporary file, then move
    auto tempFile = file.getSiblingFile (file.getFileName() + ".tmp");
    tempFile.deleteFile();

    auto jsonString = juce::JSON::toString (juce::var (root), true);
    if (! tempFile.replaceWithText (jsonString))
        return juce::Result::fail ("Could not write temporary project file: " + tempFile.getFullPathName());

    if (! tempFile.moveFileTo (file))
    {
        // Fallback: replace target directly
        if (! file.replaceWithText (jsonString))
            return juce::Result::fail ("Could not write project file: " + file.getFullPathName());
        tempFile.deleteFile();
    }

    projectFile = file;
    modified = false;
    return juce::Result::ok();
}

juce::Result ProjectModel::loadProject (const juce::File& file, juce::StringArray& outMissingFiles)
{
    if (! file.existsAsFile())
        return juce::Result::fail ("Project file does not exist: " + file.getFullPathName());

    auto jsonVar = juce::JSON::parse (file);
    if (! jsonVar.isObject())
        return juce::Result::fail ("Invalid project file format (not valid JSON)");

    auto* rootObj = jsonVar.getDynamicObject();
    if (rootObj == nullptr || rootObj->getProperty ("app").toString() != "MixPracticeStudio")
        return juce::Result::fail ("File is not a valid Mix Practice Studio project");

    const int version = rootObj->getProperty ("version");
    if (version > kProjectVersion)
        return juce::Result::fail ("Project was created with a newer version (" + juce::String (version) + ")");

    auto parentDir = file.getParentDirectory();

    // Clear current tracks
    engine.clearAllTracks();
    for (auto& tf : trackFiles) tf = juce::File();

    // Load tracks
    if (rootObj->hasProperty ("tracks"))
    {
        auto* tracksArr = rootObj->getProperty ("tracks").getArray();
        if (tracksArr != nullptr)
        {
            const double targetSr = engine.getSampleRate();
            for (auto& tVar : *tracksArr)
            {
                auto* tObj = tVar.getDynamicObject();
                if (tObj == nullptr) continue;
                const int slot = tObj->getProperty ("slot");
                if (slot < 0 || slot >= kMaxTracks) continue;

                juce::File audioFile;
                if (tObj->hasProperty ("relativePath"))
                    audioFile = parentDir.getChildFile (tObj->getProperty ("relativePath").toString());
                else if (tObj->hasProperty ("absolutePath"))
                    audioFile = juce::File (tObj->getProperty ("absolutePath").toString());

                if (audioFile.existsAsFile())
                {
                    trackFiles[(size_t) slot] = audioFile;
                    auto track = AudioFileTrack::open (audioFile, targetSr, engine.getFormatManager());
                    if (track != nullptr)
                        engine.setTrackSlot (slot, std::move (track));
                }
                else
                {
                    outMissingFiles.add (audioFile.getFullPathName());
                }
            }
        }
    }

    // Restore mixer state
    if (rootObj->hasProperty ("mixer"))
        deserializeMixerState (rootObj->getProperty ("mixer"));

    projectFile = file;
    modified = false;
    return juce::Result::ok();
}

juce::var ProjectModel::captureSceneSnapshot (const juce::String& sceneName) const
{
    auto obj = new juce::DynamicObject();
    obj->setProperty ("app", "MixPracticeStudioScene");
    obj->setProperty ("name", sceneName);
    obj->setProperty ("date", juce::Time::getCurrentTime().toISO8601 (true));
    obj->setProperty ("mixer", serializeMixerState());
    return juce::var (obj);
}

void ProjectModel::recallSceneSnapshot (const juce::var& sceneData)
{
    if (! sceneData.isObject()) return;
    auto* obj = sceneData.getDynamicObject();
    if (obj == nullptr) return;
    if (obj->hasProperty ("mixer"))
        deserializeMixerState (obj->getProperty ("mixer"));
    modified = true;
}

juce::Result ProjectModel::saveSceneToFile (const juce::File& file, const juce::String& sceneName) const
{
    auto snapshot = captureSceneSnapshot (sceneName);
    auto tempFile = file.getSiblingFile (file.getFileName() + ".tmp");
    tempFile.deleteFile();

    auto jsonString = juce::JSON::toString (snapshot, true);
    if (! tempFile.replaceWithText (jsonString))
        return juce::Result::fail ("Could not write scene snapshot file");

    if (! tempFile.moveFileTo (file))
    {
        if (! file.replaceWithText (jsonString))
            return juce::Result::fail ("Could not write scene file");
        tempFile.deleteFile();
    }
    return juce::Result::ok();
}

juce::Result ProjectModel::loadSceneFromFile (const juce::File& file)
{
    if (! file.existsAsFile())
        return juce::Result::fail ("Scene file does not exist");

    auto jsonVar = juce::JSON::parse (file);
    if (! jsonVar.isObject())
        return juce::Result::fail ("Invalid scene file (not JSON)");

    recallSceneSnapshot (jsonVar);
    return juce::Result::ok();
}

} // namespace mps
