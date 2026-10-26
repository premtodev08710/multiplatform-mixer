// Core DSP / transport tests. Build: see CMakeLists (target MixPracticeCoreTests) or
//   clang++ -std=c++17 -O2 -ISource Tests/CoreTests.cpp Source/Core/*.cpp -o build/core_tests
#include "TestHarness.h"
#include "../Source/Core/OutputProtection.h"
#include "../Source/Core/DspMath.h"
#include "../Source/Core/MixEngine.h"
#include "../Source/Core/SincResampler.h"
#include "../Source/Core/TimelineStreamer.h"

#include <atomic>
#include <chrono>
#include <random>
#include <thread>

using namespace mps;

//==============================================================================
// dB / fader / pan
TEST (dbToLinearConversion)
{
    CHECK_NEAR (dbToGain (0.0f), 1.0, 1e-7);
    CHECK_NEAR (dbToGain (-6.0206f), 0.5, 1e-5);
    CHECK_NEAR (dbToGain (20.0f), 10.0, 1e-5);
    CHECK_NEAR (dbToGain (-20.0f), 0.1, 1e-7);
    CHECK (dbToGain (kMinusInfDb) == 0.0f);
    CHECK_NEAR (gainToDb (1.0f), 0.0, 1e-6);
    CHECK_NEAR (gainToDb (0.5f), -6.0206, 1e-4);
    CHECK (gainToDb (0.0f) == kMinusInfDb);
    for (float db = -80.0f; db <= 18.0f; db += 0.5f)
        CHECK_NEAR (gainToDb (dbToGain (db)), db, 1e-3);
}

TEST (faderUnityAndTaper)
{
    CHECK (faderDbToGain (0.0f) == 1.0f);                 // unity gain is exact
    CHECK (faderDbToGain (kFaderMinDb) == 0.0f);          // bottom = -inf
    CHECK_NEAR (faderDbToGain (10.0f), 3.16227766, 1e-5);
    CHECK_NEAR (faderPosToDb (0.75f), 0.0, 1e-6);
    CHECK_NEAR (faderPosToDb (1.0f), 10.0, 1e-6);
    CHECK_NEAR (faderPosToDb (0.5f), -10.0, 1e-6);
    CHECK_NEAR (faderPosToDb (0.25f), -30.0, 1e-6);
    CHECK_NEAR (faderPosToDb (0.0625f), -60.0, 1e-6);
    CHECK_NEAR (faderPosToDb (0.0f), -90.0, 1e-6);
    for (float p = 0.0f; p <= 1.0f; p += 0.01f)
        CHECK_NEAR (faderDbToPos (faderPosToDb (p)), p, 1e-5);
}

TEST (panLaws)
{
    auto c = panMonoConstantPower (0.0f);
    CHECK_NEAR (c.left, 0.70710678, 1e-6);
    CHECK_NEAR (c.right, 0.70710678, 1e-6);
    auto l = panMonoConstantPower (-1.0f);
    CHECK_NEAR (l.left, 1.0, 1e-6);
    CHECK_NEAR (l.right, 0.0, 1e-6);
    for (float p = -1.0f; p <= 1.0f; p += 0.05f)
    {
        auto g = panMonoConstantPower (p);
        CHECK_NEAR (g.left * g.left + g.right * g.right, 1.0, 1e-5);   // constant power
    }
    auto b = balanceStereo (0.0f);
    CHECK (b.left == 1.0f && b.right == 1.0f);
    auto br = balanceStereo (1.0f);
    CHECK_NEAR (br.left, 0.0, 1e-6);
    CHECK_NEAR (br.right, 1.0, 1e-6);
}

//==============================================================================
// MixEngine harness: constant (DC) sources make expected levels easy to compute.
struct EngineRig
{
    static constexpr int kBlock = 256;
    MixerParams params;
    MixerMeters meters;
    MixEngine engine { params, meters };
    std::vector<std::vector<float>> slotBuf;
    std::vector<const float*> slotPtr;
    int slotCh[kMaxTracks] {};
    std::vector<float> outL, outR;

    EngineRig()
    {
        slotBuf.assign (kMaxTracks * 2, std::vector<float> (kBlock, 0.0f));
        for (auto& b : slotBuf) slotPtr.push_back (b.data());
        outL.assign (kBlock, 0.0f);
        outR.assign (kBlock, 0.0f);
        params.main.faderDb.set (0.0f);
        engine.prepare (48000.0, kBlock);
    }

    void setDc (int slot, float l, float r, int channels)
    {
        slotCh[slot] = channels;
        std::fill (slotBuf[(size_t) slot * 2].begin(), slotBuf[(size_t) slot * 2].end(), l);
        std::fill (slotBuf[(size_t) slot * 2 + 1].begin(), slotBuf[(size_t) slot * 2 + 1].end(), r);
    }

    void run (int blocks = 20)  // 20 * 256 samples > all ramps
    {
        for (int b = 0; b < blocks; ++b)
            engine.process (slotPtr.data(), slotCh, kBlock, outL.data(), outR.data());
    }
    float L() const { return outL.back(); }
    float R() const { return outR.back(); }
};

TEST (mixUnityMonoCentre)
{
    EngineRig rig;
    rig.setDc (0, 0.5f, 0.0f, 1);
    rig.params.channels[0].source.set (0);
    rig.run();
    CHECK_NEAR (rig.L(), 0.5 * 0.70710678, 1e-5);
    CHECK_NEAR (rig.R(), 0.5 * 0.70710678, 1e-5);
}

TEST (mixTrimFaderPolarityMainAssign)
{
    EngineRig rig;
    rig.setDc (0, 0.5f, 0.0f, 1);
    auto& ch = rig.params.channels[0];
    ch.source.set (0);
    ch.pan.set (-1.0f);                 // hard left -> L gain exactly 1
    ch.trimDb.set (6.0206f);            // x2
    ch.faderDb.set (-6.0206f);          // x0.5
    rig.run();
    CHECK_NEAR (rig.L(), 0.5, 1e-4);
    CHECK_NEAR (rig.R(), 0.0, 1e-6);

    ch.polarity.set (1.0f);
    rig.run();
    CHECK_NEAR (rig.L(), -0.5, 1e-4);

    ch.mainAssign.set (0.0f);
    rig.run();
    CHECK_NEAR (rig.L(), 0.0, 1e-7);

    ch.mainAssign.set (1.0f);
    rig.params.main.faderDb.set (-6.0206f);
    rig.run();
    CHECK_NEAR (rig.L(), -0.25, 1e-4);

    rig.params.main.mute.set (1.0f);
    rig.run();
    CHECK_NEAR (rig.L(), 0.0, 1e-7);
}

TEST (mixMuteSoloBehaviour)
{
    EngineRig rig;
    rig.setDc (0, 0.25f, 0.0f, 1);
    rig.setDc (1, 0.125f, 0.0f, 1);
    auto& a = rig.params.channels[0];
    auto& b = rig.params.channels[1];
    a.source.set (0); a.pan.set (-1.0f);
    b.source.set (1); b.pan.set (-1.0f);
    rig.run();
    CHECK_NEAR (rig.L(), 0.375, 1e-5);

    a.mute.set (1.0f);                   // mute removes channel
    rig.run();
    CHECK_NEAR (rig.L(), 0.125, 1e-5);
    a.mute.set (0.0f);

    b.solo.set (1.0f);                   // solo in place: only soloed channels audible
    rig.run();
    CHECK_NEAR (rig.L(), 0.125, 1e-5);

    a.solo.set (1.0f);                   // two soloed channels
    rig.run();
    CHECK_NEAR (rig.L(), 0.375, 1e-5);

    b.mute.set (1.0f);                   // mute wins over solo
    rig.run();
    CHECK_NEAR (rig.L(), 0.25, 1e-5);

    a.solo.set (0.0f); b.solo.set (0.0f); b.mute.set (0.0f);
    rig.run();
    CHECK_NEAR (rig.L(), 0.375, 1e-5);
}

