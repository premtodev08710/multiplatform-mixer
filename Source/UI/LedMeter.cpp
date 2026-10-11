// Mix Practice Studio - Realtime LED Segment Audio Meter Component Implementation
#include "LedMeter.h"

namespace mps
{

struct LedSegmentDef
{
    float dbThreshold;
    juce::Colour onColour;
    juce::Colour offColour;
};

static constexpr int kNumSegments = 16;
static const LedSegmentDef kSegments[kNumSegments] = {
    { -60.0f, juce::Colour (0xff00d455), juce::Colour (0xff05260f) }, // 0:  -60 dB
    { -50.0f, juce::Colour (0xff00d455), juce::Colour (0xff05260f) }, // 1:  -50 dB
    { -42.0f, juce::Colour (0xff00d455), juce::Colour (0xff05260f) }, // 2:  -42 dB
    { -36.0f, juce::Colour (0xff00d455), juce::Colour (0xff05260f) }, // 3:  -36 dB
    { -30.0f, juce::Colour (0xff00d455), juce::Colour (0xff05260f) }, // 4:  -30 dB
    { -24.0f, juce::Colour (0xff00e650), juce::Colour (0xff062d12) }, // 5:  -24 dB
    { -20.0f, juce::Colour (0xff10f040), juce::Colour (0xff083315) }, // 6:  -20 dB
    { -18.0f, juce::Colour (0xff38f538), juce::Colour (0xff0a3b18) }, // 7:  -18 dB (Nominal 0 VU Target - Top Green!)
    { -15.0f, juce::Colour (0xffa8f020), juce::Colour (0xff243306) }, // 8:  -15 dB (Yellow-Green)
    { -12.0f, juce::Colour (0xffffea00), juce::Colour (0xff3b3600) }, // 9:  -12 dB (Yellow)
    {  -9.0f, juce::Colour (0xffffd000), juce::Colour (0xff3b3000) }, // 10:  -9 dB (Yellow)
    {  -6.0f, juce::Colour (0xffffa000), juce::Colour (0xff3b2400) }, // 11:  -6 dB (Amber/Orange)
    {  -4.0f, juce::Colour (0xffff7700), juce::Colour (0xff3b1b00) }, // 12:  -4 dB (Orange)
    {  -2.0f, juce::Colour (0xffff4422), juce::Colour (0xff3b1008) }, // 13:  -2 dB (Orange-Red)
    {  -1.0f, juce::Colour (0xffff2222), juce::Colour (0xff3b0808) }, // 14:  -1 dB (Red)
    {   0.0f, juce::Colour (0xffff1111), juce::Colour (0xff400505) }  // 15:   0 dB (Clip Red)
};

LedMeter::LedMeter (Mode m) : mode (m)
{
    setOpaque (false);
    startTimerHz (30); // 30 FPS UI refresh decoupled from audio thread
}

LedMeter::~LedMeter()
{
    stopTimer();
}

void LedMeter::setMeters (AtomicPeak* leftPeak, AtomicPeak* rightPeak, std::atomic<bool>* clipFlag)
{
    peakL = leftPeak;
    peakR = rightPeak;
    clip = clipFlag;
}

void LedMeter::resetClip()
{
    clipLatched = false;
    if (clip != nullptr)
        clip->store (false, std::memory_order_relaxed);
    repaint();
}

void LedMeter::mouseDown (const juce::MouseEvent&)
{
    resetClip();
}

void LedMeter::timerCallback()
{
    float rawPeakL = 0.0f;
    float rawPeakR = 0.0f;

    if (peakL != nullptr) rawPeakL = peakL->readAndReset();
    if (peakR != nullptr) rawPeakR = peakR->readAndReset();

    if (clip != nullptr && clip->load (std::memory_order_relaxed))
        clipLatched = true;

    const float dbL = gainToDb (rawPeakL);
    const float dbR = gainToDb (rawPeakR);

    // Instant attack, smooth standard console decay (-1.5 dB per frame @ 30 FPS = ~45 dB/sec)
    const float decayStep = 1.5f;

    if (dbL > currentDbL) currentDbL = dbL;
    else currentDbL = std::max (-90.0f, currentDbL - decayStep);

    if (dbR > currentDbR) currentDbR = dbR;
    else currentDbR = std::max (-90.0f, currentDbR - decayStep);

    // Peak hold with 1.5s hold time (45 frames)
    if (currentDbL >= peakHoldDbL) { peakHoldDbL = currentDbL; peakHoldTimeL = 45; }
    else if (--peakHoldTimeL <= 0) { peakHoldDbL = std::max (-90.0f, peakHoldDbL - decayStep * 0.7f); }

    if (currentDbR >= peakHoldDbR) { peakHoldDbR = currentDbR; peakHoldTimeR = 45; }
    else if (--peakHoldTimeR <= 0) { peakHoldDbR = std::max (-90.0f, peakHoldDbR - decayStep * 0.7f); }

    repaint();
}

void LedMeter::drawSegmentedMeter (juce::Graphics& g, juce::Rectangle<float> area, float levelDb, float peakHoldDb)
{
    // Background slot bezel
    g.setColour (juce::Colour (0xff080a0c));
    g.fillRoundedRectangle (area, 2.0f);
    g.setColour (juce::Colour (0xff20242a));
    g.drawRoundedRectangle (area, 2.0f, 0.8f);

    const float gap = 1.0f;
    const float totalH = area.getHeight() - 2.0f;
    const float segH = (totalH - (kNumSegments - 1) * gap) / (float) kNumSegments;
    const float segW = area.getWidth() - 2.0f;
    const float leftX = area.getX() + 1.0f;

    for (int s = 0; s < kNumSegments; ++s)
    {
        const float sy = area.getBottom() - 1.0f - (s + 1) * segH - s * gap;
        const juce::Rectangle<float> segRect (leftX, sy, segW, segH);

        const bool isOn = (levelDb >= kSegments[s].dbThreshold);
        const bool isPeak = (peakHoldDb >= kSegments[s].dbThreshold &&
                             (s == kNumSegments - 1 || peakHoldDb < kSegments[s + 1].dbThreshold));

        if (isOn)
        {
            g.setColour (kSegments[s].onColour);
            g.fillRoundedRectangle (segRect, 1.0f);
        }
        else if (isPeak)
        {
            // Crisp Peak Hold indicator line
            g.setColour (s >= 14 ? juce::Colour (0xffff2222) : juce::Colours::white);
            g.fillRoundedRectangle (segRect, 1.0f);
        }
        else
        {
            // Dark unlit physical LED bead frame
            g.setColour (kSegments[s].offColour);
            g.fillRoundedRectangle (segRect, 1.0f);
        }
    }
}

void LedMeter::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Clip indicator LED at top
    const float clipSize = 6.0f;
    juce::Rectangle<float> clipRect (bounds.getCentreX() - clipSize * 0.5f, bounds.getY() + 1.0f, clipSize, clipSize);

