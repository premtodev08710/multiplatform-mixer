#include "AudioEngine.h"

namespace mps
{

AudioEngine::AudioEngine()
    : mixEngine (params, meters)
{
    formatManager.registerBasicFormats();

    currentTracks = std::make_shared<TrackSet>();

    // Start background disk streaming thread
    diskThreadRunning = true;
    diskThread = std::thread ([this] { diskThreadLoop(); });
}

AudioEngine::~AudioEngine()
{
    closeDevice();

    diskThreadRunning = false;
    if (diskThread.joinable())
        diskThread.join();
}

void AudioEngine::initDevice()
{
    // Initialize audio device with stereo output, no inputs needed
    deviceManager.initialiseWithDefaultDevices (0, 2);
    deviceManager.addAudioCallback (this);
}

void AudioEngine::closeDevice()
{
    deviceManager.removeAudioCallback (this);
    deviceManager.closeAudioDevice();
}

void AudioEngine::setTrackSlot (int slot, std::shared_ptr<TrackReader> reader)
{
    if (slot < 0 || slot >= kMaxTracks) return;

    std::shared_ptr<TrackSet> newTracks;
    {
        std::lock_guard<std::mutex> lock (tracksMutex);
        newTracks = std::make_shared<TrackSet> (*currentTracks);
        newTracks->slots[(size_t) slot] = std::move (reader);
        currentTracks = newTracks;
    }

    streamer.setTrackSet (newTracks);
}

std::shared_ptr<TrackReader> AudioEngine::getTrackSlot (int slot) const
{
    if (slot < 0 || slot >= kMaxTracks) return nullptr;
    std::lock_guard<std::mutex> lock (const_cast<std::mutex&> (tracksMutex));
    return currentTracks->slots[(size_t) slot];
}

void AudioEngine::swapTracks (int slotA, int slotB)
{
    if (slotA < 0 || slotA >= kMaxTracks || slotB < 0 || slotB >= kMaxTracks || slotA == slotB)
        return;

    std::shared_ptr<TrackSet> newTracks;
    {
        std::lock_guard<std::mutex> lock (tracksMutex);
        newTracks = std::make_shared<TrackSet> (*currentTracks);
        std::swap (newTracks->slots[(size_t) slotA], newTracks->slots[(size_t) slotB]);
        currentTracks = newTracks;
    }

    streamer.setTrackSet (newTracks);
}

void AudioEngine::moveTrack (int fromSlot, int toSlot)
{
    if (fromSlot < 0 || fromSlot >= kMaxTracks || toSlot < 0 || toSlot >= kMaxTracks || fromSlot == toSlot)
        return;

    std::shared_ptr<TrackSet> newTracks;
    {
        std::lock_guard<std::mutex> lock (tracksMutex);
        newTracks = std::make_shared<TrackSet> (*currentTracks);
        auto item = newTracks->slots[(size_t) fromSlot];
        if (fromSlot < toSlot)
        {
            for (int i = fromSlot; i < toSlot; ++i)
                newTracks->slots[(size_t) i] = newTracks->slots[(size_t) (i + 1)];
        }
        else
        {
            for (int i = fromSlot; i > toSlot; --i)
                newTracks->slots[(size_t) i] = newTracks->slots[(size_t) (i - 1)];
        }
        newTracks->slots[(size_t) toSlot] = item;
        currentTracks = newTracks;
    }

    streamer.setTrackSet (newTracks);
}

void AudioEngine::clearAllTracks()
{
    auto empty = std::make_shared<TrackSet>();
    {
        std::lock_guard<std::mutex> lock (tracksMutex);
        currentTracks = empty;
    }
    streamer.setTrackSet (empty);
}

void AudioEngine::play()
{
    streamer.setPlaying (true);
}

void AudioEngine::pause()
{
    streamer.setPlaying (false);
}

void AudioEngine::stop()
{
    streamer.setPlaying (false);
    streamer.seek (0);
}

void AudioEngine::seekToSample (int64_t sample)
{
    streamer.seek (sample);
}

void AudioEngine::seekToNormalized (float norm01)
{
    const int64_t len = streamer.getLength();
    if (len > 0)
    {
        const int64_t target = (int64_t) std::lround (clampf (norm01, 0.0f, 1.0f) * (float) len);
        streamer.seek (target);
    }
}

void AudioEngine::setLoop (bool enabled, int64_t startSample, int64_t endSample)
{
    streamer.setLoop (enabled, startSample, endSample);
}

void AudioEngine::diskThreadLoop()
{
    while (diskThreadRunning)
    {
        const bool didWork = streamer.service();
        if (! didWork)
        {
            // Nothing to buffer right now, sleep lightly
            std::this_thread::sleep_for (std::chrono::milliseconds (5));
        }
    }
}

void AudioEngine::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    if (device == nullptr) return;

