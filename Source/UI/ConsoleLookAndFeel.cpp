#include "ConsoleLookAndFeel.h"

namespace mps
{

const juce::Colour ConsoleLookAndFeel::bgDark       = juce::Colour (0xff121417);
const juce::Colour ConsoleLookAndFeel::bgPanel      = juce::Colour (0xff1a1d22);
const juce::Colour ConsoleLookAndFeel::bgSection    = juce::Colour (0xff23272e);
const juce::Colour ConsoleLookAndFeel::borderSubtle = juce::Colour (0xff323842);
const juce::Colour ConsoleLookAndFeel::textPrimary  = juce::Colour (0xffe6e8eb);
const juce::Colour ConsoleLookAndFeel::textMuted    = juce::Colour (0xff8b949e);
const juce::Colour ConsoleLookAndFeel::accentCyan   = juce::Colour (0xff00d2ff);
const juce::Colour ConsoleLookAndFeel::accentYellow = juce::Colour (0xffffc107);
const juce::Colour ConsoleLookAndFeel::accentRed    = juce::Colour (0xffe53935);
const juce::Colour ConsoleLookAndFeel::accentGreen  = juce::Colour (0xff4caf50);

ConsoleLookAndFeel::ConsoleLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, bgDark);
    setColour (juce::Label::textColourId, textPrimary);
    setColour (juce::TextButton::textColourOffId, textPrimary);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
}

void ConsoleLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPosProportional, float rotaryStartAngle,
                                           float rotaryEndAngle, juce::Slider& slider)
{
    const float radius = (float) std::min (width, height) * 0.45f;
    const float centreX = (float) x + (float) width * 0.5f;
    const float centreY = (float) y + (float) height * 0.5f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float rw = radius * 2.0f;
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // 1. Background Arc Track
    juce::Path bgArc;
    bgArc.addCentredArc (centreX, centreY, radius - 3.0f, radius - 3.0f, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0xff282d35));
    g.strokePath (bgArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 2. Active Arc
    juce::Path activeArc;
    activeArc.addCentredArc (centreX, centreY, radius - 3.0f, radius - 3.0f, 0.0f,
                             rotaryStartAngle, angle, true);
    g.setColour (accentCyan);
    g.strokePath (activeArc, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 3. Dial Body (tactile dark metallic circle with subtle bevel)
    const float dialRadius = radius - 8.0f;
    const juce::Rectangle<float> dialArea (centreX - dialRadius, centreY - dialRadius, dialRadius * 2.0f, dialRadius * 2.0f);

    juce::ColourGradient grad (juce::Colour (0xff3a404a), centreX, centreY - dialRadius,
                               juce::Colour (0xff1f2329), centreX, centreY + dialRadius, false);
    g.setGradientFill (grad);
    g.fillEllipse (dialArea);

    g.setColour (juce::Colour (0xff4f5865));
    g.drawEllipse (dialArea, 1.2f);

    // 4. Indicator pointer notch
    juce::Path pointer;
    const float pointerLength = dialRadius * 0.75f;
    const float pointerThickness = 2.5f;
    pointer.addRoundedRectangle (-pointerThickness * 0.5f, -dialRadius + 2.0f, pointerThickness, pointerLength, 1.0f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));

    g.setColour (slider.isEnabled() ? accentCyan : textMuted);
    g.fillPath (pointer);
}

void ConsoleLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                               const juce::Colour& backgroundColour,
                                               bool shouldDrawButtonAsHighlighted,
                                               bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    const float cornerSize = 4.0f;

    juce::Colour base = backgroundColour;
    if (base.isTransparent())
        base = bgSection;

    if (button.getToggleState())
    {
        // Engaged state
        if (button.getName().containsIgnoreCase ("mute"))
            base = accentRed;
        else if (button.getName().containsIgnoreCase ("solo"))
            base = accentYellow;
        else
            base = accentCyan;
    }
    else if (shouldDrawButtonAsDown)
    {
        base = base.darker (0.2f);
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        base = base.brighter (0.15f);
    }

    g.setColour (base);
    g.fillRoundedRectangle (bounds, cornerSize);

    // Subtle 3D border
    g.setColour (button.getToggleState() ? base.brighter (0.3f) : borderSubtle);
    g.drawRoundedRectangle (bounds, cornerSize, 1.0f);
}

void ConsoleLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                         bool /*shouldDrawButtonAsHighlighted*/,
                                         bool /*shouldDrawButtonAsDown*/)
{
    auto bounds = button.getLocalBounds();
    const juce::String btnName = button.getName().toLowerCase();

    if (btnName.startsWith ("nav_"))
    {
        const bool active = button.getToggleState();
        juce::Colour iconCol = active ? juce::Colours::white : textMuted;
        g.setColour (iconCol);

        auto iconArea = bounds.removeFromTop (bounds.getHeight() - 14).toFloat();
        const float cx = iconArea.getCentreX();
        const float cy = iconArea.getCentreY();

        if (btnName.contains ("home"))
        {
            juce::Path p;
            p.startNewSubPath (cx, cy - 6.0f);
            p.lineTo (cx + 7.0f, cy);
            p.lineTo (cx + 5.0f, cy);
            p.lineTo (cx + 5.0f, cy + 6.0f);
            p.lineTo (cx - 5.0f, cy + 6.0f);
            p.lineTo (cx - 5.0f, cy);
            p.lineTo (cx - 7.0f, cy);
            p.closeSubPath();
            g.strokePath (p, juce::PathStrokeType (1.3f));
        }
        else if (btnName.contains ("detail"))
        {
            juce::Rectangle<float> r (cx - 7.0f, cy - 6.0f, 14.0f, 12.0f);
            g.drawRoundedRectangle (r, 1.5f, 1.2f);
            g.drawHorizontalLine ((int) cy, cx - 4.0f, cx + 4.0f);
            g.fillEllipse (cx - 4.0f, cy - 4.0f, 3.0f, 3.0f);
            g.fillEllipse (cx + 1.0f, cy + 1.0f, 3.0f, 3.0f);
        }
        else if (btnName.contains ("effects") || btnName.contains ("tracks"))
        {
            juce::Path p;
            p.startNewSubPath (cx - 7.0f, cy);
            p.quadraticTo (cx - 3.5f, cy - 6.0f, cx, cy);
            p.quadraticTo (cx + 3.5f, cy + 6.0f, cx + 7.0f, cy);
            g.strokePath (p, juce::PathStrokeType (1.4f));
        }
        else if (btnName.contains ("scenes"))
        {
            for (int i = -1; i <= 1; ++i)
            {
                const float fx = cx + (float) i * 5.0f;
                g.drawVerticalLine ((int) fx, cy - 6.0f, cy + 6.0f);
                g.fillRect (fx - 2.0f, cy + (float) i * 2.0f - 1.0f, 4.0f, 2.5f);
            }
        }
        else if (btnName.contains ("meters"))
        {
            const float barH[] = { 6.0f, 10.0f, 12.0f, 8.0f };
            for (int i = 0; i < 4; ++i)
            {
                const float bx = cx - 6.0f + (float) i * 3.5f;
                g.fillRect (bx, cy + 6.0f - barH[i], 2.5f, barH[i]);
            }
        }
        else if (btnName.contains ("recorder"))
        {
            g.drawEllipse (cx - 6.0f, cy - 4.0f, 5.0f, 5.0f, 1.2f);
            g.drawEllipse (cx + 1.0f, cy - 4.0f, 5.0f, 5.0f, 1.2f);
            g.drawHorizontalLine ((int) cy + 3, cx - 4.0f, cx + 4.0f);
        }
        else if (btnName.contains ("routing"))
        {
            for (int r = -1; r <= 1; ++r)
                for (int c = -1; c <= 1; ++c)
                    g.fillEllipse (cx + (float) c * 4.0f - 1.0f, cy + (float) r * 4.0f - 1.0f, 2.0f, 2.0f);
        }
        else if (btnName.contains ("monitor"))
        {
            juce::Path p;
            p.addCentredArc (cx, cy - 1.0f, 6.0f, 6.0f, 0.0f, -juce::MathConstants<float>::pi * 0.5f, juce::MathConstants<float>::pi * 0.5f, true);
            g.strokePath (p, juce::PathStrokeType (1.3f));
            g.fillRoundedRectangle (cx - 7.0f, cy - 1.0f, 2.5f, 5.0f, 1.0f);
            g.fillRoundedRectangle (cx + 4.5f, cy - 1.0f, 2.5f, 5.0f, 1.0f);
        }
        else if (btnName.contains ("setup"))
        {
            g.drawEllipse (cx - 5.0f, cy - 5.0f, 10.0f, 10.0f, 2.0f);
            g.fillEllipse (cx - 2.0f, cy - 2.0f, 4.0f, 4.0f);
        }

        g.setFont (juce::FontOptions (10.0f, active ? juce::Font::bold : juce::Font::plain));
        g.setColour (active ? juce::Colours::white : textMuted);
        g.drawFittedText (button.getButtonText(), bounds, juce::Justification::centred, 1);
        return;
    }

    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    juce::Colour textCol = textPrimary;
    if (button.getToggleState())
    {
        if (button.getName().containsIgnoreCase ("solo"))
            textCol = juce::Colours::black; // Yellow background -> black text for high contrast
        else
            textCol = juce::Colours::white;
    }
    g.setColour (textCol);
    g.drawFittedText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, 1);
}

void ConsoleLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                                           const juce::Slider::SliderStyle style, juce::Slider& /*slider*/)
{
    if (style == juce::Slider::LinearVertical)
    {
        // Fader track slot
        const float trackWidth = 4.0f;
        const float trackX = (float) x + ((float) width - trackWidth) * 0.5f;
        juce::Rectangle<float> track ((float) trackX, (float) y + 10.0f, trackWidth, (float) height - 20.0f);

        g.setColour (juce::Colour (0xff0a0c0e));
        g.fillRoundedRectangle (track, 2.0f);
        g.setColour (juce::Colour (0xff252930));
        g.drawRoundedRectangle (track, 2.0f, 1.0f);

        // Tactile Fader Cap
        const float capW = (float) std::min (width - 4, 38);
        const float capH = 26.0f;
        const float capX = (float) x + ((float) width - capW) * 0.5f;
        const float capY = sliderPos - capH * 0.5f;

        juce::Rectangle<float> capRect (capX, capY, capW, capH);

        // Realistic metallic shading gradient
        juce::ColourGradient capGrad (juce::Colour (0xff484e58), capX, capY,
                                      juce::Colour (0xff1e2126), capX, capY + capH, false);
        capGrad.addColour (0.5, juce::Colour (0xff2e333b));
        g.setGradientFill (capGrad);
        g.fillRoundedRectangle (capRect, 3.0f);

        g.setColour (juce::Colour (0xff5a626f));
        g.drawRoundedRectangle (capRect, 3.0f, 1.0f);

        // Center white groove line on the cap
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.fillRect (capX + 4.0f, capY + capH * 0.5f - 1.0f, capW - 8.0f, 2.0f);
    }
}

} // namespace mps
