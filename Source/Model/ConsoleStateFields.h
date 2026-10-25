// Complete console-state traversal shared by project/scene serialization.
#pragma once
#include "../Core/MixerParams.h"
#include <juce_core/juce_core.h>
namespace mps {
template<class F> void visitConsoleState (MixerParams& mixer, F&& field)
{
    auto dsp = [&] (const juce::String& prefix, ChannelDspParams& p) {
        field (prefix + ".delayMs", p.delayMs);
        field (prefix + ".delayBypass", p.delayBypass);
        field (prefix + ".hpfFreq", p.hpfFreq);
        field (prefix + ".hpfBypass", p.hpfBypass);
        field (prefix + ".gateBypass", p.gateBypass);
        field (prefix + ".gateThreshDb", p.gateThreshDb);
        field (prefix + ".gateRangeDb", p.gateRangeDb);
        field (prefix + ".gateAttackMs", p.gateAttackMs);
        field (prefix + ".gateHoldMs", p.gateHoldMs);
        field (prefix + ".gateReleaseMs", p.gateReleaseMs);
        field (prefix + ".eqBypass", p.eqBypass);
        field (prefix + ".eqLowType", p.eqLowType);
        field (prefix + ".eqLowFreq", p.eqLowFreq);
        field (prefix + ".eqLowGainDb", p.eqLowGainDb);
        field (prefix + ".eqLowQ", p.eqLowQ);
        field (prefix + ".eqMid1Freq", p.eqMid1Freq);
        field (prefix + ".eqMid1GainDb", p.eqMid1GainDb);
        field (prefix + ".eqMid1Q", p.eqMid1Q);
        field (prefix + ".eqMid2Freq", p.eqMid2Freq);
        field (prefix + ".eqMid2GainDb", p.eqMid2GainDb);
        field (prefix + ".eqMid2Q", p.eqMid2Q);
        field (prefix + ".eqHighType", p.eqHighType);
        field (prefix + ".eqHighFreq", p.eqHighFreq);
        field (prefix + ".eqHighGainDb", p.eqHighGainDb);
        field (prefix + ".eqHighQ", p.eqHighQ);
        field (prefix + ".compBypass", p.compBypass);
        field (prefix + ".compThreshDb", p.compThreshDb);
        field (prefix + ".compRatio", p.compRatio);
        field (prefix + ".compKneeDb", p.compKneeDb);
        field (prefix + ".compAttackMs", p.compAttackMs);
        field (prefix + ".compReleaseMs", p.compReleaseMs);
        field (prefix + ".compMakeupDb", p.compMakeupDb);
        field (prefix + ".compMix", p.compMix);
    };
    for (int i = 0; i < 32; ++i) {
        auto& p = mixer.channels[(size_t) i];
        const auto prefix = juce::String ("channels.") + juce::String (i);
        field (prefix + ".source", p.source);
        field (prefix + ".sourceMode", p.sourceMode);
        field (prefix + ".automixGroup", p.automixGroup);
        field (prefix + ".automixWeightDb", p.automixWeightDb);
        field (prefix + ".trimDb", p.trimDb);
        field (prefix + ".polarity", p.polarity);
        field (prefix + ".faderDb", p.faderDb);
        field (prefix + ".pan", p.pan);
        field (prefix + ".mute", p.mute);
        field (prefix + ".solo", p.solo);
        field (prefix + ".mainAssign", p.mainAssign);
        field (prefix + ".monoAssign", p.monoAssign);
        field (prefix + ".phantom48v", p.phantom48v);

        field (prefix + ".insertFxSlot", p.insertFxSlot);
        field (prefix + ".muteGroups", p.muteGroups);
        field (prefix + ".dcaGroups", p.dcaGroups);
        for (int j = 0; j < 16; ++j) field (prefix + ".sends." + juce::String (j), p.sends[(size_t) j]);
        for (int j = 0; j < 16; ++j) field (prefix + ".sendMutes." + juce::String (j), p.sendMutes[(size_t) j]);
        for (int j = 0; j < 8; ++j) field (prefix + ".sendTaps." + juce::String (j), p.sendTaps[(size_t) j]);
        dsp (prefix + ".dsp", p.dsp);
    }
    for (int i = 0; i < 16; ++i) {
        auto& p = mixer.buses[(size_t) i];
        const auto prefix = juce::String ("buses.") + juce::String (i);
        field (prefix + ".faderDb", p.faderDb);
        field (prefix + ".mute", p.mute);
        field (prefix + ".pan", p.pan);
        field (prefix + ".solo", p.solo);
        field (prefix + ".mainAssign", p.mainAssign);
        field (prefix + ".monoAssign", p.monoAssign);
        field (prefix + ".insertFxSlot", p.insertFxSlot);
        field (prefix + ".dcaGroups", p.dcaGroups);
        field (prefix + ".muteGroups", p.muteGroups);
        for (int j = 0; j < 6; ++j) field (prefix + ".matrixSends." + juce::String (j), p.matrixSends[(size_t) j]);
        dsp (prefix + ".dsp", p.dsp);
    }
    for (int i = 0; i < 8; ++i) {
        auto& p = mixer.auxes[(size_t) i];
        const auto prefix = juce::String ("auxes.") + juce::String (i);
        field (prefix + ".faderDb", p.faderDb);
        field (prefix + ".mute", p.mute);
        field (prefix + ".pan", p.pan);
        field (prefix + ".solo", p.solo);
        field (prefix + ".trimDb", p.trimDb);
        field (prefix + ".polarity", p.polarity);
        field (prefix + ".phantom48v", p.phantom48v);
        field (prefix + ".source", p.source);
        field (prefix + ".mainAssign", p.mainAssign);
        field (prefix + ".monoAssign", p.monoAssign);
        field (prefix + ".dcaGroups", p.dcaGroups);
        field (prefix + ".muteGroups", p.muteGroups);
        dsp (prefix + ".dsp", p.dsp);
    }
    for (int i = 0; i < 6; ++i) {
        auto& p = mixer.matrices[(size_t) i];
        const auto prefix = juce::String ("matrices.") + juce::String (i);
        field (prefix + ".faderDb", p.faderDb);
        field (prefix + ".mute", p.mute);
        field (prefix + ".pan", p.pan);
        field (prefix + ".solo", p.solo);
        field (prefix + ".polarity", p.polarity);
        field (prefix + ".insertFxSlot", p.insertFxSlot);
        dsp (prefix + ".dsp", p.dsp);
    }
    for (int i = 0; i < 8; ++i) {
        auto& p = mixer.fxReturns[(size_t) i];
        const auto prefix = juce::String ("fxReturns.") + juce::String (i);
        field (prefix + ".faderDb", p.faderDb);
        field (prefix + ".pan", p.pan);
        field (prefix + ".mute", p.mute);
        field (prefix + ".solo", p.solo);
        field (prefix + ".mainAssign", p.mainAssign);
        field (prefix + ".monoAssign", p.monoAssign);
        field (prefix + ".dcaGroups", p.dcaGroups);
        field (prefix + ".muteGroups", p.muteGroups);
        dsp (prefix + ".dsp", p.dsp);
    }
    for (int i = 0; i < 8; ++i) {
        auto& p = mixer.dcas[(size_t) i];
        const auto prefix = juce::String ("dcas.") + juce::String (i);
        field (prefix + ".faderDb", p.faderDb);
        field (prefix + ".mute", p.mute);
    }
    for (int i = 0; i < 1; ++i) {
        auto& p = mixer.main;
        const auto prefix = juce::String ("main.") + juce::String (i);
        field (prefix + ".faderDb", p.faderDb);
        field (prefix + ".mute", p.mute);
        field (prefix + ".insertFxSlot", p.insertFxSlot);
        for (int j = 0; j < 6; ++j) field (prefix + ".matrixSends." + juce::String (j), p.matrixSends[(size_t) j]);
    }
    field ("monitorMode", mixer.monitorMode);
    field ("monitorLevelDb", mixer.monitorLevelDb);
    field ("monitorDim", mixer.monitorDim);
    field ("monitorDimDb", mixer.monitorDimDb);
    field ("monitorMute", mixer.monitorMute);
    field ("monitorMono", mixer.monitorMono);
    field ("outputSource", mixer.outputSource);
    field ("monoFaderDb", mixer.monoFaderDb); field ("monoMute", mixer.monoMute);
    for (int i = 0; i < 2; ++i) field("automixEnabled." + juce::String(i), mixer.automixEnabled[(size_t)i]);
    for (int i = 0; i < 8; ++i)
        for (int side = 0; side < 2; ++side)
            field ("fxSlots." + juce::String(i) + ".insertTargets." + juce::String(side),
                   mixer.fxSlots[(size_t)i].insertTargets[(size_t)side]);
}
} // namespace mps
