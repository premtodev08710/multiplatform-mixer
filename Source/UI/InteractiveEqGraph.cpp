// Mix Practice Studio - Interactive Logarithmic Parametric EQ Response Graph Implementation
#include "InteractiveEqGraph.h"

namespace mps
{

const juce::Colour InteractiveEqGraph::nodeColours[5] = {
    juce::Colour (0xffe53935), // HPF (Red)
    juce::Colour (0xff00d2ff), // Low (Cyan)
    juce::Colour (0xffffc107), // Mid 1 (Yellow)
    juce::Colour (0xffe040fb), // Mid 2 (Magenta)
    juce::Colour (0xffff9800)  // High (Orange)
};

InteractiveEqGraph::InteractiveEqGraph()
{
    setOpaque (false);
}

void InteractiveEqGraph::setParams (ChannelDspParams* p, double sampleRate)
{
    dspParams = p;
    sr = sampleRate > 1000.0 ? sampleRate : 48000.0;
    repaint();
}

void InteractiveEqGraph::setSelectedBand (int band)
{
    if (selectedBand != band && band >= 0 && band < 5)
    {
        selectedBand = band;
        repaint();
        if (onBandSelected)
            onBandSelected (selectedBand);
    }
}

void InteractiveEqGraph::resized()
{
    repaint();
}

float InteractiveEqGraph::freqToX (float f) const noexcept
{
    const float w = (float) getWidth();
    if (w <= 1.0f) return 0.0f;
    f = clampf (f, 20.0f, 20000.0f);
    const float logMin = std::log10 (20.0f);
    const float logMax = std::log10 (20000.0f);
    const float norm = (std::log10 (f) - logMin) / (logMax - logMin);
    return norm * (w - 40.0f) + 30.0f; // Leave margin for Y axis labels on left
}

float InteractiveEqGraph::xToFreq (float x) const noexcept
{
    const float w = (float) getWidth();
    if (w <= 1.0f) return 1000.0f;
    const float norm = clampf ((x - 30.0f) / (w - 40.0f), 0.0f, 1.0f);
    const float logMin = std::log10 (20.0f);
    const float logMax = std::log10 (20000.0f);
    return std::pow (10.0f, logMin + norm * (logMax - logMin));
}

float InteractiveEqGraph::gainToY (float gDb) const noexcept
{
    const float h = (float) getHeight();
    if (h <= 1.0f) return 0.0f;
    gDb = clampf (gDb, -18.0f, 18.0f);
    // +18 dB top, -18 dB bottom, 0 dB center
    const float norm = (18.0f - gDb) / 36.0f;
    return norm * (h - 22.0f) + 6.0f;
}

float InteractiveEqGraph::yToGain (float y) const noexcept
{
    const float h = (float) getHeight();
    if (h <= 1.0f) return 0.0f;
    const float norm = clampf ((y - 6.0f) / (h - 22.0f), 0.0f, 1.0f);
    return 18.0f - norm * 36.0f;
}

int InteractiveEqGraph::findHandleUnderMouse (juce::Point<float> pos) const
{
    if (dspParams == nullptr) return -1;

    struct HandleInfo { int idx; float f; float g; };
    HandleInfo handles[5] = {
        { 0, dspParams->hpfFreq.get(), 0.0f },
        { 1, dspParams->eqLowFreq.get(), dspParams->eqLowGainDb.get() },
        { 2, dspParams->eqMid1Freq.get(), dspParams->eqMid1GainDb.get() },
        { 3, dspParams->eqMid2Freq.get(), dspParams->eqMid2GainDb.get() },
        { 4, dspParams->eqHighFreq.get(), dspParams->eqHighGainDb.get() }
    };

    const float hitRadius = 16.0f;
    for (int i = 0; i < 5; ++i)
    {
        const float hx = freqToX (handles[i].f);
        const float hy = gainToY (handles[i].g);
        if (pos.getDistanceFrom ({ hx, hy }) <= hitRadius)
            return handles[i].idx;

        // Check top cursor badges
        if (i >= 1 && std::abs (pos.x - hx) <= 12.0f && pos.y >= 2.0f && pos.y <= 24.0f)
            return handles[i].idx;
    }
    return -1;
}

void InteractiveEqGraph::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    // 1. Dark Graph Matrix Background
    g.setColour (juce::Colour (0xff0e1014));
    g.fillRoundedRectangle (bounds, 3.0f);
    g.setColour (juce::Colour (0xff242a34));
    g.drawRoundedRectangle (bounds, 3.0f, 1.0f);

