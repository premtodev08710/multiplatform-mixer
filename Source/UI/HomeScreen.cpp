#include "HomeScreen.h"

namespace mps
{

HomeScreen::HomeScreen (AudioEngine& eng, ProjectModel& proj)
    : engine (eng), project (proj)
{
    // 1. Top Bank Selector Bar (with miniature 8-column LED meters)
    bankSelectorBar.setMeters (&engine.getMixerMeters());
    bankSelectorBar.onBankSelected = [this] (int bank) {
        currentBank = bank;
        rebuildStrips();
    };
    addAndMakeVisible (bankSelectorBar);

    // 2. Left Control Panel
    sendsOnFadersButton.setClickingTogglesState (true);
    sendsOnFadersButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffffa000)); // Glowing Amber
    sendsOnFadersButton.setTooltip ("Toggle Sends-on-Fader mode to adjust channel FX sends directly on the faders");
    sendsOnFadersButton.addListener (this);
    addAndMakeVisible (sendsOnFadersButton);

    sofTargetCombo.addItem ("SEND: FX 1 (HALL)", 1);
    sofTargetCombo.addItem ("SEND: FX 2 (AMBI)", 2);
    sofTargetCombo.addItem ("SEND: FX 3 (DELAY)", 3);
    sofTargetCombo.addItem ("SEND: FX 4 (CHOR)", 4);
    sofTargetCombo.setSelectedId (1, juce::dontSendNotification);
    sofTargetCombo.addListener (this);
    addAndMakeVisible (sofTargetCombo);

    autoMixLabel.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    autoMixLabel.setJustificationType (juce::Justification::centred);
    autoMixLabel.setColour (juce::Label::textColourId, ConsoleLookAndFeel::textMuted);
    addAndMakeVisible (autoMixLabel);

    autoMixXButton.setClickingTogglesState (true);
    autoMixXButton.addListener (this);
    addAndMakeVisible (autoMixXButton);

    autoMixYButton.setClickingTogglesState (true);
    autoMixYButton.addListener (this);
    addAndMakeVisible (autoMixYButton);

    muteGroupsLabel.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    muteGroupsLabel.setJustificationType (juce::Justification::centred);
    muteGroupsLabel.setColour (juce::Label::textColourId, ConsoleLookAndFeel::textMuted);
    addAndMakeVisible (muteGroupsLabel);

    for (int i = 0; i < 6; ++i)
    {
        muteGroupButtons[i].setButtonText (juce::String (i + 1));
        muteGroupButtons[i].setClickingTogglesState (true);
        muteGroupButtons[i].setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffe53935));
        muteGroupButtons[i].addListener (this);
        addAndMakeVisible (muteGroupButtons[i]);
    }

    showSolosButton.setClickingTogglesState (true);
    showSolosButton.addListener (this);
    addAndMakeVisible (showSolosButton);

    lockMutesButton.setClickingTogglesState (true);
    lockMutesButton.addListener (this);
    addAndMakeVisible (lockMutesButton);

    // 3. Master Strip
    masterStrip = std::make_unique<ChannelStripComponent> (0, engine, project, ChannelStripComponent::StripType::Master);
    addAndMakeVisible (masterStrip.get());

    rebuildStrips();
    startTimerHz (20); // Fast metering refresh for top bank mini-meters
}

HomeScreen::~HomeScreen()
{
    stopTimer();
}

void HomeScreen::timerCallback()
{
    bankSelectorBar.repaint();
}

