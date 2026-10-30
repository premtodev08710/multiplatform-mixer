#include "ProjectModel.h"
#include "ConsoleStateFields.h"

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
        chObj->setProperty ("muteGroups", cp.muteGroups.getInt());
        chObj->setProperty ("dcaGroups", cp.dcaGroups.getInt());

        // Sends 1..16
        juce::Array<juce::var> sendsArr;
        juce::Array<juce::var> sendMutesArr;
        for (int s = 0; s < 16; ++s)
        {
            sendsArr.add (cp.sends[(size_t) s].get());
            sendMutesArr.add (cp.sendMutes[(size_t) s].getBool());
        }
        chObj->setProperty ("sends", sendsArr);
        chObj->setProperty ("sendMutes", sendMutesArr);

        juce::Array<juce::var> sendTapsArr;
        for (int t = 0; t < 8; ++t)
            sendTapsArr.add (cp.sendTaps[(size_t) t].getInt());
        chObj->setProperty ("sendTaps", sendTapsArr);

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

    // FX engine configuration, including three pages of controls and both
    // graphic-EQ channels, is part of the project/scene snapshot.
    mixerObj->setProperty ("fxCatalogVersion", 2);
    juce::Array<juce::var> fxSlotArray;
    for (int i = 0; i < 8; ++i)
    {
        const auto& fp = params.fxSlots[(size_t) i];
        auto fxObj = new juce::DynamicObject();
        fxObj->setProperty ("index", i);
        fxObj->setProperty ("type", fp.fxType.getInt());
        fxObj->setProperty ("bypass", fp.bypass.getBool());
        fxObj->setProperty ("insertMode", i >= 4 || fp.insertMode.getBool());
        fxObj->setProperty ("mix", fp.mix.get());
        fxObj->setProperty ("parameterEncoding", fp.parameterEncoding.getInt());

        juce::Array<juce::var> parameters;
        for (int p = 0; p < FxSlotParams::kNumParameters; ++p)
            parameters.add (fp.parameter (p).get());
        fxObj->setProperty ("parameters", parameters);
        fxObj->setProperty ("rightParametersEnabled", fp.rightParametersEnabled.getBool());
        juce::Array<juce::var> parametersR;
        for (int p = 0; p < FxSlotParams::kNumParameters; ++p) parametersR.add (fp.rightParameters[(size_t)p].get());
        fxObj->setProperty ("parametersR", parametersR);

        juce::Array<juce::var> geqL, geqR;
        for (int band = 0; band < 31; ++band)
        {
            geqL.add (fp.geqGainsL[(size_t) band].get());
            geqR.add (fp.geqGainsR[(size_t) band].get());
        }
        fxObj->setProperty ("geqL", geqL);
        fxObj->setProperty ("geqR", geqR);
        fxSlotArray.add (juce::var (fxObj));
    }
    mixerObj->setProperty ("fxSlots", fxSlotArray);

    // Channel Stereo Links
    juce::Array<juce::var> linksArr;
    for (int p = 0; p < 16; ++p)
        linksArr.add (params.channelPairLinked[(size_t) p].load());
    mixerObj->setProperty ("channelLinks", linksArr);

    // DCA Groups 1..8
    juce::Array<juce::var> dcaArray;
    for (int d = 0; d < 8; ++d)
    {
        auto& dp = params.dcas[(size_t) d];
        auto dcaObj = new juce::DynamicObject();
        dcaObj->setProperty ("index", d);
        dcaObj->setProperty ("faderDb", dp.faderDb.get());
        dcaObj->setProperty ("mute", dp.mute.getBool());
        dcaArray.add (juce::var (dcaObj));
    }
    mixerObj->setProperty ("dcas", dcaArray);

    auto* console = new juce::DynamicObject();
    visitConsoleState (params, [&] (const juce::String& key, Param& value) {
        console->setProperty (juce::Identifier (key), value.get());
    });
    mixerObj->setProperty ("consoleState", juce::var (console));
    juce::Array<juce::var> activeMutes;
    for (auto& active : params.muteGroupActive) activeMutes.add (active.load());
    mixerObj->setProperty ("muteGroupActive", activeMutes);
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
                if (chObj->hasProperty ("muteGroups")) cp.muteGroups.set ((float) (int) chObj->getProperty ("muteGroups"));
                if (chObj->hasProperty ("dcaGroups"))  cp.dcaGroups.set ((float) (int) chObj->getProperty ("dcaGroups"));

                if (chObj->hasProperty ("sends"))
                {
                    auto* sArr = chObj->getProperty ("sends").getArray();
                    if (sArr != nullptr)
                    {
                        for (int s = 0; s < 16 && s < sArr->size(); ++s)
                            cp.sends[(size_t) s].set ((float) (double) (*sArr)[s]);
                    }
                }

                if (chObj->hasProperty ("sendMutes"))
                {
                    auto* mArr = chObj->getProperty ("sendMutes").getArray();
                    if (mArr != nullptr)
                    {
                        for (int s = 0; s < 16 && s < mArr->size(); ++s)
                            cp.sendMutes[(size_t) s].setBool ((bool) (*mArr)[s]);
                    }
                }

                if (chObj->hasProperty ("sendTaps"))
                {
                    auto* tArr = chObj->getProperty ("sendTaps").getArray();
                    if (tArr != nullptr)
                    {
                        for (int t = 0; t < 8 && t < tArr->size(); ++t)
                            cp.sendTaps[(size_t) t].set ((float) (int) (*tArr)[t]);
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

    if (rootObj->hasProperty ("fxSlots"))
    {
        auto* arr = rootObj->getProperty ("fxSlots").getArray();
        if (arr != nullptr)
        {
            for (auto& fxVar : *arr)
            {
                auto* fxObj = fxVar.getDynamicObject();
                if (fxObj == nullptr) continue;
                const int i = (int) fxObj->getProperty ("index");
                if (i < 0 || i >= 8) continue;

                auto& fp = params.fxSlots[(size_t) i];
                if (fxObj->hasProperty ("type"))
                {
                    const int storedType = (int) fxObj->getProperty ("type");
                    const int catalogVersion = (int) rootObj->getProperty ("fxCatalogVersion");
                    if (catalogVersion >= 2)
                    {
                        fp.fxType.set ((float) juce::jlimit (0, getNumFxTypes() - 1, storedType));
                    }
                    else
                    {
                        // Projects created before the X32-Mix catalog was added
                        // stored the legacy 41-effect order. Translate it so
                        // those projects keep the same algorithm after loading.
                        static constexpr int legacyFxTypeMap[41] = {
                            0, 5, 6, 7, 1, 8, 9, 10, 11, 12,
                            13, 16, 14, 15, 17, 18, 19, 60, 28, 27,
                            30, 29, 44, 46, 41, 42, 39, 38, 37, 31,
                            33, 35, 51, 50, 49, 48, 53, 52, 56, 58,
                            20
                        };
                        const int legacyIndex = juce::jlimit (0, 40, storedType);
                        fp.fxType.set ((float) legacyFxTypeMap[legacyIndex]);
                    }
                }
                if (fxObj->hasProperty ("bypass"))
                    fp.bypass.setBool ((bool) fxObj->getProperty ("bypass"));
                if (fxObj->hasProperty ("insertMode"))
                    fp.insertMode.setBool (i >= 4 || (bool) fxObj->getProperty ("insertMode"));
                if (fxObj->hasProperty ("mix"))
                    fp.mix.set ((float) (double) fxObj->getProperty ("mix"));

                fp.parameterEncoding.set ((float)(int)fxObj->getProperty ("parameterEncoding"));
                fp.rightParametersEnabled.setBool ((bool) fxObj->getProperty ("rightParametersEnabled"));
                if (auto* valuesR = fxObj->getProperty ("parametersR").getArray())
                    for (int p = 0; p < std::min (valuesR->size(), FxSlotParams::kNumParameters); ++p)
                        fp.rightParameters[(size_t)p].set ((float)(double)(*valuesR)[p]);
                if (fxObj->hasProperty ("parameters"))
                {
                    auto* values = fxObj->getProperty ("parameters").getArray();
                    if (values != nullptr)
                        for (int p = 0; p < FxSlotParams::kNumParameters && p < values->size(); ++p)
                            fp.parameter (p).set ((float) (double) (*values)[p]);
                }

                const auto restoreGeq = [] (const juce::var& source, std::array<Param, 31>& destination)
                {
                    auto* values = source.getArray();
                    if (values == nullptr) return;
                    for (int band = 0; band < 31 && band < values->size(); ++band)
                        destination[(size_t) band].set ((float) (double) (*values)[band]);
                };
                if (fxObj->hasProperty ("geqL")) restoreGeq (fxObj->getProperty ("geqL"), fp.geqGainsL);
                if (fxObj->hasProperty ("geqR")) restoreGeq (fxObj->getProperty ("geqR"), fp.geqGainsR);
            }
        }
    }

    if (rootObj->hasProperty ("channelLinks"))
    {
        auto* lArr = rootObj->getProperty ("channelLinks").getArray();
        if (lArr != nullptr)
        {
            for (int p = 0; p < 16 && p < lArr->size(); ++p)
                params.channelPairLinked[(size_t) p].store ((bool) (*lArr)[p]);
        }
    }

    if (rootObj->hasProperty ("dcas"))
    {
        auto* dArr = rootObj->getProperty ("dcas").getArray();
        if (dArr != nullptr)
        {
            for (auto& dVar : *dArr)
            {
                auto* dObj = dVar.getDynamicObject();
                if (dObj == nullptr) continue;
                const int d = dObj->getProperty ("index");
                if (d < 0 || d >= 8) continue;
                auto& dp = params.dcas[(size_t) d];
                if (dObj->hasProperty ("faderDb")) dp.faderDb.set ((float) (double) dObj->getProperty ("faderDb"));
                if (dObj->hasProperty ("mute"))    dp.mute.setBool ((bool) dObj->getProperty ("mute"));
            }
        }
    }
    // Optional extension: older projects retain their legacy restore behavior.
    if (auto* console = rootObj->getProperty ("consoleState").getDynamicObject())
        visitConsoleState (params, [&] (const juce::String& key, Param& value) {
            const juce::Identifier id (key);
            if (console->hasProperty (id)) value.set ((float) (double) console->getProperty (id));
        });
    if (auto* active = rootObj->getProperty ("muteGroupActive").getArray())
        for (int i = 0; i < 6 && i < active->size(); ++i)
            params.muteGroupActive[(size_t) i].store ((bool) (*active)[i]);

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
            std::vector<std::pair<int, std::shared_ptr<TrackReader>>> batchSlots;
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
                        batchSlots.emplace_back (slot, std::move (track));
                }
                else
                {
                    outMissingFiles.add (audioFile.getFullPathName());
                }
            }
            if (! batchSlots.empty())
                engine.setTrackSlots (batchSlots);
        }
    }

    // Restore mixer state
    if (rootObj->hasProperty ("mixer"))
        deserializeMixerState (rootObj->getProperty ("mixer"));

    if (onTracksChanged)
        onTracksChanged();

    projectFile = file;
    modified = false;
    return juce::Result::ok();
}