    if (clipLatched)
    {
        g.setColour (juce::Colour (0xffff1744));
        g.fillEllipse (clipRect);
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.drawEllipse (clipRect, 1.0f);
    }
    else
    {
        g.setColour (juce::Colour (0xff320a0a)); // unlit dark red LED
        g.fillEllipse (clipRect);
    }

    // Main meter bars area
    const float meterTop = bounds.getY() + clipSize + 4.0f;
    const float meterHeight = bounds.getBottom() - meterTop - 2.0f;
    if (meterHeight <= 8.0f) return;

    if (mode == Mode::Mono)
    {
        juce::Rectangle<float> barArea (bounds.getX() + 1.0f, meterTop, bounds.getWidth() - 2.0f, meterHeight);
        drawSegmentedMeter (g, barArea, currentDbL, peakHoldDbL);
    }
    else // Stereo
    {
        const float barW = (bounds.getWidth() - 3.0f) * 0.5f;
        juce::Rectangle<float> barL (bounds.getX() + 1.0f, meterTop, barW, meterHeight);
        juce::Rectangle<float> barR (bounds.getX() + 2.0f + barW, meterTop, barW, meterHeight);

        drawSegmentedMeter (g, barL, currentDbL, peakHoldDbL);
        drawSegmentedMeter (g, barR, currentDbR, peakHoldDbR);
    }
}

} // namespace mps
