// Mix Practice Studio - Professional Console Rotary Dial Implementation
#include "ConsoleDial.h"
#include <cmath>

namespace mps
{

static constexpr float kStartAngle = -juce::MathConstants<float>::pi * 0.75f; // ~225 deg
static constexpr float kEndAngle   =  juce::MathConstants<float>::pi * 0.75f; // ~135 deg

ConsoleDial::ConsoleDial (const juce::String& t, double minVal, double maxVal,
                          double defVal, double step, bool isLog)
    : title (t), minValue (minVal), maxValue (maxVal), defaultValue (defVal),
      stepSize (step), currentValue (defVal), isLogarithmic (isLog)
{
    setOpaque (false);
}

void ConsoleDial::setTitle (const juce::String& t)
{
    title = t;
    repaint();
}

void ConsoleDial::setRange (double minVal, double maxVal, double step, bool isLog)
{
    minValue = minVal;
    maxValue = maxVal;
    stepSize = step;
    isLogarithmic = isLog;
    setValue (currentValue, juce::dontSendNotification);
}

void ConsoleDial::setValue (double val, juce::NotificationType notification)
{
    double clamped = juce::jlimit (minValue, maxValue, val);
    if (stepSize > 0.0)
        clamped = std::round (clamped / stepSize) * stepSize;

    if (std::abs (clamped - currentValue) > 1e-6)
    {
        currentValue = clamped;
        repaint();
        if (notification != juce::dontSendNotification && onValueChanged)
            onValueChanged (currentValue);
    }
}

void ConsoleDial::setTickLabels (const std::vector<TickLabel>& labels)
{
    tickLabels = labels;
    repaint();
}

double ConsoleDial::valueToProportion (double v) const
{
    if (maxValue <= minValue) return 0.0;
    v = juce::jlimit (minValue, maxValue, v);
    if (isLogarithmic && minValue > 0.0)
    {
        const double logMin = std::log10 (minValue);
        const double logMax = std::log10 (maxValue);
        return juce::jlimit (0.0, 1.0, (std::log10 (v) - logMin) / (logMax - logMin));
    }
    return (v - minValue) / (maxValue - minValue);
}

double ConsoleDial::proportionToValue (double prop) const
{
    prop = juce::jlimit (0.0, 1.0, prop);
    if (isLogarithmic && minValue > 0.0)
    {
        const double logMin = std::log10 (minValue);
        const double logMax = std::log10 (maxValue);
        return std::pow (10.0, logMin + prop * (logMax - logMin));
    }
    return minValue + prop * (maxValue - minValue);
}

float ConsoleDial::valueToAngle (double v) const
{
    const float p = (float) valueToProportion (v);
    return kStartAngle + p * (kEndAngle - kStartAngle);
}

void ConsoleDial::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    // 1. Draw Title at top
    float topOffset = 0.0f;
    if (title.isNotEmpty())
    {
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.setColour (juce::Colour (0xffe0e4ea));
        g.drawText (title, 0, 0, (int) w, 14, juce::Justification::centred);
        topOffset = 13.0f;
    }

    // Geometry of Dial & Arcs
    const float availH = h - topOffset;
    const float dialDiameter = std::min (w - 28.0f, availH - 24.0f);
    const float dialRadius = std::max (10.0f, dialDiameter * 0.5f);
    const float centreX = w * 0.5f;
    const float centreY = topOffset + (availH * 0.5f) + 1.0f;

    // 2. Draw Peripheral Tick Labels
    if (! tickLabels.empty())
    {
        g.setFont (juce::FontOptions (7.5f));
        const float labelRadius = dialRadius + 11.5f;

        for (const auto& tick : tickLabels)
        {
            const float angle = valueToAngle (tick.value);
            // Angle 0 is 12 o'clock (straight up), so sin(angle) is X, -cos(angle) is Y
            const float lx = centreX + std::sin (angle) * labelRadius;
            const float ly = centreY - std::cos (angle) * labelRadius;

            g.setColour (juce::Colour (0xff8a94a2));
            g.drawText (tick.text, (int) lx - 14, (int) ly - 6, 28, 12, juce::Justification::centred);
        }
    }

