// Mix Practice Studio - Transfer Curve Graph for Gate & Dynamics
#pragma once

#include "ConsoleLookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace mps
{

class TransferCurveGraph : public juce::Component
{
public:
    enum class Mode { Gate, Compressor };

    TransferCurveGraph (Mode mode = Mode::Gate);
    ~TransferCurveGraph() override = default;

    void setMode (Mode m);

    // Gate parameters
    void setGateParams (float thresholdDb, float rangeDb);

    // Compressor parameters
    void setCompParams (float thresholdDb, float ratio, float kneeDb);

    void paint (juce::Graphics& g) override;

private:
    Mode mode = Mode::Gate;

    // Gate state
    float gateThreshold = -40.0f;
    float gateRange = 30.0f;

    // Compressor state
    float compThreshold = -20.0f;
    float compRatio = 4.0f;
    float compKnee = 2.0f;
};

} // namespace mps
