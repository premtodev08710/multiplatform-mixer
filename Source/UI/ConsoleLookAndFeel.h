// Mix Practice Studio - Custom Look & Feel for dark console audio interface
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace mps
{

class ConsoleLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ConsoleLookAndFeel();
    ~ConsoleLookAndFeel() override = default;

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics& g, juce::TextButton& button,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    // Palette
    static const juce::Colour bgDark;       // 0xff121417
    static const juce::Colour bgPanel;      // 0xff1a1d22
    static const juce::Colour bgSection;    // 0xff23272e
    static const juce::Colour borderSubtle; // 0xff323842
    static const juce::Colour textPrimary;  // 0xffe6e8eb
    static const juce::Colour textMuted;    // 0xff8b949e
    static const juce::Colour accentCyan;   // 0xff00d2ff
    static const juce::Colour accentYellow; // 0xffffc107 (Solo)
    static const juce::Colour accentRed;    // 0xffe53935 (Mute / Clip)
    static const juce::Colour accentGreen;  // 0xff4caf50 (Meter)
};

} // namespace mps
