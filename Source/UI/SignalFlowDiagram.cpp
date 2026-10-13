// Mix Practice Studio - Channel Signal Flow Diagram Implementation
#include "SignalFlowDiagram.h"

namespace mps
{

SignalFlowDiagram::SignalFlowDiagram()
{
    setOpaque (false);

    swapButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff282e38));
    addAndMakeVisible (swapButton);

    preButton.setClickingTogglesState (true);
    preButton.setToggleState (true, juce::dontSendNotification);
    preButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xffe53935)); // Red when PRE
    addAndMakeVisible (preButton);

    insButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff282e38));
    addAndMakeVisible (insButton);

    postButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff282e38));
    addAndMakeVisible (postButton);
}

void SignalFlowDiagram::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float midY = bounds.getY() + 26.0f;

    // 1. Horizontal Signal Bus Line
    const float startX = bounds.getX() + 10.0f;
    const float endX = bounds.getRight() - 20.0f;

    g.setColour (juce::Colour (0xff9aa4b2));
    g.drawLine (startX, midY, endX, midY, 1.5f);

    // Right Arrowhead
    juce::Path arrow;
    arrow.startNewSubPath (endX, midY - 4.0f);
    arrow.lineTo (endX + 6.0f, midY);
    arrow.lineTo (endX, midY + 4.0f);
    arrow.closeSubPath();
    g.fillPath (arrow);

    // 2. Draw Processing Blocks along the line
    const float blockW = 28.0f;
    const float blockH = 22.0f;

    const float blockPositions[] = {
        startX + 50.0f,   // Gain [|>]
        startX + 120.0f,  // Delay [Δt]
        startX + 190.0f,  // LowCut [/]
        startX + 260.0f,  // EQ [~]
        startX + 310.0f,  // Insert [INS]
        startX + 360.0f   // Dyn [|--|]
    };

    auto drawBlock = [&] (float cx, const juce::String& text, int iconType) {
        juce::Rectangle<float> rect (cx - blockW * 0.5f, midY - blockH * 0.5f, blockW, blockH);

        // Fill background
        g.setColour (juce::Colour (0xff1b1f26));
        g.fillRoundedRectangle (rect, 2.0f);
        g.setColour (juce::Colour (0xff4b5563));
        g.drawRoundedRectangle (rect, 2.0f, 1.0f);

        g.setColour (juce::Colour (0xffe5eaf2));
        if (iconType == 0) // Amp triangle [|>]
        {
            juce::Path p;
            p.startNewSubPath (rect.getX() + 7.0f, rect.getY() + 4.0f);
            p.lineTo (rect.getRight() - 7.0f, rect.getCentreY());
            p.lineTo (rect.getX() + 7.0f, rect.getBottom() - 4.0f);
            p.closeSubPath();
            g.strokePath (p, juce::PathStrokeType (1.2f));
        }
        else if (iconType == 1) // Delta t
        {
            g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
            g.drawText ("\u0394t", rect.toNearestInt(), juce::Justification::centred);
        }
        else if (iconType == 2) // Low Cut curve
        {
            juce::Path p;
            p.startNewSubPath (rect.getX() + 5.0f, rect.getBottom() - 6.0f);
            p.lineTo (rect.getCentreX() - 2.0f, rect.getBottom() - 6.0f);
            p.lineTo (rect.getRight() - 5.0f, rect.getY() + 6.0f);
            g.strokePath (p, juce::PathStrokeType (1.4f));
        }
        else if (iconType == 3) // EQ bell curve
        {
            juce::Path p;
            p.startNewSubPath (rect.getX() + 5.0f, rect.getCentreY() + 3.0f);
            p.quadraticTo (rect.getCentreX(), rect.getY() + 4.0f, rect.getRight() - 5.0f, rect.getCentreY() + 3.0f);
            g.strokePath (p, juce::PathStrokeType (1.4f));
        }
        else if (iconType == 4) // Insert INS
        {
            g.setFont (juce::FontOptions (8.0f, juce::Font::bold));
            g.drawText ("INS", rect.toNearestInt(), juce::Justification::centred);
        }
        else if (iconType == 5) // Dynamics knee
        {
            juce::Path p;
            p.startNewSubPath (rect.getX() + 6.0f, rect.getBottom() - 5.0f);
            p.lineTo (rect.getCentreX(), rect.getCentreY());
            p.lineTo (rect.getRight() - 6.0f, rect.getCentreY() - 2.0f);
            g.strokePath (p, juce::PathStrokeType (1.4f));
        }
        else if (iconType == 6) // Fader / Output
        {
            g.fillRect (rect.getCentreX() - 3.0f, rect.getY() + 4.0f, 6.0f, rect.getHeight() - 8.0f);
            g.drawHorizontalLine ((int) rect.getCentreY(), rect.getX() + 4.0f, rect.getRight() - 4.0f);
        }
    };

    if (currentMode == StripMode::Input)
    {
        drawBlock (blockPositions[0], "", 0); // Gain
        drawBlock (blockPositions[1], "", 1); // Delay
        drawBlock (blockPositions[2], "", 2); // LowCut
        drawBlock (isSwapped ? blockPositions[5] : blockPositions[3], "", 3); // EQ
        drawBlock (blockPositions[4], "", 4); // INS
        drawBlock (isSwapped ? blockPositions[3] : blockPositions[5], "", 5); // Dyn
    }
    else if (currentMode == StripMode::Aux)
    {
        drawBlock (startX + 60.0f, "", 0);   // Gain
        drawBlock (startX + 220.0f, "", 3);  // EQ
        drawBlock (endX - 35.0f, "", 6);     // Fader
    }
    else if (currentMode == StripMode::FxReturn)
    {
        drawBlock (endX - 35.0f, "", 6);     // Fader
    }
    else if (currentMode == StripMode::Bus || currentMode == StripMode::Matrix)
    {
        const float busX = endX - 110.0f;
        drawBlock (busX - 55.0f, "", 2);     // Filter/Curve
        drawBlock (busX - 20.0f, "", 3);     // EQ
        drawBlock (busX + 15.0f, "", 6);     // Fader
    }
}

void SignalFlowDiagram::resized()
{
    const auto bounds = getLocalBounds();
    const int startX = bounds.getX() + 10;
    const int endX = bounds.getRight() - 20;

    const bool showFlowButtons = (currentMode == StripMode::Input || currentMode == StripMode::Bus || currentMode == StripMode::Matrix);
    swapButton.setVisible (showFlowButtons);
    preButton.setVisible (showFlowButtons);
    insButton.setVisible (showFlowButtons);
    postButton.setVisible (showFlowButtons);

    if (showFlowButtons)
    {
        if (currentMode == StripMode::Bus || currentMode == StripMode::Matrix)
        {
            const int busX = endX - 120;
            swapButton.setBounds (busX - 14, 2, 46, 16);
            preButton.setBounds (busX - 32, 40, 30, 18);
            insButton.setBounds (busX + 4, 40, 30, 18);
            postButton.setBounds (busX + 40, 40, 32, 18);
        }
        else
        {
            swapButton.setBounds (startX + 275, 4, 46, 16);
            preButton.setBounds (startX + 252, 42, 30, 18);
            insButton.setBounds (startX + 288, 42, 30, 18);
            postButton.setBounds (startX + 324, 42, 32, 18);
        }
    }
}

} // namespace mps
