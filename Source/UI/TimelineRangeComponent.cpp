// Mix Practice Studio - Interactive Timeline & Song Section Range Selector Implementation
#include "TimelineRangeComponent.h"
#include <cmath>

namespace mps
{

TimelineRangeComponent::TimelineRangeComponent()
{
    setMouseCursor (juce::MouseCursor::NormalCursor);
}

void TimelineRangeComponent::setPlayhead (double normPos)
{
    const double p = juce::jlimit (0.0, 1.0, normPos);
    if (std::abs (playheadPos - p) > 0.0001)
    {
        playheadPos = p;
        repaint();
    }
}

void TimelineRangeComponent::setRange (double startNorm, double endNorm, bool notify)
{
    rangeStart = juce::jlimit (0.0, 0.999, startNorm);
    rangeEnd = juce::jlimit (rangeStart + 0.001, 1.0, endNorm);
    repaint();

    if (notify && onRangeChanged)
        onRangeChanged (rangeStart, rangeEnd);
}

void TimelineRangeComponent::setLoopActive (bool active)
{
    if (loopActive != active)
    {
        loopActive = active;
        repaint();
    }
}

void TimelineRangeComponent::setTotalLength (int64_t frames, double sr)
{
    totalFrames = frames;
    sampleRate = sr > 0.0 ? sr : 48000.0;
    repaint();
}

float TimelineRangeComponent::normToX (double norm) const
{
    const float w = (float) getWidth() - 16.0f;
    return 8.0f + (float) norm * std::max (10.0f, w);
}

double TimelineRangeComponent::xToNorm (float x) const
{
    const float w = (float) getWidth() - 16.0f;
    if (w <= 10.0f) return 0.0;
    return juce::jlimit (0.0, 1.0, (double) (x - 8.0f) / (double) w);
}

void TimelineRangeComponent::mouseMove (const juce::MouseEvent& e)
{
    const float x = e.position.x;
    const float xs = normToX (rangeStart);
    const float xe = normToX (rangeEnd);

    if (std::abs (x - xs) <= kHandleHitWidth || std::abs (x - xe) <= kHandleHitWidth)
    {
        setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
    }
    else if (x > xs && x < xe && e.position.y <= 10.0f)
    {
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    }
    else
    {
        setMouseCursor (juce::MouseCursor::NormalCursor);
    }
}

void TimelineRangeComponent::mouseDown (const juce::MouseEvent& e)
{
    const float x = e.position.x;
    const float xs = normToX (rangeStart);
    const float xe = normToX (rangeEnd);

    dragStartMouseX = x;
    dragInitialStart = rangeStart;
    dragInitialEnd = rangeEnd;

    if (std::abs (x - xs) <= kHandleHitWidth)
    {
        currentDrag = DragMode::RangeStart;
    }
    else if (std::abs (x - xe) <= kHandleHitWidth)
    {
        currentDrag = DragMode::RangeEnd;
    }
    else if (x > xs && x < xe && (e.position.y <= 12.0f || e.mods.isAltDown()))
    {
        currentDrag = DragMode::RangeMove;
    }
    else
    {
        currentDrag = DragMode::Playhead;
        const double clickNorm = xToNorm (x);
        setPlayhead (clickNorm);
        if (onSeek) onSeek (clickNorm);
    }
}

void TimelineRangeComponent::mouseDrag (const juce::MouseEvent& e)
{
    const double currentNorm = xToNorm (e.position.x);

    if (currentDrag == DragMode::RangeStart)
    {
        const double newStart = juce::jlimit (0.0, rangeEnd - 0.005, currentNorm);
        setRange (newStart, rangeEnd, true);
    }
    else if (currentDrag == DragMode::RangeEnd)
    {
        const double newEnd = juce::jlimit (rangeStart + 0.005, 1.0, currentNorm);
        setRange (rangeStart, newEnd, true);
    }
    else if (currentDrag == DragMode::RangeMove)
    {
        const double delta = currentNorm - xToNorm (dragStartMouseX);
        const double span = dragInitialEnd - dragInitialStart;
        double newStart = dragInitialStart + delta;
        double newEnd = dragInitialEnd + delta;

        if (newStart < 0.0)
        {
            newStart = 0.0;
            newEnd = span;
        }
        else if (newEnd > 1.0)
        {
            newEnd = 1.0;
            newStart = 1.0 - span;
        }

        setRange (newStart, newEnd, true);
    }
    else if (currentDrag == DragMode::Playhead)
    {
        setPlayhead (currentNorm);
        if (onSeek) onSeek (currentNorm);
    }
}

void TimelineRangeComponent::mouseUp (const juce::MouseEvent&)
{
    currentDrag = DragMode::None;
}

void TimelineRangeComponent::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float h = bounds.getHeight();
    const float xLeft = normToX (0.0);
    const float xRight = normToX (1.0);
    const float trackW = xRight - xLeft;

