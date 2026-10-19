// Mix Practice Studio - Effects Rack UI Screen
#pragma once

#include "../Audio/AudioEngine.h"
#include "../Model/ProjectModel.h"
#include "ChannelStripComponent.h"
#include "ConsoleLookAndFeel.h"
#include "FxFaceplateComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

namespace mps
{

class EffectsScreen : public juce::Component,
                      public juce::Button::Listener
{
public:
    EffectsScreen (AudioEngine& engine, ProjectModel& project);
    ~EffectsScreen() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void buttonClicked (juce::Button* button) override;

    void refreshFromModel();

    void setSelectedStrip (ChannelStripComponent::StripType type, int index)
    {
        currentStripType = type;
        currentStripIndex = index;
    }
    ChannelStripComponent::StripType getSelectedStripType() const noexcept { return currentStripType; }
    int getSelectedStripIndex() const noexcept { return currentStripIndex; }

    FxFaceplateComponent& getFaceplate() noexcept { return faceplate; }

    std::function<void()> onBack;
    std::function<void(ChannelStripComponent::StripType, int)> onStripNavigated;

private:
    AudioEngine& engine;
    ProjectModel& project;

    ChannelStripComponent::StripType currentStripType = ChannelStripComponent::StripType::InputChannel;
    int currentStripIndex = 0;

    // Top X32 Navigation Header Bar
    juce::TextButton backButton { "< Mixer" };
    juce::TextButton libraryHeaderButton { "FX1-4 LIBRARY (8 PAGES)" };

    // FX 1 to FX 8 Slots Tabs
    std::array<juce::TextButton, 8> slotTabs;
    int selectedSlot = 0; // 0..7

    // Legacy group buttons kept for test compatibility
    enum RackGroup { Group1To4 = 0, Group5To8, NumGroups };
    juce::TextButton groupButtons[NumGroups];

    // Hardware Simulated Front Panel Faceplate (X32 Rack Module)
    FxFaceplateComponent faceplate { engine };

    bool ignoreCallbacks = false;
};

} // namespace mps
