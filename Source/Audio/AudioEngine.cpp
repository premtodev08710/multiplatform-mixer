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
    if (xml == nullptr)
        xml = std::make_unique<juce::XmlElement> ("DEVICESETUP");

    const double sr = currentSampleRate.load();
    if (sr > 0.0)
        xml->setAttribute ("deviceSampleRate", sr);

    auto file = getAudioSettingsFile();
    xml->writeTo (file);
}

void AudioEngine::loadAudioDeviceSettings()
{
    auto file = getAudioSettingsFile();
    std::unique_ptr<juce::XmlElement> xml;
    if (file.existsAsFile())
        xml = juce::XmlDocument::parse (file);

    double savedSr = 0.0;
    if (xml != nullptr && xml->hasAttribute ("deviceSampleRate"))
        savedSr = xml->getDoubleAttribute ("deviceSampleRate", 0.0);

    juce::String err;
    if (xml != nullptr)
        err = deviceManager.initialise (0, 2, xml.get(), true);
    else
        err = deviceManager.initialiseWithDefaultDevices (0, 2);

    if (auto* dev = deviceManager.getCurrentAudioDevice())
    {
        if (dev->getCurrentSampleRate() > 0.0)
            currentSampleRate.store (dev->getCurrentSampleRate());
    }
    else if (isSampleRateSupported (savedSr))
    {
        currentSampleRate.store (savedSr);
    }
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

void AudioEngine::setTrackSlots (const std::vector<std::pair<int, std::shared_ptr<TrackReader>>>& slots)
{
    if (slots.empty()) return;

    std::shared_ptr<TrackSet> newTracks;
    {
        std::lock_guard<std::mutex> lock (tracksMutex);
        newTracks = std::make_shared<TrackSet> (*currentTracks);
        for (const auto& item : slots)
        {
            if (item.first >= 0 && item.first < kMaxTracks)
                newTracks->slots[(size_t) item.first] = item.second;
        }
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

    reinitializeAudioForSampleRate (device->getCurrentSampleRate(), device->getCurrentBufferSizeSamples());
}

double AudioEngine::getDefaultSampleRate() const noexcept
{
    if (auto* dev = deviceManager.getCurrentAudioDevice())
    {
        const auto rates = dev->getAvailableSampleRates();
        for (double r : rates)
            if (std::abs (r - kDefaultSampleRate) <= 1.0)
                return r;

        const double cur = dev->getCurrentSampleRate();
        if (cur > 0.0)
            return cur;

        if (! rates.isEmpty() && rates[0] > 0.0)
            return rates[0];
    }

    const double current = currentSampleRate.load();
    return current > 0.0 ? current : kDefaultSampleRate;
}

juce::Array<double> AudioEngine::getAvailableSampleRates() const
{
    if (auto* dev = deviceManager.getCurrentAudioDevice())
    {
        auto rates = dev->getAvailableSampleRates();
        if (! rates.isEmpty())
            return rates;
    }

    return { 44100.0, 48000.0, 88200.0, 96000.0 };
}

bool AudioEngine::isSampleRateSupported (double sampleRate) const
{
    if (! std::isfinite (sampleRate) || sampleRate < 8000.0 || sampleRate > 384000.0)
        return false;

    const auto rates = getAvailableSampleRates();
    for (double r : rates)
    {
        if (std::abs (r - sampleRate) <= 1.0)
            return true;
    }

    return false;
}

void AudioEngine::reinitializeAudioForSampleRate (double newSampleRate, int blockSize)
{
    const double oldSr = currentSampleRate.load();
    const double validSr = (std::isfinite (newSampleRate) && newSampleRate > 0.0)
                           ? newSampleRate
                           : getDefaultSampleRate();
    const int validBs = std::max (64, blockSize > 0 ? blockSize : currentBlockSize.load());

    // Stop active recording if sample rate changes to avoid header/sample-rate mismatch
    if (recording.load() && std::abs (recordingSampleRate - validSr) > 1.0)
        stopRecording();

    const bool wasPlaying = streamer.isPlaying();
    const int64_t oldPlayhead = streamer.getPlayhead();
    const bool loopOn = streamer.getLoopEnabled();
    const int64_t oldLoopStart = streamer.getLoopStart();
    const int64_t oldLoopEnd = streamer.getLoopEnd();

    // Pause and flush old ring buffer frames before re-initializing context
    streamer.setPlaying (false);
    streamer.clearBuffer();

    // Re-allocate and clear audio scratch buffers, MixEngine DSP, and OutputProtection
    preparePlayback (validSr, validBs);

    // Check if any loaded AudioFileTrack needs resampling to the new sample rate
    std::shared_ptr<TrackSet> snapshot;
    {
        std::lock_guard<std::mutex> lock (tracksMutex);
        snapshot = currentTracks;
    }

    bool anyTrackNeedsReload = false;
    if (snapshot != nullptr)
    {
        for (int s = 0; s < kMaxTracks; ++s)
        {
            if (auto aft = std::dynamic_pointer_cast<AudioFileTrack> (snapshot->slots[(size_t) s]))
            {
                if (std::abs (aft->getTargetSampleRate() - validSr) > 1.0)
                {
                    anyTrackNeedsReload = true;
                    break;
                }
            }
        }
    }

    if (anyTrackNeedsReload && snapshot != nullptr)
    {
        auto reloadedTracks = std::make_shared<TrackSet>();
        for (int s = 0; s < kMaxTracks; ++s)
        {
            auto existing = snapshot->slots[(size_t) s];
            if (auto aft = std::dynamic_pointer_cast<AudioFileTrack> (existing))
            {
                if (std::abs (aft->getTargetSampleRate() - validSr) > 1.0 && aft->getFile().existsAsFile())
                {
                    auto reopened = AudioFileTrack::open (aft->getFile(), validSr, formatManager);
                    reloadedTracks->slots[(size_t) s] = reopened != nullptr ? std::move (reopened) : existing;
                }
                else
                {
                    reloadedTracks->slots[(size_t) s] = existing;
                }
            }
            else
            {
                reloadedTracks->slots[(size_t) s] = existing;
            }
        }

        {
            std::lock_guard<std::mutex> lock (tracksMutex);
            currentTracks = reloadedTracks;
        }
        streamer.setTrackSet (reloadedTracks);
    }

    // Rescale timeline playhead and loop boundaries proportionally to avoid time/position drift
    if (oldSr > 0.0 && std::abs (validSr - oldSr) > 1.0)
    {
        const double ratio = validSr / oldSr;
        const int64_t newPlayhead = std::max<int64_t> (0, (int64_t) std::llround ((double) oldPlayhead * ratio));
        const int64_t newLoopStart = std::max<int64_t> (0, (int64_t) std::llround ((double) oldLoopStart * ratio));
        const int64_t newLoopEnd = std::max<int64_t> (0, (int64_t) std::llround ((double) oldLoopEnd * ratio));

        streamer.setLoop (loopOn, newLoopStart, newLoopEnd);
        streamer.seek (newPlayhead);
    }

    if (wasPlaying)
        streamer.setPlaying (true);
}

double AudioEngine::setSampleRate (double requestedSampleRate)
{
    // 1. Validation: Verify requested sample rate is supported; fallback to device default if not
    double targetSr = getDefaultSampleRate();
    if (isSampleRateSupported (requestedSampleRate))
    {
        const auto rates = getAvailableSampleRates();
        for (double r : rates)
        {
            if (std::abs (r - requestedSampleRate) <= 1.0)
            {
                targetSr = r;
                break;
            }
        }
    }

    // 2. Re-initialization: Stop/clear old buffers & update hardware device and audio processors
    const bool wasPlaying = streamer.isPlaying();
    streamer.setPlaying (false);
    streamer.clearBuffer();

    if (auto* dev = deviceManager.getCurrentAudioDevice())
    {
        juce::AudioDeviceManager::AudioDeviceSetup setup;
        deviceManager.getAudioDeviceSetup (setup);

        if (std::abs (dev->getCurrentSampleRate() - targetSr) > 1.0
            || std::abs (setup.sampleRate - targetSr) > 1.0)
        {
            const double fallbackSr = dev->getCurrentSampleRate() > 0.0
                                      ? dev->getCurrentSampleRate()
                                      : getDefaultSampleRate();
            setup.sampleRate = targetSr;
            const juce::String err = deviceManager.setAudioDeviceSetup (setup, true);

            if (err.isNotEmpty())
            {
                // Hardware rejected the rate: fall back to safe default rate to prevent freeze
                setup.sampleRate = fallbackSr;
                deviceManager.setAudioDeviceSetup (setup, true);
                targetSr = fallbackSr;
            }
        }

        if (auto* activeDev = deviceManager.getCurrentAudioDevice())
        {
            if (activeDev->getCurrentSampleRate() > 0.0)
                targetSr = activeDev->getCurrentSampleRate();
        }
    }

    reinitializeAudioForSampleRate (targetSr, currentBlockSize.load());

    if (wasPlaying)
        streamer.setPlaying (true);

    // 3. State Sync: Persist and sync the applied sample rate across the system
    currentSampleRate.store (targetSr);
    saveAudioDeviceSettings();

    return targetSr;
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
    outputProtection.prepare (sr);
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

    // Split oversized device callbacks without allocating or resetting DSP state.
    if (engineOutL.empty()) return;
    float* outL = nullptr;
    float* outR = nullptr;
    for (int ch = 0; ch < numOutputChannels; ++ch)
        if (outputChannelData[ch] != nullptr)
        {
            if (outL == nullptr) outL = outputChannelData[ch];
            else { outR = outputChannelData[ch]; break; }
        }

    for (int offset = 0; offset < numSamples;)
    {
        const int blockFrames = std::min (numSamples - offset, (int) engineOutL.size());
        // 1. Pull synchronized multitrack frame from the lock-free ring
        streamer.pull (slotPointers.data(), blockFrames);
        streamer.getSlotChannelCounts (slotChannels.data());

        // 2. Mix and process DSP
        mixEngine.process (slotConstPointers.data(), slotChannels.data(), blockFrames,
                           engineOutL.data(), engineOutR.data());

        outputProtection.process (engineOutL.data(), engineOutR.data(), blockFrames);

        // Queue the monitored stereo output to disk without waiting on the audio thread.
        if (recording.load (std::memory_order_relaxed))
        {
            const juce::SpinLock::ScopedTryLockType lock (recordingLock);
            if (lock.isLocked() && recordingWriter)
            {
                const float* channels[] = { engineOutL.data(), engineOutR.data() };
                if (getSampleRate() != recordingSampleRate || !recordingWriter->write (channels, blockFrames))
                    recordingOverruns.fetch_add (1, std::memory_order_relaxed);
            }
            else recordingOverruns.fetch_add (1, std::memory_order_relaxed);
        }

        if (outL != nullptr && outR != nullptr)
        {
            std::memcpy (outL + offset, engineOutL.data(), sizeof (float) * (size_t) blockFrames);
            std::memcpy (outR + offset, engineOutR.data(), sizeof (float) * (size_t) blockFrames);
        }
        else if (outL != nullptr)
        {
            for (int i = 0; i < blockFrames; ++i)
                outL[offset + i] = 0.5f * (engineOutL[(size_t) i] + engineOutR[(size_t) i]);
        }
        offset += blockFrames;
    }
}

} // namespace mps
