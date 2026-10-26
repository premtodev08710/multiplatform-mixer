// Mix Practice Studio - shared multitrack transport with disk read-ahead.
//
// Design (why tracks can never drift apart):
//  * There is ONE timeline position for all tracks. A producer (disk thread) reads the
//    same timeline frame range from every track and writes it into ONE multi-channel
//    lock-free ring buffer (single producer / single consumer).
//  * The audio thread consumes whole frames (all tracks at once) from that ring, so every
//    track is always at the same sample position - there are no per-track players.
//  * Seek / stop / loop changes / track-set changes bump a "generation" number. Frames are
//    tagged with the generation they were produced for; the audio thread fades out and
//    discards stale frames and fades in once enough new frames are buffered.
//  * Tracks shorter than the timeline produce silence after their end.
//  * The audio thread never reads files, never locks and never allocates.
#pragma once

#include "MixerParams.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

namespace mps
{
/** Random-access audio source already at the engine sample rate. Called only from the
    producer (disk) thread. */
class TrackReader
{
public:
    virtual ~TrackReader() = default;
    virtual int getNumChannels() const = 0;          // 1 or 2
    virtual int64_t getLengthInFrames() const = 0;
    /** Fill dest[0..getNumChannels()-1] with numFrames frames starting at startFrame
        (always within [0, length)). Return false on read error (dest is then zeroed). */
    virtual bool read (float* const* dest, int64_t startFrame, int numFrames) = 0;
};

struct TrackSet
{
    std::array<std::shared_ptr<TrackReader>, kMaxTracks> slots;
};

class TimelineStreamer
{
public:
    struct Config
    {
        int ringFrames = 1 << 18;           // must be power of two (~5.5 s @ 48 kHz, rock-solid headroom against dropouts)
        int chunkFrames = 4096;             // producer read size (minimizes disk seek overhead)
        int startThresholdFrames = 16384;   // frames buffered before starting playback (~340 ms @ 48 kHz)
        int fadeFrames = 256;               // transport fade in/out length
    };

    TimelineStreamer();
    explicit TimelineStreamer (Config);

    //==== control (message thread) ============================================
    void setTrackSet (std::shared_ptr<const TrackSet> set);   // keeps playhead position
    void seek (int64_t frame);
    void setLoop (bool enabled, int64_t startFrame, int64_t endFrame);
    void setPlaying (bool shouldPlay) noexcept { playing.store (shouldPlay, std::memory_order_release); }
    void setFadeFrames (int frames) noexcept   { fadeFrames.store (std::max (1, frames)); }

    bool isPlaying() const noexcept            { return playing.load (std::memory_order_acquire); }
    int64_t getPlayhead() const noexcept       { return playhead.load (std::memory_order_acquire); }
    int64_t getLength() const noexcept         { return length.load (std::memory_order_acquire); }
    bool getLoopEnabled() const;
    int64_t getLoopStart() const;
    int64_t getLoopEnd() const;
    /** True once after playback stopped automatically at the end of the material. */
    bool consumeReachedEnd() noexcept          { return reachedEnd.exchange (false); }
    uint64_t getUnderrunCount() const noexcept { return underruns.load (std::memory_order_relaxed); }
    int getSlotChannels (int slot) const noexcept { return slotChannels[(size_t) slot].load (std::memory_order_acquire); }
    /** Fraction of the ring that is filled with current-generation audio (UI diagnostic). */
    float getBufferFill() const noexcept;

    //==== producer (disk thread) ==============================================
    /** Reads ahead as far as space allows. Returns true if any work was done. */
    bool service();

    //==== consumer (audio thread) =============================================
    /** Writes numFrames frames into slotOut[slot*2 + ch] (all kMaxTracks*2 pointers must be
        valid). Fills silence where nothing is playing. Real-time safe. */
    void pull (float* const* slotOut, int numFrames) noexcept;

    /** Copies the per-slot channel counts for the mixer (audio thread). */
    void getSlotChannelCounts (int* dest) const noexcept;

private:
    static constexpr int kRingChannels = kMaxTracks * 2;

    // generations: frames older than requestedGen are stale
    static bool genBefore (uint32_t a, uint32_t b) noexcept { return (int32_t) (a - b) < 0; }
    uint64_t countStale (uint64_t readPos, uint64_t avail, uint32_t target) const noexcept;
    void copyOut (float* const* slotOut, int outOffset, uint64_t readPos, int count,
                  float& gain, float gainStep, float gainTarget) noexcept;

    Config cfg;
    uint64_t mask = 0;
    std::vector<float> ring;            // [channel][frame]
    std::vector<int64_t> framePos;      // timeline position of each ring frame
    std::vector<uint32_t> frameGen;     // generation of each ring frame

    std::atomic<uint64_t> writeCount { 0 }, readCount { 0 };

    // control state (protected by controlMutex; never touched by audio thread)
    mutable std::mutex controlMutex;
    struct Pending
    {
        int64_t position = 0;
        bool loopOn = false;
        int64_t loopA = 0, loopB = 0;
        std::shared_ptr<const TrackSet> tracks;
    } pending;
    std::atomic<uint32_t> requestedGen { 1 };

    // producer-only state
    uint32_t prodGen = 0;
    int64_t prodPos = 0;
    bool prodLoopOn = false;
    int64_t prodLoopA = 0, prodLoopB = 0;
    bool prodFinished = false;
    int prodWrapFade = 0;
    std::shared_ptr<const TrackSet> prodTracks;
    std::vector<float> scratch;         // [channel][chunk]

    // shared status
    std::atomic<uint32_t> endGen { 0 };         // generation whose data reached the end
    std::array<std::atomic<int>, kMaxTracks> slotChannels {};
    std::atomic<int64_t> length { 0 };
    std::atomic<int64_t> playhead { 0 };
    std::atomic<bool> playing { false };
    std::atomic<bool> reachedEnd { false };
    std::atomic<uint64_t> underruns { 0 };
    std::atomic<int> fadeFrames { 256 };

    // consumer-only state
    float gain = 0.0f;
    bool running = false;
};

} // namespace mps