void HomeScreen::rebuildStrips()
{
    channelStrips.clear();

    if (currentBank >= 0 && currentBank <= 3) // Input Channels 1..32
    {
        const int startCh = currentBank * 8;
        for (int i = 0; i < 8; ++i)
        {
            const int ch = startCh + i;
            auto strip = std::make_unique<ChannelStripComponent> (
                ch, engine, project, ChannelStripComponent::StripType::InputChannel);
            strip->setSelected (ch == selectedChannel);
            strip->setSendsOnFader (sendsOnFaderActive ? sofTarget : -1);
            strip->onSelected = [this] (int selIdx) {
                setSelectedChannel (selIdx);
                if (onChannelSelected) onChannelSelected (selIdx);
            };
            addAndMakeVisible (strip.get());
            channelStrips.push_back (std::move (strip));
        }
    }
    else if (currentBank == 4) // Aux 1-8
    {
        for (int i = 0; i < 8; ++i)
        {
            auto strip = std::make_unique<ChannelStripComponent> (
                i, engine, project, ChannelStripComponent::StripType::Aux);
            addAndMakeVisible (strip.get());
            channelStrips.push_back (std::move (strip));
        }
    }
    else if (currentBank == 5) // FX 1L-4R (FX Returns!)
    {
        for (int i = 0; i < 8; ++i)
        {
            auto strip = std::make_unique<ChannelStripComponent> (
                i, engine, project, ChannelStripComponent::StripType::FxReturn);
            addAndMakeVisible (strip.get());
            channelStrips.push_back (std::move (strip));
        }
    }
    else if (currentBank == 6) // Bus 1-8
    {
        for (int i = 0; i < 8; ++i)
        {
            auto strip = std::make_unique<ChannelStripComponent> (
                i, engine, project, ChannelStripComponent::StripType::Bus);
            addAndMakeVisible (strip.get());
            channelStrips.push_back (std::move (strip));
        }
    }
    else if (currentBank == 7) // Bus 9-16 (Where Bus 13..16 are FX 1..4 Sends!)
    {
        for (int i = 0; i < 8; ++i)
        {
            auto strip = std::make_unique<ChannelStripComponent> (
                8 + i, engine, project, ChannelStripComponent::StripType::Bus);
            addAndMakeVisible (strip.get());
            channelStrips.push_back (std::move (strip));
        }
    }
    else if (currentBank == 8) // Mtx-Main (Mtx 1-6, Main C, Main LR)
    {
        for (int i = 0; i < 6; ++i)
        {
            auto strip = std::make_unique<ChannelStripComponent> (
                i, engine, project, ChannelStripComponent::StripType::Matrix);
            addAndMakeVisible (strip.get());
            channelStrips.push_back (std::move (strip));
        }
        auto stripC = std::make_unique<ChannelStripComponent> (
            0, engine, project, ChannelStripComponent::StripType::MainC);
        addAndMakeVisible (stripC.get());
        channelStrips.push_back (std::move (stripC));

        auto stripM = std::make_unique<ChannelStripComponent> (
            0, engine, project, ChannelStripComponent::StripType::Master);
        addAndMakeVisible (stripM.get());
        channelStrips.push_back (std::move (stripM));
    }
    else // DCA 1-8
    {
        for (int i = 0; i < 8; ++i)
        {
            auto strip = std::make_unique<ChannelStripComponent> (
                i, engine, project, ChannelStripComponent::StripType::Bus);
            addAndMakeVisible (strip.get());
            channelStrips.push_back (std::move (strip));
        }
    }

    resized();
}

void HomeScreen::setSelectedChannel (int ch)
{
    selectedChannel = ch;
    for (auto& s : channelStrips)
    {
        if (s->getStripType() == ChannelStripComponent::StripType::InputChannel)
            s->setSelected (s->getChannelIndex() == selectedChannel);
    }
}

void HomeScreen::buttonClicked (juce::Button* b)
{
    if (b == &sendsOnFadersButton)
    {
        sendsOnFaderActive = sendsOnFadersButton.getToggleState();
        rebuildStrips();
    }
}

void HomeScreen::comboBoxChanged (juce::ComboBox* cb)
{
    if (cb == &sofTargetCombo)
    {
        sofTarget = cb->getSelectedId() - 1;
        if (sendsOnFaderActive)
            rebuildStrips();
    }
}

void HomeScreen::paint (juce::Graphics& g)
{
    g.fillAll (ConsoleLookAndFeel::bgDark);

    // Left control panel background
    auto leftArea = getLocalBounds().removeFromLeft (116).reduced (4, 4);
    leftArea.removeFromTop (48); // below bank selector
    g.setColour (ConsoleLookAndFeel::bgPanel);
    g.fillRoundedRectangle (leftArea.toFloat(), 4.0f);
    g.setColour (ConsoleLookAndFeel::borderSubtle);
    g.drawRoundedRectangle (leftArea.toFloat(), 4.0f, 1.0f);
}

