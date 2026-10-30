// Mix Practice Studio - Multitrack Transport Control Bar Implementation
#include "TransportBar.h"
#include "ConsoleLookAndFeel.h"
#include <cmath>

namespace mps
{

static juce::String formatTime (int64_t frames, double sr)
{
    if (sr <= 0.0 || frames <= 0) return "00:00.00";
    const double totalSecs = (double) frames / sr;
    const int mins = (int) (totalSecs / 60.0);
    const double secs = std::fmod (totalSecs, 60.0);
    return juce::String::formatted ("%02d:%05.2f", mins, secs);
}

TransportBar::TransportBar (AudioEngine& eng) : engine (eng)
{
    // 1. Play / Pause / Stop Buttons
    playButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2e7d32));
    playButton.addListener (this);
    addAndMakeVisible (playButton);

    pauseButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xfff57f17));
    pauseButton.addListener (this);
    addAndMakeVisible (pauseButton);

    stopButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xffc62828));
    stopButton.addListener (this);
    addAndMakeVisible (stopButton);

    loopButton.setClickingTogglesState (true);
    loopButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffffb74d));
    loopButton.addListener (this);
    addAndMakeVisible (loopButton);

    // 2. Section Selection Buttons
    setAButton.setTooltip ("Set Section Start [ A ] at current playhead");
    setAButton.addListener (this);
    addAndMakeVisible (setAButton);

    setBButton.setTooltip ("Set Section End [ B ] at current playhead");
    setBButton.addListener (this);
    addAndMakeVisible (setBButton);

    allRangeButton.setTooltip ("Select Entire Song Range");
    allRangeButton.addListener (this);
    addAndMakeVisible (allRangeButton);

    sectionMenuButton.setTooltip ("Choose song section preset (Intro, Verse, Chorus, Solo...)");
    sectionMenuButton.addListener (this);
    addAndMakeVisible (sectionMenuButton);

    // Save Button
    saveButton.setTooltip (juce::String::fromUTF8 ("Save current project & tracks (บันทึกโปรเจกต์และการตั้งค่า)"));
    saveButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff00838f)); // Cyan
    saveButton.addListener (this);
    addAndMakeVisible (saveButton);

    // 3. Interactive Timeline Range
    timeline.onSeek = [this] (double normPos) {
        engine.seekToNormalized ((float) normPos);
    };

    timeline.onRangeChanged = [this] (double startNorm, double endNorm) {
        const int64_t len = engine.getStreamer().getLength();
        if (len > 0)
        {
            const int64_t a = (int64_t) std::round (startNorm * (double) len);
            const int64_t b = (int64_t) std::round (endNorm * (double) len);
            engine.setLoop (loopButton.getToggleState(), a, b);
        }
    };
    addAndMakeVisible (timeline);

    // 4. Time Readout Labels
    timeLabel.setFont (juce::FontOptions (12.5f, juce::Font::bold));
    timeLabel.setColour (juce::Label::textColourId, ConsoleLookAndFeel::accentCyan);
    timeLabel.setText ("00:00.00 / 00:00.00", juce::dontSendNotification);
    addAndMakeVisible (timeLabel);

    rangeLabel.setFont (juce::FontOptions (10.5f));
    rangeLabel.setColour (juce::Label::textColourId, juce::Colour (0xffffb74d)); // Amber
    rangeLabel.setText ("A: 00:00.00  B: 00:00.00", juce::dontSendNotification);
    addAndMakeVisible (rangeLabel);

    statusLabel.setFont (juce::FontOptions (10.0f));
    statusLabel.setColour (juce::Label::textColourId, ConsoleLookAndFeel::textMuted);
    statusLabel.setText ("48.0 kHz", juce::dontSendNotification);
    addAndMakeVisible (statusLabel);

    cpuLabel.setFont (juce::FontOptions (10.0f));
    cpuLabel.setColour (juce::Label::textColourId, ConsoleLookAndFeel::textMuted);
    cpuLabel.setText ("CPU: 0%", juce::dontSendNotification);
    addAndMakeVisible (cpuLabel);

    startTimerHz (25); // 25 Hz update for silky smooth playhead needle
}

TransportBar::~TransportBar()
{
    stopTimer();
}

void TransportBar::setLoopRange (double startNorm, double endNorm)
{
    timeline.setRange (startNorm, endNorm, true);
}

