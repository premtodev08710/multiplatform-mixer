// Mix Practice Studio - Realtime LED Segment Audio Meter Component
#pragma once

#include "../Core/MixerParams.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace mps
{

class LedMeter : public juce::Component, private juce::Timer
{
public:
    enum class Mode { Mono, Stereo };

    LedMeter (Mode mode = Mode::Mono);
    ~LedMeter() override;

    void setMeters (AtomicPeak* leftPeak, AtomicPeak* rightPeak, std::atomic<bool>* clipFlag);
    void resetClip();

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;

private:
    void timerCallback() override;

    void drawSegmentedMeter (juce::Graphics& g, juce::Rectangle<float> area, float levelDb, float peakHoldDb);

    Mode mode = Mode::Mono;
    AtomicPeak* peakL = nullptr;
    AtomicPeak* peakR = nullptr;
    std::atomic<bool>* clip = nullptr;

    float currentDbL = -100.0f;
    float currentDbR = -100.0f;
    float peakHoldDbL = -100.0f;
    float peakHoldDbR = -100.0f;
    int peakHoldTimeL = 0;
    int peakHoldTimeR = 0;

    bool clipLatched = false;
};

} // namespace mps
