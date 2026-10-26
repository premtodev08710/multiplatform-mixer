#pragma once

#include "../Audio/AudioEngine.h"
#include "X32FxRack.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <functional>

namespace mps
{

inline bool isLexiconLarcReverb (FxType type) noexcept
{
    return type == FxType::HallReverb
        || type == FxType::AmbienceReverb
        || type == FxType::RichPlateReverb
        || type == FxType::RoomReverb
        || type == FxType::ChamberReverb
        || type == FxType::VintageRoomReverb;
}

/** Comprehensive Realistic Hardware Simulation Faceplate matching
    exact professional X32 console effects aesthetics as provided in
    behringer_x32_edit_fx_rack_simulator.html and behringer_x32_fx_rack_simulator.cpp */
class FxFaceplateComponent : public juce::Component,
                             public juce::Button::Listener,
                             private juce::Timer
{
public:
    FxFaceplateComponent (AudioEngine& engine);
    ~FxFaceplateComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setSlot (int slotIndex);
    int getSlot() const noexcept { return currentSlot; }
    void refreshFromSlot();

    void setLarcPageIndex (int page);
    int getLarcPageIndex() const noexcept { return larcPageIndex; }

    void openCarousel();
    void showPresetsMenu (juce::Component* target);

    juce::String getInsertTargetText (int slotIdx, int insertPointIndex) const;
    void showInsertMenu (juce::Component* target, int insertPointIndex);

    juce::String getInsertTargetTextForSlot (int slotIdx) const;
    void showInsertMenu (juce::Component* target);
    void syncMixerEntityInserts();

    void buttonClicked (juce::Button* button) override;
    void visibilityChanged() override;

    std::function<void(FxType)> onTypeChangeRequested;
    std::function<void()> onParametersChanged;

private:
    void timerCallback() override;
    void drawLedMeter (juce::Graphics& g, float x, float y, float peak01);

    AudioEngine& engine;
    int currentSlot = 0; // 0..7
    int larcPageIndex = 0;
    int activeEngineLayer = 0; // 0 = Engine 1, 1 = Engine 2

    // Clipboard for Copy / Paste
    bool hasClipboard = false;
    FxType clipboardType = FxType::HallReverb;
    std::array<float, 18> clipboardParams {}, clipboardParamsR {};
    bool clipboardRightEnabled = false;
    float clipboardEncoding = 0.0f;
    std::array<float, 31> clipboardGeq {};

    X32FxLookAndFeel rackLnf;

    // Top Rack Control Panel Controls
    juce::TextButton insertButtons[2]; // Insert 1, Insert 2
    juce::TextButton muteButtons[2];   // MUTE 1, MUTE 2
    juce::TextButton effectsButton { "EFFECTS" };
    juce::TextButton presetsButton { "PRESETS" };

    // Bottom Rack Footer Controls
    juce::TextButton copyButton { "Copy" };
    juce::TextButton pasteButton { "Paste" };
    juce::TextButton engineButtons[2] { juce::TextButton ("1"), juce::TextButton ("2") };

    // Hardware Canvas and Carousel Overlay from X32FxRack
    X32HardwareUnitCanvas unitCanvas;
    X32EffectCarouselOverlay carousel;

    float peakInL = 0.0f;
    float peakInR = 0.0f;
    float peakOutL = 0.0f;
    float peakOutR = 0.0f;

    bool ignoreCallbacks = false;
};

} // namespace mps