juce::File ProjectModel::getDefaultProjectFile()
{
    auto appDataDir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                          .getChildFile ("MixPracticeStudio");
    if (! appDataDir.exists())
        appDataDir.createDirectory();

    return appDataDir.getChildFile ("last_session.mpsproj");
}

bool ProjectModel::hasSavedSession()
{
    return getDefaultProjectFile().existsAsFile();
}

juce::Result ProjectModel::autoSaveSession()
{
    auto file = getDefaultProjectFile();
    return saveProject (file, false);
}

juce::Result ProjectModel::autoLoadSession (juce::StringArray& outMissingFiles)
{
    auto file = getDefaultProjectFile();
    if (! file.existsAsFile())
        return juce::Result::fail ("No previous session found");

    return loadProject (file, outMissingFiles);
}

juce::var ProjectModel::captureSceneSnapshot (const juce::String& sceneName) const
{
    auto obj = new juce::DynamicObject();
    obj->setProperty ("app", "MixPracticeStudioScene");
    obj->setProperty ("name", sceneName);
    obj->setProperty ("date", juce::Time::getCurrentTime().toISO8601 (true));
    obj->setProperty ("mixer", serializeMixerState());

    // Also store multitrack audio file paths
    juce::Array<juce::var> tracksArray;
    for (int s = 0; s < kMaxTracks; ++s)
    {
        auto srcFile = trackFiles[(size_t) s];
        if (srcFile.existsAsFile())
        {
            auto trackObj = new juce::DynamicObject();
            trackObj->setProperty ("slot", s);
            trackObj->setProperty ("absolutePath", srcFile.getFullPathName());
            tracksArray.add (juce::var (trackObj));
        }
    }
    obj->setProperty ("tracks", tracksArray);
    return juce::var (obj);
}

