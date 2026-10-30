// Mix Practice Studio - Audio Import Progress Overlay (0-100% Progress Bar)
#pragma once

#include "ConsoleLookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace mps
{

class AudioImportProgressOverlay : public juce::Component
{
public:
    AudioImportProgressOverlay()
    {
        setAlwaysOnTop (true);
        setInterceptsMouseClicks (true, true); // Block interaction with background while loading
    }

    void setProgress (float progress01, const juce::String& status, const juce::String& detail)
    {
        progressFraction = juce::jlimit (0.0f, 1.0f, progress01);
        statusMessage = status;
        detailMessage = detail;
        repaint();
    }

    void reset()
    {
        progressFraction = 0.0f;
        statusMessage.clear();
        detailMessage.clear();
        repaint();
    }

    float getProgress() const noexcept { return progressFraction; }

    void paint (juce::Graphics& g) override
    {
        // Dark translucent overlay backdrop
        g.fillAll (juce::Colours::black.withAlpha (0.65f));

        const int boxW = std::min (480, getWidth() - 32);
        const int boxH = 150;
        const auto bounds = getLocalBounds();
        auto boxArea = bounds.withSizeKeepingCentre (boxW, boxH).toFloat();

        // Modal Dialog Box Background
        g.setColour (ConsoleLookAndFeel::bgPanel);
        g.fillRoundedRectangle (boxArea, 8.0f);

        // Subtle glowing border
        g.setColour (ConsoleLookAndFeel::accentCyan.withAlpha (0.6f));
        g.drawRoundedRectangle (boxArea, 8.0f, 1.5f);

        auto inner = boxArea.reduced (16.0f);

        // Header Title
        g.setColour (ConsoleLookAndFeel::accentCyan);
        g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
        g.drawText (juce::CharPointer_UTF8 ("\xe2\xac\x87 IMPORTING AUDIO TRACKS"),
                    inner.removeFromTop (22.0f), juce::Justification::centredLeft);

        // Status description
        g.setColour (ConsoleLookAndFeel::textPrimary);
        g.setFont (juce::FontOptions (13.0f));
        g.drawText (statusMessage, inner.removeFromTop (20.0f), juce::Justification::centredLeft, true);

        // Detailed filename
        g.setColour (ConsoleLookAndFeel::textMuted);
        g.setFont (juce::FontOptions (11.0f));
        g.drawText (detailMessage, inner.removeFromTop (18.0f), juce::Justification::centredLeft, true);

        inner.removeFromTop (8.0f);

        // Progress Bar
        auto barArea = inner.removeFromTop (24.0f);
        g.setColour (ConsoleLookAndFeel::bgDark);
        g.fillRoundedRectangle (barArea, 5.0f);
        g.setColour (ConsoleLookAndFeel::borderSubtle);
        g.drawRoundedRectangle (barArea, 5.0f, 1.0f);

        const float fillW = barArea.getWidth() * progressFraction;
        if (fillW > 2.0f)
        {
            auto fillArea = barArea.withWidth (fillW);
            juce::ColourGradient grad (ConsoleLookAndFeel::accentCyan, fillArea.getX(), fillArea.getY(),
                                       juce::Colour (0xff00b0ff), fillArea.getRight(), fillArea.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (fillArea, 5.0f);
        }

        // Percentage label inside/over progress bar
        const int percent = (int) std::lround (progressFraction * 100.0f);
        g.setColour (percent > 55 ? ConsoleLookAndFeel::bgDark : ConsoleLookAndFeel::textPrimary);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (juce::String (percent) + " %", barArea, juce::Justification::centred, false);
    }

private:
    float progressFraction = 0.0f;
    juce::String statusMessage;
    juce::String detailMessage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioImportProgressOverlay)
};

} // namespace mps
