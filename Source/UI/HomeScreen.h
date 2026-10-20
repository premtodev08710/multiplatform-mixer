// Mix Practice Studio - Home Screen (Channel Strips + Bank Selectors + Master Strip)
#pragma once

#include "BankSelectorBar.h"
#include "ChannelStripComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace mps
{

class HomeScreen : public juce::Component,
                   public juce::Button::Listener,
                   public juce::ComboBox::Listener,
                   public juce::FileDragAndDropTarget,
                   private juce::Timer
{
public:
    HomeScreen (AudioEngine& engine, ProjectModel& project);
    ~HomeScreen() override;

    int getSelectedChannel() const noexcept { return selectedChannel; }
    void setSelectedChannel (int ch);
    int getCurrentBank() const noexcept { return currentBank; }

    void paint (juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

    void buttonClicked (juce::Button* button) override;
    void comboBoxChanged (juce::ComboBox* comboBox) override;
    void rebuildStrips();

    // File Drag and Drop directly onto Mixer strips
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    std::function<void (int)> onChannelSelected;
    std::function<void (int)> onOpenDetail;
    std::function<void (ChannelStripComponent::StripType, int)> onOpenDetailStrip;

private:
    void timerCallback() override;

    AudioEngine& engine;
    ProjectModel& project;

    int currentBank = 0; // 0..9 matching BankSelectorBar
    int selectedChannel = 0;

    // Top Bank Selector Bar with live miniature LED meters
    BankSelectorBar bankSelectorBar;

    // Left Control Panel
    juce::GroupComponent leftPanelGroup;
    juce::TextButton sendsOnFadersButton { "SENDS ON FADERS" };
    juce::ComboBox sofTargetCombo;
    bool sendsOnFaderActive = false;
    int sofTarget = 0; // 0 = FX 1, 1 = FX 2, 2 = FX 3, 3 = FX 4

    juce::Label autoMixLabel { {}, "AUTO MIXING" };
    juce::TextButton autoMixXButton { "X" };
    juce::TextButton autoMixYButton { "Y" };

    juce::Label muteGroupsLabel { {}, "MUTE GROUPS" };
    juce::TextButton muteGroupButtons[6];

    juce::TextButton showSolosButton { "SHOW SOLOS" };
    juce::TextButton lockMutesButton { "LOCK MUTES" };

    // Visible Channel Strips
    std::vector<std::unique_ptr<ChannelStripComponent>> channelStrips;

    // Master Strip (anchored on right when not in Mtx-Main bank)
    std::unique_ptr<ChannelStripComponent> masterStrip;
};

} // namespace mps
