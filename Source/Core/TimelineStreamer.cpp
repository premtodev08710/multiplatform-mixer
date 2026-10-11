#include "TimelineStreamer.h"
#include <cmath>
#include <cstring>

namespace mps
{
TimelineStreamer::TimelineStreamer() : TimelineStreamer (Config{}) {}

TimelineStreamer::TimelineStreamer (Config c) : cfg (c)
{
    // force power of two
    int rf = 1;
    while (rf < cfg.ringFrames) rf <<= 1;
    cfg.ringFrames = rf;
    cfg.chunkFrames = std::max (64, std::min (cfg.chunkFrames, cfg.ringFrames / 4));
    cfg.startThresholdFrames = std::max (1, std::min (cfg.startThresholdFrames, cfg.ringFrames / 2));
    mask = (uint64_t) (cfg.ringFrames - 1);

    ring.assign ((size_t) kRingChannels * (size_t) cfg.ringFrames, 0.0f);
    framePos.assign ((size_t) cfg.ringFrames, 0);
    frameGen.assign ((size_t) cfg.ringFrames, 0);
    scratch.assign ((size_t) kRingChannels * (size_t) cfg.chunkFrames, 0.0f);
    fadeFrames.store (std::max (1, cfg.fadeFrames));
    for (auto& s : slotChannels) s.store (0);
}

//==============================================================================
void TimelineStreamer::setTrackSet (std::shared_ptr<const TrackSet> set)
{
    std::lock_guard<std::mutex> lock (controlMutex);
    pending.tracks = std::move (set);
    pending.position = playhead.load();
    requestedGen.fetch_add (1, std::memory_order_acq_rel);
}

void TimelineStreamer::seek (int64_t frame)
{
    frame = std::max<int64_t> (0, frame);
    std::lock_guard<std::mutex> lock (controlMutex);
    pending.position = frame;
    playhead.store (frame, std::memory_order_release);
    reachedEnd.store (false);
    requestedGen.fetch_add (1, std::memory_order_acq_rel);
}

void TimelineStreamer::setLoop (bool enabled, int64_t a, int64_t b)
{
    std::lock_guard<std::mutex> lock (controlMutex);
    const bool wasOn = pending.loopOn;
    const bool changed = enabled != wasOn || a != pending.loopA || b != pending.loopB;
    pending.loopOn = enabled;
    pending.loopA = std::max<int64_t> (0, a);
    pending.loopB = std::max<int64_t> (0, b);

    // Only flush read-ahead audio when the loop affects what will be played.
    if (changed && (enabled || wasOn))
    {
        pending.position = playhead.load();
        requestedGen.fetch_add (1, std::memory_order_acq_rel);
    }
}

bool TimelineStreamer::getLoopEnabled() const { std::lock_guard<std::mutex> l (controlMutex); return pending.loopOn; }
int64_t TimelineStreamer::getLoopStart() const { std::lock_guard<std::mutex> l (controlMutex); return pending.loopA; }
int64_t TimelineStreamer::getLoopEnd() const   { std::lock_guard<std::mutex> l (controlMutex); return pending.loopB; }

float TimelineStreamer::getBufferFill() const noexcept
{
    const auto avail = writeCount.load (std::memory_order_acquire) - readCount.load (std::memory_order_acquire);
    return (float) avail / (float) cfg.ringFrames;
}

void TimelineStreamer::getSlotChannelCounts (int* dest) const noexcept
{
    for (int s = 0; s < kMaxTracks; ++s)
        dest[s] = slotChannels[(size_t) s].load (std::memory_order_acquire);
}

//==============================================================================
bool TimelineStreamer::service()
{
    {
        std::lock_guard<std::mutex> lock (controlMutex);
        const uint32_t g = requestedGen.load (std::memory_order_acquire);
        if (g != prodGen)
        {
            prodGen = g;
            prodPos = pending.position;
            prodLoopOn = pending.loopOn;
            prodLoopA = pending.loopA;
            prodLoopB = pending.loopB;
            prodTracks = pending.tracks;
            prodFinished = false;
            prodWrapFade = 0;

            int64_t len = 0;
            for (int s = 0; s < kMaxTracks; ++s)
            {
                int ch = 0;
                if (prodTracks != nullptr)
                    if (auto& r = prodTracks->slots[(size_t) s])
                    {
                        ch = std::min (2, std::max (1, r->getNumChannels()));
                        len = std::max (len, r->getLengthInFrames());
                    }
                slotChannels[(size_t) s].store (ch, std::memory_order_release);
            }
            length.store (len, std::memory_order_release);
        }
    }

    if (prodFinished) return false;

    const int64_t len = length.load (std::memory_order_acquire);
    const int fade = fadeFrames.load();
    const int chunk = cfg.chunkFrames;
    const auto ringFrames = (uint64_t) cfg.ringFrames;
    bool worked = false;
    int& wrapFadeRemaining = prodWrapFade;

    for (;;)
    {
        if (requestedGen.load (std::memory_order_acquire) != prodGen)
            return true; // new command: let the caller call service() again right away

        const uint64_t w = writeCount.load (std::memory_order_relaxed);
        const uint64_t r = readCount.load (std::memory_order_acquire);
        const uint64_t freeFrames = ringFrames - (w - r);
        if (freeFrames < (uint64_t) std::max (64, chunk / 4))
            break;

        int n = (int) std::min<uint64_t> ((uint64_t) chunk, freeFrames);

        const int64_t loopB = len > 0 ? std::min (prodLoopB, len) : prodLoopB;
        const bool loopValid = prodLoopOn && (loopB - prodLoopA) >= (int64_t) std::max (fade * 4, 64);
        const bool looping = loopValid && prodPos < loopB;

        if (looping)
        {
            n = (int) std::min<int64_t> (n, loopB - prodPos);
        }
        else
        {
            if (prodPos >= len)
            {
                prodFinished = true;
                endGen.store (prodGen, std::memory_order_release);
                return true;
            }
            n = (int) std::min<int64_t> (n, len - prodPos);
        }

        // ---- read every slot for the SAME timeline range
        for (int s = 0; s < kMaxTracks; ++s)
        {
            float* d0 = scratch.data() + (size_t) (s * 2) * (size_t) chunk;
            float* d1 = scratch.data() + (size_t) (s * 2 + 1) * (size_t) chunk;
            TrackReader* reader = prodTracks != nullptr ? prodTracks->slots[(size_t) s].get() : nullptr;

            if (reader == nullptr)
            {
                std::memset (d0, 0, sizeof (float) * (size_t) n);
                std::memset (d1, 0, sizeof (float) * (size_t) n);
                continue;
            }

            const int numCh = std::min (2, std::max (1, reader->getNumChannels()));
            const int64_t trackLen = reader->getLengthInFrames();
            const int m = (int) std::max<int64_t> (0, std::min<int64_t> (n, trackLen - prodPos));
            if (m > 0)
            {
                float* dest[2] = { d0, d1 };
                if (! reader->read (dest, prodPos, m))
                {
                    std::memset (d0, 0, sizeof (float) * (size_t) m);
                    std::memset (d1, 0, sizeof (float) * (size_t) m);
                }
            }
            if (m < n) // track is shorter than the timeline: silence after its end
            {
                std::memset (d0 + m, 0, sizeof (float) * (size_t) (n - m));
                std::memset (d1 + m, 0, sizeof (float) * (size_t) (n - m));
            }
            if (numCh == 1)
                std::memcpy (d1, d0, sizeof (float) * (size_t) n);
        }

        // ---- short dip around the loop wrap point to avoid clicks
        if (looping)
        {
            for (int i = 0; i < n; ++i)
            {
                const int64_t pos = prodPos + i;
                float g = 1.0f;
                if (wrapFadeRemaining > 0)
                {
                    g = std::min (g, (float) (fade - wrapFadeRemaining + 1) / (float) fade);
                    --wrapFadeRemaining;
                }
                const int64_t toEnd = loopB - pos;
                if (toEnd <= fade)
                    g = std::min (g, (float) (toEnd - 1) / (float) fade);
                if (g < 1.0f)
                    for (int ch = 0; ch < kRingChannels; ++ch)
                        scratch[(size_t) ch * (size_t) chunk + (size_t) i] *= g;
            }
        }

        // ---- publish into the ring
        const uint64_t start = w & mask;
        const int first = (int) std::min<uint64_t> ((uint64_t) n, ringFrames - start);
        const int second = n - first;
        for (int ch = 0; ch < kRingChannels; ++ch)
        {
            const float* src = scratch.data() + (size_t) ch * (size_t) chunk;
            float* dst = ring.data() + (size_t) ch * ringFrames;
            std::memcpy (dst + start, src, sizeof (float) * (size_t) first);
            if (second > 0)
                std::memcpy (dst, src + first, sizeof (float) * (size_t) second);
        }
        for (int i = 0; i < n; ++i)
        {
            const auto idx = (size_t) ((w + (uint64_t) i) & mask);
            framePos[idx] = prodPos + i;
            frameGen[idx] = prodGen;
        }
        writeCount.store (w + (uint64_t) n, std::memory_order_release);

        prodPos += n;
        if (looping && prodPos >= loopB)
        {
            prodPos = prodLoopA;
            wrapFadeRemaining = fade;
        }
        worked = true;
    }

    return worked;
}

//==============================================================================
uint64_t TimelineStreamer::countStale (uint64_t r, uint64_t avail, uint32_t target) const noexcept
{
    if (avail == 0 || ! genBefore (frameGen[(size_t) (r & mask)], target))
        return 0;
    // generations are non-decreasing in FIFO order -> binary search for the first fresh frame
    uint64_t lo = 0, hi = avail;
    while (lo < hi)
    {
        const uint64_t mid = lo + (hi - lo) / 2;
        if (genBefore (frameGen[(size_t) ((r + mid) & mask)], target)) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}

void TimelineStreamer::copyOut (float* const* slotOut, int outOffset, uint64_t readPos, int count,
                                float& g, float step, float gainTarget) noexcept
{
    if (count <= 0) return;
    const auto ringFrames = (uint64_t) cfg.ringFrames;
    const uint64_t start = readPos & mask;
    const int first = (int) std::min<uint64_t> ((uint64_t) count, ringFrames - start);
    const int second = count - first;

    for (int ch = 0; ch < kRingChannels; ++ch)
    {
        const float* src = ring.data() + (size_t) ch * ringFrames;
        float* dst = slotOut[ch] + outOffset;
        std::memcpy (dst, src + start, sizeof (float) * (size_t) first);
        if (second > 0)
            std::memcpy (dst + first, src, sizeof (float) * (size_t) second);
    }

    if (g == gainTarget && g == 1.0f)
        return;

    // gain ramp shared by ALL tracks -> relative alignment/balance is untouched
    for (int i = 0; i < count; ++i)
    {
        if (g < gainTarget) g = std::min (gainTarget, g + step);
        else if (g > gainTarget) g = std::max (gainTarget, g - step);
        if (g != 1.0f)
            for (int ch = 0; ch < kRingChannels; ++ch)
                slotOut[ch][outOffset + i] *= g;
    }
}

void TimelineStreamer::pull (float* const* slotOut, int n) noexcept
{
    if (n <= 0) return;
    for (int ch = 0; ch < kRingChannels; ++ch)
        std::memset (slotOut[ch], 0, sizeof (float) * (size_t) n);

    const uint32_t target = requestedGen.load (std::memory_order_acquire);
    uint64_t r = readCount.load (std::memory_order_relaxed);
    const uint64_t w = writeCount.load (std::memory_order_acquire);
    uint64_t avail = w - r;
    const int fade = std::max (1, fadeFrames.load (std::memory_order_relaxed));
    const float step = 1.0f / (float) fade;

    // ---- 1. stale frames (after seek/stop/loop/track change): discard immediately
    const uint64_t stale = countStale (r, avail, target);
    if (stale > 0)
    {
        gain = 0.0f;
        running = false;
        r += stale;
        avail -= stale;
        readCount.store (r, std::memory_order_release);
    }

    // ---- 2. paused / stopped
    if (! playing.load (std::memory_order_acquire))
    {
        if (gain > 0.0f && avail > 0)
        {
            const int need = (int) std::ceil (gain / step);
            const int k = (int) std::min<uint64_t> (avail, (uint64_t) std::min (n, need));
            copyOut (slotOut, 0, r, k, gain, step, 0.0f);
            playhead.store (framePos[(size_t) ((r + (uint64_t) k - 1) & mask)] + 1, std::memory_order_release);
            r += (uint64_t) k;
            readCount.store (r, std::memory_order_release);
        }
        gain = 0.0f;
        running = false;
        return;
    }

    // ---- 3. playing
    const bool producerDone = endGen.load (std::memory_order_acquire) == target;
    if (! running)
    {
        if (avail == 0 && producerDone)
        {
            playing.store (false, std::memory_order_release);
            reachedEnd.store (true);
            playhead.store (length.load(), std::memory_order_release);
            return;
        }
        if (avail < (uint64_t) cfg.startThresholdFrames && ! producerDone)
            return; // still buffering after a seek: stay silent, keep tracks aligned
        if (avail == 0)
            return;
        running = true;
    }

    const int k = (int) std::min<uint64_t> ((uint64_t) n, avail);
    if (k > 0)
    {
        copyOut (slotOut, 0, r, k, gain, step, 1.0f);
        playhead.store (framePos[(size_t) ((r + (uint64_t) k - 1) & mask)] + 1, std::memory_order_release);
        r += (uint64_t) k;
        readCount.store (r, std::memory_order_release);
    }

    if (k < n)
    {
        if (endGen.load (std::memory_order_acquire) == target)
        {
            playing.store (false, std::memory_order_release);
            reachedEnd.store (true);
        }
        else
        {
            underruns.fetch_add (1, std::memory_order_relaxed);
        }
        running = false;
        gain = 0.0f;
    }
}

} // namespace mps
