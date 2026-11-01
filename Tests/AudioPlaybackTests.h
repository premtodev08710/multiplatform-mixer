#pragma once
#include "../Source/Audio/AudioEngine.h"
#include <cstdio>

// File decoder -> read-ahead thread -> device callback -> mixer -> output protection.
// This uses real WAV readers but no hardware device and cannot certify Android timing.
inline int runAudioPlaybackTests()
{
    using namespace mps;
    int failures = 0;
    const auto monoFile = juce::File::createTempFile (".wav");
    const auto stereoFile = juce::File::createTempFile (".wav");
    auto writeTone = [&] (const juce::File& file, double rate, int channels, double frequency, float amplitude)
    {
        file.deleteFile();
        auto stream = std::unique_ptr<juce::OutputStream> (file.createOutputStream());
        juce::WavAudioFormat wav;
        auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions()
            .withSampleRate (rate).withNumChannels (channels).withBitsPerSample (24));
        if (!writer) { ++failures; return; }
        juce::AudioBuffer<float> data (channels, (int) rate * 8);
        for (int c = 0; c < channels; ++c)
            for (int i = 0; i < data.getNumSamples(); ++i)
                data.setSample (c, i, amplitude * (c == 0 ? 1.0f : -0.5f)
                    * (float) std::sin (2.0 * juce::MathConstants<double>::pi * frequency * i / rate));
        if (!writer->writeFromAudioSampleBuffer (data, 0, data.getNumSamples())) ++failures;
    };
    writeTone (monoFile, 48000, 1, 173, 0.04f);
    writeTone (stereoFile, 44100, 2, 271, 0.03f);
    for (int block : { 64, 192, 256, 1024, 5000 })
    {
        AudioEngine host; host.preparePlayback (48000, 256);
        auto first = AudioFileTrack::open (monoFile, 48000, host.getFormatManager());
        auto second = AudioFileTrack::open (stereoFile, 48000, host.getFormatManager());
        if (!first || !second) { ++failures; continue; }
        host.getMixerParams().channels[0].source.set (0);
        host.getMixerParams().channels[1].source.set (1);
        host.setTrackSlot (0, first);
        for (int i = 0; i < 2000 && host.getStreamer().getBufferFill() < 0.9f; ++i)
            juce::Thread::sleep (1);
        host.play();
        std::vector<float> left ((size_t) block), right ((size_t) block);
        float* outputs[] = { left.data(), right.data() };
        juce::AudioIODeviceCallbackContext context;
        double maxError = 0;
        int verified = 0;
        // Warm up gains, add a resampled stereo track, then verify sustained output.
        for (int b = 0; b < 120; ++b)
        {
            if (b == 25) host.setTrackSlot (1, second);
            host.audioDeviceIOCallbackWithContext (nullptr, 0, outputs, 2, block, context);
            const auto end = host.getStreamer().getPlayhead();
            if (b >= 60 && end < 48000 * 8 - block && host.getStreamer().isPlaying())
                for (int i = 0; i < block; ++i)
                {
                    const double t = (end - block + i) / 48000.0;
                    const double a = 0.04 * std::sqrt (0.5) * std::sin (2 * juce::MathConstants<double>::pi * 173 * t);
                    const double bTone = 0.03 * std::sin (2 * juce::MathConstants<double>::pi * 271 * t);
                    maxError = std::max (maxError, std::abs (left[(size_t) i] - a - bTone));
                    maxError = std::max (maxError, std::abs (right[(size_t) i] - a + 0.5 * bTone));
                    ++verified;
                }
            juce::Thread::sleep (2);
        }
        if (verified == 0 || maxError > 0.0001 || host.getStreamer().getUnderrunCount() != 0
            || host.getOutputOverloadBlocks() != 0) ++failures;
        std::printf ("Audio callback block=%d verified=%d max_error=%.9f underruns=%llu overloads=%llu\n",
            block, verified, maxError, (unsigned long long) host.getStreamer().getUnderrunCount(),
            (unsigned long long) host.getOutputOverloadBlocks());
    }
    monoFile.deleteFile(); stereoFile.deleteFile();
    std::printf ("Audio file playback: %d failures\n", failures);
    return failures == 0 ? 0 : 1;
}