    const double sr = device->getCurrentSampleRate();
    const int bs = std::max (64, device->getCurrentBufferSizeSamples());

    currentSampleRate.store (sr);
    currentBlockSize.store (bs);

    // Pre-allocate scratch buffers
    slotBuffers.assign ((size_t) kMaxTracks * 2, std::vector<float> ((size_t) bs, 0.0f));
    slotPointers.resize ((size_t) kMaxTracks * 2);
    slotConstPointers.resize ((size_t) kMaxTracks * 2);
    for (size_t i = 0; i < slotBuffers.size(); ++i)
    {
        slotPointers[i] = slotBuffers[i].data();
        slotConstPointers[i] = slotBuffers[i].data();
    }

    slotChannels.assign ((size_t) kMaxTracks, 0);
    engineOutL.assign ((size_t) bs, 0.0f);
    engineOutR.assign ((size_t) bs, 0.0f);

    mixEngine.prepare (sr, bs);
}

void AudioEngine::audioDeviceStopped()
{
}

void AudioEngine::audioDeviceIOCallbackWithContext (const float* const* /*inputChannelData*/,
                                                    int /*numInputChannels*/,
                                                    float* const* outputChannelData,
                                                    int numOutputChannels,
                                                    int numSamples,
                                                    const juce::AudioIODeviceCallbackContext& /*context*/)
{
    if (numSamples <= 0 || outputChannelData == nullptr || numOutputChannels <= 0)
        return;

    // Zero hardware outputs first in case of early return or extra channels
    for (int ch = 0; ch < numOutputChannels; ++ch)
        if (outputChannelData[ch] != nullptr)
            std::memset (outputChannelData[ch], 0, sizeof (float) * (size_t) numSamples);

    // Ensure scratch buffers are large enough for the requested callback block size
    if ((size_t) numSamples > engineOutL.size())
    {
        engineOutL.resize ((size_t) numSamples, 0.0f);
        engineOutR.resize ((size_t) numSamples, 0.0f);
    }

    // 1. Pull synchronized multitrack frame from the lock-free ring
    streamer.pull (slotPointers.data(), numSamples);
    streamer.getSlotChannelCounts (slotChannels.data());

    // 2. Mix and process DSP
    mixEngine.process (slotConstPointers.data(), slotChannels.data(), numSamples,
                       engineOutL.data(), engineOutR.data());

    // 3. Robust write to active hardware outputs (handles arbitrary channel mapping)
    float* outL = nullptr;
    float* outR = nullptr;
    for (int ch = 0; ch < numOutputChannels; ++ch)
    {
        if (outputChannelData[ch] != nullptr)
        {
            if (outL == nullptr)
                outL = outputChannelData[ch];
            else if (outR == nullptr)
            {
                outR = outputChannelData[ch];
                break;
            }
        }
    }

    if (outL != nullptr && outR != nullptr)
    {
        std::memcpy (outL, engineOutL.data(), sizeof (float) * (size_t) numSamples);
        std::memcpy (outR, engineOutR.data(), sizeof (float) * (size_t) numSamples);
    }
    else if (outL != nullptr)
    {
        for (int i = 0; i < numSamples; ++i)
            outL[i] = 0.5f * (engineOutL[(size_t) i] + engineOutR[(size_t) i]);
    }
}

} // namespace mps
