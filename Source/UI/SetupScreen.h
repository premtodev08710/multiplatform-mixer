// Mix Practice Studio - Audio Device & Hardware Setup Screen
#pragma once

#include "../Audio/AudioEngine.h"
#include "ConsoleLookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>

namespace mps
{

class SetupScreen : public juce::Component,
                    public juce::Button::Listener,
                    private juce::Timer
{
public:
    SetupScreen (AudioEngine& engine);
    ~SetupScreen() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void buttonClicked (juce::Button* button) override;

private:
    void timerCallback() override;

    AudioEngine& engine;

    // JUCE Audio Device Selector Component (CoreAudio output selector, buffer, sample rate)
    std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;

    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::TextButton testToneButton { "Play Test Tone (1 kHz Beep)" };
    juce::TextButton useSpeakersButton { "Switch to MacBook Speakers" };
    juce::TextButton useHeadphonesButton { "Switch to Headphones" };

    bool isPlayingTestTone = false;
    double testTonePhase = 0.0;
};

} // namespace mps
