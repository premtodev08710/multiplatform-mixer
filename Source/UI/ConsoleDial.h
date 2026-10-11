// Mix Practice Studio - Professional Console Rotary Dial with Peripheral Tick Labels
#pragma once

#include "ConsoleLookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <string>

namespace mps
{

class ConsoleDial : public juce::Component
{
public:
    struct TickLabel
    {
        double value;
        juce::String text;
    };

    ConsoleDial (const juce::String& title = {}, double minVal = 0.0, double maxVal = 1.0,
                 double defaultVal = 0.0, double step = 0.01, bool isLog = false);
    ~ConsoleDial() override = default;

    void setTitle (const juce::String& t);
    void setRange (double minVal, double maxVal, double step = 0.01, bool isLog = false);
    void setDefaultValue (double val) noexcept { defaultValue = val; }
    void setValue (double val, juce::NotificationType notification = juce::dontSendNotification);
    double getValue() const noexcept { return currentValue; }

    void setTickLabels (const std::vector<TickLabel>& labels);
    void setUnit (const juce::String& u) { unit = u; }
    void setArcColour (juce::Colour col) { arcColour = col; }
    void setValueFormatter (std::function<juce::String (double)> f) { valueFormatter = std::move (f); }

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    juce::String getFormattedValueText() const;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;

    std::function<void (double)> onValueChanged;
    std::function<juce::String (double)> valueFormatter;

private:
    double valueToProportion (double v) const;
    double proportionToValue (double prop) const;
    float valueToAngle (double v) const;

    juce::String title;
    juce::String unit;
    double minValue = 0.0;
    double maxValue = 1.0;
    double defaultValue = 0.0;
    double stepSize = 0.01;
    double currentValue = 0.0;
    bool isLogarithmic = false;

    std::vector<TickLabel> tickLabels;
    juce::Colour arcColour { juce::Colour (0xff00d2ff) };

    juce::Point<float> dragStartPos;
    double dragStartValue = 0.0;
};

} // namespace mps
