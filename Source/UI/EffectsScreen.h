// Mix Practice Studio - Effects Rack UI Screen
#pragma once

#include "../Audio/AudioEngine.h"
#include "../Model/ProjectModel.h"
#include "ConsoleLookAndFeel.h"
#include "ConsoleDial.h"
#include "LedMeter.h"
#include "FxFaceplateComponent.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

namespace mps
{

class EffectsScreen : public juce::Component,
                      public juce::Button::Listener,
                      public juce::ComboBox::Listener
{
public:
    EffectsScreen (AudioEngine& engine, ProjectModel& project);
    ~EffectsScreen() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void buttonClicked (juce::Button* button) override;
    void comboBoxChanged (juce::ComboBox* comboBox) override;

    void refreshFromModel();

    std::function<void()> onBack;

private:
    juce::TextButton backButton { "< Mixer" };
    AudioEngine& engine;
    ProjectModel& project;

    // Top rack group tabs: FX 1-4 (Sends/Returns) vs FX 5-8 (Inserts)
    enum RackGroup { Group1To4 = 0, Group5To8, NumGroups };
    juce::TextButton groupButtons[NumGroups];
    int currentGroup = Group1To4;
    int selectedSlot = 0; // 0..7

    // 8 Rack Slots Header Controls
    struct SlotUi
    {
        juce::Label slotTitle;
        juce::ComboBox typeCombo;
        juce::TextButton bypassButton { "IN" };
        juce::TextButton modeButton { "SEND/RET" };
        ConsoleDial mixDial { "MIX", 0.0, 100.0, 100.0, 1.0 };
        LedMeter inMeter { LedMeter::Mode::Stereo };
        LedMeter outMeter { LedMeter::Mode::Stereo };
        juce::TextButton selectButton { "EDIT" };
    };
    std::array<SlotUi, 8> slotUis;

    // Realistic Hardware Simulated Front Panel Faceplate
    FxFaceplateComponent faceplate { engine };

    void setupSlots();
    void updateGroupVisibility();
    bool ignoreCallbacks = false;
};

} // namespace mps