TEST (mixStereoSourceModes)
{
    EngineRig rig;
    rig.setDc (0, 0.4f, 0.2f, 2);
    auto& ch = rig.params.channels[0];
    ch.source.set (0);
    rig.run();                                       // native stereo, balance centre = unity
    CHECK_NEAR (rig.L(), 0.4, 1e-4);
    CHECK_NEAR (rig.R(), 0.2, 1e-4);
    CHECK (rig.meters.channels[0].width.load() == 2);

    ch.sourceMode.set ((float) SourceMode::MonoSum); // (L+R)/2 = 0.3, centre -3 dB
    rig.run();
    CHECK_NEAR (rig.L(), 0.3 * 0.70710678, 1e-4);
    CHECK_NEAR (rig.R(), 0.3 * 0.70710678, 1e-4);
    CHECK (rig.meters.channels[0].width.load() == 1);

    ch.sourceMode.set ((float) SourceMode::RightOnly);
    ch.pan.set (1.0f);
    rig.run();
    CHECK_NEAR (rig.R(), 0.2, 1e-4);
    CHECK_NEAR (rig.L(), 0.0, 1e-4);
}

TEST (mixMeterTaps)
{
    EngineRig rig;
    rig.setDc (0, 0.5f, 0.0f, 1);
    auto& ch = rig.params.channels[0];
    ch.source.set (0);
    ch.faderDb.set (-20.0f);
    rig.run();
    auto& m = rig.meters.channels[0];
    m.preL.readAndReset(); m.postL.readAndReset();
    rig.run (1);
    CHECK_NEAR (m.preL.readAndReset(), 0.5, 1e-5);                          // pre-fader ignores fader
    CHECK_NEAR (m.postL.readAndReset(), 0.5 * 0.1 * 0.70710678, 1e-5);      // post-fader includes fader+pan
    CHECK (! m.clip.load());
    ch.trimDb.set (12.0f);
    rig.run();
    CHECK (m.clip.load());                                                  // 0.5 * 4 >= 0 dBFS
}

TEST (mixPreampTrimFullRange)
{
    EngineRig rig;
    rig.setDc (0, 0.001f, 0.0f, 1);                                        // -60 dBFS quiet stem
    auto& ch = rig.params.channels[0];
    ch.source.set (0);
    ch.faderDb.set (-90.0f);                                               // Fader down at -inf
    ch.trimDb.set (60.0f);                                                 // +60 dB preamp gain (= 1000x)
    rig.run();
    auto& m = rig.meters.channels[0];
    // Pre-fader meter MUST respond even when fader is at -inf
    CHECK_NEAR (m.preL.readAndReset(), 1.0, 0.01);                          // 0.001 * 1000 = 1.0 (0 dBFS)
    // Post-fader meter MUST remain 0
    CHECK_NEAR (m.postL.readAndReset(), 0.0, 1e-6);
}

TEST (mixGainChangesAreSmoothed)
{
    EngineRig rig;
    rig.setDc (0, 1.0f, 0.0f, 1);
    auto& ch = rig.params.channels[0];
    ch.source.set (0);
    ch.pan.set (-1.0f);
    ch.faderDb.set (-6.0206f);
    rig.run();
    ch.faderDb.set (0.0f);
    rig.engine.process (rig.slotPtr.data(), rig.slotCh, EngineRig::kBlock, rig.outL.data(), rig.outR.data());
    float maxStep = 0.0f;
    for (int i = 1; i < EngineRig::kBlock; ++i)
        maxStep = std::max (maxStep, std::abs (rig.outL[(size_t) i] - rig.outL[(size_t) i - 1]));
    CHECK (rig.outL[0] < 0.51f);           // no instant jump
    CHECK (maxStep < 0.001f);              // 0.5 change spread over 20 ms (960 samples)
    ch.mute.set (1.0f);
    rig.engine.process (rig.slotPtr.data(), rig.slotCh, EngineRig::kBlock, rig.outL.data(), rig.outR.data());
    CHECK (rig.outL[0] > 0.4f);            // mute ramps, does not click
}

//==============================================================================
// Transport / streamer
struct FnTrack : TrackReader
{
    int ch; int64_t len; std::function<float (int64_t, int)> fn;
    FnTrack (int c, int64_t l, std::function<float (int64_t, int)> f) : ch (c), len (l), fn (std::move (f)) {}
    int getNumChannels() const override { return ch; }
    int64_t getLengthInFrames() const override { return len; }
    bool read (float* const* dest, int64_t start, int n) override
    {
        for (int c = 0; c < ch; ++c)
            for (int i = 0; i < n; ++i) dest[c][i] = fn (start + i, c);
        return true;
    }
};

struct StreamRig
{
    TimelineStreamer s;
    std::vector<std::vector<float>> buf;
    std::vector<float*> ptr;
    int block;

    explicit StreamRig (int blockSize = 480, TimelineStreamer::Config cfg = {}) : s (cfg), block (blockSize)
    {
        buf.assign (kMaxTracks * 2, std::vector<float> ((size_t) blockSize, 0.0f));
        for (auto& b : buf) ptr.push_back (b.data());
    }
    void pull() { s.pull (ptr.data(), block); }
    void serviceAll() { while (s.service()) {} }
    float at (int slot, int ch, int i) const { return buf[(size_t) slot * 2 + (size_t) ch][(size_t) i]; }
};

// Track k (k = 0..n-1) value at timeline pos p: ((p % 1000) + 1) * (k + 1) * 1e-3
static float enc (int64_t p, int k) { return (float) ((p % 1000) + 1) * (float) (k + 1) * 1.0e-3f; }

static std::shared_ptr<TrackSet> makeEncodedSet (int numTracks, int64_t len)
{
    auto set = std::make_shared<TrackSet>();
    for (int k = 0; k < numTracks; ++k)
        set->slots[(size_t) k] = std::make_shared<FnTrack> (k % 2 == 0 ? 1 : 2, len,
                                                            [k] (int64_t p, int) { return enc (p, k); });
    return set;
}

/** Every output frame: track k must equal track 0 * (k+1) -> all tracks at the same position. */
static int checkAligned (const StreamRig& r, int numTracks)
{
    int bad = 0;
    for (int i = 0; i < r.block; ++i)
    {
        const float a0 = r.at (0, 0, i);
        for (int k = 1; k < numTracks; ++k)
            for (int c = 0; c < 2; ++c)
                if (std::abs (r.at (k, c, i) - a0 * (float) (k + 1)) > 1.0e-5f * (float) (k + 1) + 1.0e-7f)
                    ++bad;
    }
    return bad;
}

TEST (transportImpulsesStayAligned)
{
    // 8 tracks of different lengths, each with an impulse at the same timeline position.
    StreamRig r (441);
    const int64_t impulseAt = 30001;
    auto set = std::make_shared<TrackSet>();
    for (int k = 0; k < 8; ++k)
        set->slots[(size_t) k] = std::make_shared<FnTrack> (1, 40000 + k * 7919,
            [impulseAt] (int64_t p, int) { return p == impulseAt ? 1.0f : 0.0f; });
    r.s.setTrackSet (set);
    r.s.setPlaying (true);

    std::vector<int64_t> found (8, -1);
    int64_t outFrame = 0;
    for (int b = 0; b < 200; ++b)
    {
        r.serviceAll();
        r.pull();
        for (int i = 0; i < r.block; ++i)
            for (int k = 0; k < 8; ++k)
                if (r.at (k, 0, i) > 0.5f) found[(size_t) k] = outFrame + i;
        outFrame += r.block;
    }
    for (int k = 0; k < 8; ++k)
        CHECK (found[(size_t) k] == found[0] && found[0] >= 0);
    CHECK (found[0] == impulseAt);     // started at 0, no underruns -> exact position
    CHECK (r.s.getUnderrunCount() == 0);
}

TEST (transportSeekPauseKeepAlignment)
{
    StreamRig r (512);
    const int n = 8;
    r.s.setTrackSet (makeEncodedSet (n, 48000 * 20));
    r.s.setPlaying (true);
    std::mt19937 rng (1234);
    int bad = 0;
    for (int b = 0; b < 3000; ++b)
    {
        if (b % 97 == 13) r.s.seek ((int64_t) (rng() % (48000 * 18)));
        if (b % 211 == 50) r.s.setPlaying (false);
        if (b % 211 == 80) r.s.setPlaying (true);
        r.serviceAll();
        r.pull();
        bad += checkAligned (r, n);
    }
    CHECK (bad == 0);
}

