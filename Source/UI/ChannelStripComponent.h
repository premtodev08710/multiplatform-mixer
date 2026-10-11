// Mix Practice Studio - Channel Strip Component (X32/M32 Console Style)
#pragma once

#include "../Audio/AudioEngine.h"
#include "../Model/ProjectModel.h"
#include "ConsoleLookAndFeel.h"
#include "LedMeter.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace mps
{

class ChannelStripComponent : public juce::Component,
                              public juce::Slider::Listener,
                              public juce::Button::Listener,
                              private juce::Timer
{
public:
    enum class StripType
    {
        InputChannel,
        FxReturn,
        Aux,
        Bus,
        Matrix,
        MainC,
        Master
    };

    ChannelStripComponent (int channelIndex, AudioEngine& engine, ProjectModel& project,
                           StripType type = StripType::InputChannel);
    ~ChannelStripComponent() override;

    int getChannelIndex() const noexcept { return channelIndex; }
    StripType getStripType() const noexcept { return stripType; }
    bool isSelected() const noexcept { return selected; }
    void setSelected (bool s);

    void setSendsOnFader (int targetFx); // -1 = normal channel fader, 0..3 = send to FX 1..4
    int getSendsOnFader() const noexcept { return sendsOnFaderTarget; }

    void updateFromModel();

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;

    void sliderValueChanged (juce::Slider* slider) override;
    void buttonClicked (juce::Button* button) override;

    std::function<void (int)> onSelected;

private:
    void timerCallback() override;

    int channelIndex = 0;
    AudioEngine& engine;
    ProjectModel& project;
    StripType stripType = StripType::InputChannel;
    bool selected = false;
    int sendsOnFaderTarget = -1;

    // UI Controls
    juce::Label scribbleName;
    juce::Label scribbleSource;
    juce::Label gainLabel;
    juce::Slider panSlider;
    juce::TextButton soloButton { "SOLO" };
    juce::TextButton muteButton { "MUTE" };
    juce::Slider faderSlider;
    juce::Label faderValueLabel;
    LedMeter meter;

    bool ignoreCallbacks = false;
};

} // namespace mps