void ProjectModel::recallSceneSnapshot (const juce::var& sceneData)
{
    if (! sceneData.isObject()) return;
    auto* obj = sceneData.getDynamicObject();
    if (obj == nullptr) return;
    if (obj->hasProperty ("mixer"))
        deserializeMixerState (obj->getProperty ("mixer"));

    // Reload multitrack audio if stored in this scene
    if (obj->hasProperty ("tracks"))
    {
        auto* tracksArr = obj->getProperty ("tracks").getArray();
        if (tracksArr != nullptr && ! tracksArr->isEmpty())
        {
            engine.clearAllTracks();
            for (auto& tf : trackFiles) tf = juce::File();

            const double targetSr = engine.getSampleRate();
            std::vector<std::pair<int, std::shared_ptr<TrackReader>>> batchSlots;
            for (auto& tVar : *tracksArr)
            {
                auto* tObj = tVar.getDynamicObject();
                if (tObj == nullptr) continue;
                const int slot = tObj->getProperty ("slot");
                if (slot < 0 || slot >= kMaxTracks) continue;

                if (tObj->hasProperty ("absolutePath"))
                {
                    juce::File audioFile (tObj->getProperty ("absolutePath").toString());
                    if (audioFile.existsAsFile())
                    {
                        trackFiles[(size_t) slot] = audioFile;
                        auto track = AudioFileTrack::open (audioFile, targetSr, engine.getFormatManager());
                        if (track != nullptr)
                            batchSlots.emplace_back (slot, std::move (track));
                    }
                }
            }
            if (! batchSlots.empty())
                engine.setTrackSlots (batchSlots);
            if (onTracksChanged) onTracksChanged();
        }
    }

    modified = true;
}