    // 1. Trough Background
    const juce::Rectangle<float> trackRect (xLeft, 2.0f, trackW, h - 4.0f);
    g.setColour (juce::Colour (0xff101317));
    g.fillRoundedRectangle (trackRect, 3.0f);
    g.setColour (juce::Colour (0xff252c38));
    g.drawRoundedRectangle (trackRect, 3.0f, 1.0f);

    // 2. Timeline Tick Marks (time graduation)
    if (totalFrames > 0 && sampleRate > 0.0 && trackW > 80.0f)
    {
        const double totalSecs = (double) totalFrames / sampleRate;
        const double intervalSecs = totalSecs > 180.0 ? 30.0 : (totalSecs > 60.0 ? 15.0 : 5.0);
        const int numTicks = (int) (totalSecs / intervalSecs);

        g.setFont (juce::FontOptions (7.5f));
        g.setColour (juce::Colour (0xff485262));

        for (int i = 1; i <= numTicks; ++i)
        {
            const double s = (double) i * intervalSecs;
            const float tx = normToX (s / totalSecs);
            g.drawVerticalLine ((int) tx, trackRect.getY() + 3.0f, trackRect.getBottom() - 3.0f);

            if (trackW > 300.0f)
            {
                const int mins = (int) (s / 60.0);
                const int secs = (int) std::fmod (s, 60.0);
                const juce::String tickText = juce::String::formatted ("%d:%02d", mins, secs);
                g.drawText (tickText, (int) tx - 15, (int) trackRect.getBottom() - 11, 30, 10, juce::Justification::centred);
            }
        }
    }

    // 3. Highlighted Song Section Range [A - B]
    const float xs = normToX (rangeStart);
    const float xe = normToX (rangeEnd);
    const float rw = std::max (2.0f, xe - xs);
    const juce::Rectangle<float> rangeRect (xs, trackRect.getY() + 1.0f, rw, trackRect.getHeight() - 2.0f);

    const juce::Colour loopColour = loopActive ? juce::Colour (0xffffb74d) : juce::Colour (0xff00d2ff);

    // Shaded range body
    g.setColour (loopColour.withAlpha (loopActive ? 0.28f : 0.16f));
    g.fillRect (rangeRect);

    // Top grab ribbon & bottom accent line
    g.setColour (loopColour);
    g.fillRect (rangeRect.getX(), rangeRect.getY(), rangeRect.getWidth(), 3.5f);
    g.fillRect (rangeRect.getX(), rangeRect.getBottom() - 1.5f, rangeRect.getWidth(), 1.5f);

    // In-Point Handle Marker [ A ]
    const juce::Rectangle<float> handleA (xs - 5.0f, 1.0f, 10.0f, h - 2.0f);
    g.setColour (loopColour);
    g.fillRect (xs - 1.5f, 1.0f, 3.0f, h - 2.0f);
    juce::Path flagA;
    flagA.addRoundedRectangle (xs - 7.0f, 1.0f, 8.0f, 9.0f, 1.5f);
    g.fillPath (flagA);
    g.setColour (juce::Colours::black);
    g.setFont (juce::FontOptions (7.5f, juce::Font::bold));
    g.drawText ("A", (int) xs - 7, 1, 8, 9, juce::Justification::centred);

    // Out-Point Handle Marker [ B ]
    const juce::Rectangle<float> handleB (xe - 5.0f, 1.0f, 10.0f, h - 2.0f);
    g.setColour (loopColour);
    g.fillRect (xe - 1.5f, 1.0f, 3.0f, h - 2.0f);
    juce::Path flagB;
    flagB.addRoundedRectangle (xe - 1.0f, 1.0f, 8.0f, 9.0f, 1.5f);
    g.fillPath (flagB);
    g.setColour (juce::Colours::black);
    g.setFont (juce::FontOptions (7.5f, juce::Font::bold));
    g.drawText ("B", (int) xe - 1, 1, 8, 9, juce::Justification::centred);

    // 4. Playhead Needle (White/Cyan needle line)
    const float px = normToX (playheadPos);
    g.setColour (juce::Colours::white);
    g.fillRect (px - 1.0f, 0.0f, 2.0f, h);

    // Diamond cap on top of playhead
    juce::Path needleCap;
    needleCap.addTriangle (px - 4.5f, 0.0f, px + 4.5f, 0.0f, px, 7.0f);
    g.setColour (juce::Colour (0xff00e5ff));
    g.fillPath (needleCap);
    g.setColour (juce::Colours::white);
    g.strokePath (needleCap, juce::PathStrokeType (1.0f));
}

} // namespace mps