TEST (transportSeekLandsOnRequestedFrame)
{
    StreamRig r (256);
    r.s.setTrackSet (makeEncodedSet (2, 48000 * 10));
    r.s.setPlaying (true);
    for (int b = 0; b < 10; ++b) { r.serviceAll(); r.pull(); }
    const int64_t target = 123457;
    r.s.seek (target);
    CHECK (r.s.getPlayhead() == target);
    // run until audio comes back, then verify continuity from the target position
    int64_t firstPos = -1;
    for (int b = 0; b < 20 && firstPos < 0; ++b)
    {
        r.serviceAll();
        r.pull();
        // the first non-silent block after the seek is the fade-in starting at target
        for (int i = 0; i < r.block; ++i)
            if (r.at (0, 0, i) != 0.0f) { firstPos = r.s.getPlayhead() - r.block + i; break; }
    }
    CHECK (firstPos == target);
    // after the fade-in, the decoded value must match the playhead exactly
    r.serviceAll(); r.pull();
    const int64_t endPos = r.s.getPlayhead();
    CHECK_NEAR (r.at (0, 0, r.block - 1), enc (endPos - 1, 0), 1e-6);
}

TEST (transportLoopAB)
{
    StreamRig r (300);
    const int n = 4;
    r.s.setTrackSet (makeEncodedSet (n, 48000 * 10));
    const int64_t A = 48000, B = 48000 + 9000;
    r.s.setLoop (true, A, B);
    r.s.seek (A);
    r.s.setPlaying (true);
    int bad = 0;
    int64_t minPos = INT64_MAX, maxPos = 0;
    for (int b = 0; b < 400; ++b)
    {
        r.serviceAll();
        r.pull();
        bad += checkAligned (r, n);
        const int64_t ph = r.s.getPlayhead();
        if (b > 2) { minPos = std::min (minPos, ph); maxPos = std::max (maxPos, ph); }
    }
    CHECK (bad == 0);
    CHECK (minPos >= A);
    CHECK (maxPos <= B);
    CHECK (r.s.isPlaying());                // looping never reaches the end
    CHECK (r.s.getUnderrunCount() == 0);
}

TEST (transportTrackReorderingSwap)
{
    StreamRig r (256);
    auto set = std::make_shared<TrackSet>();
    set->slots[0] = std::make_shared<FnTrack> (1, 48000, [] (int64_t, int) { return 0.25f; });
    set->slots[1] = std::make_shared<FnTrack> (1, 48000, [] (int64_t, int) { return 0.75f; });
    r.s.setTrackSet (set);
    r.s.setPlaying (true);
    for (int b = 0; b < 10; ++b) { r.serviceAll(); r.pull(); }
    CHECK_NEAR (r.at (0, 0, 100), 0.25, 1e-4);
    CHECK_NEAR (r.at (1, 0, 100), 0.75, 1e-4);

    // Swap slots 0 and 1
    auto newSet = std::make_shared<TrackSet> (*set);
    std::swap (newSet->slots[0], newSet->slots[1]);
    r.s.setTrackSet (newSet);
    for (int b = 0; b < 10; ++b) { r.serviceAll(); r.pull(); }
    CHECK_NEAR (r.at (0, 0, 100), 0.75, 1e-4);
    CHECK_NEAR (r.at (1, 0, 100), 0.25, 1e-4);
}

TEST (transportShortTrackPlaysSilenceAndStopsAtEnd)
{
    StreamRig r (500);
    auto set = std::make_shared<TrackSet>();
    set->slots[0] = std::make_shared<FnTrack> (1, 10000, [] (int64_t, int) { return 0.5f; });
    set->slots[1] = std::make_shared<FnTrack> (2, 4000, [] (int64_t, int) { return 0.25f; });
    r.s.setTrackSet (set);
    r.s.setPlaying (true);
    bool shortSilentWhileLongPlays = false;
    for (int b = 0; b < 40; ++b)
    {
        r.serviceAll();
        r.pull();
        const int64_t ph = r.s.getPlayhead();
        if (ph > 5000 && ph < 9000)
            shortSilentWhileLongPlays = r.at (1, 0, r.block - 1) == 0.0f && r.at (1, 1, r.block - 1) == 0.0f
                                        && r.at (0, 0, r.block - 1) == 0.5f;
    }
    CHECK (shortSilentWhileLongPlays);
    CHECK (! r.s.isPlaying());
    CHECK (r.s.consumeReachedEnd());
    CHECK (r.s.getPlayhead() == 10000);
    CHECK (r.s.getLength() == 10000);
}

TEST (transportThreadedStress)
{
    // Real producer thread + simulated audio thread, random transport commands.
    TimelineStreamer::Config cfg; cfg.ringFrames = 1 << 14;
    StreamRig r (256, cfg);
    const int n = 16;
    r.s.setTrackSet (makeEncodedSet (n, 48000 * 60));
    r.s.setPlaying (true);
    std::atomic<bool> stop { false };
    std::thread producer ([&] { while (! stop) { if (! r.s.service()) std::this_thread::sleep_for (std::chrono::microseconds (300)); } });
    std::mt19937 rng (99);
    int bad = 0;
    for (int b = 0; b < 6000; ++b)
    {
        if (rng() % 150 == 0) r.s.seek ((int64_t) (rng() % (48000 * 55)));
        if (rng() % 400 == 0) r.s.setLoop (rng() % 2 == 0, 48000 * 3, 48000 * 4);
        if (rng() % 300 == 0) r.s.setPlaying (! r.s.isPlaying());
        r.pull();
        bad += checkAligned (r, n);
        if (b % 4 == 0) std::this_thread::sleep_for (std::chrono::microseconds (200));
    }
    stop = true;
    producer.join();
    CHECK (bad == 0);
}

//==============================================================================
// Resampler
static std::vector<float> resampleWhole (const std::vector<float>& in, double inSr, double outSr)
{
    SincResampler rs (inSr, outSr);
    const int64_t outLen = rs.getOutputLength ((int64_t) in.size());
    std::vector<float> out ((size_t) outLen);
    const int blk = 1000;
    for (int64_t o = 0; o < outLen; o += blk)
    {
        const int num = (int) std::min<int64_t> (blk, outLen - o);
        int64_t s, c;
        rs.getInputRange (o, num, s, c);
        std::vector<float> win ((size_t) c, 0.0f);
        for (int64_t i = 0; i < c; ++i)
        {
            const int64_t k = s + i;
            if (k >= 0 && k < (int64_t) in.size()) win[(size_t) i] = in[(size_t) k];
        }
        rs.process (win.data(), s, c, out.data() + o, o, num);
    }
    return out;
}

TEST (resamplerLengthDcAndSine)
{
    SincResampler rs (44100.0, 48000.0);
    CHECK (rs.getOutputLength (44100) == 48000);
    CHECK (rs.getOutputLength (441) == 480);

    std::vector<float> dc (44100, 0.5f);
    auto o = resampleWhole (dc, 44100.0, 48000.0);
    CHECK (o.size() == 48000);
    CHECK_NEAR (o[24000], 0.5, 1e-3);

    std::vector<float> sine (44100);
    for (size_t i = 0; i < sine.size(); ++i) sine[i] = (float) std::sin (2.0 * 3.141592653589793 * 1000.0 * (double) i / 44100.0);
    auto os = resampleWhole (sine, 44100.0, 48000.0);
    double maxErr = 0.0;
    for (size_t i = 2000; i < 46000; ++i)
        maxErr = std::max (maxErr, std::abs ((double) os[i] - std::sin (2.0 * 3.141592653589793 * 1000.0 * (double) i / 48000.0)));
    CHECK (maxErr < 2.0e-3);     // phase-exact, unity gain
}