void ProjectModel::createNewScene()
{
    engine.clearAllTracks();
    for (auto& tf : trackFiles) tf = juce::File();
    setupDefaults();
    engine.getMixerParams().resetToDefaults();
    projectFile = juce::File();
    modified = true;
    if (onTracksChanged) onTracksChanged();
}

juce::File ProjectModel::getScenesStorageFile()
{
    auto appDataDir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                          .getChildFile ("MixPracticeStudio");
    if (! appDataDir.exists())
        appDataDir.createDirectory();
    return appDataDir.getChildFile ("scenes.json");
}

juce::var ProjectModel::loadStoredScenes() const
{
    auto file = getScenesStorageFile();
    if (! file.existsAsFile())
        return juce::var();
    return juce::JSON::parse (file);
}

juce::Result ProjectModel::saveStoredScenes (const juce::var& scenesArray)
{
    auto file = getScenesStorageFile();
    auto jsonString = juce::JSON::toString (scenesArray, true);
    if (! file.replaceWithText (jsonString))
        return juce::Result::fail ("Could not save scenes to device");
    return juce::Result::ok();
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

static float parseX32Freq (juce::String token) noexcept
{
    token = token.trim().toLowerCase();
    if (token.containsChar ('k'))
    {
        auto parts = juce::StringArray::fromTokens (token, "k", "");
        if (parts.size() == 2)
        {
            float scale = std::pow (10.0f, -(float) parts[1].length());
            float kHz = parts[0].getFloatValue() + parts[1].getFloatValue() * scale;
            return kHz * 1000.0f;
        }
    }
    return token.getFloatValue();
}

static float parseX32Db (juce::String token) noexcept
{
    token = token.trim();
    if (token == "-oo" || token == "-inf" || token.equalsIgnoreCase ("OFF"))
        return kFaderMinDb;
    return token.getFloatValue();
}

static float parseX32Pan (juce::String token) noexcept
{
    return std::clamp (token.trim().getFloatValue() / 100.0f, -1.0f, 1.0f);
}

static juce::Colour parseX32Color (juce::String token) noexcept
{
    token = token.trim().toUpperCase();
    if (token == "RD") return juce::Colour (0xffe53935);
    if (token == "GN") return juce::Colour (0xff43a047);
    if (token == "YE") return juce::Colour (0xffeed202);
    if (token == "BL") return juce::Colour (0xff1e88e5);
    if (token == "MG") return juce::Colour (0xffd81b60);
    if (token == "CY") return juce::Colour (0xff00acc1);
    if (token == "WH") return juce::Colour (0xffeceff1);
    return juce::Colour (0xff37474f);
}

static int parseX32Bitmask (juce::String token) noexcept
{
    token = token.trim();
    if (token.startsWithChar ('%'))
    {
        int mask = 0;
        for (int i = 1; i < token.length(); ++i)
        {
            const auto c = token[i];
            if (c == '0' || c == '1')
                mask = (mask << 1) | (c - '0');
        }
        return mask;
    }
    return token.getIntValue();
}

juce::Result ProjectModel::loadX32SceneText (const juce::String& scnText)
{
    if (scnText.isEmpty())
        return juce::Result::fail ("Empty X32 scene content");

    auto lines = juce::StringArray::fromLines (scnText);
    if (lines.isEmpty())
        return juce::Result::fail ("No lines in X32 scene");

    auto& params = engine.getMixerParams();

    for (const auto& rawLine : lines)
    {
        auto line = rawLine.trim();
        if (line.isEmpty() || line.startsWithChar ('#'))
            continue;

        auto tokens = juce::StringArray::fromTokens (line, " \t\r\n", "\"");
        if (tokens.isEmpty())
            continue;

        const auto addr = tokens[0];

        // 1. Channel controls: /ch/01../ch/32
        if (addr.startsWith ("/ch/"))
        {
            const int chNum = addr.substring (4, 6).getIntValue();
            const int ch = chNum - 1; // 0..31
            if (ch < 0 || ch >= kMaxInputChannels)
                continue;

            auto& cp = params.channels[(size_t) ch];
            const auto sub = addr.substring (6);

            if (sub == "/config" && tokens.size() >= 2)
            {
                setChannelName (ch, tokens[1]);
                if (tokens.size() > 3)
                    setChannelColour (ch, parseX32Color (tokens[3]));
                if (tokens.size() > 4)
                {
                    const int src = tokens[4].getIntValue();
                    cp.source.set ((float) (src > 0 ? src - 1 : -1));
                }
            }
            else if (sub == "/delay" && tokens.size() >= 3)
            {
                cp.dsp.delayBypass.setBool (tokens[1] == "OFF");
                cp.dsp.delayMs.set (tokens[2].getFloatValue());
            }
            else if (sub == "/preamp" && tokens.size() >= 4)
            {
                cp.trimDb.set (tokens[1].getFloatValue());
                cp.polarity.setBool (tokens[2] == "ON");
                cp.dsp.hpfBypass.setBool (tokens[3] == "OFF");
                if (tokens.size() >= 6)
                    cp.dsp.hpfFreq.set (parseX32Freq (tokens[5]));
            }
            else if (sub == "/gate" && tokens.size() >= 8)
            {
                cp.dsp.gateBypass.setBool (tokens[1] == "OFF");
                cp.dsp.gateThreshDb.set (tokens[3].getFloatValue());
                cp.dsp.gateRangeDb.set (tokens[4].getFloatValue());
                cp.dsp.gateAttackMs.set (tokens[5].getFloatValue());
                cp.dsp.gateHoldMs.set (tokens[6].getFloatValue());
                cp.dsp.gateReleaseMs.set (tokens[7].getFloatValue());
            }
            else if (sub == "/dyn" && tokens.size() >= 11)
            {
                cp.dsp.compBypass.setBool (tokens[1] == "OFF");
                cp.dsp.compThreshDb.set (tokens[5].getFloatValue());
                cp.dsp.compRatio.set (tokens[6].getFloatValue());
                cp.dsp.compKneeDb.set (tokens[7].getFloatValue());
                cp.dsp.compAttackMs.set (tokens[8].getFloatValue());
                cp.dsp.compReleaseMs.set (tokens[10].getFloatValue());
            }
            else if (sub == "/eq" && tokens.size() >= 2)
            {
                cp.dsp.eqBypass.setBool (tokens[1] == "OFF");
            }
            else if (sub == "/eq/1" && tokens.size() >= 5)
            {
                cp.dsp.eqLowType.set (tokens[1] == "LShv" ? 0.0f : 1.0f);
                cp.dsp.eqLowFreq.set (parseX32Freq (tokens[2]));
                cp.dsp.eqLowGainDb.set (tokens[3].getFloatValue());
                cp.dsp.eqLowQ.set (tokens[4].getFloatValue());
            }
            else if (sub == "/eq/2" && tokens.size() >= 5)
            {
                cp.dsp.eqMid1Freq.set (parseX32Freq (tokens[2]));
                cp.dsp.eqMid1GainDb.set (tokens[3].getFloatValue());
                cp.dsp.eqMid1Q.set (tokens[4].getFloatValue());
            }
            else if (sub == "/eq/3" && tokens.size() >= 5)
            {
                cp.dsp.eqMid2Freq.set (parseX32Freq (tokens[2]));
                cp.dsp.eqMid2GainDb.set (tokens[3].getFloatValue());
                cp.dsp.eqMid2Q.set (tokens[4].getFloatValue());
            }
            else if (sub == "/eq/4" && tokens.size() >= 5)
            {
                cp.dsp.eqHighType.set (tokens[1] == "HShv" ? 2.0f : 1.0f);
                cp.dsp.eqHighFreq.set (parseX32Freq (tokens[2]));
                cp.dsp.eqHighGainDb.set (tokens[3].getFloatValue());
                cp.dsp.eqHighQ.set (tokens[4].getFloatValue());
            }
            else if (sub == "/mix" && tokens.size() >= 3)
            {
                // In X32 OSC: ON = unmuted, OFF = muted
                cp.mute.setBool (tokens[1] == "OFF");
                cp.faderDb.set (parseX32Db (tokens[2]));
                if (tokens.size() > 3)
                    cp.solo.setBool (tokens[3] == "ON");
                if (tokens.size() > 4)
                    cp.pan.set (parseX32Pan (tokens[4]));
                if (tokens.size() > 5)
                    cp.mainAssign.setBool (tokens[5] == "ON");
            }
            else if (sub == "/grp" && tokens.size() >= 2)
            {
                cp.dcaGroups.set ((float) parseX32Bitmask (tokens[1]));
                if (tokens.size() >= 3)
                    cp.muteGroups.set ((float) parseX32Bitmask (tokens[2]));
            }
            else if (sub.startsWith ("/mix/") && tokens.size() >= 3)
            {
                const int busIdx = sub.substring (5).getIntValue() - 1;
                if (busIdx >= 0 && busIdx < 16)
                {
                    // tokens[1] is ON or OFF (OFF = muted)
                    if (tokens[1] == "OFF")
                        cp.sendMutes[(size_t) busIdx].setBool (true);
                    else if (tokens[1] == "ON")
                        cp.sendMutes[(size_t) busIdx].setBool (false);

                    cp.sends[(size_t) busIdx].set (parseX32Db (tokens[2]));

                    // Check for tap point token
                    for (int k = 3; k < tokens.size(); ++k)
                    {
                        const auto tok = tokens[k].trim();
                        int tapIdx = -1;
                        if (tok == "IN/LC") tapIdx = 0;
                        else if (tok == "1EQ" || tok == "PRE-EQ") tapIdx = 1;
                        else if (tok == "EQ->" || tok == "POST-EQ") tapIdx = 2;
                        else if (tok == "PRE") tapIdx = 3;
                        else if (tok == "POST") tapIdx = 4;
                        else if (tok == "GRP") tapIdx = 5;

                        if (tapIdx >= 0)
                        {
                            const int pairIdx = busIdx / 2;
                            if (pairIdx >= 0 && pairIdx < 8)
                                cp.sendTaps[(size_t) pairIdx].set ((float) tapIdx);
                            break;
                        }
                    }
                }
            }
        }
        // 2. Main LR Output: /main/st
        else if (addr.startsWith ("/main/st/mix") && tokens.size() >= 3)
        {
            params.main.mute.setBool (tokens[1] == "OFF");
            params.main.faderDb.set (parseX32Db (tokens[2]));
        }
        // 3. Mix Buses: /bus/01..16
        else if (addr.startsWith ("/bus/"))
        {
            const int busNum = addr.substring (5, 7).getIntValue();
            const int b = busNum - 1;
            if (b >= 0 && b < 16)
            {
                if (addr.endsWith ("/mix") && tokens.size() >= 3)
                {
                    params.buses[(size_t) b].mute.setBool (tokens[1] == "OFF");
                    params.buses[(size_t) b].faderDb.set (parseX32Db (tokens[2]));
                }
            }
        }
        // 4. FX Returns: /fxrtn/01..08
        else if (addr.startsWith ("/fxrtn/"))
        {
            const int rtnNum = addr.substring (7, 9).getIntValue();
            const int r = rtnNum - 1;
            if (r >= 0 && r < 8)
            {
                if (addr.endsWith ("/mix") && tokens.size() >= 3)
                {
                    params.fxReturns[(size_t) r].mute.setBool (tokens[1] == "OFF");
                    params.fxReturns[(size_t) r].faderDb.set (parseX32Db (tokens[2]));
                    if (tokens.size() > 4)
                        params.fxReturns[(size_t) r].pan.set (parseX32Pan (tokens[4]));
                }
            }
        }
        // 5. FX Engines: /fx/1..8
        else if (addr.startsWith ("/fx/") && addr.length() == 5 && tokens.size() >= 2)
        {
            const int slot = addr.substring (4, 5).getIntValue() - 1;
            if (slot >= 0 && slot < 8)
            {
                const auto fxTypeStr = tokens[1].toUpperCase();
                int matchedType = -1;
                for (int t = 0; t < getNumFxTypes(); ++t)
                {
                    const auto& meta = getFxMetadata ((FxType) t);
                    if (fxTypeStr == juce::String (meta.shortName).toUpperCase()
                        || fxTypeStr == juce::String (meta.name).toUpperCase())
                    {
                        matchedType = t;
                        break;
                    }
                }

                // Common legacy X32 type tags that are shorter than the app
                // picker labels.
                if (matchedType < 0)
                {
                    if (fxTypeStr == "PLAT") matchedType = (int) FxType::PlateReverb;
                    else if (fxTypeStr == "VRM") matchedType = (int) FxType::VintageRoomReverb;
                    else if (fxTypeStr == "DLY") matchedType = (int) FxType::StereoDelay;
                    else if (fxTypeStr == "CHO") matchedType = (int) FxType::StereoChorus;
                    else if (fxTypeStr == "GEQ") matchedType = (int) FxType::StereoGEQ;
                    else if (fxTypeStr == "LIM") matchedType = (int) FxType::StereoPrecisionLimiter;
                }
                if (matchedType >= 0)
                    params.fxSlots[(size_t) slot].setDefaultsForType ((FxType) matchedType);
            }
        }
        // 6. DCAs: /dca/1..8
        else if (addr.startsWith ("/dca/") && addr.length() == 6 && tokens.size() >= 3)
        {
            const int d = addr.substring (5, 6).getIntValue() - 1;
            if (d >= 0 && d < 8)
            {
                params.dcas[(size_t) d].mute.setBool (tokens[1] == "OFF");
                params.dcas[(size_t) d].faderDb.set (parseX32Db (tokens[2]));
            }
        }
    }

    modified = true;
    if (onTracksChanged)
        onTracksChanged();

    return juce::Result::ok();
}

juce::Result ProjectModel::loadX32SceneFile (const juce::File& file)
{
    if (! file.existsAsFile())
        return juce::Result::fail ("X32 scene file does not exist");

    return loadX32SceneText (file.loadFileAsString());
}

juce::Result ProjectModel::loadSceneFromFile (const juce::File& file)
{
    if (! file.existsAsFile())
        return juce::Result::fail ("Scene file does not exist");

    const auto firstLine = file.loadFileAsString().upToFirstOccurrenceOf ("\n", false, false).trim();
    if (file.hasFileExtension (".scn") || firstLine.startsWithChar ('#') || firstLine.startsWithChar ('/'))
    {
        return loadX32SceneFile (file);
    }

    auto jsonVar = juce::JSON::parse (file);
    if (! jsonVar.isObject())
        return juce::Result::fail ("Invalid scene file (not JSON or .scn)");

    recallSceneSnapshot (jsonVar);
    return juce::Result::ok();
}

} // namespace mps
