// Mix Practice Studio - Dual Trapezoid Console Pan Indicator Widget
#pragma once

#include "ConsoleLookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace mps
{

class TrapezoidPanIndicator : public juce::Component
{
public:
    TrapezoidPanIndicator();
    ~TrapezoidPanIndicator() override = default;

    void setPan (float panVal, juce::NotificationType notification = juce::dontSendNotification);
    float getPan() const noexcept { return pan; }

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;

    std::function<void (float)> onPanChanged;

private:
    float pan = 0.0f; // -1.0 (L) to +1.0 (R)
};

} // namespace mps
