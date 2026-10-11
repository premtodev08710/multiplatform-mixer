// Mix Practice Studio - Dual Trapezoid Console Pan Indicator Widget Implementation
#include "TrapezoidPanIndicator.h"

namespace mps
{

TrapezoidPanIndicator::TrapezoidPanIndicator()
{
    setOpaque (false);
}

void TrapezoidPanIndicator::setPan (float panVal, juce::NotificationType notification)
{
    const float clamped = juce::jlimit (-1.0f, 1.0f, panVal);
    if (std::abs (clamped - pan) > 0.001f)
    {
        pan = clamped;
        repaint();
        if (notification != juce::dontSendNotification && onPanChanged)
            onPanChanged (pan);
    }
}

void TrapezoidPanIndicator::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();
    const float midX = bounds.getX() + w * 0.5f;

    // 1. Sunken background container
    g.setColour (juce::Colour (0xff121417));
    g.fillRoundedRectangle (bounds, 2.0f);
    g.setColour (juce::Colour (0xff262b33));
    g.drawRoundedRectangle (bounds, 2.0f, 1.0f);

    const float topY = bounds.getY() + 2.0f;
    const float botY = bounds.getBottom() - 2.0f;
    const float leftX = bounds.getX() + 2.0f;
    const float rightX = bounds.getRight() - 2.0f;

    // 2. Left Blue Wedge (peaks at left edge, slopes to center bottom)
    juce::Path leftWedge;
    leftWedge.startNewSubPath (leftX, topY);
    leftWedge.lineTo (midX - 1.0f, botY);
    leftWedge.lineTo (leftX, botY);
    leftWedge.closeSubPath();

    // Fill left wedge: subtle blue gradient
    juce::ColourGradient blueGrad (juce::Colour (0xff1976d2), leftX, topY,
                                   juce::Colour (0xff0d2b45), midX, botY, false);
    g.setGradientFill (blueGrad);
    g.fillPath (leftWedge);

    // Fine vertical lines grid inside left wedge
    g.setColour (juce::Colour (0x4400d2ff));
    for (float x = leftX + 4.0f; x < midX - 2.0f; x += 4.0f)
    {
        const float norm = (x - leftX) / (midX - leftX);
        const float curTop = topY + norm * (botY - topY);
        g.drawVerticalLine ((int) x, curTop, botY);
    }

    // 3. Right Orange/Red Wedge (peaks at right edge, slopes to center bottom)
    juce::Path rightWedge;
    rightWedge.startNewSubPath (rightX, topY);
    rightWedge.lineTo (rightX, botY);
    rightWedge.lineTo (midX + 1.0f, botY);
    rightWedge.closeSubPath();

    // Fill right wedge: subtle orange/amber gradient
    juce::ColourGradient orangeGrad (juce::Colour (0xffe65100), rightX, topY,
                                     juce::Colour (0xff3e1b00), midX, botY, false);
    g.setGradientFill (orangeGrad);
    g.fillPath (rightWedge);

    // Fine vertical lines grid inside right wedge
    g.setColour (juce::Colour (0x44ff9800));
    for (float x = midX + 3.0f; x < rightX - 2.0f; x += 4.0f)
    {
        const float norm = (rightX - x) / (rightX - midX);
        const float curTop = topY + norm * (botY - topY);
        g.drawVerticalLine ((int) x, curTop, botY);
    }

    // 4. Center Divider Line
    g.setColour (juce::Colour (0xff323a45));
    g.drawVerticalLine ((int) midX, topY, botY);

    // 5. White Position Pointer Line
    const float normPos = (pan + 1.0f) * 0.5f; // 0.0 to 1.0
    const float cursorX = leftX + normPos * (rightX - leftX);

    g.setColour (juce::Colours::white);
    g.drawVerticalLine ((int) cursorX, bounds.getY() + 1.0f, bounds.getBottom() - 1.0f);

    // Pointer notch at top
    juce::Path pointerHead;
    pointerHead.startNewSubPath (cursorX - 3.0f, bounds.getY() + 1.0f);
    pointerHead.lineTo (cursorX + 3.0f, bounds.getY() + 1.0f);
    pointerHead.lineTo (cursorX, bounds.getY() + 5.0f);
    pointerHead.closeSubPath();
    g.fillPath (pointerHead);
}

void TrapezoidPanIndicator::mouseDown (const juce::MouseEvent& e)
{
    mouseDrag (e);
}

void TrapezoidPanIndicator::mouseDrag (const juce::MouseEvent& e)
{
    const auto bounds = getLocalBounds().toFloat().reduced (2.0f);
    const float norm = juce::jlimit (0.0f, 1.0f, (e.position.x - bounds.getX()) / bounds.getWidth());
    setPan (norm * 2.0f - 1.0f, juce::sendNotificationAsync);
}

void TrapezoidPanIndicator::mouseDoubleClick (const juce::MouseEvent&)
{
    setPan (0.0f, juce::sendNotificationAsync);
}

} // namespace mps
