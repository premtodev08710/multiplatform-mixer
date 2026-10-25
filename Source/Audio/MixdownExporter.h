#pragma once
#include "AudioEngine.h"
#include "AudioFileTrack.h"
#include "../Model/ConsoleStateFields.h"
#include <map>

namespace mps {
// Each export owns its mixer, file readers and DSP history. Playback is untouched.
class MixdownExporter final : public juce::Thread
{
public:
    static juce::File findMp3Encoder()
    {
       #if JUCE_MAC || JUCE_LINUX
        for (const auto* path : { "/opt/homebrew/bin/lame", "/usr/local/bin/lame", "/usr/bin/lame" })
            if (juce::File (path).existsAsFile()) return juce::File (path);
       #endif
        auto bundled = juce::File::getSpecialLocation (juce::File::currentExecutableFile).getSiblingFile ("lame.exe");
        return bundled.existsAsFile() ? bundled : juce::File();
    }

    MixdownExporter (AudioEngine& host, juce::File target, bool mp3, double tailSeconds)
        : Thread ("Offline mix export"), destination (std::move(target)), encodeMp3 (mp3),
          sampleRate (host.getSampleRate()), tail (juce::jlimit (0.0, 30.0, tailSeconds))
    {
        // Snapshot on the message thread. Monitor/Solo are audition controls, not the mix.
        auto& live = host.getMixerParams();
        std::map<juce::String, float> values;
        visitConsoleState (live, [&] (const juce::String& key, Param& value) { values[key] = value.get(); });
        visitConsoleState (params, [&] (const juce::String& key, Param& value) { value.set (values.at(key)); });
        for (int s = 0; s < 8; ++s)
        {
            auto& from = live.fxSlots[(size_t)s]; auto& to = params.fxSlots[(size_t)s];
            to.fxType.set (from.fxType.get()); to.bypass.set (from.bypass.get());
            to.insertMode.set (from.insertMode.get()); to.mix.set (from.mix.get());
            for (int p = 0; p < FxSlotParams::kNumParameters; ++p) to.parameter(p).set (from.parameter(p).get());
            for (int b = 0; b < 31; ++b) { to.geqGainsL[(size_t)b].set(from.geqGainsL[(size_t)b].get()); to.geqGainsR[(size_t)b].set(from.geqGainsR[(size_t)b].get()); }
        }
        for (int g = 0; g < 6; ++g) params.muteGroupActive[(size_t)g].store(live.muteGroupActive[(size_t)g].load());
        for (auto& p : params.channels) p.solo.setBool(false);
        for (auto& p : params.buses) p.solo.setBool(false);
        for (auto& p : params.auxes) p.solo.setBool(false);
        for (auto& p : params.fxReturns) p.solo.setBool(false);
        for (auto& p : params.matrices) p.solo.setBool(false);
        params.outputSource.set(0); params.monitorMode.set(0); params.monitorLevelDb.set(0);
        params.monitorMute.setBool(false); params.monitorDim.setBool(false); params.monitorMono.setBool(false);
        for (int i = 0; i < kMaxTracks; ++i)
            if (auto file = std::dynamic_pointer_cast<AudioFileTrack> (host.getTrackSlot(i))) files[(size_t)i] = file->getFile();
    }