void HomeScreen::resized()
{
    auto area = getLocalBounds().reduced (4);

    // 1. Top Bank Selector Bar (Full width across mixer)
    bankSelectorBar.setBounds (area.removeFromTop (48));
    area.removeFromTop (4);

    // 2. Left Control Panel (~116px wide)
    auto leftArea = area.removeFromLeft (116).reduced (4, 6);

    // Sends On Faders button & target dropdown
    sendsOnFadersButton.setBounds (leftArea.removeFromTop (32));
    leftArea.removeFromTop (4);
    sofTargetCombo.setBounds (leftArea.removeFromTop (24));
    leftArea.removeFromTop (12);

    // Auto Mixing (X, Y)
    autoMixLabel.setBounds (leftArea.removeFromTop (16));
    auto mixBtnArea = leftArea.removeFromTop (26);
    autoMixXButton.setBounds (mixBtnArea.removeFromLeft (mixBtnArea.getWidth() / 2).reduced (2, 0));
    autoMixYButton.setBounds (mixBtnArea.reduced (2, 0));
    leftArea.removeFromTop (12);

    // Mute Groups 1..6
    muteGroupsLabel.setBounds (leftArea.removeFromTop (16));
    for (int row = 0; row < 3; ++row)
    {
        auto mgArea = leftArea.removeFromTop (24);
        muteGroupButtons[row * 2].setBounds (mgArea.removeFromLeft (mgArea.getWidth() / 2).reduced (2, 1));
        muteGroupButtons[row * 2 + 1].setBounds (mgArea.reduced (2, 1));
        leftArea.removeFromTop (2);
    }
    leftArea.removeFromTop (12);

    // Show Solos & Lock Mutes
    showSolosButton.setBounds (leftArea.removeFromTop (24));
    leftArea.removeFromTop (6);
    lockMutesButton.setBounds (leftArea.removeFromTop (24));

    area.removeFromLeft (6);

    // 3. Master strip (Anchored on right when not in Mtx-Main)
    const bool showMasterOnRight = (currentBank != 8);
    masterStrip->setVisible (showMasterOnRight);
    if (showMasterOnRight)
    {
        const int masterW = 86;
        masterStrip->setBounds (area.removeFromRight (masterW).reduced (2, 0));
        area.removeFromRight (4);
    }

    // 4. Distribute remaining mixer width equally among the 8 strips
    const int numStrips = (int) channelStrips.size();
    if (numStrips > 0)
    {
        const int stripW = area.getWidth() / numStrips;
        for (int i = 0; i < numStrips; ++i)
        {
            channelStrips[(size_t) i]->setBounds (area.removeFromLeft (stripW).reduced (2, 0));
        }
    }
}

bool HomeScreen::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& path : files)
    {
        const auto ext = juce::File (path).getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".aif" || ext == ".aiff" || ext == ".flac" || ext == ".mp3" || ext == ".ogg")
            return true;
    }
    return false;
}

void HomeScreen::filesDropped (const juce::StringArray& files, int /*x*/, int /*y*/)
{
    juce::Array<juce::File> audioFiles;
    for (const auto& path : files)
    {
        const juce::File f (path);
        const auto ext = f.getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".aif" || ext == ".aiff" || ext == ".flac" || ext == ".mp3" || ext == ".ogg")
            audioFiles.add (f);
    }

    if (audioFiles.isEmpty()) return;

    audioFiles.sort();
    const double targetSr = engine.getSampleRate();

    int startSlot = 0;
    for (int s = 0; s < kMaxTracks; ++s)
    {
        if (engine.getTrackSlot (s) == nullptr)
        {
            startSlot = s;
            break;
        }
    }

    for (int i = 0; i < audioFiles.size() && (startSlot + i) < kMaxTracks; ++i)
    {
        const int slot = startSlot + i;
        const auto& file = audioFiles[i];

        auto track = AudioFileTrack::open (file, targetSr, engine.getFormatManager());
        if (track != nullptr)
        {
            engine.setTrackSlot (slot, track);
            project.setTrackFile (slot, file);

            auto& cp = engine.getMixerParams().channels[(size_t) slot];
            if (cp.source.getInt() < 0)
                cp.source.set ((float) slot);

            auto meta = project.getChannelMetadata (slot);
            if (meta.name.startsWith ("CH ") || meta.name.isEmpty())
                project.setChannelName (slot, file.getFileNameWithoutExtension().substring (0, 8));
        }
    }

    if (project.onTracksChanged)
        project.onTracksChanged();
    else
        rebuildStrips();
}

} // namespace mps