    // 2. Frequency Grid Lines & Labels
    const float gridFreqs[] = {
        20.0f, 40.0f, 60.0f, 80.0f, 100.0f, 200.0f, 300.0f, 400.0f,
        600.0f, 800.0f, 1000.0f, 2000.0f, 3000.0f, 4000.0f, 5000.0f,
        6000.0f, 8000.0f, 10000.0f, 20000.0f
    };
    const char* gridLabels[] = {
        "20", "40", "60", "80", "100", "200", "300", "400",
        "600", "800", "1k", "2k", "3k", "4k", "5k",
        "6k", "8k", "10k", "20k"
    };

    g.setFont (juce::FontOptions (8.0f));
    for (size_t i = 0; i < 19; ++i)
    {
        const float gx = freqToX (gridFreqs[i]);
        if (gx < 30.0f || gx > w - 10.0f) continue;

        // Subtle dark grid line
        g.setColour (juce::Colour (0xff1b2028));
        g.drawVerticalLine ((int) gx, bounds.getY() + 2.0f, bounds.getBottom() - 14.0f);

        // Label at bottom
        g.setColour (juce::Colour (0xff707a8a));
        g.drawText (gridLabels[i], (int) gx - 12, (int) bounds.getBottom() - 14, 24, 12,
                    juce::Justification::centred);
    }

    // 3. dB Gain Horizontal Lines & Labels (+15, +10, +5, 0, -5, -10, -15)
    const float dBLines[] = { 15.0f, 10.0f, 5.0f, 0.0f, -5.0f, -10.0f, -15.0f };
    const char* dBLabels[] = { "+15", "+10", "+5", "0 dB", "-5", "-10", "-15" };

    for (size_t i = 0; i < 7; ++i)
    {
        const float gy = gainToY (dBLines[i]);
        const bool isZero = (dBLines[i] == 0.0f);

        // 0 dB is solid white line matching console reference
        g.setColour (isZero ? juce::Colour (0xffffffff) : juce::Colour (0xff1f2530));
        g.drawHorizontalLine ((int) gy, 30.0f, w - 10.0f);

        g.setColour (isZero ? juce::Colour (0xffffffff) : juce::Colour (0xff707a8a));
        g.setFont (juce::FontOptions (8.5f, isZero ? juce::Font::bold : juce::Font::plain));
        g.drawText (dBLabels[i], 2, (int) gy - 6, 26, 12, juce::Justification::centredRight);
    }

    // Low Cut / HPF box icon at top left
    juce::Rectangle<float> hpfIconRect (34.0f, 6.0f, 16.0f, 14.0f);
    g.setColour (juce::Colour (0xff323a48));
    g.drawRect (hpfIconRect, 1.0f);
    juce::Path hpfCurve;
    hpfCurve.startNewSubPath (hpfIconRect.getX() + 2.0f, hpfIconRect.getBottom() - 2.0f);
    hpfCurve.lineTo (hpfIconRect.getCentreX(), hpfIconRect.getBottom() - 2.0f);
    hpfCurve.lineTo (hpfIconRect.getRight() - 2.0f, hpfIconRect.getY() + 2.0f);
    g.setColour (juce::Colour (0xff9aa4b2));
    g.strokePath (hpfCurve, juce::PathStrokeType (1.2f));

    if (dspParams == nullptr) return;

    // 4. Calculate real biquad filter coefficients
    const float sampleRate = (float) sr;
    const bool hpfOn = ! dspParams->hpfBypass.getBool();
    const bool eqOn = ! dspParams->eqBypass.getBool();

    BiquadCoeffs hpfCoeffs;
    if (hpfOn)
        hpfCoeffs = calcHPF (dspParams->hpfFreq.get(), 0.7071f, sampleRate);

