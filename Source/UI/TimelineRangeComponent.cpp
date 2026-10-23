// Mix Practice Studio - Interactive Timeline & Song Section Range Selector Implementation
#include "TimelineRangeComponent.h"
#include <cmath>

namespace mps
{

TimelineRangeComponent::TimelineRangeComponent()
{
    setMouseCursor (juce::MouseCursor::NormalCursor);

    auto setupBtn = [this] (juce::TextButton& btn, const juce::String& tip, int bgHex, int textHex) {
        btn.setTooltip (tip);
        btn.setColour (juce::TextButton::buttonColourId, juce::Colour (bgHex));
        btn.setColour (juce::TextButton::textColourOffId, juce::Colour (textHex));
        btn.addListener (this);
        addChildComponent (btn);
    };

    setupBtn (zoomOutBtn,  juce::String::fromUTF8 ("Zoom Out (ขยายเวลาออก)"),      0xdd1e2532, 0xff00d2ff);
    setupBtn (zoomInBtn,   juce::String::fromUTF8 ("Zoom In (ซูมเข้าเวลา)"),         0xdd1e2532, 0xff00d2ff);
    setupBtn (zoomFitBtn,  juce::String::fromUTF8 ("Fit 1x (แสดงทั้งเพลง)"),         0xdd1e2532, 0xffcbd5e1);
    setupBtn (zoomLoopBtn, juce::String::fromUTF8 ("Zoom to Loop A-B (ซูมเข้าลูป)"), 0xdd2e2212, 0xffffb74d);

    setShowZoomButtons (true);
}

void TimelineRangeComponent::setShowZoomButtons (bool show)
{
    showZoomButtons = show;
    zoomOutBtn.setVisible (show);
    zoomInBtn.setVisible (show);
    zoomFitBtn.setVisible (show);
    zoomLoopBtn.setVisible (show);
    resized();
}

float TimelineRangeComponent::getTrackX() const
{
    return 8.0f;
}

float TimelineRangeComponent::getTrackWidth() const
{
    return std::max (20.0f, (float) getWidth() - 16.0f);
}

float TimelineRangeComponent::getTrackY() const
{
    return 2.0f;
}

float TimelineRangeComponent::getTrackHeight() const
{
    const float navH = (zoomLevel > 1.001) ? 7.0f : 0.0f;
    return std::max (14.0f, (float) getHeight() - 4.0f - navH);
}

float TimelineRangeComponent::normToX (double norm) const
{
    const float left = getTrackX();
    const float w = getTrackWidth();
    const double visibleSpan = 1.0 / std::max (1.0, zoomLevel);
    const double viewNorm = (norm - viewOffset) / visibleSpan;
    return left + (float) viewNorm * w;
}

double TimelineRangeComponent::xToNorm (float x) const
{
    const float left = getTrackX();
    const float w = getTrackWidth();
    if (w <= 10.0f) return 0.0;
    const double viewNorm = (double) (x - left) / (double) w;
    const double visibleSpan = 1.0 / std::max (1.0, zoomLevel);
    const double norm = viewOffset + viewNorm * visibleSpan;
    return juce::jlimit (0.0, 1.0, norm);
}

void TimelineRangeComponent::setPlayhead (double normPos)
{
    const double p = juce::jlimit (0.0, 1.0, normPos);
    if (std::abs (playheadPos - p) > 0.00005)
    {
        playheadPos = p;

        if (autoFollowPlayhead && zoomLevel > 1.001 && currentDrag == DragMode::None)
        {
            const double span = 1.0 / zoomLevel;
            const double viewEnd = viewOffset + span;
            if (playheadPos > viewEnd)
            {
                setViewOffset (std::min (1.0 - span, playheadPos - span * 0.25));
            }
            else if (playheadPos < viewOffset)
            {
                setViewOffset (std::max (0.0, playheadPos - span * 0.15));
            }
        }

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

void TimelineRangeComponent::setZoomLevel (double newZoom, double centerNorm)
{
    const double oldZoom = zoomLevel;
    zoomLevel = juce::jlimit (1.0, 32.0, newZoom);

    if (centerNorm < 0.0)
    {
        const double oldSpan = 1.0 / oldZoom;
        centerNorm = (playheadPos >= viewOffset && playheadPos <= viewOffset + oldSpan)
                     ? playheadPos
                     : (viewOffset + oldSpan * 0.5);
    }
    centerNorm = juce::jlimit (0.0, 1.0, centerNorm);

    // Keep centerNorm anchored at the same relative position
    const double oldRel = (centerNorm - viewOffset) * oldZoom;
    const double newSpan = 1.0 / zoomLevel;
    viewOffset = centerNorm - (oldRel * newSpan);

    const double maxOffset = std::max (0.0, 1.0 - newSpan);
    viewOffset = juce::jlimit (0.0, maxOffset, viewOffset);

    resized();
    repaint();

    if (onZoomChanged)
        onZoomChanged (zoomLevel);
}

void TimelineRangeComponent::zoomIn()
{
    setZoomLevel (zoomLevel * 1.5);
}

void TimelineRangeComponent::zoomOut()
{
    setZoomLevel (zoomLevel / 1.5);
}

void TimelineRangeComponent::resetZoom()
{
    setZoomLevel (1.0);
}

void TimelineRangeComponent::zoomToLoopRange()
{
    const double span = rangeEnd - rangeStart;
    if (span > 0.0005)
    {
        const double targetZoom = juce::jlimit (1.0, 32.0, 0.75 / span);
        const double center = (rangeStart + rangeEnd) * 0.5;
        setZoomLevel (targetZoom, center);
    }
}

void TimelineRangeComponent::setViewOffset (double newOffset)
{
    const double visibleSpan = 1.0 / std::max (1.0, zoomLevel);
    const double maxOffset = std::max (0.0, 1.0 - visibleSpan);
    viewOffset = juce::jlimit (0.0, maxOffset, newOffset);
    repaint();
}

void TimelineRangeComponent::resized()
{
    if (showZoomButtons && getWidth() >= 260)
    {
        const int h = std::min (18, getHeight() - 4);
        const int y = 2;
        int right = getWidth() - 10;

        zoomLoopBtn.setBounds (right - 46, y, 46, h);
        right -= 48;
        zoomFitBtn.setBounds (right - 26, y, 26, h);
        right -= 28;
        zoomInBtn.setBounds (right - 22, y, 22, h);
        right -= 24;
        zoomOutBtn.setBounds (right - 22, y, 22, h);

        zoomOutBtn.setVisible (true);
        zoomInBtn.setVisible (true);
        zoomFitBtn.setVisible (true);
        zoomLoopBtn.setVisible (true);
    }
    else
    {
        zoomOutBtn.setVisible (false);
        zoomInBtn.setVisible (false);
        zoomFitBtn.setVisible (false);
        zoomLoopBtn.setVisible (false);
    }
}

void TimelineRangeComponent::buttonClicked (juce::Button* b)
{
    if (b == &zoomInBtn)
        zoomIn();
    else if (b == &zoomOutBtn)
        zoomOut();
    else if (b == &zoomFitBtn)
        resetZoom();
    else if (b == &zoomLoopBtn)
        zoomToLoopRange();
}

void TimelineRangeComponent::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (std::abs (wheel.deltaY) > 0.001f)
    {
        // Smooth zoom centered at mouse position
        const double mouseNorm = xToNorm (e.position.x);
        const double factor = wheel.deltaY > 0.0f ? 1.25 : (1.0 / 1.25);
        setZoomLevel (zoomLevel * factor, mouseNorm);
    }
    else if (std::abs (wheel.deltaX) > 0.001f && zoomLevel > 1.001)
    {
        // Horizontal scroll/pan
        const double panDelta = -(double) wheel.deltaX * (1.0 / zoomLevel) * 0.5;
        setViewOffset (viewOffset + panDelta);
    }
}

void TimelineRangeComponent::mouseDoubleClick (const juce::MouseEvent& e)
{
    const double clickNorm = xToNorm (e.position.x);
    if (zoomLevel <= 1.05)
        setZoomLevel (4.0, clickNorm);
    else
        resetZoom();
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
    else if (x > xs && x < xe && e.position.y <= getTrackY() + 14.0f)
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
    const float y = e.position.y;
    const float xs = normToX (rangeStart);
    const float xe = normToX (rangeEnd);
    const float px = normToX (playheadPos);

    dragStartMouseX = x;
    dragStartMouseY = y;
    dragInitialStart = rangeStart;
    dragInitialEnd = rangeEnd;
    dragInitialViewOffset = viewOffset;

    // 1. Mini Navigator click/drag at bottom
    if (zoomLevel > 1.001 && y >= getTrackY() + getTrackHeight())
    {
        currentDrag = DragMode::MiniNavDrag;
        const double left = getTrackX();
        const double w = getTrackWidth();
        const double clickNorm = juce::jlimit (0.0, 1.0, (double) (x - left) / (double) w);
        const double span = 1.0 / zoomLevel;
        setViewOffset (clickNorm - span * 0.5);
        return;
    }

    // 2. Right click or Shift+drag = Pan viewport
    if (e.mods.isRightButtonDown() || e.mods.isShiftDown())
    {
        currentDrag = DragMode::PanTrack;
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        return;
    }

    // 3. Handle A (hit zone enlarged for touch)
    if (std::abs (x - xs) <= kHandleHitWidth)
    {
        currentDrag = DragMode::RangeStart;
        return;
    }

    // 4. Handle B
    if (std::abs (x - xe) <= kHandleHitWidth)
    {
        currentDrag = DragMode::RangeEnd;
        return;
    }

    // 5. Playhead grab
    if (std::abs (x - px) <= 14.0f && y <= getTrackY() + 16.0f)
    {
        currentDrag = DragMode::Playhead;
        return;
    }

    // 6. Range Move (drag top ribbon or inside range with Alt)
    if (x > xs && x < xe && (y <= getTrackY() + 14.0f || e.mods.isAltDown()))
    {
        currentDrag = DragMode::RangeMove;
        return;
    }

    // 7. General seek: clicking anywhere on the track
    currentDrag = DragMode::Playhead;
    const double clickNorm = xToNorm (x);
    setPlayhead (clickNorm);
    if (onSeek) onSeek (clickNorm);
}

void TimelineRangeComponent::mouseDrag (const juce::MouseEvent& e)
{
    const double currentNorm = xToNorm (e.position.x);

    if (currentDrag == DragMode::MiniNavDrag)
    {
        const double left = getTrackX();
        const double w = getTrackWidth();
        const double dragNorm = juce::jlimit (0.0, 1.0, (double) (e.position.x - left) / (double) w);
        const double span = 1.0 / zoomLevel;
        setViewOffset (dragNorm - span * 0.5);
    }
    else if (currentDrag == DragMode::PanTrack)
    {
        const double deltaX = e.position.x - dragStartMouseX;
        const double deltaNorm = -(deltaX / getTrackWidth()) * (1.0 / zoomLevel);
        setViewOffset (dragInitialViewOffset + deltaNorm);
    }
    else if (currentDrag == DragMode::RangeStart)
    {
        const double newStart = juce::jlimit (0.0, rangeEnd - 0.002, currentNorm);
        setRange (newStart, rangeEnd, true);
    }
    else if (currentDrag == DragMode::RangeEnd)
    {
        const double newEnd = juce::jlimit (rangeStart + 0.002, 1.0, currentNorm);
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
    setMouseCursor (juce::MouseCursor::NormalCursor);
}

void TimelineRangeComponent::paint (juce::Graphics& g)
{
    const float left = getTrackX();
    const float top = getTrackY();
    const float trackW = getTrackWidth();
    const float trackH = getTrackHeight();

    // 1. Trough Background
    const juce::Rectangle<float> trackRect (left, top, trackW, trackH);
    g.setColour (juce::Colour (0xff101317));
    g.fillRoundedRectangle (trackRect, 4.0f);
    g.setColour (juce::Colour (0xff252c38));
    g.drawRoundedRectangle (trackRect, 4.0f, 1.0f);

    // 2. Timeline Tick Marks & Time Graduation (Adaptive to zoom level)
    if (totalFrames > 0 && sampleRate > 0.0 && trackW > 60.0f)
    {
        const double totalSecs = (double) totalFrames / sampleRate;
        const double visibleSecs = totalSecs / std::max (1.0, zoomLevel);

        double intervalSecs = 30.0;
        if (visibleSecs <= 4.0)       intervalSecs = 0.25;
        else if (visibleSecs <= 8.0)  intervalSecs = 0.5;
        else if (visibleSecs <= 15.0) intervalSecs = 1.0;
        else if (visibleSecs <= 35.0) intervalSecs = 2.0;
        else if (visibleSecs <= 70.0) intervalSecs = 5.0;
        else if (visibleSecs <= 150.0) intervalSecs = 10.0;
        else if (visibleSecs <= 300.0) intervalSecs = 15.0;
        else                          intervalSecs = 30.0;

        const double startSec = std::max (0.0, std::floor ((viewOffset * totalSecs) / intervalSecs) * intervalSecs);
        const double endSec   = std::min (totalSecs, std::ceil (((viewOffset + 1.0 / zoomLevel) * totalSecs) / intervalSecs) * intervalSecs);

        g.setFont (juce::FontOptions (8.5f));

        for (double s = startSec; s <= endSec; s += intervalSecs)
        {
            const float tx = normToX (s / totalSecs);
            if (tx < left - 2.0f || tx > left + trackW + 2.0f)
                continue;

            const bool isMajor = (std::fmod (s, intervalSecs * 2.0) < 0.001);

            g.setColour (isMajor ? juce::Colour (0xff5a687d) : juce::Colour (0xff323a48));
            const float tickH = isMajor ? (trackH * 0.40f) : (trackH * 0.20f);
            g.drawVerticalLine ((int) tx, trackRect.getBottom() - tickH, trackRect.getBottom() - 1.0f);

            // Draw time readout on major ticks
            if (isMajor && trackW > 200.0f)
            {
                const int mins = (int) (s / 60.0);
                const double secs = std::fmod (s, 60.0);
                juce::String tickText;
                if (intervalSecs < 1.0)
                    tickText = juce::String::formatted ("%d:%04.1f", mins, secs);
                else
                    tickText = juce::String::formatted ("%d:%02d", mins, (int) secs);

                g.setColour (juce::Colour (0xff8292a8));
                g.drawText (tickText, (int) tx - 22, (int) trackRect.getY() + 3, 44, 12, juce::Justification::centred);
            }
        }
    }

    // 3. Highlighted Song Section Range [A - B]
    const float xs = normToX (rangeStart);
    const float xe = normToX (rangeEnd);

    // Only draw if at least partially visible
    if (xe >= left && xs <= left + trackW)
    {
        const float visXs = juce::jlimit (left, left + trackW, xs);
        const float visXe = juce::jlimit (left, left + trackW, xe);
        const float rw = std::max (2.0f, visXe - visXs);
        const juce::Rectangle<float> rangeRect (visXs, trackRect.getY() + 1.0f, rw, trackRect.getHeight() - 2.0f);

        const juce::Colour loopColour = loopActive ? juce::Colour (0xffffb74d) : juce::Colour (0xff00d2ff);

        // Shaded range body
        g.setColour (loopColour.withAlpha (loopActive ? 0.26f : 0.14f));
        g.fillRect (rangeRect);

        // Top grab ribbon (tappable to move whole range)
        g.setColour (loopColour);
        g.fillRect (rangeRect.getX(), rangeRect.getY(), rangeRect.getWidth(), 4.0f);
        g.fillRect (rangeRect.getX(), rangeRect.getBottom() - 2.0f, rangeRect.getWidth(), 2.0f);

        // In-Point Handle Marker [ A ]
        if (xs >= left - 10.0f && xs <= left + trackW + 10.0f)
        {
            g.setColour (loopColour);
            g.fillRect (xs - 1.5f, top, 3.0f, trackH);

            // Flag A
            juce::Path flagA;
            flagA.addRoundedRectangle (xs - 10.0f, top, 11.0f, 13.0f, 2.0f);
            g.fillPath (flagA);

            // Bottom anchor arrow ▲
            juce::Path arrowA;
            arrowA.addTriangle (xs - 4.0f, top + trackH, xs + 4.0f, top + trackH, xs, top + trackH - 5.0f);
            g.fillPath (arrowA);

            g.setColour (juce::Colours::black);
            g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
            g.drawText ("A", (int) xs - 10, (int) top, 11, 13, juce::Justification::centred);
        }

        // Out-Point Handle Marker [ B ]
        if (xe >= left - 10.0f && xe <= left + trackW + 10.0f)
        {
            g.setColour (loopColour);
            g.fillRect (xe - 1.5f, top, 3.0f, trackH);

            // Flag B
            juce::Path flagB;
            flagB.addRoundedRectangle (xe - 1.0f, top, 11.0f, 13.0f, 2.0f);
            g.fillPath (flagB);

            // Bottom anchor arrow ▲
            juce::Path arrowB;
            arrowB.addTriangle (xe - 4.0f, top + trackH, xe + 4.0f, top + trackH, xe, top + trackH - 5.0f);
            g.fillPath (arrowB);

            g.setColour (juce::Colours::black);
            g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
            g.drawText ("B", (int) xe - 1, (int) top, 11, 13, juce::Justification::centred);
        }
    }

    // 4. Playhead Needle (White line + Cyan Diamond Grabber)
    const float px = normToX (playheadPos);
    if (px >= left - 4.0f && px <= left + trackW + 4.0f)
    {
        // Needle line
        g.setColour (juce::Colours::white);
        g.fillRect (px - 1.0f, top, 2.0f, trackH);

        // Prominent grab cap
        juce::Path needleCap;
        needleCap.addTriangle (px - 6.0f, top, px + 6.0f, top, px, top + 9.0f);
        g.setColour (juce::Colour (0xff00e5ff));
        g.fillPath (needleCap);
        g.setColour (juce::Colours::white);
        g.strokePath (needleCap, juce::PathStrokeType (1.2f));
    }

    // 5. Mini Song Overview & Navigator (When zoomed in)
    if (zoomLevel > 1.001)
    {
        const float navY = top + trackH + 2.0f;
        const float navH = 5.0f;
        const juce::Rectangle<float> miniTrack (left, navY, trackW, navH);

        // Dark mini background
        g.setColour (juce::Colour (0xff141822));
        g.fillRoundedRectangle (miniTrack, 2.0f);

        // Mini Loop Range
        const float mxs = left + (float) rangeStart * trackW;
        const float mxe = left + (float) rangeEnd * trackW;
        g.setColour (loopActive ? juce::Colour (0xffffb74d).withAlpha (0.4f) : juce::Colour (0xff00d2ff).withAlpha (0.25f));
        g.fillRect (mxs, navY, std::max (2.0f, mxe - mxs), navH);

        // Mini Visible Window Viewport
        const float vx = left + (float) viewOffset * trackW;
        const float vw = std::max (6.0f, (float) (1.0 / zoomLevel) * trackW);
        const juce::Rectangle<float> viewRect (vx, navY - 1.0f, vw, navH + 2.0f);

        g.setColour (juce::Colour (0xff00e5ff).withAlpha (0.35f));
        g.fillRoundedRectangle (viewRect, 2.0f);
        g.setColour (juce::Colour (0xff00e5ff));
        g.drawRoundedRectangle (viewRect, 2.0f, 1.0f);

        // Mini Playhead dot
        const float mpx = left + (float) playheadPos * trackW;
        g.setColour (juce::Colours::white);
        g.fillEllipse (mpx - 1.5f, navY - 0.5f, 3.0f, 6.0f);

        // Zoom Badge Text in upper-left corner
        g.setColour (juce::Colour (0xff00e5ff).withAlpha (0.85f));
        g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        g.drawText (juce::String (zoomLevel, 1) + "x", (int) left + 6, (int) top + 2, 35, 12, juce::Justification::centredLeft);
    }
}

} // namespace mps
