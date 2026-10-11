// Core DSP / transport tests. Build: see CMakeLists (target MixPracticeCoreTests) or
//   clang++ -std=c++17 -O2 -ISource Tests/CoreTests.cpp Source/Core/*.cpp -o build/core_tests
#include "TestHarness.h"
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
    CHECK (getNumFxTypes() == 41);
    for (int i = 0; i < 41; ++i)
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

    // 2. Channel Send to FX 2 (Ambience)
    ch0.insertFxSlot.set (0.0f); // Remove insert
    ch0.sends[1].set (0.0f);     // Send 0 dB to FX 2
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
    ch0.sends[0].set (0.0f);        // Send 0 dB to FX 1
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
    ch0.sends[0].set (-90.0f);
    // Process enough blocks for the reverb tail to ring out
    rig.run (150);
    CHECK_NEAR (rig.L(), 0.0f, 0.005f);
    CHECK_NEAR (rig.R(), 0.0f, 0.005f);
}

int main() { return mpstest::runAll ("MixPracticeCoreTests"); }