void TransportBar::timerCallback()
{
    auto& s = engine.getStreamer();
    const bool isPlaying = s.isPlaying();
    const int64_t ph = s.getPlayhead();
    const int64_t len = s.getLength();
    const double sr = engine.getSampleRate();

    playButton.setToggleState (isPlaying, juce::dontSendNotification);

    if (len > 0)
    {
        timeline.setTotalLength (len, sr);
        timeline.setPlayhead ((double) ph / (double) len);
    }
    else
    {
        timeline.setTotalLength (0, sr);
        timeline.setPlayhead (0.0);
    }

    // Time text
    timeLabel.setText (formatTime (ph, sr) + " / " + formatTime (len, sr), juce::dontSendNotification);

    // Range readout
    if (len > 0)
    {
        const int64_t a = (int64_t) std::round (timeline.getRangeStart() * (double) len);
        const int64_t b = (int64_t) std::round (timeline.getRangeEnd() * (double) len);
        const int64_t span = b - a;
        rangeLabel.setText ("[" + formatTime (a, sr) + " - " + formatTime (b, sr) + "] (" + formatTime (span, sr) + ")",
                            juce::dontSendNotification);
    }
    else
    {
        rangeLabel.setText ("No Tracks Loaded", juce::dontSendNotification);
    }

    // Buffer, device, sample rate and buffer size readout
    const float bufFill = s.getBufferFill();
    auto* dev = engine.getDeviceManager().getCurrentAudioDevice();
    const double curSr = engine.getSampleRate();
    const int curBs = dev != nullptr ? dev->getCurrentBufferSizeSamples() : engine.getBufferSize();
    const juce::String srTxt = curSr > 1000.0 ? (juce::String (curSr * 0.001, 1) + " kHz") : (juce::String ((int) curSr) + " Hz");
    statusLabel.setText (srTxt + " | " + juce::String (curBs) + " spl | Buf: " + juce::String ((int) (bufFill * 100.0f)) + "%",
                         juce::dontSendNotification);
    if (dev != nullptr)
        statusLabel.setTooltip ("Device: " + dev->getName() + " | Sample Rate: " + juce::String ((int) curSr) + " Hz | Buffer Size: " + juce::String (curBs) + " samples");

    const double cpu = engine.getDeviceManager().getCpuUsage();
    cpuLabel.setText ("CPU: " + juce::String ((int) std::lround (cpu * 100.0)) + "%", juce::dontSendNotification);
}

void TransportBar::buttonClicked (juce::Button* b)
{
    if (b == &playButton)
    {
        const int64_t len = engine.getStreamer().getLength();
        if (len <= 0)
        {
            rangeLabel.setText ("No Tracks Loaded - Import audio first", juce::dontSendNotification);
            return;
        }

        const int64_t ph = engine.getStreamer().getPlayhead();
        if (ph >= len || engine.getStreamer().consumeReachedEnd())
            engine.seekToSample (0);
        engine.play();
    }
    else if (b == &pauseButton)
    {
        engine.pause();
    }
    else if (b == &stopButton)
    {
        engine.stop();
    }
    else if (b == &loopButton)
    {
        const bool on = loopButton.getToggleState();
        timeline.setLoopActive (on);

        const int64_t len = engine.getStreamer().getLength();
        if (len > 0)
        {
            const int64_t a = (int64_t) std::round (timeline.getRangeStart() * (double) len);
            const int64_t b = (int64_t) std::round (timeline.getRangeEnd() * (double) len);
            engine.setLoop (on, a, b);

            if (on)
            {
                const int64_t ph = engine.getStreamer().getPlayhead();
                if (ph < a || ph >= b)
                    engine.seekToSample (a);
            }
        }
    }
    else if (b == &setAButton)
    {
        const int64_t len = engine.getStreamer().getLength();
        if (len > 0)
        {
            const int64_t ph = engine.getStreamer().getPlayhead();
            const double norm = juce::jlimit (0.0, timeline.getRangeEnd() - 0.005, (double) ph / (double) len);
            timeline.setRange (norm, timeline.getRangeEnd(), true);
        }
    }
    else if (b == &setBButton)
    {
        const int64_t len = engine.getStreamer().getLength();
        if (len > 0)
        {
            const int64_t ph = engine.getStreamer().getPlayhead();
            const double norm = juce::jlimit (timeline.getRangeStart() + 0.005, 1.0, (double) ph / (double) len);
            timeline.setRange (timeline.getRangeStart(), norm, true);
        }
    }
    else if (b == &allRangeButton)
    {
        timeline.setRange (0.0, 1.0, true);
    }
    else if (b == &sectionMenuButton)
    {
        showSectionPresetsMenu();
    }
    else if (b == &saveButton)
    {
        if (onQuickSave)
            onQuickSave();
    }
}