TEST (resamplerKeepsTimeAlignment)
{
    // An impulse at 1.0 s in a 44.1k file must land at 1.0 s in the 48k output.
    std::vector<float> in (88200, 0.0f);
    in[44100] = 1.0f;
    auto o = resampleWhole (in, 44100.0, 48000.0);
    size_t peak = 0;
    for (size_t i = 0; i < o.size(); ++i) if (o[i] > o[peak]) peak = i;
    CHECK (peak == 48000);

    std::vector<float> in96 (192000, 0.0f);
    in96[96000] = 1.0f;
    auto o2 = resampleWhole (in96, 96000.0, 48000.0);
    size_t peak2 = 0;
    for (size_t i = 0; i < o2.size(); ++i) if (o2[i] > o2[peak2]) peak2 = i;
    CHECK (peak2 == 48000);
}

//==============================================================================
// DSP Unit Tests
TEST (delayLineAccuracy)
{
    DelayLine dl;
    dl.prepare (48000.0);
    const int delaySamples = 480; // 10 ms at 48kHz
    int detectedDelay = -1;
    for (int i = 0; i < 1000; ++i)
    {
        const float in = (i == 0) ? 1.0f : 0.0f;
        const float out = dl.process (in, 10.0f);
        if (out > 0.5f) { detectedDelay = i; break; }
    }
    CHECK (detectedDelay == delaySamples);
}

TEST (hpfResponse)
{
    const float sr = 48000.0f;
    const float fc = 100.0f;
    const auto hpf = calcHPF (fc, 0.7071f, sr);
    // At fc, magnitude response of Butterworth is -3.01 dB
    CHECK_NEAR (hpf.getMagnitudeResponseDb (fc, sr), -3.0103f, 0.15f);
    // Well above fc (e.g. 1000 Hz), response is 0 dB
    CHECK_NEAR (hpf.getMagnitudeResponseDb (1000.0f, sr), 0.0f, 0.05f);
    // One octave below fc (50 Hz), 12 dB/oct rolloff -> ~ -12 dB
    CHECK (hpf.getMagnitudeResponseDb (50.0f, sr) < -11.0f);
}

TEST (parametricEqBiquadResponse)
{
    const float sr = 48000.0f;
    const float fc = 1000.0f;
    const float gainDb = 6.0f;
    const auto bell = calcBell (fc, gainDb, 1.414f, sr);
    // Center frequency magnitude response should match gainDb
    CHECK_NEAR (bell.getMagnitudeResponseDb (fc, sr), gainDb, 0.05f);
    // Far away frequencies should be 0 dB
    CHECK_NEAR (bell.getMagnitudeResponseDb (50.0f, sr), 0.0f, 0.1f);
    CHECK_NEAR (bell.getMagnitudeResponseDb (20000.0f, sr), 0.0f, 0.1f);

    const auto cut = calcBell (fc, -9.0f, 2.0f, sr);
    CHECK_NEAR (cut.getMagnitudeResponseDb (fc, sr), -9.0f, 0.05f);

    const auto lowShelf = calcLowShelf (200.0f, 8.0f, 0.707f, sr);
    CHECK_NEAR (lowShelf.getMagnitudeResponseDb (30.0f, sr), 8.0f, 0.2f);
    CHECK_NEAR (lowShelf.getMagnitudeResponseDb (5000.0f, sr), 0.0f, 0.1f);

    const auto highShelf = calcHighShelf (5000.0f, 6.0f, 0.707f, sr);
    CHECK_NEAR (highShelf.getMagnitudeResponseDb (15000.0f, sr), 6.0f, 0.2f);
    CHECK_NEAR (highShelf.getMagnitudeResponseDb (100.0f, sr), 0.0f, 0.1f);
}

TEST (gateAction)
{
    GateDsp gate;
    gate.prepare (48000.0);
    // Test signal at -10 dB (0.316) with threshold -20 dB -> gate opens
    float gOpen = 0.0f;
    for (int i = 0; i < 480; ++i)
        gOpen = gate.processGain (0.316f, true, -20.0f, -60.0f, 1.0f, 10.0f, 50.0f);
    CHECK_NEAR (gOpen, 1.0f, 0.01f);

    // Test signal at -50 dB (0.00316) with threshold -20 dB -> gate closes to range -60 dB
    float gClosed = 1.0f;
    for (int i = 0; i < 4800; ++i)
        gClosed = gate.processGain (0.00316f, true, -20.0f, -60.0f, 1.0f, 10.0f, 10.0f);
    CHECK (gainToDb (gClosed) <= -55.0f);
}

TEST (compressorAction)
{
    CompressorDsp comp;
    comp.prepare (48000.0);
    // Signal at 0 dBFS (1.0), threshold -20 dB, ratio 2:1, knee 0
    // Input is 20 dB above threshold -> target compressed output is -20 + (20 / 2) = -10 dB
    // Target gain reduction = 10 dB
    float grDb = 0.0f;
    float g = 1.0f;
    for (int i = 0; i < 4800; ++i)
        g = comp.processGain (1.0f, true, -20.0f, 2.0f, 0.0f, 1.0f, 50.0f, grDb);
    CHECK_NEAR (grDb, 10.0f, 0.2f);
    CHECK_NEAR (gainToDb (g), -10.0f, 0.2f);
}

TEST (channelDspEndToEndProcessing)
{
    ChannelDspProcessor proc;
    proc.prepare (48000.0);
    ChannelDspParams params;
    ChannelDspMeters meters;

    // Enable EQ (+6 dB at 1kHz) and Compressor (-20 dB threshold)
    params.eqBypass.setBool (false);
    params.eqMid1Freq.set (1000.0f);
    params.eqMid1GainDb.set (6.0f);
    params.compBypass.setBool (false);
    params.compThreshDb.set (-20.0f);
    params.compRatio.set (4.0f);

    std::vector<float> bufL (4800, 0.1f); // -20 dBFS input
    std::vector<float> bufR (4800, 0.1f);

    proc.process (bufL.data(), bufR.data(), 2, 4800, params, meters);

    // Buffers should have been modified without NaNs or Infinities
    for (float sample : bufL)
    {
        CHECK (! std::isnan (sample));
        CHECK (! std::isinf (sample));
    }
}

TEST (eqAudibleSignalModification)
{
    const double sr = 48000.0;
    const int n = 4800; // 100 ms

    // Generate 1 kHz sine wave
    std::vector<float> sine1k (n);
    for (int i = 0; i < n; ++i)
        sine1k[i] = 0.1f * std::sin (2.0f * kPi * 1000.0f * (float) i / (float) sr);

    ChannelDspProcessor procFlat, procBoost, procCut;
    procFlat.prepare (sr);
    procBoost.prepare (sr);
    procCut.prepare (sr);

    ChannelDspParams pFlat, pBoost, pCut;
    ChannelDspMeters m;

    // Flat (0 dB)
    pFlat.eqBypass.setBool (false);
    pFlat.eqMid1Freq.set (1000.0f);
    pFlat.eqMid1GainDb.set (0.0f);

    // Boost +6 dB at 1kHz
    pBoost.eqBypass.setBool (false);
    pBoost.eqMid1Freq.set (1000.0f);
    pBoost.eqMid1GainDb.set (6.0f);

    // Cut -12 dB at 1kHz
    pCut.eqBypass.setBool (false);
    pCut.eqMid1Freq.set (1000.0f);
    pCut.eqMid1GainDb.set (-12.0f);

    auto bufFlat = sine1k;
    auto bufBoost = sine1k;
    auto bufCut = sine1k;

    procFlat.process (bufFlat.data(), nullptr, 1, n, pFlat, m);
    procBoost.process (bufBoost.data(), nullptr, 1, n, pBoost, m);
    procCut.process (bufCut.data(), nullptr, 1, n, pCut, m);

    // Measure steady state peak (last 2400 samples)
    float peakFlat = 0.0f, peakBoost = 0.0f, peakCut = 0.0f;
    for (int i = n / 2; i < n; ++i)
    {
        peakFlat = std::max (peakFlat, std::abs (bufFlat[i]));
        peakBoost = std::max (peakBoost, std::abs (bufBoost[i]));
        peakCut = std::max (peakCut, std::abs (bufCut[i]));
    }

    // Flat should remain ~0.1
    CHECK_NEAR (peakFlat, 0.1f, 0.01f);
    // +6 dB boost should be ~0.2 (2x linear)
    CHECK_NEAR (peakBoost, 0.2f, 0.02f);
    // -12 dB cut should be ~0.025 (0.25x linear)
    CHECK_NEAR (peakCut, 0.025f, 0.005f);

    // HPF test: 50 Hz sine through 200 Hz HPF
    std::vector<float> sine50 (n);
    for (int i = 0; i < n; ++i)
        sine50[i] = 0.1f * std::sin (2.0f * kPi * 50.0f * (float) i / (float) sr);

    ChannelDspProcessor procHpf;
    procHpf.prepare (sr);
    ChannelDspParams pHpf;
    pHpf.hpfBypass.setBool (false);
    pHpf.hpfFreq.set (200.0f);

    procHpf.process (sine50.data(), nullptr, 1, n, pHpf, m);
    float peakHpf = 0.0f;
    for (int i = n / 2; i < n; ++i)
        peakHpf = std::max (peakHpf, std::abs (sine50[i]));

    CHECK (peakHpf < 0.035f);
}