    ~MixdownExporter() override { signalThreadShouldExit(); stopThread (-1); }
    bool isFinished() const noexcept { return finished.load (std::memory_order_acquire); }
    double getProgress() const noexcept { return progress.load(); }
    const juce::Result& getResult() const { jassert(isFinished()); return result; }
    bool clipped() const noexcept { return clipping.load(); }
    void run() override
    {
        result = render();
        finished.store (true, std::memory_order_release);
    }

private:
    juce::Result render()
    {
        if (destination.exists()) return juce::Result::fail ("Choose a new filename; existing files are preserved.");
        const auto encoder = findMp3Encoder();
        if (encodeMp3 && !encoder.existsAsFile()) return juce::Result::fail ("MP3 export requires an installed LAME encoder. Export WAV on this device.");
        juce::AudioFormatManager formats; formats.registerBasicFormats();
        std::array<std::shared_ptr<AudioFileTrack>, kMaxTracks> readers;
        std::array<int, kMaxTracks> widths {};
        int64_t length = 0;
        for (int s = 0; s < kMaxTracks; ++s)
        {
            if (threadShouldExit()) return juce::Result::fail ("Export cancelled.");
            if (files[(size_t)s] == juce::File()) continue;
            readers[(size_t)s] = AudioFileTrack::open (files[(size_t)s], sampleRate, formats);
            if (!readers[(size_t)s]) return juce::Result::fail ("Cannot read " + files[(size_t)s].getFileName());
            widths[(size_t)s] = readers[(size_t)s]->getNumChannels();
            length = juce::jmax (length, readers[(size_t)s]->getLengthInFrames());
        }
        if (length == 0) return juce::Result::fail ("Import audio tracks before exporting.");
        juce::TemporaryFile wavTemp (destination.getSiblingFile (destination.getFileName() + ".render.wav"));
        auto stream = std::unique_ptr<juce::OutputStream> (wavTemp.getFile().createOutputStream());
        if (!stream) return juce::Result::fail ("Cannot create export file.");
        juce::WavAudioFormat wav;
        auto options = juce::AudioFormatWriterOptions().withSampleRate(sampleRate).withNumChannels(2).withBitsPerSample(24);
        auto writer = wav.createWriterFor(stream, options);
        if (!writer) return juce::Result::fail ("Cannot create export file.");
        constexpr int block = 1024;
        std::array<std::vector<float>, kMaxTracks*2> buffers;
        std::array<const float*, kMaxTracks*2> pointers;
        for (size_t i = 0; i < buffers.size(); ++i) { buffers[i].resize(block); pointers[i] = buffers[i].data(); }
        std::array<float, block> left {}, right {};
        MixerMeters meters;
        MixEngine engine (params, meters); engine.prepare(sampleRate, block);
        // Open source/fader ramps on silence so sample zero retains its transient.
        for (int warm = 0; warm < 5; ++warm) engine.process(pointers.data(), widths.data(), block, left.data(), right.data());
        const int64_t total = length + (int64_t)std::llround (tail * sampleRate);
        for (int64_t frame = 0; frame < total; frame += block)
        {
            if (threadShouldExit()) return juce::Result::fail ("Export cancelled.");
            const int count = (int)juce::jmin<int64_t>(block, total-frame);
            for (auto& b : buffers) std::fill (b.begin(), b.end(), 0.0f);
            for (int s = 0; s < kMaxTracks; ++s)
                if (auto& reader = readers[(size_t)s])
                {
                    const int available = (int)juce::jmax<int64_t>(0, juce::jmin<int64_t>(count, reader->getLengthInFrames()-frame));
                    float* dest[] = { buffers[(size_t)s*2].data(), buffers[(size_t)s*2+1].data() };
                    if (available > 0 && !reader->read(dest, frame, available)) return juce::Result::fail ("Audio read failed during export.");
                }
            engine.process(pointers.data(), widths.data(), count, left.data(), right.data());
            for (int i = 0; i < count; ++i)
                if (!std::isfinite(left[(size_t)i]) || !std::isfinite(right[(size_t)i])) return juce::Result::fail ("Non-finite audio detected; check DSP settings.");
                else if (std::abs(left[(size_t)i]) >= 1 || std::abs(right[(size_t)i]) >= 1) clipping.store(true);
            const float* stereo[] = { left.data(), right.data() };
            if (!writer->writeFromFloatArrays(stereo, 2, count)) return juce::Result::fail ("Could not write export; check free disk space.");
            progress.store ((double)(frame+count)/(double)total * (encodeMp3 ? 0.9 : 1.0));
        }
        writer.reset();
        if (threadShouldExit()) return juce::Result::fail ("Export cancelled.");
        if (encodeMp3)
        {
            juce::TemporaryFile mp3Temp (destination);
            juce::ChildProcess process;
            if (!process.start(juce::StringArray { encoder.getFullPathName(), "--silent", "-b", "320", wavTemp.getFile().getFullPathName(), mp3Temp.getFile().getFullPathName() }))
                return juce::Result::fail ("Could not start MP3 encoder.");
            while (!process.waitForProcessToFinish(100))
                if (threadShouldExit()) { process.kill(); return juce::Result::fail("Export cancelled."); }
            if (process.getExitCode() != 0 || mp3Temp.getFile().getSize() == 0) return juce::Result::fail("MP3 encoding failed.");
            if (threadShouldExit()) return juce::Result::fail ("Export cancelled.");
            if (destination.exists() || !mp3Temp.getFile().moveFileTo(destination)) return juce::Result::fail("Cannot save MP3 destination.");
        }
        else if (destination.exists() || !wavTemp.getFile().moveFileTo(destination)) return juce::Result::fail("Cannot save WAV destination.");
        progress.store(1.0);
        return juce::Result::ok();
    }
    MixerParams params;
    std::array<juce::File, kMaxTracks> files;
    juce::File destination;
    bool encodeMp3;
    double sampleRate, tail;
    std::atomic<double> progress { 0.0 };
    std::atomic<bool> finished { false }, clipping { false };
    juce::Result result = juce::Result::ok();
};
} // namespace mps
