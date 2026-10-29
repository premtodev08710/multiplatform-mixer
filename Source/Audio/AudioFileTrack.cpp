#include "AudioFileTrack.h"

namespace mps
{

std::shared_ptr<AudioFileTrack> AudioFileTrack::open (const juce::File& file,
                                                     double targetSampleRate,
                                                     juce::AudioFormatManager& formatManager)
{
    if (! file.existsAsFile())
        return nullptr;

    auto r = std::unique_ptr<juce::AudioFormatReader> (formatManager.createReaderFor (file));
    if (r == nullptr)
        return nullptr;

    const double origSr = r->sampleRate;
    std::unique_ptr<juce::AudioFormatReader> readerToUse = std::move (r);

    // If sample rate matches closely, use directly
    if (std::abs (origSr - targetSampleRate) > 1.0 && targetSampleRate > 1000.0)
    {
        // Resample offline to a cache file at the target sample rate
        auto tempDir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                           .getChildFile ("MixPracticeStudioCache");
        tempDir.createDirectory();

        const juce::String fileHash = juce::String::toHexString (file.getFullPathName().hashCode64());
        auto cacheFile = tempDir.getChildFile (file.getFileNameWithoutExtension()
                                               + "_" + fileHash + "_cached_"
                                               + juce::String ((int) targetSampleRate) + ".wav");

        bool cacheValid = false;
        if (cacheFile.existsAsFile() && cacheFile.getLastModificationTime() >= file.getLastModificationTime())
        {
            auto cr = std::unique_ptr<juce::AudioFormatReader> (formatManager.createReaderFor (cacheFile));
            if (cr != nullptr && std::abs (cr->sampleRate - targetSampleRate) <= 1.0)
            {
                readerToUse = std::move (cr);
                cacheValid = true;
            }
        }

        if (! cacheValid)
        {
            // Convert using our SincResampler (12 zero-crossings for fast studio-quality conversion)
            SincResampler resampler (origSr, targetSampleRate, 12, 256);
            const int64_t inLen = readerToUse->lengthInSamples;
            const int64_t outLen = resampler.getOutputLength (inLen);
            const int numCh = std::min (2, (int) readerToUse->numChannels);

            cacheFile.deleteFile();
            std::unique_ptr<juce::OutputStream> outStream = cacheFile.createOutputStream();
            if (outStream != nullptr)
            {
                juce::WavAudioFormat wavFormat;
                juce::AudioFormatWriterOptions options;
                options = options.withSampleRate (targetSampleRate)
                                 .withNumChannels (numCh)
                                 .withBitsPerSample (32)
                                 .withSampleFormat (juce::AudioFormatWriterOptions::SampleFormat::floatingPoint);

                auto writer = wavFormat.createWriterFor (outStream, options);

                if (writer != nullptr)
                {
                    const int chunkOut = 4096;
                    std::vector<float> chBufL, chBufR;

                    for (int64_t outPos = 0; outPos < outLen; outPos += chunkOut)
                    {
                        const int nOut = (int) std::min<int64_t> (chunkOut, outLen - outPos);
                        int64_t inStart = 0, inCount = 0;
                        resampler.getInputRange (outPos, nOut, inStart, inCount);

                        juce::AudioBuffer<float> inReadBuf (numCh, (int) inCount);
                        inReadBuf.clear();

                        const int64_t validInStart = std::max<int64_t> (0, inStart);
                        const int64_t validInEnd = std::min<int64_t> (inLen, inStart + inCount);
                        const int numToRead = (int) std::max<int64_t> (0, validInEnd - validInStart);

                        if (numToRead > 0)
                        {
                            const int offsetInBuf = (int) (validInStart - inStart);
                            readerToUse->read (&inReadBuf, offsetInBuf, numToRead, validInStart, true, numCh >= 2);
                        }

                        juce::AudioBuffer<float> outBuf (numCh, nOut);
                        for (int ch = 0; ch < numCh; ++ch)
                        {
                            resampler.process (inReadBuf.getReadPointer (ch), inStart, inCount,
                                               outBuf.getWritePointer (ch), outPos, nOut);
                        }

                        writer->writeFromAudioSampleBuffer (outBuf, 0, nOut);
                    }
                    writer.reset(); // flushes and closes stream

                    auto cr = std::unique_ptr<juce::AudioFormatReader> (formatManager.createReaderFor (cacheFile));
                    if (cr != nullptr)
                    {
                        readerToUse = std::move (cr);
                    }
                }
            }
        }
    }

    return std::shared_ptr<AudioFileTrack> (
        new AudioFileTrack (file, file.getFileNameWithoutExtension(), std::move (readerToUse), origSr, targetSampleRate));
}

AudioFileTrack::AudioFileTrack (const juce::File& file, juce::String name,
                                std::unique_ptr<juce::AudioFormatReader> r,
                                double origSr, double targetSr)
    : sourceFile (file), trackName (std::move (name)), reader (std::move (r)),
      originalSampleRate (origSr > 0.0 ? origSr : targetSr), targetSampleRate (targetSr)
{
    if (reader != nullptr)
    {
        numChannels = std::min (2, (int) reader->numChannels);
        lengthInFrames = (int64_t) reader->lengthInSamples;
    }
}

bool AudioFileTrack::read (float* const* dest, int64_t startFrame, int numFrames)
{
    if (reader == nullptr || numFrames <= 0) return false;
    std::lock_guard<std::mutex> lock (readerMutex);

    if (startFrame < 0 || startFrame >= lengthInFrames)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            std::memset (dest[ch], 0, sizeof (float) * (size_t) numFrames);
        return true;
    }

    const int toRead = (int) std::min<int64_t> ((int64_t) numFrames, lengthInFrames - startFrame);
    bool ok = reader->read (dest, numChannels, (juce::int64) startFrame, toRead);

    if (toRead < numFrames)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            std::memset (dest[ch] + toRead, 0, sizeof (float) * (size_t) (numFrames - toRead));
    }

    return ok;
}

} // namespace mps
