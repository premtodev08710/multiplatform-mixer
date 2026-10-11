// Mix Practice Studio - Interactive Logarithmic Parametric EQ Response Graph
#pragma once

#include "../Core/ChannelDsp.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace mps
{

class InteractiveEqGraph : public juce::Component
{
public:
    InteractiveEqGraph();
    ~InteractiveEqGraph() override = default;

    void setParams (ChannelDspParams* params, double sampleRate);

    void setSelectedBand (int band);
    int getSelectedBand() const noexcept { return selectedBand; }

    void paint (juce::Graphics& g) override;
    void resized() override;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    std::function<void()> onParamsChanged;
    std::function<void (int)> onBandSelected;

private:
    float freqToX (float f) const noexcept;
    float xToFreq (float x) const noexcept;
    float gainToY (float gDb) const noexcept;
    float yToGain (float y) const noexcept;

    int findHandleUnderMouse (juce::Point<float> pos) const;

    ChannelDspParams* dspParams = nullptr;
    double sr = 48000.0;
    int draggingNode = -1; // 0=HPF, 1=Low, 2=Mid1, 3=Mid2, 4=High
    int selectedBand = 1;

    static const juce::Colour nodeColours[5];
};

} // namespace mps