TEST (fxMetadataAndEnums)
{
    CHECK (getNumFxTypes() == 61);
    for (int i = 0; i < 61; ++i)
    {
        const auto meta = getFxMetadata ((FxType) i);
        CHECK (meta.name != nullptr);
        CHECK (meta.shortName != nullptr);
        CHECK (meta.categoryName != nullptr);
        CHECK (meta.hardwareEmulation != nullptr);
    }
}

TEST (reverbDspProducesDecayTail)
{
    const double sr = 48000.0;
    ReverbDsp r;
    r.prepare (sr);

    const int n = 24000; // 0.5s
    std::vector<float> bufL (n, 0.0f);
    std::vector<float> bufR (n, 0.0f);
    bufL[0] = 1.0f; bufR[0] = 1.0f; // Single unit impulse

    r.process (bufL.data(), bufR.data(), n, FxType::HallReverb, 2.5f, 0.0f, 30.0f, 70.0f, 1.0f);

    // After the impulse, reverb comb & allpass filters should have produced dense decaying tail
    float tailEnergy = 0.0f;
    for (size_t i = 1000; i < bufL.size(); ++i)
        tailEnergy += bufL[i] * bufL[i] + bufR[i] * bufR[i];

    CHECK (tailEnergy > 0.001f);
}

