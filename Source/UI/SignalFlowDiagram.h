// Mix Practice Studio - Channel Signal Flow Diagram Component
#pragma once

#include "ConsoleLookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace mps
{

class SignalFlowDiagram : public juce::Component
{
public:
    SignalFlowDiagram();
    ~SignalFlowDiagram() override = default;

    void setPreEq (bool pre) { isPre = pre; repaint(); }
    void setSwap (bool swap) { isSwapped = swap; repaint(); }

    void paint (juce::Graphics& g) override;
    void resized() override;

    juce::TextButton swapButton { "SWAP" };
    juce::TextButton preButton { "PRE" };
    juce::TextButton insButton { "INS" };
    juce::TextButton postButton { "POST" };

private:
    bool isPre = true;
    bool isSwapped = false;
};

} // namespace mps
