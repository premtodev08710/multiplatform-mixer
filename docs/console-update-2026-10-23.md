# Console update — 23 October 2026

This is an implementation update toward the X32 workflows inspected in this chat. It is not full X32 feature or sound parity. The comparison report describes the state before these changes.

## Implemented

- Input sends now use actual Input/Low Cut, Pre EQ, Post EQ, Pre Fader, Post Fader or Group taps. Each adjacent send pair shares its tap. Pre-fader taps remain independent of channel/DCA fader level; channel/group mute still silences sends. Group ignores the send level and follows the channel fader.
- Bus DSP and Matrix DSP now process their stored channel DSP parameters. The shared processor currently has four EQ bands, not the six-band X32 bus EQ.
- Bus 13–16 process through their bus master/DSP before feeding FX 1–4 in send/return mode, instead of using a separate unprocessed feed.
- Routing tab now adjusts six post-fader Matrix sends from Main LR or any Bus. Matrix inputs start disconnected, rather than copying Main into every Matrix. Matrix polarity, balance, mute and fader affect its output.
- Device output pair can audition Main LR, Bus 1–16 or Matrix 1–6. This is one stereo output pair, not a full multichannel hardware patchbay.
- Monitor tab provides solo-in-place, PFL, AFL, monitor level, mono, dim and mute. PFL/AFL do not cut the Main mix. Monitor uses the selected device output pair; a dedicated physical headphone output is not implemented.
- Recorder tab writes the selected monitored stereo output to 24-bit WAV at the current device sample rate. A background writer handles disk I/O; the audio callback never waits for the recording control lock. Dropped blocks are reported. Existing files are not overwritten. This is stereo recording, not X-LIVE/multitrack capture.
- AutoMix X/Y now enables gain sharing for channels assigned in Detail, with per-channel weighting. This is an in-app RMS-based gain-sharing implementation, not a claim of reproducing the console's proprietary algorithm.
- Aux strips now consume a selected track (or OFF), process DSP, trim/polarity, groups, fader/pan, solo and Main/Mono assignments. They do not capture physical Aux/USB hardware inputs.
- Mono/Center now has real summing, a master fader/mute wired to the MainC strip, metering and device-output selection. LCR panning is not implemented.
- Meters tab displays 32 channels, 16 buses, six matrices, eight FX returns, eight Aux and Main L/R/C.
- Project/scene state now includes all current channel/Bus/Aux/Matrix/return/DCA scalar parameters and DSP, Matrix sends, insert targets, output/monitor selection and active mute groups. Legacy snapshots continue to load. Existing FX parameters, GEQ and channel links retain their existing serialization.
- Fixed seven Detail controls (delay, low cut and Gate parameters) that previously edited an input channel even when a different strip was selected.
- Fixed inconsistent scratch-buffer capacity when prepare() is called with a block smaller than the engine's 4096-sample minimum.

## Validation

- Core suite: 46/46 tests, 10,618 checks, zero failures.
- The same 46 tests pass under AddressSanitizer and UndefinedBehaviorSanitizer.
- Scene JSON round trip: 4,590 parameter values checked, zero failures; active mute groups and legacy snapshot loading checked separately.
- Recorder test: WAV reopens as stereo 48 kHz, 2,048 samples; existing-file protection checked.
- Rendered the four new tool pages at widths 640 and 1024; visually checked Routing and Monitor.
- Native macOS application builds successfully. Other platforms and a connected X32 have not been tested in this update.

## Remaining for broader parity

LCR, live hardware Aux inputs, six-band Bus EQ, advanced Gate/Compressor detector and external-key modes, Talkback, Oscillator, full physical routing, scene safes/recall scope, and complete bidirectional OSC control remain outstanding. The existing UI for these features must not be treated as proof of working DSP.

The newly built native macOS application is in `build-mac/MixPracticeStudio_artefacts/Release/Mix Practice Studio.app`. The existing iOS-wrapped application in `/Applications` has not been replaced.