void TransportBar::showSectionPresetsMenu()
{
    juce::PopupMenu m;
    m.addItem (1, "Full Song (100%)");
    m.addSeparator();
    m.addItem (2, "Intro (0% - 25%)");
    m.addItem (3, "Verse 1 (25% - 50%)");
    m.addItem (4, "Chorus (50% - 75%)");
    m.addItem (5, "Outro / Solo (75% - 100%)");
    m.addSeparator();
    m.addItem (6, "First Half (0% - 50%)");
    m.addItem (7, "Second Half (50% - 100%)");
    m.addSeparator();
    m.addItem (8, "Loop 15 Seconds around Playhead");
    m.addItem (9, "Loop 30 Seconds around Playhead");
    m.addItem (10, "Loop 60 Seconds around Playhead");

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&sectionMenuButton),
                     [this] (int res) {
                         const int64_t len = engine.getStreamer().getLength();
                         const double sr = engine.getSampleRate();
                         if (len <= 0 || sr <= 0.0) return;

                         if (res == 1) setLoopRange (0.0, 1.0);
                         else if (res == 2) setLoopRange (0.0, 0.25);
                         else if (res == 3) setLoopRange (0.25, 0.50);
                         else if (res == 4) setLoopRange (0.50, 0.75);
                         else if (res == 5) setLoopRange (0.75, 1.00);
                         else if (res == 6) setLoopRange (0.0, 0.50);
                         else if (res == 7) setLoopRange (0.50, 1.00);
                         else if (res >= 8 && res <= 10)
                         {
                             const double durSecs = (res == 8 ? 15.0 : (res == 9 ? 30.0 : 60.0));
                             const double curSecs = (double) engine.getStreamer().getPlayhead() / sr;
                             const double totalSecs = (double) len / sr;
                             const double startSecs = std::max (0.0, curSecs - (durSecs * 0.25));
                             const double endSecs = std::min (totalSecs, startSecs + durSecs);
                             setLoopRange (startSecs / totalSecs, endSecs / totalSecs);
                         }

                         // Auto-enable loop if preset picked
                         if (res > 0 && ! loopButton.getToggleState())
                         {
                             loopButton.setToggleState (true, juce::sendNotificationSync);
                         }
                     });
}

void TransportBar::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (ConsoleLookAndFeel::bgPanel);
    g.fillRect (bounds);
    g.setColour (ConsoleLookAndFeel::borderSubtle);
    g.drawHorizontalLine (0, 0.0f, bounds.getWidth());
}

void TransportBar::resized()
{
    auto area = getLocalBounds().reduced (8, 4);

    // Row 1: Interactive Song Timeline & Range Selector (enlarged for touch and zooming)
    const int timelineH = std::max (24, std::min (34, area.getHeight() - 28));
    timeline.setBounds (area.removeFromTop (timelineH));

    area.removeFromTop (4);

    // Row 2: Transport Controls, Range Buttons & Status Display
    auto row2 = area;

    // Use a tighter, width-aware row on small phones. Keep every control
    // present while reserving enough room for the time and status readouts.
    const bool compact = getWidth() < 1000;
    const int btnW = compact ? 44 : 54;
    playButton.setBounds (row2.removeFromLeft (btnW));
    row2.removeFromLeft (4);
    pauseButton.setBounds (row2.removeFromLeft (btnW));
    row2.removeFromLeft (4);
    stopButton.setBounds (row2.removeFromLeft (btnW));
    row2.removeFromLeft (4);
    loopButton.setBounds (row2.removeFromLeft (btnW));
    row2.removeFromLeft (compact ? 5 : 10);

    // Section markers
    const int markerW = compact ? 30 : 36;
    setAButton.setBounds (row2.removeFromLeft (markerW));
    row2.removeFromLeft (2);
    setBButton.setBounds (row2.removeFromLeft (markerW));
    row2.removeFromLeft (2);
    allRangeButton.setBounds (row2.removeFromLeft (compact ? 34 : 40));
    row2.removeFromLeft (3);
    sectionMenuButton.setBounds (row2.removeFromLeft (compact ? 62 : 78));
    row2.removeFromLeft (compact ? 3 : 8);

    // Save Button
    saveButton.setBounds (row2.removeFromLeft (compact ? 44 : 54));
    row2.removeFromLeft (compact ? 4 : 12);

    // Right-aligned status (CPU %, Sample Rate, Buffer Size, Ring Buffer Fill %)
    cpuLabel.setBounds (row2.removeFromRight (compact ? 42 : 55));
    row2.removeFromRight (3);
    statusLabel.setBounds (row2.removeFromRight (compact ? 130 : 180));
    row2.removeFromRight (compact ? 4 : 8);

    // Middle time & range labels
    const int timeW = compact ? 112 : 150;
    timeLabel.setBounds (row2.removeFromLeft (timeW));
    row2.removeFromLeft (compact ? 3 : 8);
    rangeLabel.setBounds (row2);
}

} // namespace mps
