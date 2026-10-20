// Mix Practice Studio - Interactive Timeline & Song Section Range Selector
#pragma once

#include "ConsoleLookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace mps
{

class TimelineRangeComponent : public juce::Component,
                               public juce::Button::Listener
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

    // Time Zoom & Pan Controls
    double getZoomLevel() const noexcept   { return zoomLevel; }
    void setZoomLevel (double newZoom, double centerNorm = -1.0);
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void zoomToLoopRange();

    double getViewOffset() const noexcept  { return viewOffset; }
    void setViewOffset (double newOffset);

    void setShowZoomButtons (bool show);
    void setAutoFollowPlayhead (bool follow) noexcept { autoFollowPlayhead = follow; }

    std::function<void (double normPos)> onSeek;
    std::function<void (double startNorm, double endNorm)> onRangeChanged;
    std::function<void (double zoom)> onZoomChanged;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;
    void buttonClicked (juce::Button* button) override;

private:
    enum class DragMode { None, Playhead, RangeStart, RangeEnd, RangeMove, MiniNavDrag, PanTrack };

    float getTrackX() const;
    float getTrackWidth() const;
    float getTrackY() const;
    float getTrackHeight() const;

    float normToX (double norm) const;
    double xToNorm (float x) const;

    double playheadPos = 0.0;
    double rangeStart = 0.0;
    double rangeEnd = 1.0;
    bool loopActive = false;

    int64_t totalFrames = 0;
    double sampleRate = 48000.0;

    // Zoom & Pan state
    double zoomLevel = 1.0;  // 1.0x (full song) to 32.0x (ultra detail)
    double viewOffset = 0.0; // 0.0 to 1.0 - (1.0 / zoomLevel)
    bool showZoomButtons = true;
    bool autoFollowPlayhead = true;

    DragMode currentDrag = DragMode::None;
    float dragStartMouseX = 0.0f;
    float dragStartMouseY = 0.0f;
    double dragInitialStart = 0.0;
    double dragInitialEnd = 1.0;
    double dragInitialViewOffset = 0.0;

    static constexpr float kHandleHitWidth = 20.0f; // Big touch-friendly hit zone

    // Integrated Zoom Controls
    juce::TextButton zoomOutBtn  { juce::CharPointer_UTF8 ("\xe2\x88\x92") }; // −
    juce::TextButton zoomInBtn   { "+" };
    juce::TextButton zoomFitBtn  { "1x" };
    juce::TextButton zoomLoopBtn { juce::CharPointer_UTF8 ("\xf0\x9f\x94\x8d A-B") }; // 🔍 A-B
};

} // namespace mps
