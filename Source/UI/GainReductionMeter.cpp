// Mix Practice Studio - Vertical Gain Reduction LED Ladder Meter Implementation
#include "GainReductionMeter.h"

namespace mps
{

GainReductionMeter::GainReductionMeter (ScaleType type)
    : scaleType (type)
{
    setOpaque (false);
    updateSteps();
}

void GainReductionMeter::setScaleType (ScaleType type)
{
    if (scaleType != type)
    {
        scaleType = type;
        updateSteps();
        repaint();
    }
}

void GainReductionMeter::setReductionDb (float grDb)
{
    const float absGr = std::abs (grDb);
    if (std::abs (absGr - reductionDb) > 0.05f)
    {
        reductionDb = absGr;
        repaint();
    }
}

void GainReductionMeter::updateSteps()
{
    if (scaleType == ScaleType::Gate)
    {
        stepValues = { 0.5f, 6.0f, 12.0f, 18.0f, 24.0f, 30.0f, 36.0f, 42.0f, 48.0f, 54.0f };
        stepLabels = { "Thr", "6", "12", "18", "24", "30", "36", "42", "48", "54" };
    }
    else
    {
        stepValues = { 0.5f, 2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 12.0f, 14.0f, 16.0f, 18.0f };
        stepLabels = { "Thr", "2", "4", "6", "8", "10", "12", "14", "16", "18" };
    }
}

void GainReductionMeter::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    // LED Column on left (width ~10px)
    const float ledColW = 9.0f;
    const float ledX = bounds.getX() + 2.0f;
    const float labelX = ledX + ledColW + 4.0f;
    const float labelW = w - labelX;

    const size_t numSteps = stepValues.size();
    if (numSteps == 0) return;

    const float segSpacing = 2.0f;
    const float segHeight = (h - (float) (numSteps + 1) * segSpacing) / (float) numSteps;

    g.setFont (juce::FontOptions (8.0f, juce::Font::bold));

    for (size_t i = 0; i < numSteps; ++i)
    {
        const float segY = bounds.getY() + segSpacing + (float) i * (segHeight + segSpacing);
        const juce::Rectangle<float> segRect (ledX, segY, ledColW, segHeight);

        const bool isLit = (reductionDb >= stepValues[i]);

        // Draw LED Pill
        if (isLit)
        {
            // Amber/Red glow when reducing gain
            const juce::Colour litColour = (i == 0)
                ? juce::Colour (0xffffb300) // Thr: Amber
                : (i < 5) ? juce::Colour (0xffff9800) // Orange
                          : juce::Colour (0xffe53935); // High reduction: Red
            g.setColour (litColour);
            g.fillRoundedRectangle (segRect, 1.5f);

            g.setColour (juce::Colours::white.withAlpha (0.4f));
            g.drawRoundedRectangle (segRect, 1.5f, 0.8f);
        }
        else
        {
            // Unlit dark LED slot
            g.setColour (juce::Colour (0xff181b20));
            g.fillRoundedRectangle (segRect, 1.5f);

            g.setColour (juce::Colour (0xff242930));
            g.drawRoundedRectangle (segRect, 1.5f, 0.6f);
        }

        // Draw Label on the right
        g.setColour (isLit ? juce::Colour (0xffe0e4ea) : juce::Colour (0xff667080));
        g.drawText (stepLabels[i], (int) labelX, (int) segY - 1, (int) labelW, (int) segHeight + 2,
                    juce::Justification::centredLeft);
    }
}

} // namespace mps
