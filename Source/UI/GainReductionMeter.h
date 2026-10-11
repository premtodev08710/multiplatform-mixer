// Mix Practice Studio - Vertical Gain Reduction LED Ladder Meter
#pragma once

#include "ConsoleLookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace mps
{

class GainReductionMeter : public juce::Component
{
public:
    enum class ScaleType { Gate, Compressor };

    GainReductionMeter (ScaleType type = ScaleType::Gate);
    ~GainReductionMeter() override = default;

    void setScaleType (ScaleType type);
    void setReductionDb (float grDb);

    void paint (juce::Graphics& g) override;

private:
    ScaleType scaleType = ScaleType::Gate;
    float reductionDb = 0.0f; // Positive dB of gain reduction (e.g. 0 to 54)
    std::vector<float> stepValues;
    std::vector<juce::String> stepLabels;

    void updateSteps();
};

} // namespace mps
