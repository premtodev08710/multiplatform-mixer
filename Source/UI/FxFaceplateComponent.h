// Mix Practice Studio - Hardware Simulated FX Faceplate Component
#pragma once

#include "../Audio/AudioEngine.h"
#include "ConsoleLookAndFeel.h"
#include "ConsoleDial.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <string>
#include <functional>

namespace mps
{

/** Comprehensive Realistic Hardware Simulation Faceplate matching
    exact professional console effects aesthetics (Cream Fader Reverbs,
    Blue/Green LCD Reverbs, Vintage EMT250/Lexicon 480L, Colorful Modulators,
    Dimension-C, Rotary Woodgrain, 31-Band GEQs, Dual De-Essers, Pultec EQP,
    Royal Blue Wave Designer, Precision Limiter, Combinator, and Analog Tubes). */
class FxFaceplateComponent : public juce::Component,
                             public juce::Button::Listener,
                             public juce::Slider::Listener,
                             private juce::Timer
{
public:
    FxFaceplateComponent (AudioEngine& engine);
    ~FxFaceplateComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setSlot (int slotIndex);
    void refreshFromSlot();

    void buttonClicked (juce::Button* button) override;
    void sliderValueChanged (juce::Slider* slider) override;

    std::function<void(FxType)> onTypeChangeRequested;
    std::function<void()> onParametersChanged;

private:
    void timerCallback() override;

    AudioEngine& engine;
    int currentSlot = 0; // 0..7

    // Dynamic 12 Dials for 1-row or 2-row knob configurations
    static constexpr int kMaxDials = 12;
    std::array<ConsoleDial, kMaxDials> dials;
    std::array<int, kMaxDials> dialParameterIndices {};
    std::array<juce::TextButton, 3> parameterPageButtons;
    int currentParameterPage = 0;

    // 6 Vertical Faders (used for Cream Reverbs and Precision Limiter)
    std::array<juce::Slider, 6> faders;
    std::array<juce::Label, 6> faderLabels;
    std::array<juce::Label, 6> faderValLabels;

    // Reverb Preset Ribbon Buttons (Hall, Ambience, Plate, Room, Chamber, Concert)
    std::array<juce::TextButton, 6> reverbRibbonButtons;

    // 1176 Ratio Pushbuttons
    juce::TextButton ratioButtons[5]; // 4:1, 8:1, 12:1, 20:1, ALL

    // Dimension D / Dimension C Pushbuttons
    juce::TextButton dimButtons[5];  // OFF, 1, 2, 3, 4

    // 31-Band GEQ Sliders & Controls
    std::array<juce::Slider, 31> geqSliders;
    std::array<juce::Label, 31> geqLabels;
    juce::TextButton geqFlatButton { "FLAT" };
    juce::TextButton geqChAButton { "CH A" };
    juce::TextButton geqChBButton { "CH B" };
    int activeGeqChannel = 0; // 0=A, 1=B

    // Quick In/Bypass & Mode switches on Faceplate
    juce::TextButton powerSwitch { "POWER" };
    juce::TextButton bypassSwitch { "BYPASS" };
    juce::TextButton stereoLinkSwitch { "STEREO LINK" };
    juce::TextButton rotarySpeedSwitch { "SLOW / FAST" };

    // Dynamic Ballistic Metering & Animation State
    float vuNeedleL = 0.0f;
    float vuTargetL = 0.0f;
    float vuNeedleR = 0.0f;
    float vuTargetR = 0.0f;
    float tubeGlowPhase = 0.0f;
    float lfoPhase = 0.0f;
    std::array<float, 5> combMeters { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    // Layout Methods
    void layoutCreamReverb (juce::Rectangle<int> area);
    void layoutLcdReverb (juce::Rectangle<int> area);
    void layoutVintageReverb (juce::Rectangle<int> area);
    void layoutVintageRoom (juce::Rectangle<int> area);
    void layoutStereoDelay (juce::Rectangle<int> area);
    void layoutChorusFlangerPhaser (juce::Rectangle<int> area);
    void layoutDimensionC (juce::Rectangle<int> area);
    void layoutMoodFilter (juce::Rectangle<int> area);
    void layoutRotarySpeaker (juce::Rectangle<int> area);
    void layoutTremoloPanner (juce::Rectangle<int> area);
    void layoutSuboctaver (juce::Rectangle<int> area);
    void layoutGEQ (juce::Rectangle<int> area);
    void layoutDeEsser (juce::Rectangle<int> area);
    void layoutXtecEq (juce::Rectangle<int> area);
    void layoutWaveDesigner (juce::Rectangle<int> area);
    void layoutPrecisionLimiter (juce::Rectangle<int> area);
    void layoutCombinator (juce::Rectangle<int> area);
    void layoutLA2A (juce::Rectangle<int> area);
    void layout1176 (juce::Rectangle<int> area);
    void layoutFairchild (juce::Rectangle<int> area);
    void layoutGuitarAmp (juce::Rectangle<int> area);
    void layoutTubeStage (juce::Rectangle<int> area);
    void layoutGenericRack (juce::Rectangle<int> area);

    // Specialized Hardware Faceplate Renderers
    void paintCreamReverb (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& activeName);
    void paintLcdReverb (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& title, juce::Colour lcdColor);
    void paintVintageReverb (juce::Graphics& g, juce::Rectangle<float> area);
    void paintVintageRoom (juce::Graphics& g, juce::Rectangle<float> area);
    void paintStereoDelay (juce::Graphics& g, juce::Rectangle<float> area);
    void paintModulationRack (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& title, juce::Colour bodyColour);
    void paintDimensionC (juce::Graphics& g, juce::Rectangle<float> area);
    void paintMoodFilter (juce::Graphics& g, juce::Rectangle<float> area);
    void paintRotarySpeaker (juce::Graphics& g, juce::Rectangle<float> area);
    void paintTremoloPanner (juce::Graphics& g, juce::Rectangle<float> area);
    void paintSuboctaver (juce::Graphics& g, juce::Rectangle<float> area);
    void paintDeEsser (juce::Graphics& g, juce::Rectangle<float> area, bool isDual);
    void paintXtecEq (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& title);
    void paintWaveDesigner (juce::Graphics& g, juce::Rectangle<float> area);
    void paintPrecisionLimiter (juce::Graphics& g, juce::Rectangle<float> area);
    void paintCombinator (juce::Graphics& g, juce::Rectangle<float> area);
    void paintGraphicEq (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& title);
    void paintLA2A (juce::Graphics& g, juce::Rectangle<float> area);
    void paint1176 (juce::Graphics& g, juce::Rectangle<float> area);
    void paintFairchild (juce::Graphics& g, juce::Rectangle<float> area);
    void paintGuitarAmp (juce::Graphics& g, juce::Rectangle<float> area);
    void paintTubeStage (juce::Graphics& g, juce::Rectangle<float> area);
    void paintGenericStudioRack (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& title, const juce::String& subTitle);

    // Helpers
    void drawAnalogVuMeter (juce::Graphics& g, juce::Rectangle<float> bounds, float needlePos01, const juce::String& label);
    void drawDigitalLcdDisplay (juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& mainText, const juce::String& subText, juce::Colour bgTint);
    void drawVacuumTubeGlow (juce::Graphics& g, juce::Rectangle<float> bounds, float intensity);
    void drawRackMountScrews (juce::Graphics& g, juce::Rectangle<float> area);

    bool ignoreCallbacks = false;
};

} // namespace mps