TEST (delayDspEchoImpulse)
{
    const double sr = 48000.0;
    DelayDsp d;
    d.prepare (sr);

    const int n = 4800; // 100 ms
    std::vector<float> bufL (n, 0.0f);
    std::vector<float> bufR (n, 0.0f);
    bufL[0] = 1.0f; bufR[0] = 1.0f; // Impulse at sample 0

    // 50 ms delay (2400 samples)
    d.process (bufL.data(), bufR.data(), n, FxType::StereoDelay, 50.0f, 50.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    // At sample 2400, delayed impulse should arrive with amplitude ~1.0
    float peakNearDelay = 0.0f;
    for (int i = 2390; i <= 2410; ++i)
        peakNearDelay = std::max (peakNearDelay, std::abs (bufL[i]));

    CHECK_NEAR (peakNearDelay, 1.0f, 0.01f);
}

TEST (graphicEqDsp31Bands)
{
    const double sr = 48000.0;
    GraphicEqDsp geq;
    geq.prepare (sr);

    const int n = 4800;
    std::vector<float> sine1k (n);
    for (int i = 0; i < n; ++i)
        sine1k[i] = 0.1f * std::sin (2.0f * kPi * 1000.0f * (float) i / (float) sr);

    std::array<Param, 31> gainsL, gainsR;
    for (auto& g : gainsL) g.init ({ -15.0f, 15.0f, 0.0f });
    for (auto& g : gainsR) g.init ({ -15.0f, 15.0f, 0.0f });

    // Boost band 17 (1000 Hz) by +12 dB (4x linear)
    gainsL[17].set (12.0f);
    gainsR[17].set (12.0f);

    auto testBufL = sine1k;
    auto testBufR = sine1k;
    geq.process (testBufL.data(), testBufR.data(), n, gainsL, gainsR, true);

    float peakOut = 0.0f;
    for (int i = n / 2; i < n; ++i)
        peakOut = std::max (peakOut, std::abs (testBufL[i]));

    // +12 dB boost on 0.1 should yield ~0.4 peak
    CHECK_NEAR (peakOut, 0.4f, 0.05f);
}

TEST (fxRackSendReturnAndInsertInMixEngine)
{
    EngineRig rig;
    rig.setDc (0, 0.2f, 0.2f, 2);
    auto& ch0 = rig.params.channels[0];
    ch0.source.set (0);

    // 1. Channel Insert with Ultimo Compressor (1176)
    ch0.insertFxSlot.set (1.0f); // Insert FX 1
    rig.params.fxSlots[0].setDefaultsForType (FxType::UltimoCompressor);
    rig.params.fxSlots[0].p1.set (60.0f); // High input drive

    rig.run (10);
    // Audio should process cleanly through 1176 insert without crashing or NaNs
    CHECK (! std::isnan (rig.L()));
    CHECK (! std::isnan (rig.R()));

    // 2. Channel Send to FX 2 (Ambience - Bus 14 / FX Send 2)
    ch0.insertFxSlot.set (0.0f); // Remove insert
    ch0.sends[13].set (0.0f);    // Send 0 dB to FX 2 (Bus 14)
    rig.params.fxSlots[1].setDefaultsForType (FxType::Ambience);

    rig.run (10);
    CHECK (rig.L() > 0.05f);
    CHECK (rig.R() > 0.05f);
}

TEST (fxSendReturnMuteAndFaderControl)
{
    EngineRig rig;
    rig.setDc (0, 0.2f, 0.2f, 2);
    auto& ch0 = rig.params.channels[0];
    ch0.source.set (0);
    ch0.mainAssign.setBool (false); // Turn off direct dry channel assignment to Main so we only hear the FX Return!
    ch0.sends[12].set (0.0f);       // Send 0 dB to FX 1 (Bus 13 / FX Send 1)
    rig.params.fxSlots[0].setDefaultsForType (FxType::Ambience);

    // Warm up reverb (50 blocks = ~0.27s)
    rig.run (50);

    // 1. With FX Return 1L/1R unmuted and at 0 dB, wet reverb should reach Main LR
    CHECK (rig.L() > 0.01f);
    CHECK (rig.R() > 0.01f);
    CHECK (rig.meters.fxReturns[0].read() > 0.01f);
    CHECK (rig.meters.fxReturns[1].read() > 0.01f);

    // 2. Muting FX Return 1L and 1R should cut reverb output completely
    rig.params.fxReturns[0].mute.setBool (true);
    rig.params.fxReturns[1].mute.setBool (true);
    rig.run (20); // Let smoothers settle
    CHECK_NEAR (rig.L(), 0.0f, 0.005f);
    CHECK_NEAR (rig.R(), 0.0f, 0.005f);

    // 3. Unmuting restores reverb
    rig.params.fxReturns[0].mute.setBool (false);
    rig.params.fxReturns[1].mute.setBool (false);
    rig.run (20);
    CHECK (rig.L() > 0.01f);
    CHECK (rig.R() > 0.01f);

    // 4. Turning FX Send down (-90 dB) silences the FX Send
    ch0.sends[12].set (-90.0f);
    // Process enough blocks for the reverb tail to ring out
    rig.run (250);
    CHECK_NEAR (rig.L(), 0.0f, 0.005f);
    CHECK_NEAR (rig.R(), 0.0f, 0.005f);
}

TEST (bus1To12AreMixBusesNotFx)
{
    EngineRig rig;
    rig.setDc (0, 0.2f, 0.2f, 2);
    auto& ch0 = rig.params.channels[0];
    ch0.source.set (0);
    ch0.mainAssign.setBool (false);

    // Turn off all FX sends (Bus 13..16)
    for (int s = 12; s < 16; ++s)
        ch0.sends[(size_t) s].set (-90.0f);

    rig.params.fxSlots[0].setDefaultsForType (FxType::Ambience);

    // Send to Bus 1 and Bus 6 (sends 0 and 5)
    ch0.sends[0].set (0.0f); // Bus 1 Send 0 dB
    ch0.sends[5].set (0.0f); // Bus 6 Send 0 dB
    rig.run (30);

    // 1. Bus 1 and Bus 6 meters must receive audio!
    CHECK (rig.meters.buses[0].read() > 0.05f);
    CHECK (rig.meters.buses[5].read() > 0.05f);

    // 2. Bus 2 (unassigned) meter must be near 0
    CHECK_NEAR (rig.meters.buses[1].read(), 0.0f, 0.001f);

    // 3. FX 1 Return must NOT receive anything from Bus 1 (Bus 1 is a mix bus, NOT FX!)
    CHECK_NEAR (rig.meters.fxReturns[0].read(), 0.0f, 0.001f);
}

TEST (multiStripModelAndParameters)
{
    MixerParams params;

    // 1. Aux (8 channels)
    CHECK_NEAR (params.auxes[0].faderDb.get(), 0.0f, 0.01f);
    CHECK_NEAR (params.auxes[0].pan.get(), 0.0f, 0.01f);
    CHECK (! params.auxes[0].mute.getBool());
    CHECK (params.auxes[0].mainAssign.getBool());
    params.auxes[2].trimDb.set (12.0f);
    CHECK_NEAR (params.auxes[2].trimDb.get(), 12.0f, 0.01f);

    // 2. FxReturns (8 channels: 1L-4R)
    CHECK_NEAR (params.fxReturns[0].pan.get(), -1.0f, 0.01f); // 1L hard left
    CHECK_NEAR (params.fxReturns[1].pan.get(), 1.0f, 0.01f);  // 1R hard right
    CHECK (params.fxReturns[0].mainAssign.getBool());

    // 3. Buses (16 channels)
    CHECK_NEAR (params.buses[0].faderDb.get(), 0.0f, 0.01f);
    for (const auto& bus : params.buses)
        CHECK (! bus.mainAssign.getBool());
    params.buses[5].dcaGroups.set (3.0f); // DCA 1 & 2
    CHECK (params.buses[5].dcaGroups.getInt() == 3);

    // 4. Matrices (6 channels)
    CHECK_NEAR (params.matrices[0].faderDb.get(), 0.0f, 0.01f);
    CHECK (! params.matrices[0].polarity.getBool());
    params.matrices[3].polarity.setBool (true);
    CHECK (params.matrices[3].polarity.getBool());
}

TEST (mainBusAndMainLrRouting)
{
    EngineRig rig;
    rig.setDc (0, 0.4f, 0.4f, 2);
    auto& ch0 = rig.params.channels[0];
    ch0.source.set (0);

    // 1. Channel direct to Main LR:
    ch0.mainAssign.setBool (true);
    rig.run (20);
    CHECK (rig.L() > 0.05f); // Sound comes out to Main LR!

    // Sending to a bus with its default assignment must not boost Main LR.
    const float directLevel = rig.L();
    ch0.sends[0].set (0.0f);
    rig.run (20);
    CHECK_NEAR (rig.L(), directLevel, 0.001f);

    // 2. Disconnect Channel from Main LR:
    ch0.mainAssign.setBool (false);
    rig.run (20);
    CHECK_NEAR (rig.L(), 0.0f, 0.001f); // Silent!

    // 3. Route Channel to Bus 1, and Bus 1 to Main LR:
    ch0.sends[0].set (0.0f); // Send 0 dB to Bus 1
    rig.params.buses[0].faderDb.set (0.0f);
    rig.params.buses[0].mainAssign.setBool (true); // Bus 1 Main LR ON
    rig.run (20);
    CHECK (rig.L() > 0.05f); // Sound from Bus 1 comes out to Main LR!

    // 4. Toggle Bus 1 Main LR OFF:
    rig.params.buses[0].mainAssign.setBool (false); // Bus 1 Main LR OFF
    rig.run (20);
    CHECK_NEAR (rig.L(), 0.0f, 0.001f); // Silent again!
}

TEST (authenticHardwareBrandFxModeling)
{
    const double sr = 48000.0;

    // 1. Roland Dimension D: spatial decorrelation and width
    {
        ModulationDsp mod;
        mod.prepare (sr);
        std::vector<float> L (512, 0.5f), R (512, 0.5f);
        mod.process (L.data(), R.data(), 512, FxType::DimensionC, 0.25f, 50.0f, 0.0f, 0.0f, 1.0f);
        // Antiphase BBD lines produce non-zero difference between Left and Right
        float diffEnergy = 0.0f;
        for (size_t i = 50; i < L.size(); ++i)
            diffEnergy += std::abs (L[i] - R[i]);
        CHECK (diffEnergy > 0.001f);
    }

    // 2. Pultec EQP-1A: Low-End Trick boosts bass and sculpts lower-mids
    {
        PremiumTubeEqDsp eq;
        eq.prepare (sr);
        // Test with 60 Hz tone
        std::vector<float> L (1024, 0.0f), R (1024, 0.0f);
        for (size_t i = 0; i < L.size(); ++i)
            L[i] = R[i] = std::sin (2.0f * kPi * 60.0f * (float) i / (float) sr);
        // Pultec Low Boost (6dB) + Low Atten (4dB) at 60 Hz
        eq.process (L.data(), R.data(), 1024, FxType::StereoXtecEQ1, 6.0f, 60.0f, 4.0f, 0.0f, 0.0f, 8000.0f, 1.0f);
        float peak = 0.0f;
        for (float s : L) peak = std::max (peak, std::abs (s));
        CHECK (peak > 1.05f); // Boosted bass!
    }

    // 3. Urei 1176LN: All-Buttons-In mode produces heavy FET compression and saturation
    {
        PremiumDynamicsDsp dyn;
        dyn.prepare (sr);
        std::vector<float> L (2048, 0.8f), R (2048, 0.8f);
        // Ratio = 24 (All-In mode), Input = 45, Output = 20
        dyn.process (L.data(), R.data(), 2048, FxType::UltimoCompressor, 45.0f, 20.0f, 24.0f, 10.0f, 1.0f);
        float compressedPeak = 0.0f;
        for (size_t i = 500; i < L.size(); ++i)
            compressedPeak = std::max (compressedPeak, std::abs (L[i]));
        CHECK (compressedPeak < 0.6f); // Heavy FET limiting!
    }

    // 4. BBE Sonic Maximizer: low band phase delay
    {
        PremiumTubeEqDsp bbe;
        bbe.prepare (sr);
        std::vector<float> L (512, 0.0f), R (512, 0.0f);
        L[0] = R[0] = 1.0f; // Impulse
        bbe.process (L.data(), R.data(), 512, FxType::SoundMaxer, 20.0f, 20.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        // Verify signal was processed without crashing or nan
        CHECK (! std::isnan (L[100]));
    }
}

TEST (busInsertFxProcessing)
{
    EngineRig rig;
    rig.setDc (0, 0.3f, 0.3f, 2);
    auto& ch0 = rig.params.channels[0];
    ch0.source.set (0);
    ch0.mainAssign.setBool (false);

    // Route Channel 0 to Bus 1 (send 0 dB)
    ch0.sends[0].set (0.0f);
    auto& bus0 = rig.params.buses[0];
    bus0.faderDb.set (0.0f);
    bus0.mainAssign.setBool (true);

    // 1. Without insert FX: normal sound to Main LR
    rig.run (20);
    const float dryLevel = rig.L();
    CHECK (dryLevel > 0.05f);

    // 2. Insert FX Slot 1 on Bus 1:
    rig.params.fxSlots[0].setDefaultsForType (FxType::StereoGuitarAmp);
    bus0.insertFxSlot.set (1.0f); // Insert FX Slot 1 on Bus 1
    rig.run (20);
    const float insertedLevel = rig.L();
    CHECK (insertedLevel > 0.05f); // Bus Insert FX successfully processes audio!

    // 3. Unassign insert FX (0):
    bus0.insertFxSlot.set (0.0f);
    rig.run (20);
    CHECK_NEAR (rig.L(), dryLevel, 0.01f);
}

TEST (all61FxModelsProcessDefaultAndNormalizedParameters)
{
    const double sr = 48000.0;
    const int N = 32768; // ~680ms so delays/reverbs with 100% wet mix output their delayed reflections
    std::vector<float> inL (N, 0.0f), inR (N, 0.0f);
    for (int i = 0; i < N; ++i)
    {
        const float t = (float) i / (float) sr;
        inL[(size_t) i] = 0.35f * std::sin (2.0f * kPi * 220.0f * t) + 0.15f * std::sin (2.0f * kPi * 1200.0f * t);
        inR[(size_t) i] = 0.35f * std::cos (2.0f * kPi * 220.0f * t) + 0.15f * std::sin (2.0f * kPi * 1500.0f * t);
    }

    for (int idx = 0; idx < (int) FxType::NumFxTypes; ++idx)
    {
        const auto type = (FxType) idx;
        FxSlotProcessor proc;
        proc.prepare (sr);

        // Mode A: Default parameters
        {
            FxSlotParams params;
            params.setDefaultsForType (type);
            params.geqGainsL[10].set (4.5f);
            params.geqGainsR[10].set (-3.5f);

            std::vector<float> outL (N, 0.0f), outR (N, 0.0f);
            proc.process (inL.data(), inR.data(), outL.data(), outR.data(), N, params);

            float energy = 0.0f;
            bool finiteOk = true;
            for (int i = 0; i < N; ++i)
            {
                if (! std::isfinite (outL[(size_t) i]) || ! std::isfinite (outR[(size_t) i]))
                    finiteOk = false;
                energy += std::abs (outL[(size_t) i]) + std::abs (outR[(size_t) i]);
            }
            if (energy <= 0.01f)
                std::printf ("  [Mode A Low Energy] idx=%d (%s) energy=%f\n", idx, getFxMetadata (type).name, energy);
            CHECK (finiteOk);
            CHECK (energy > 0.01f);
        }

        // Mode B: UI-normalized [0.0..1.0] parameters
        {
            proc.reset();
            FxSlotParams params;
            params.setDefaultsForType (type);
            params.parameterEncoding.set (2.0f);
            for (int p = 0; p < 18; ++p)
                params.parameter (p).set (0.25f);
            params.geqGainsL[15].set (6.0f);
            params.geqGainsR[15].set (-6.0f);

            std::vector<float> outL (N, 0.0f), outR (N, 0.0f);
            proc.process (inL.data(), inR.data(), outL.data(), outR.data(), N, params);

            float energy = 0.0f;
            bool finiteOk = true;
            for (int i = 0; i < N; ++i)
            {
                if (! std::isfinite (outL[(size_t) i]) || ! std::isfinite (outR[(size_t) i]))
                    finiteOk = false;
                energy += std::abs (outL[(size_t) i]) + std::abs (outR[(size_t) i]);
            }
            if (energy <= 0.01f)
                std::printf ("  [Mode B Low Energy] idx=%d (%s) energy=%f\n", idx, getFxMetadata (type).name, energy);
            CHECK (finiteOk);
            CHECK (energy > 0.01f);
        }
    }
}

TEST (sendTapsRespectFaderAndMute)
{
    EngineRig rig;
    rig.setDc (0, 0.2f, 0.2f, 2);
    auto& ch = rig.params.channels[0];
    ch.source.set (0); ch.mainAssign.setBool (false);
    ch.sends[0].set (0); rig.params.outputSource.set (1);
    ch.faderDb.set (-90);
    for (int tap = 0; tap < 4; ++tap)
    {
        ch.sendTaps[0].set ((float) tap); rig.run();
        CHECK_NEAR (rig.L(), 0.2, 1e-5);
    }
    ch.sendTaps[0].set (4); rig.run(); CHECK_NEAR (rig.L(), 0.0, 1e-6);
    ch.sendTaps[0].set (0); ch.mute.setBool (true); rig.run(); CHECK_NEAR (rig.L(), 0.0, 1e-6);
    ch.mute.setBool (false); ch.faderDb.set (0); ch.sends[0].set (-90);
    ch.sendTaps[0].set (5); rig.run(); CHECK_NEAR (rig.L(), 0.2, 1e-5);
}

TEST (sendTapsCaptureEqAndDynamicsStages)
{
    EngineRig rig; rig.setDc (0, 0.2f, 0.2f, 2);
    auto& ch = rig.params.channels[0]; ch.source.set (0);
    ch.sends[0].set (0); rig.params.outputSource.set (1);
    ch.dsp.eqLowGainDb.set (6.0206f);
    ch.dsp.compBypass.setBool (false); ch.dsp.compRatio.set (1);
    ch.dsp.compMakeupDb.set (6.0206f);
    ch.sendTaps[0].set (1); rig.run (60); CHECK_NEAR (rig.L(), 0.2, 0.001);
    ch.sendTaps[0].set (2); rig.run (60); CHECK_NEAR (rig.L(), 0.4, 0.001);
    ch.sendTaps[0].set (3); rig.run (60); CHECK_NEAR (rig.L(), 0.8, 0.001);
}

TEST (busDspAndMatrixRoutingReachDeviceOutput)
{
    EngineRig rig; rig.setDc (0, 0.1f, 0.1f, 2);
    auto& ch = rig.params.channels[0]; ch.source.set (0); ch.mainAssign.setBool (false); ch.sends[0].set (0);
    auto& bus = rig.params.buses[0]; bus.mainAssign.setBool (false);
    bus.dsp.eqLowGainDb.set (6.0206f); bus.matrixSends[0].set (0);
    rig.params.outputSource.set (17);
    rig.run (60); CHECK_NEAR (rig.L(), 0.2, 0.001);
    rig.params.matrices[0].dsp.eqLowGainDb.set (6.0206f);
    rig.run (60); CHECK_NEAR (rig.L(), 0.4, 0.001);
    rig.params.matrices[0].mute.setBool (true);
    rig.run(); CHECK_NEAR (rig.L(), 0.0, 1e-7);
    rig.params.matrices[0].mute.setBool (false); bus.mute.setBool (true);
    rig.run (60); CHECK_NEAR (rig.L(), 0.0, 1e-5);
}

TEST (busEqSpectrumAndCompressorMetersFollowSelectedBus)
{
    EngineRig rig;
    rig.setDc (0, 0.4f, 0.4f, 2);
    for (int i = 0; i < EngineRig::kBlock; ++i)
    {
        const float sample = 0.4f * std::sin (2.0f * 3.14159265f * i / 48.0f);
        rig.slotBuf[0][(size_t) i] = sample;
        rig.slotBuf[1][(size_t) i] = sample;
    }
    auto& ch = rig.params.channels[0];
    ch.source.set (0); ch.mainAssign.setBool (false); ch.sends[0].set (0);
    auto& bus = rig.params.buses[0];
    bus.dsp.compBypass.setBool (false); bus.dsp.compThreshDb.set (-30);
    bus.dsp.compRatio.set (10); bus.dsp.compAttackMs.set (0.1f);
    rig.engine.setRtaChannel ((int) kMaxInputChannels);
    rig.run (80);
    CHECK (rig.meters.busDetails[0].dsp.compGainReductionDb.load() > 5.0f);
    float peak = 0;
    for (auto& band : rig.meters.busDetails[0].rtaBands) peak = std::max (peak, band.read());
    CHECK (peak > 0.001f);
    CHECK_NEAR (rig.meters.busDetails[1].dsp.compGainReductionDb.load(), 0, 0.001);
    rig.engine.setRtaChannel ((int) kMaxInputChannels + 1);
    rig.run (80);
    for (auto& band : rig.meters.busDetails[1].rtaBands) CHECK_NEAR (band.read(), 0, 1e-7);
    rig.engine.setRtaChannel ((int) kMaxInputChannels);
    rig.setDc (0, 0, 0, 2); rig.run (250);
    for (auto& band : rig.meters.busDetails[0].rtaBands) CHECK (band.read() < 0.0001f);
}

TEST (matrixHasNoUnassignedMainLeak)
{
    EngineRig rig; rig.setDc (0, 0.2f, 0.2f, 2); rig.params.channels[0].source.set (0);
    rig.params.outputSource.set (17); rig.run(); CHECK_NEAR (rig.L(), 0.0, 1e-7);
    rig.params.main.matrixSends[0].set (0); rig.run(); CHECK_NEAR (rig.L(), 0.2, 1e-4);
    rig.params.main.mute.setBool (true); rig.run(); CHECK_NEAR (rig.L(), 0.0, 1e-6);
}

TEST (prepareCapacityIncludesBusAndFxBuffers)
{
    EngineRig rig; rig.setDc (0, 0.2f, 0.2f, 2); rig.params.channels[0].source.set (0);
    // prepare promises a minimum capacity of 4096 even when the device asks for 256.
    std::vector<float> l (4096), r (4096);
    std::vector<std::vector<float>> slots (64, std::vector<float> (4096, 0.2f));
    std::array<const float*, 64> pointers;
    for (int i = 0; i < 64; ++i) pointers[(size_t)i] = slots[(size_t)i].data();
    rig.params.channels[0].sends[0].set (0);
    rig.engine.process (pointers.data(), rig.slotCh, 4096, l.data(), r.data());
    CHECK (std::isfinite (l.back()));
}

TEST (pflAndAflLeaveMainMixIntact)
{
    EngineRig rig; rig.setDc (0, 0.2f, 0.2f, 2); rig.setDc (1, 0.3f, 0.3f, 2);
    auto& a = rig.params.channels[0]; a.source.set (0); a.solo.setBool (true); a.faderDb.set (-6.0206f);
    rig.params.channels[1].source.set (1);
    rig.params.monitorMode.set (1); rig.run();
    CHECK_NEAR (rig.L(), 0.2, 1e-5);
    CHECK_NEAR (rig.meters.main.l.read(), 0.4, 1e-4);
    rig.params.monitorMode.set (2); rig.run(); CHECK_NEAR (rig.L(), 0.1, 1e-5);
    rig.params.monitorDim.setBool (true); rig.run(); CHECK_NEAR (rig.L(), 0.01, 1e-5);
    rig.params.monitorMute.setBool (true); rig.run(); CHECK_NEAR (rig.L(), 0.0, 1e-7);
}

TEST (auxSourceDspAndMonoCenterOutput)
{
    EngineRig rig; rig.setDc (0, 0.2f, 0.2f, 2);
    auto& a = rig.params.auxes[0];
    rig.run(); CHECK_NEAR(rig.L(), 0, 1e-7);
    a.source.set (0); a.dsp.eqBypass.setBool(true); rig.run(); CHECK_NEAR(rig.L(),0.2,1e-5);
    a.mainAssign.setBool(false); a.monoAssign.setBool(true); rig.params.outputSource.set(23);
    rig.run(); CHECK_NEAR(rig.L(),0.2,1e-5); CHECK_NEAR(rig.R(),0.2,1e-5);
    rig.params.monoFaderDb.set(-6.0206f); rig.run(); CHECK_NEAR(rig.L(),0.1,1e-5);
    rig.params.monoMute.setBool(true); rig.run(); CHECK_NEAR(rig.L(),0.0,1e-7);
    rig.params.monoMute.setBool(false); a.mute.setBool(true); rig.run(); CHECK_NEAR(rig.L(),0.0,1e-7);
}

TEST (automixGroupsShareGainWithoutAffectingUngroupedChannels)
{
    EngineRig rig; rig.setDc(0,0.2f,0.2f,2); rig.setDc(1,0.2f,0.2f,2);
    for (int c = 0; c < 2; ++c) { auto& p = rig.params.channels[(size_t)c]; p.source.set((float)c); p.automixGroup.set(1); p.dsp.eqBypass.setBool(true); }
    rig.run(); CHECK_NEAR(rig.L(),0.4,1e-5);
    rig.params.automixEnabled[0].setBool(true); rig.run(60); CHECK_NEAR(rig.L(),0.2,1e-4);
    rig.params.channels[1].mute.setBool(true); rig.run(60); CHECK_NEAR(rig.L(),0.2,1e-4);
    rig.params.automixEnabled[0].setBool(false); rig.run(); CHECK_NEAR(rig.L(),0.2,1e-4);
}

TEST (fxReturnDspAudibleEqAndFilters)
{
    EngineRig rig;
    rig.setDc (0, 0.2f, 0.2f, 2);
    auto& ch0 = rig.params.channels[0];
    ch0.source.set (0);
    ch0.mainAssign.setBool (false); // Only listen via FX return
    ch0.sends[12].set (0.0f);        // Send 0 dB to FX 1 (Bus 13)
    rig.params.fxSlots[0].setDefaultsForType (FxType::Ambience);

    // Warm up reverb (50 blocks = ~0.27s)
    rig.run (50);
    const float rawReturn = rig.L();
    CHECK (rawReturn > 0.01f);

    // Mute FX return Left and Right -> output drops to zero
    rig.params.fxReturns[0].mute.setBool (true);
    rig.params.fxReturns[1].mute.setBool (true);
    rig.run (20);
    CHECK_NEAR (rig.L(), 0.0f, 0.005f);
    rig.params.fxReturns[0].mute.setBool (false);
    rig.params.fxReturns[1].mute.setBool (false);
    rig.run (20);
    CHECK (rig.L() > 0.01f);

    // Enable High Pass Filter at 400 Hz on both FX Returns -> modifies audio energy
    rig.params.fxReturns[0].dsp.hpfBypass.setBool (false);
    rig.params.fxReturns[0].dsp.hpfFreq.set (400.0f);
    rig.params.fxReturns[1].dsp.hpfBypass.setBool (false);
    rig.params.fxReturns[1].dsp.hpfFreq.set (400.0f);
    rig.run (30);
    const float filteredReturn = rig.L();
    CHECK (std::abs (filteredReturn - rawReturn) > 0.0005f);
}

TEST (mixerParamsResetToDefaults)
{
    MixerParams p;
    p.channels[0].faderDb.set (-25.0f);
    p.channels[0].mute.setBool (true);
    p.fxSlots[0].fxType.set ((float) FxType::GuitarAmp);
    p.buses[0].faderDb.set (-12.0f);

    p.resetToDefaults();

    CHECK_NEAR (p.channels[0].faderDb.get(), 0.0f, 1e-5);
    CHECK (! p.channels[0].mute.getBool());
    CHECK (p.fxSlots[0].fxType.getInt() == (int) FxType::HallReverb);
    CHECK (p.fxSlots[2].fxType.getInt() == (int) FxType::StereoDelay);
    CHECK (p.fxSlots[3].fxType.getInt() == (int) FxType::StereoChorus);
    CHECK_NEAR (p.buses[0].faderDb.get(), 0.0f, 1e-5);
}

TEST (outputProtectionPreservesCleanAudioAndStereoOverload)
{
    OutputProtection protection;
    protection.prepare (48000);
    float l[] = { 0.0f, 0.5f, -0.9f, 1.0f };
    float r[] = { 0.2f, -0.4f, 0.7f, -1.0f };
    protection.process (l, r, 4);
    CHECK_NEAR (l[1], 0.5f, 0);
    CHECK_NEAR (l[2], -0.9f, 0);
    CHECK_NEAR (r[3], -1.0f, 0);
    for (int i = 0; i < 48000; ++i)
    {
        float a = 4.0f * std::sin (i * 0.13f), b = a * 0.25f;
        protection.process (&a, &b, 1);
        CHECK (std::abs (a) <= 1.000001f);
        CHECK_NEAR (b, a * 0.25f, 1e-7);
    }
    for (int i = 0; i < 96000; ++i)
    {
        float a = 0.5f, b = 0.25f;
        protection.process (&a, &b, 1);
        if (i == 95999) CHECK_NEAR (a, 0.5f, 0.0001f);
    }
}

TEST (neutralEqIsTransparent)
{
    ChannelDspProcessor dsp;
    ChannelDspParams params;
    ChannelDspMeters meters;
    dsp.prepare (48000);
    std::vector<float> l (512), r (512);
    for (int i = 0; i < 512; ++i) { l[i] = std::sin (i * 0.31f); r[i] = std::cos (i * 0.17f); }
    const auto expectedL = l, expectedR = r;
    dsp.process (l.data(), r.data(), 2, 512, params, meters);
    for (int i = 0; i < 512; ++i)
    {
        CHECK_NEAR (l[i], expectedL[i], 0);
        CHECK_NEAR (r[i], expectedR[i], 0);
    }
}

int main() { return mpstest::runAll ("MixPracticeCoreTests"); }



