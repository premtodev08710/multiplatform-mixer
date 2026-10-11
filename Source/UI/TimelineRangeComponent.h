// Mix Practice Studio - Interactive Timeline & Song Section Range Selector
#pragma once

#include "ConsoleLookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace mps
{

class TimelineRangeComponent : public juce::Component
{
public:
    TimelineRangeComponent();
    ~TimelineRangeComponent() override = default;

    void setPlayhead (double normPos);
    void setRange (double startNorm, double endNorm, bool notify = false);
    void setLoopActive (bool active);
    void setTotalLength (int64_t frames, double sampleRate);

    double getPlayhead() const noexcept    { return playheadPos; }
    double getRangeStart() const noexcept  { return rangeStart; }
    double getRangeEnd() const noexcept    { return rangeEnd; }
    bool isLoopActive() const noexcept     { return loopActive; }

    std::function<void (double normPos)> onSeek;
    std::function<void (double startNorm, double endNorm)> onRangeChanged;

    void paint (juce::Graphics& g) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

private:
    enum class DragMode { None, Playhead, RangeStart, RangeEnd, RangeMove };

    float normToX (double norm) const;
    double xToNorm (float x) const;

    double playheadPos = 0.0;
    double rangeStart = 0.0;
    double rangeEnd = 1.0;
    bool loopActive = false;

    int64_t totalFrames = 0;
    double sampleRate = 48000.0;

    DragMode currentDrag = DragMode::None;
    float dragStartMouseX = 0.0f;
    double dragInitialStart = 0.0;
    double dragInitialEnd = 1.0;

    static constexpr float kHandleHitWidth = 10.0f;
};

} // namespace mps