    std::array<BiquadCoeffs, 4> eqCoeffs;
    if (eqOn)
    {
        const auto lowType = (EqBandType) dspParams->eqLowType.getInt();
        eqCoeffs[0] = (lowType == EqBandType::LowShelf)
            ? calcLowShelf (dspParams->eqLowFreq.get(), dspParams->eqLowGainDb.get(), dspParams->eqLowQ.get(), sampleRate)
            : calcBell (dspParams->eqLowFreq.get(), dspParams->eqLowGainDb.get(), dspParams->eqLowQ.get(), sampleRate);

        eqCoeffs[1] = calcBell (dspParams->eqMid1Freq.get(), dspParams->eqMid1GainDb.get(), dspParams->eqMid1Q.get(), sampleRate);
        eqCoeffs[2] = calcBell (dspParams->eqMid2Freq.get(), dspParams->eqMid2GainDb.get(), dspParams->eqMid2Q.get(), sampleRate);

        const auto highType = (EqBandType) dspParams->eqHighType.getInt();
        eqCoeffs[3] = (highType == EqBandType::HighShelf)
            ? calcHighShelf (dspParams->eqHighFreq.get(), dspParams->eqHighGainDb.get(), dspParams->eqHighQ.get(), sampleRate)
            : calcBell (dspParams->eqHighFreq.get(), dspParams->eqHighGainDb.get(), dspParams->eqHighQ.get(), sampleRate);
    }

    // 5. Draw 4 Vertical Dashed Cursor Lines with Badges [1], [2], [3], [4]
    const float bandFreqs[4] = {
        dspParams->eqLowFreq.get(),
        dspParams->eqMid1Freq.get(),
        dspParams->eqMid2Freq.get(),
        dspParams->eqHighFreq.get()
    };

    for (int b = 0; b < 4; ++b)
    {
        const float bx = freqToX (bandFreqs[b]);
        const juce::Colour bCol = nodeColours[b + 1];

        // Dashed vertical line
        g.setColour (bCol.withAlpha (0.7f));
        const float dashLengths[] = { 3.0f, 3.0f };
        juce::Line<float> line (bx, 20.0f, bx, bounds.getBottom() - 14.0f);
        g.drawDashedLine (line, dashLengths, 2, 1.2f);

        // Square badge [b+1] at top
        const juce::Rectangle<float> badgeRect (bx - 7.0f, 4.0f, 14.0f, 14.0f);
        const bool isSelected = (selectedBand == b + 1);

        g.setColour (isSelected ? bCol : juce::Colour (0xff1e222a));
        g.fillRect (badgeRect);

        g.setColour (isSelected ? juce::Colours::white : bCol);
        g.drawRect (badgeRect, 1.2f);

        g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        g.setColour (isSelected ? juce::Colours::black : bCol);
        g.drawText (juce::String (b + 1), badgeRect.toNearestInt(), juce::Justification::centred);
    }

    // 6. Draw Composite Response Curve (Solid white curve)
    juce::Path curvePath;
    const int startX = 30;
    const int endX = (int) w - 10;

    for (int x = startX; x < endX; ++x)
    {
        const float f = xToFreq ((float) x);
        float totalDb = 0.0f;

        if (hpfOn)
            totalDb += hpfCoeffs.getMagnitudeResponseDb (f, sampleRate);

        if (eqOn)
        {
            for (int b = 0; b < 4; ++b)
                totalDb += eqCoeffs[(size_t) b].getMagnitudeResponseDb (f, sampleRate);
        }

        const float y = gainToY (totalDb);
        if (x == startX) curvePath.startNewSubPath ((float) x, y);
        else curvePath.lineTo ((float) x, y);
    }

