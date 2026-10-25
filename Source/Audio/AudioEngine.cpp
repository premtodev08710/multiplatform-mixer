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
    stopRecording();
    recordingThread.stopThread (5000);

    diskThreadRunning = false;
    if (diskThread.joinable())
        diskThread.join();
}

juce::Result AudioEngine::startRecording (const juce::File& file)
{
    if (recording.load()) return juce::Result::fail ("Recording is already active");
    if (file.exists()) return juce::Result::fail ("Choose a new file; existing recordings are preserved");
    auto stream = file.createOutputStream();
    if (!stream || !stream->openedOk()) return juce::Result::fail ("Cannot create WAV file");
    juce::WavAudioFormat wav;
    auto writer = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (stream.get(), getSampleRate(), 2, 24, {}, 0));
    if (!writer) return juce::Result::fail ("Cannot create WAV writer");
    stream.release();
    recordingThread.startThread();
    auto threaded = std::make_unique<juce::AudioFormatWriter::ThreadedWriter> (writer.release(), recordingThread, 32768);
    const juce::SpinLock::ScopedLockType lock (recordingLock);
    recordingSampleRate = getSampleRate();
    recordingOverruns.store (0);
    recordingWriter = std::move (threaded);
    recording.store (true);
    return juce::Result::ok();
}

void AudioEngine::stopRecording()
{
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> finished;
    {
        const juce::SpinLock::ScopedLockType lock (recordingLock);
        recording.store (false);
        finished = std::move (recordingWriter);
    }
    // Flush queued samples and close the WAV header off the audio thread.
    finished.reset();
}

void AudioEngine::initDevice()
{
    loadAudioDeviceSettings();
    deviceManager.addAudioCallback (this);
}

void AudioEngine::closeDevice()
{
    saveAudioDeviceSettings();
    deviceManager.removeAudioCallback (this);
    deviceManager.closeAudioDevice();
}

static juce::File getAudioSettingsFile()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("MixPracticeStudio");
    if (! dir.exists())
        dir.createDirectory();

    return dir.getChildFile ("audio_settings.xml");
}

void AudioEngine::saveAudioDeviceSettings()
{
    auto xml = deviceManager.createStateXml();
    if (xml != nullptr)
    {
        auto file = getAudioSettingsFile();
        xml->writeTo (file);
    }
}

void AudioEngine::loadAudioDeviceSettings()
{
    auto file = getAudioSettingsFile();
    std::unique_ptr<juce::XmlElement> xml;
    if (file.existsAsFile())
        xml = juce::XmlDocument::parse (file);

    juce::String err;
    if (xml != nullptr)
        err = deviceManager.initialise (0, 2, xml.get(), true);
    else
        err = deviceManager.initialiseWithDefaultDevices (0, 2);
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

    preparePlayback (device->getCurrentSampleRate(), device->getCurrentBufferSizeSamples());
}

void AudioEngine::preparePlayback (double sampleRate, int blockSize)
{
    const double sr = sampleRate > 0 ? sampleRate : 48000.0;
    const int bs = std::max (64, blockSize);
    currentSampleRate.store (sr);
    currentBlockSize.store (bs);

    // Pre-allocate scratch buffers with safe capacity (at least 4096 samples)
    // to prevent buffer overflows on Android where callback block sizes can exceed the device's reported buffer size.
    const int safeCapacity = std::max (4096, bs * 4);

    slotBuffers.assign ((size_t) kMaxTracks * 2, std::vector<float> ((size_t) safeCapacity, 0.0f));
    slotPointers.resize ((size_t) kMaxTracks * 2);
    slotConstPointers.resize ((size_t) kMaxTracks * 2);
    for (size_t i = 0; i < slotBuffers.size(); ++i)
    {
        slotPointers[i] = slotBuffers[i].data();
        slotConstPointers[i] = slotBuffers[i].data();
    }

    slotChannels.assign ((size_t) kMaxTracks, 0);
    engineOutL.assign ((size_t) safeCapacity, 0.0f);
    engineOutR.assign ((size_t) safeCapacity, 0.0f);

    mixEngine.prepare (sr, safeCapacity);
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

    juce::ScopedNoDenormals noDenormals;

    // Ensure scratch buffers are large enough for the requested callback block size
    if (slotBuffers.empty() || (size_t) numSamples > slotBuffers[0].size())
    {
        const size_t newCap = (size_t) std::max (4096, numSamples);
        for (auto& buf : slotBuffers)
            buf.resize (newCap, 0.0f);
        for (size_t i = 0; i < slotBuffers.size(); ++i)
        {
            slotPointers[i] = slotBuffers[i].data();
            slotConstPointers[i] = slotBuffers[i].data();
        }
        engineOutL.resize (newCap, 0.0f);
        engineOutR.resize (newCap, 0.0f);
        mixEngine.prepare (getSampleRate(), (int) newCap);
    }

    // 1. Pull synchronized multitrack frame from the lock-free ring
    streamer.pull (slotPointers.data(), numSamples);
    streamer.getSlotChannelCounts (slotChannels.data());

    // 2. Mix and process DSP
    mixEngine.process (slotConstPointers.data(), slotChannels.data(), numSamples,
                       engineOutL.data(), engineOutR.data());

    // Queue the monitored stereo output to disk without waiting on the audio thread.
    if (recording.load (std::memory_order_relaxed))
    {
        const juce::SpinLock::ScopedTryLockType lock (recordingLock);
        if (lock.isLocked() && recordingWriter)
        {
            const float* channels[] = { engineOutL.data(), engineOutR.data() };
            if (getSampleRate() != recordingSampleRate || !recordingWriter->write (channels, numSamples))
                recordingOverruns.fetch_add (1, std::memory_order_relaxed);
        }
        else recordingOverruns.fetch_add (1, std::memory_order_relaxed);
    }

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
