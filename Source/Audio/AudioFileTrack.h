// Mix Practice Studio - Concrete TrackReader using JUCE AudioFormatReader
#pragma once

#include "../Core/TimelineStreamer.h"
#include "../Core/SincResampler.h"
#include <juce_audio_formats/juce_audio_formats.h>

namespace mps
{

class AudioFileTrack : public TrackReader
{
public:
    static std::shared_ptr<AudioFileTrack> open (const juce::File& file,
                                                 double targetSampleRate,
                                                 juce::AudioFormatManager& formatManager);

    ~AudioFileTrack() override = default;

    int getNumChannels() const override { return numChannels; }
    int64_t getLengthInFrames() const override { return lengthInFrames; }
    bool read (float* const* dest, int64_t startFrame, int numFrames) override;

    const juce::File& getFile() const noexcept { return sourceFile; }
    const juce::String& getName() const noexcept { return trackName; }
    double getOriginalSampleRate() const noexcept { return originalSampleRate; }
    double getTargetSampleRate() const noexcept { return targetSampleRate; }

private:
    AudioFileTrack (const juce::File& file, juce::String name,
                    std::unique_ptr<juce::AudioFormatReader> reader,
                    double targetSr);

    juce::File sourceFile;
    juce::String trackName;
    std::unique_ptr<juce::AudioFormatReader> reader;
    int numChannels = 1;
    int64_t lengthInFrames = 0;
    double originalSampleRate = 48000.0;
    double targetSampleRate = 48000.0;
    std::mutex readerMutex; // reader->read() called from disk/producer thread
};

} // namespace mps