    // Solid white response curve
    g.setColour (juce::Colours::white);
    g.strokePath (curvePath, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 7. Draw Interactive Node Handles on the curve
    struct HandleDrawInfo { int idx; const char* label; float f; float g; };
    HandleDrawInfo handles[5] = {
        { 0, "HPF", dspParams->hpfFreq.get(), 0.0f },
        { 1, "1",   dspParams->eqLowFreq.get(), dspParams->eqLowGainDb.get() },
        { 2, "2",   dspParams->eqMid1Freq.get(), dspParams->eqMid1GainDb.get() },
        { 3, "3",   dspParams->eqMid2Freq.get(), dspParams->eqMid2GainDb.get() },
        { 4, "4",   dspParams->eqHighFreq.get(), dspParams->eqHighGainDb.get() }
    };

    for (int i = 0; i < 5; ++i)
    {
        const float hx = freqToX (handles[i].f);
        const float hy = gainToY (handles[i].g);
        const bool isSelected = (selectedBand == i);
        const float handleSize = isSelected ? 20.0f : 16.0f;
        const juce::Rectangle<float> handleRect (hx - handleSize * 0.5f, hy - handleSize * 0.5f, handleSize, handleSize);

        // Highlight ring on selected node
        if (isSelected)
        {
            g.setColour (nodeColours[i].withAlpha (0.4f));
            g.fillEllipse (handleRect.expanded (4.0f));
        }

        g.setColour (nodeColours[i]);
        g.fillEllipse (handleRect);

        g.setColour (juce::Colours::white);
        g.drawEllipse (handleRect, 1.2f);

        g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        g.setColour (juce::Colours::black);
        g.drawText (handles[i].label, handleRect.toNearestInt(), juce::Justification::centred);
    }
}

void InteractiveEqGraph::mouseDown (const juce::MouseEvent& e)
{
    draggingNode = findHandleUnderMouse (e.position);
    if (draggingNode >= 0)
    {
        setSelectedBand (draggingNode);
    }
    repaint();
}

void InteractiveEqGraph::mouseDrag (const juce::MouseEvent& e)
{
    if (draggingNode < 0 || dspParams == nullptr) return;

    if (draggingNode > 0)
        dspParams->eqBypass.set (0.0f); // Auto-activate EQ when tweaking bands
    else if (draggingNode == 0)
        dspParams->hpfBypass.set (0.0f); // Auto-activate HPF when tweaking HPF

    const float f = xToFreq (e.position.x);
    const float g = yToGain (e.position.y);

    switch (draggingNode)
    {
        case 0: // HPF
            dspParams->hpfFreq.set (clampf (f, 20.0f, 400.0f));
            break;
        case 1: // Low
            dspParams->eqLowFreq.set (clampf (f, 20.0f, 2000.0f));
            dspParams->eqLowGainDb.set (clampf (g, -15.0f, 15.0f));
            break;
        case 2: // Mid 1
            dspParams->eqMid1Freq.set (clampf (f, 50.0f, 5000.0f));
            dspParams->eqMid1GainDb.set (clampf (g, -15.0f, 15.0f));
            break;
        case 3: // Mid 2
            dspParams->eqMid2Freq.set (clampf (f, 200.0f, 10000.0f));
            dspParams->eqMid2GainDb.set (clampf (g, -15.0f, 15.0f));
            break;
        case 4: // High
            dspParams->eqHighFreq.set (clampf (f, 1000.0f, 20000.0f));
            dspParams->eqHighGainDb.set (clampf (g, -15.0f, 15.0f));
            break;
    }

    if (onParamsChanged) onParamsChanged();
    repaint();
}

void InteractiveEqGraph::mouseUp (const juce::MouseEvent&)
{
    draggingNode = -1;
    repaint();
}

void InteractiveEqGraph::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    const int hovered = findHandleUnderMouse (e.position);
    if (hovered < 0 || dspParams == nullptr) return;

    const float delta = wheel.deltaY * 0.5f;

    switch (hovered)
    {
        case 1: dspParams->eqLowQ.set (clampf (dspParams->eqLowQ.get() + delta, 0.3f, 10.0f)); break;
        case 2: dspParams->eqMid1Q.set (clampf (dspParams->eqMid1Q.get() + delta, 0.3f, 10.0f)); break;
        case 3: dspParams->eqMid2Q.set (clampf (dspParams->eqMid2Q.get() + delta, 0.3f, 10.0f)); break;
        case 4: dspParams->eqHighQ.set (clampf (dspParams->eqHighQ.get() + delta, 0.3f, 10.0f)); break;
    }

    if (onParamsChanged) onParamsChanged();
    repaint();
}

} // namespace mps