    // 3. Background Arc Track
    const float arcRadius = dialRadius + 2.5f;
    juce::Path bgArc;
    bgArc.addCentredArc (centreX, centreY, arcRadius, arcRadius, 0.0f,
                         kStartAngle, kEndAngle, true);
    g.setColour (juce::Colour (0xff20242a));
    g.strokePath (bgArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 4. Value Arc (Active illuminated arc)
    const float currentAngle = valueToAngle (currentValue);
    juce::Path valArc;
    valArc.addCentredArc (centreX, centreY, arcRadius, arcRadius, 0.0f,
                          kStartAngle, currentAngle, true);
    g.setColour (isEnabled() ? arcColour : juce::Colour (0xff555f6d));
    g.strokePath (valArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 5. Dial Body (tactile beveled gunmetal cap)
    const juce::Rectangle<float> dialRect (centreX - dialRadius, centreY - dialRadius,
                                           dialRadius * 2.0f, dialRadius * 2.0f);

    // Outer dark bezel shadow
    g.setColour (juce::Colour (0xff0e1013));
    g.fillEllipse (dialRect.expanded (1.5f));

    // Cap Gradient
    juce::ColourGradient capGrad (juce::Colour (0xff404652), centreX, centreY - dialRadius,
                                  juce::Colour (0xff1c2026), centreX, centreY + dialRadius, false);
    g.setGradientFill (capGrad);
    g.fillEllipse (dialRect);

    // Inner bevel rim
    g.setColour (juce::Colour (0xff586272));
    g.drawEllipse (dialRect, 1.0f);

    // Inner sunken face
    const float innerR = dialRadius * 0.72f;
    const juce::Rectangle<float> innerRect (centreX - innerR, centreY - innerR, innerR * 2.0f, innerR * 2.0f);
    g.setColour (juce::Colour (0xff1a1d24));
    g.fillEllipse (innerRect);
    g.setColour (juce::Colour (0xff2b323e));
    g.drawEllipse (innerRect, 1.0f);

    // 6. Indicator Pointer Notch on Bevel Rim (clean notch that doesn't obstruct center)
    juce::Path pointer;
    const float ptrLen = std::max (3.0f, (dialRadius - innerR) + 0.5f);
    const float ptrThick = 2.5f;
    pointer.addRoundedRectangle (-ptrThick * 0.5f, -dialRadius + 1.0f, ptrThick, ptrLen, 0.8f);
    pointer.applyTransform (juce::AffineTransform::rotation (currentAngle).translated (centreX, centreY));

    g.setColour (isEnabled() ? arcColour : juce::Colour (0xff8a94a2));
    g.fillPath (pointer);

    // 7. High-Contrast Console Digital Readout inside inner dial face
    if (innerR >= 11.0f)
    {
        const float fontSize = std::min (10.0f, innerR * 0.62f);
        g.setFont (juce::FontOptions (fontSize, juce::Font::bold));
        g.setColour (isEnabled() ? juce::Colour (0xffe6f0fa) : juce::Colour (0xff707a8a));
        g.drawText (getFormattedValueText(), innerRect, juce::Justification::centred, false);
    }
}

juce::String ConsoleDial::getFormattedValueText() const
{
    if (valueFormatter)
        return valueFormatter (currentValue);

    if (unit.isNotEmpty())
    {
        if (unit == "dB")
            return (currentValue > 0.05 ? "+" : "") + juce::String (currentValue, 1) + " dB";
        return juce::String (currentValue, 1) + " " + unit;
    }

    if (title.containsIgnoreCase ("GAIN") || title.containsIgnoreCase ("TRIM"))
    {
        return (currentValue > 0.05 ? "+" : "") + juce::String (currentValue, 1) + " dB";
    }

    if (title.containsIgnoreCase ("THRESH") || title.containsIgnoreCase ("RANGE") ||
        title.containsIgnoreCase ("WEIGHT"))
    {
        return (currentValue > 0.05 ? "+" : "") + juce::String (currentValue, 1) + " dB";
    }

    if (title.containsIgnoreCase ("FREQ") || title.containsIgnoreCase ("CUT"))
    {
        if (currentValue >= 1000.0)
            return juce::String (currentValue / 1000.0, 1) + "k";
        return juce::String ((int) std::round (currentValue)) + "Hz";
    }

    if (title.containsIgnoreCase ("ATTACK") || title.containsIgnoreCase ("HOLD") ||
        title.containsIgnoreCase ("RELEASE") || title.containsIgnoreCase ("DELAY"))
    {
        if (currentValue >= 1000.0)
            return juce::String (currentValue / 1000.0, 1) + "s";
        return juce::String (currentValue, currentValue >= 10.0 ? 0 : 1) + "ms";
    }

    if (title.containsIgnoreCase ("RATIO"))
        return juce::String (currentValue, 1) + ":1";

    if (title.containsIgnoreCase ("Q"))
        return juce::String (currentValue, 2);

    return juce::String (currentValue, 1);
}

void ConsoleDial::mouseDown (const juce::MouseEvent& e)
{
    dragStartPos = e.position;
    dragStartValue = currentValue;
}

void ConsoleDial::mouseDrag (const juce::MouseEvent& e)
{
    const float dy = dragStartPos.y - e.position.y;
    const float sensitivity = e.mods.isShiftDown() ? 0.001f : 0.005f;
    const double deltaProp = (double) dy * sensitivity;

    const double currentProp = valueToProportion (dragStartValue);
    const double newProp = juce::jlimit (0.0, 1.0, currentProp + deltaProp);
    setValue (proportionToValue (newProp), juce::sendNotificationAsync);
}

void ConsoleDial::mouseDoubleClick (const juce::MouseEvent&)
{
    setValue (defaultValue, juce::sendNotificationAsync);
}

} // namespace mps
