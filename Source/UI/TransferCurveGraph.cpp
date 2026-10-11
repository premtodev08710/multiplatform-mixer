// Mix Practice Studio - Transfer Curve Graph Implementation
#include "TransferCurveGraph.h"

namespace mps
{

TransferCurveGraph::TransferCurveGraph (Mode m)
    : mode (m)
{
    setOpaque (false);
}

void TransferCurveGraph::setMode (Mode m)
{
    mode = m;
    repaint();
}

void TransferCurveGraph::setGateParams (float thresholdDb, float rangeDb)
{
    gateThreshold = thresholdDb;
    gateRange = rangeDb;
    repaint();
}

void TransferCurveGraph::setCompParams (float thresholdDb, float ratio, float kneeDb)
{
    compThreshold = thresholdDb;
    compRatio = std::max (1.0f, ratio);
    compKnee = std::max (0.0f, kneeDb);
    repaint();
}

void TransferCurveGraph::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    // 1. Dark container background
    g.setColour (juce::Colour (0xff0d0f12));
    g.fillRoundedRectangle (bounds, 3.0f);
    g.setColour (juce::Colour (0xff242932));
    g.drawRoundedRectangle (bounds, 3.0f, 1.0f);

    // 2. Fine Grid lines (10 x 10 divisions)
    g.setColour (juce::Colour (0xff1a1e24));
    const int numDivs = 10;
    for (int i = 1; i < numDivs; ++i)
    {
        const float x = bounds.getX() + (float) i * (w / (float) numDivs);
        const float y = bounds.getY() + (float) i * (h / (float) numDivs);
        g.drawVerticalLine ((int) x, bounds.getY() + 1.0f, bounds.getBottom() - 1.0f);
        g.drawHorizontalLine ((int) y, bounds.getX() + 1.0f, bounds.getRight() - 1.0f);
    }

    // 3. Diagonal Reference Line (1:1 unity gain)
    g.setColour (juce::Colour (0xff282f3a));
    g.drawLine (bounds.getX(), bounds.getBottom(), bounds.getRight(), bounds.getY(), 1.0f);

    // 4. Calculate and draw transfer curve
    const float minDb = (mode == Mode::Gate) ? -80.0f : -60.0f;
    const float maxDb = 0.0f;
    const float dbSpan = maxDb - minDb;

    auto dbToX = [&] (float db) {
        return bounds.getX() + ((db - minDb) / dbSpan) * w;
    };
    auto dbToY = [&] (float db) {
        return bounds.getBottom() - ((db - minDb) / dbSpan) * h;
    };

    juce::Path curve;
    const int numPoints = (int) w;

    if (mode == Mode::Gate)
    {
        for (int i = 0; i < numPoints; ++i)
        {
            const float norm = (float) i / (float) (numPoints - 1);
            const float inDb = minDb + norm * dbSpan;
            float outDb;

            if (inDb >= gateThreshold)
            {
                outDb = inDb;
            }
            else
            {
                // Attenuation down by gateRange
                outDb = std::max (minDb, inDb - gateRange);
            }

            const float x = dbToX (inDb);
            const float y = dbToY (outDb);

            if (i == 0) curve.startNewSubPath (x, y);
            else curve.lineTo (x, y);
        }
    }
    else // Compressor
    {
        const float halfKnee = compKnee * 0.5f;
        const float kneeLower = compThreshold - halfKnee;
        const float kneeUpper = compThreshold + halfKnee;

        for (int i = 0; i < numPoints; ++i)
        {
            const float norm = (float) i / (float) (numPoints - 1);
            const float inDb = minDb + norm * dbSpan;
            float outDb;

            if (inDb <= kneeLower || compKnee <= 0.001f)
            {
                if (inDb <= compThreshold)
                    outDb = inDb;
                else
                    outDb = compThreshold + (inDb - compThreshold) / compRatio;
            }
            else if (inDb >= kneeUpper)
            {
                outDb = compThreshold + (inDb - compThreshold) / compRatio;
            }
            else
            {
                // Soft-knee quadratic polynomial
                const float diff = inDb - kneeLower;
                outDb = inDb + ((1.0f / compRatio - 1.0f) * diff * diff) / (2.0f * compKnee);
            }

            const float x = dbToX (inDb);
            const float y = dbToY (outDb);

            if (i == 0) curve.startNewSubPath (x, y);
            else curve.lineTo (x, y);
        }
    }

    // Glowing blue curve stroke
    g.setColour (juce::Colour (0xff00d2ff));
    g.strokePath (curve, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

} // namespace mps
