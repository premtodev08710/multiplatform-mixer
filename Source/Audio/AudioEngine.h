// Mix Practice Studio - Audio Device Manager and Realtime Engine Host
#pragma once

#include "../Core/MixEngine.h"
#include "../Core/MixerParams.h"
#include "../Core/TimelineStreamer.h"
#include "AudioFileTrack.h"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

namespace mps
{

class AudioEngine : public juce::AudioIODeviceCallback
{
public:
    AudioEngine();
    ~AudioEngine() override;

    void initDevice();
    void closeDevice();

    void saveAudioDeviceSettings();
    void loadAudioDeviceSettings();

    juce::AudioDeviceManager& getDeviceManager() noexcept { return deviceManager; }
    juce::AudioFormatManager& getFormatManager() noexcept { return formatManager; }
    MixerParams& getMixerParams() noexcept { return params; }
    MixerMeters& getMixerMeters() noexcept { return meters; }
    TimelineStreamer& getStreamer() noexcept { return streamer; }
    MixEngine& getMixEngine() noexcept { return mixEngine; }

    double getSampleRate() const noexcept { return currentSampleRate.load(); }
    int getBufferSize() const noexcept { return currentBlockSize.load(); }

    // Multitrack slot management
    void setTrackSlot (int slot, std::shared_ptr<TrackReader> reader);
    std::shared_ptr<TrackReader> getTrackSlot (int slot) const;
    void swapTracks (int slotA, int slotB);
    void moveTrack (int fromSlot, int toSlot);
    void clearAllTracks();

    // Transport helper functions
    void play();
    void pause();
    void stop();
    void seekToSample (int64_t sample);
    void seekToNormalized (float norm01);
    void setLoop (bool enabled, int64_t startSample, int64_t endSample);

    // Audio device callback
    void audioDeviceAboutToStart (juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const juce::AudioIODeviceCallbackContext& context) override;

private:
    void diskThreadLoop();

    juce::AudioDeviceManager deviceManager;
    juce::AudioFormatManager formatManager;

    MixerParams params;
    MixerMeters meters;
    TimelineStreamer streamer;
    MixEngine mixEngine;

    std::shared_ptr<TrackSet> currentTracks;
    std::mutex tracksMutex;

    std::atomic<double> currentSampleRate { 48000.0 };
    std::atomic<int> currentBlockSize { 512 };

    // Disk streaming thread
    std::thread diskThread;
    std::atomic<bool> diskThreadRunning { false };

    // Audio thread pre-allocated scratch buffers
    std::vector<std::vector<float>> slotBuffers;
    std::vector<float*> slotPointers;
    std::vector<const float*> slotConstPointers;
    std::vector<int> slotChannels;
    std::vector<float> engineOutL;
    std::vector<float> engineOutR;
};

} // namespace mps
