// Mix Practice Studio - Project and Scene Model (Versioned JSON)
#pragma once

#include "../Audio/AudioEngine.h"
#include <juce_core/juce_core.h>

namespace mps
{

struct ChannelMetadata
{
    juce::String name;
    juce::Colour colour = juce::Colour (0xff2196f3); // default X32 blue
    juce::String icon;
};

class ProjectModel
{
public:
    static constexpr int kProjectVersion = 1;

    ProjectModel (AudioEngine& engine);

    const juce::File& getProjectFile() const noexcept { return projectFile; }
    bool isModified() const noexcept { return modified; }
    void setModified (bool m = true) noexcept { modified = m; }

    // Channel metadata
    const ChannelMetadata& getChannelMetadata (int channel) const;
    void setChannelName (int channel, const juce::String& name);
    void setChannelColour (int channel, juce::Colour colour);

    // Track file paths
    juce::File getTrackFile (int slot) const;
    void setTrackFile (int slot, const juce::File& file);
    void swapTracks (int slotA, int slotB);
    void moveTrack (int fromSlot, int toSlot);

    std::function<void()> onTracksChanged;

    // Save & Load Project
    juce::Result saveProject (const juce::File& file, bool copyAudioIntoProject = false);
    juce::Result loadProject (const juce::File& file, juce::StringArray& outMissingFiles);

    // Scenes (Snapshots of mixer state)
    juce::var captureSceneSnapshot (const juce::String& sceneName) const;
    void recallSceneSnapshot (const juce::var& sceneData);

    juce::Result saveSceneToFile (const juce::File& file, const juce::String& sceneName) const;
    juce::Result loadSceneFromFile (const juce::File& file);

    // Default init (e.g. Kick, Snare, Bass, Guitar, Vocal default channel names and colors)
    void setupDefaults();

private:
    AudioEngine& engine;
    juce::File projectFile;
    bool modified = false;

    std::array<ChannelMetadata, kMaxInputChannels> channelMeta;
    std::array<juce::File, kMaxTracks> trackFiles;

    juce::var serializeMixerState() const;
    void deserializeMixerState (const juce::var& obj);
};

} // namespace mps
