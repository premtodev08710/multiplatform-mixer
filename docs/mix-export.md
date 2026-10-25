# Export a finished mix

Open **Recorder**, choose **WAV 24-bit** or **MP3 320 kbps**, set the additional FX tail (0–30 seconds; default 3), then choose **EXPORT FULL MIX…** and a new filename.

The exporter renders the entire song from frame zero through the longest loaded track, followed by the selected tail. It uses a snapshot of Main LR mixing parameters, including channel/Bus DSP, faders, pan, mute, groups and FX. Solo, monitor level/mute/dim/mono, output audition selection, transport position and loop boundaries are excluded. Playback and live mixer controls are not changed. Rendering runs in the background with progress and cancellation.

WAV is stereo PCM 24-bit at the current device sample rate. MP3 is stereo 320 kbps and requires an installed LAME executable: standard Homebrew/system locations on macOS/Linux or a sibling `lame.exe` on Windows. This update does not bundle or install LAME. WAV export remains available without it; MP3 was tested with the encoder installed on this Mac.

Existing destination files are preserved. Failed/cancelled exports remove temporary output. If peaks reach 0 dBFS, completion reports clipping; reduce the mix level and export again. Tail length is explicit rather than automatically detected, so extend it for long reverbs or delays.

This differs from **RECORD STEREO WAV…**, which captures the monitored output in real time, including Solo and Monitor controls.

Validation on macOS: native app builds; integration tests export/read back stereo WAV and MP3, verify WAV frame count/sample rate and sample levels across unequal track lengths and the tail, confirm settings are snapshotted without changing playback/audition state, preserve existing destinations, and cancel without publishing a file. MP3 decode uses the same enabled JUCE software decoder as the production app. The combined console/recording/export integration run reports zero failures.
