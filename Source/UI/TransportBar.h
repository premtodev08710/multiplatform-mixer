// Mix Practice Studio - Multitrack Transport Control Bar Component
#pragma once

#include "../Audio/AudioEngine.h"
#include "TimelineRangeComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace mps
{

class TransportBar : public juce::Component,
                     public juce::Button::Listener,
                     private juce::Timer
{
public:
    TransportBar (AudioEngine& engine);
    ~TransportBar() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void buttonClicked (juce::Button* button) override;

    void setLoopRange (double startNorm, double endNorm);

private:
    void timerCallback() override;
    void showSectionPresetsMenu();

    AudioEngine& engine;

    // Timeline & Section Selector
    TimelineRangeComponent timeline;

    // Transport buttons
    juce::TextButton playButton { "PLAY" };
    juce::TextButton pauseButton { "PAUSE" };
    juce::TextButton stopButton { "STOP" };
    juce::TextButton loopButton { "LOOP" };

    // Section Range buttons
    juce::TextButton setAButton { "[ A ]" };
    juce::TextButton setBButton { "[ B ]" };
    juce::TextButton allRangeButton { "ALL" };
    juce::TextButton sectionMenuButton { juce::CharPointer_UTF8 ("SECTION \xe2\x96\xbc") };

    // Time & Range Labels
    juce::Label timeLabel;
    juce::Label rangeLabel;
    juce::Label statusLabel;
    juce::Label cpuLabel;
};

} // namespace mps
