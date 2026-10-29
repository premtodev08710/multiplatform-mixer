#include "HomeScreen.h"
#include "../Core/AppEdition.h"

namespace mps
{

HomeScreen::HomeScreen (AudioEngine& eng, ProjectModel& proj)
    : engine (eng), project (proj)
{
    // 1. Top Bank Selector Bar (with miniature 8-column LED meters)
    bankSelectorBar.setMeters (&engine.getMixerMeters());
    bankSelectorBar.onBankSelected = [this] (int bank) {
        if (! AppEdition::isPro() && bank >= 1 && bank <= 3)
        {
            juce::AlertWindow::showMessageBoxAsync (
                juce::AlertWindow::InfoIcon,
                "PRO FEATURE: 32 Channels",
                juce::CharPointer_UTF8 (
                    "\xe0\xb9\x80\xe0\xb8\xa7\xe0\xb8\xad\xe0\xb8\xa3\xe0\xb9\x8c\xe0\xb8\x8a\xe0\xb8\xb1\xe0\xb8\x99\xe0\xb8\x9f\xe0\xb8\xa3\xe0\xb8\xb5\xe0\xb8\x88\xe0\xb8\xb3\xe0\xb8\x81\xe0\xb8\xb1\xe0\xb8\x94\xe0\xb9\x84\xe0\xb8\xa7\xe0\xb9\x89\xe0\xb8\x97\xe0\xb8\xb5\xe0\xb9\x88 8 \xe0\xb9\x81\xe0\xb8\x97\xe0\xb8\xa3\xe0\xb9\x87\xe0\xb8\x81 (CH 01-08)\n"
                    "\xe0\xb8\xad\xe0\xb8\xb1\xe0\xb8\x9b\xe0\xb9\x80\xe0\xb8\x81\xe0\xb8\xa3\xe0\xb8\x94\xe0\xb9\x80\xe0\xb8\x9b\xe0\xb9\x87\xe0\xb8\x99\xe0\xb9\x80\xe0\xb8\xa7\xe0\xb8\xad\xe0\xb8\xa3\xe0\xb9\x8c\xe0\xb8\x8a\xe0\xb8\xb1\xe0\xb8\x99 Pro \xe0\xb9\x80\xe0\xb8\x9e\xe0\xb8\xb7\xe0\xb9\x88\xe0\xb8\xad\xe0\xb9\x83\xe0\xb8\x8a\xe0\xb9\x89\xe0\xb8\x87\xe0\xb8\xb2\xe0\xb8\x99\xe0\xb9\x80\xe0\xb8\x95\xe0\xb9\x87\xe0\xb8\xa1 32 \xe0\xb9\x81\xe0\xb8\x97\xe0\xb8\xa3\xe0\xb9\x87\xe0\xb8\x81!"
                )
            );
            bankSelectorBar.setSelectedBank (0, false);
            currentBank = 0;
            rebuildStrips();
            return;
        }

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

    for (int b = 1; b <= 12; ++b)
        sofTargetCombo.addItem ("BUS " + juce::String (b) + " (MON " + juce::String (b) + ")", b);
    sofTargetCombo.addItem ("BUS 13 (FX 1: REVERB)", 13);
    sofTargetCombo.addItem ("BUS 14 (FX 2: ROOM)", 14);
    sofTargetCombo.addItem ("BUS 15 (FX 3: DELAY)", 15);
    sofTargetCombo.addItem ("BUS 16 (FX 4: CHORUS)", 16);
    sofTargetCombo.setSelectedId (13, juce::dontSendNotification);
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
}

HomeScreen::~HomeScreen()
{
    stopTimer();
}

void HomeScreen::visibilityChanged()
{
    if (isShowing())
        startTimerHz (20);
    else
        stopTimer();
}

void HomeScreen::timerCallback()
{
    autoMixXButton.setToggleState(engine.getMixerParams().automixEnabled[0].getBool(), juce::dontSendNotification);
    autoMixYButton.setToggleState(engine.getMixerParams().automixEnabled[1].getBool(), juce::dontSendNotification);
    if (! isShowing())
    {
        stopTimer();
        return;
    }
    bankSelectorBar.refreshMeters();
}

void HomeScreen::refreshStrips()
{
    if (channelStrips.empty() || builtBank != currentBank)
    {
        rebuildStrips();
        return;
    }

    for (auto& s : channelStrips)
    {
        if (s->getStripType() == ChannelStripComponent::StripType::InputChannel)
        {
            s->setSelected (s->getChannelIndex() == selectedChannel);
            s->setSendsOnFader (sendsOnFaderActive ? sofTarget : -1);
        }
        s->updateFromModel();
    }
    if (masterStrip != nullptr)
        masterStrip->updateFromModel();
}

void HomeScreen::rebuildStrips()
{
    if (! channelStrips.empty() && builtBank == currentBank)
    {
        refreshStrips();
        return;
    }

    builtBank = currentBank;
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
            strip->onOpenDetail = [this] (int selIdx) {
                setSelectedChannel (selIdx);
                if (onChannelSelected) onChannelSelected (selIdx);
                if (onOpenDetailStrip) onOpenDetailStrip (ChannelStripComponent::StripType::InputChannel, selIdx);
                else if (onOpenDetail) onOpenDetail (selIdx);
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
            strip->onOpenDetail = [this, i] (int) {
                if (onOpenDetailStrip) onOpenDetailStrip (ChannelStripComponent::StripType::Aux, i);
            };
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
            strip->onOpenDetail = [this, i] (int) {
                if (onOpenDetailStrip) onOpenDetailStrip (ChannelStripComponent::StripType::FxReturn, i);
            };
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
            strip->onOpenDetail = [this, i] (int) {
                if (onOpenDetailStrip) onOpenDetailStrip (ChannelStripComponent::StripType::Bus, i);
            };
            addAndMakeVisible (strip.get());
            channelStrips.push_back (std::move (strip));
        }
    }
    else if (currentBank == 7) // Bus 9-16
    {
        for (int i = 0; i < 8; ++i)
        {
            const int busIdx = 8 + i;
            auto strip = std::make_unique<ChannelStripComponent> (
                busIdx, engine, project, ChannelStripComponent::StripType::Bus);
            strip->onOpenDetail = [this, busIdx] (int) {
                if (onOpenDetailStrip) onOpenDetailStrip (ChannelStripComponent::StripType::Bus, busIdx);
            };
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
            strip->onOpenDetail = [this, i] (int) {
                if (onOpenDetailStrip) onOpenDetailStrip (ChannelStripComponent::StripType::Matrix, i);
            };
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
                i, engine, project, ChannelStripComponent::StripType::Dca);
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
    if (b == &autoMixXButton) engine.getMixerParams().automixEnabled[0].setBool(autoMixXButton.getToggleState());
    if (b == &autoMixYButton) engine.getMixerParams().automixEnabled[1].setBool(autoMixYButton.getToggleState());
    if (b == &sendsOnFadersButton)
    {
        sendsOnFaderActive = sendsOnFadersButton.getToggleState();
        refreshStrips();
    }
    for (int i = 0; i < 6; ++i)
    {
        if (b == &muteGroupButtons[i])
        {
            engine.getMixerParams().muteGroupActive[(size_t) i].store (
                muteGroupButtons[i].getToggleState(), std::memory_order_relaxed);
            for (auto& s : channelStrips)
                s->updateFromModel();
            break;
        }
    }
}

void HomeScreen::comboBoxChanged (juce::ComboBox* cb)
{
    if (cb == &sofTargetCombo)
    {
        sofTarget = cb->getSelectedId() - 1;
        if (sendsOnFaderActive)
            refreshStrips();
    }
}

void HomeScreen::paint (juce::Graphics& g)
{
    g.fillAll (ConsoleLookAndFeel::bgDark);

    // Left control panel background
    auto leftArea = getLocalBounds().removeFromLeft (116).reduced (4, 4);
    leftArea.removeFromTop (40); // below bank selector
    g.setColour (ConsoleLookAndFeel::bgPanel);
    g.fillRoundedRectangle (leftArea.toFloat(), 4.0f);
    g.setColour (ConsoleLookAndFeel::borderSubtle);
    g.drawRoundedRectangle (leftArea.toFloat(), 4.0f, 1.0f);
}

void HomeScreen::resized()
{
    auto area = getLocalBounds().reduced (4);

    // 1. Top Bank Selector Bar (Full width across mixer)
    bankSelectorBar.setBounds (area.removeFromTop (40));
    area.removeFromTop (2);

    // Keep the mixer strips usable on compact phones by reclaiming width from
    // the utility and master columns when the available area is narrow.
    const bool compactWidth = getWidth() < 1000;
    const int sidePanelW = compactWidth ? 92 : 116;
    const int masterW = compactWidth ? 68 : 86;
    auto leftArea = area.removeFromLeft (sidePanelW).reduced (4, 6);

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

static bool isSupportedAudioFileHome (const juce::File& f)
{
    const auto ext = f.getFileExtension().toLowerCase();
    return ext == ".wav" || ext == ".aif" || ext == ".aiff" || ext == ".flac"
        || ext == ".mp3" || ext == ".ogg" || ext == ".m4a" || ext == ".aac";
}

static void collectAudioFilesRecursivelyHome (const juce::File& f, juce::Array<juce::File>& result)
{
    if (f.isDirectory())
    {
        auto children = f.findChildFiles (juce::File::findFiles, true, "*");
        for (const auto& child : children)
        {
            if (isSupportedAudioFileHome (child))
                result.add (child);
        }
    }
    else if (isSupportedAudioFileHome (f))
    {
        result.add (f);
    }
}

bool HomeScreen::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& path : files)
    {
        const juce::File f (path);
        if (f.isDirectory() || isSupportedAudioFileHome (f))
            return true;
    }
    return false;
}

void HomeScreen::filesDropped (const juce::StringArray& files, int /*x*/, int /*y*/)
{
    juce::Array<juce::File> audioFiles;
    for (const auto& path : files)
        collectAudioFilesRecursivelyHome (juce::File (path), audioFiles);

    if (audioFiles.isEmpty()) return;

    audioFiles.sort();

    // Decode & pre-buffer audio files asynchronously on a background thread so the
    // UI thread and audio playback never stall or click, then connect in one atomic batch.
    juce::Component::SafePointer<HomeScreen> safeThis (this);
    juce::Thread::launch ([safeThis, audioFiles] {
        if (safeThis == nullptr) return;

        auto& eng = safeThis->engine;
        const double targetSr = (eng.getSampleRate() > 1000.0) ? eng.getSampleRate() : AudioEngine::kDefaultSampleRate;

        int startSlot = -1;
        const int maxTracksAllowed = AppEdition::getMaxTracks();
        for (int s = 0; s < maxTracksAllowed; ++s)
        {
            if (eng.getTrackSlot (s) == nullptr)
            {
                startSlot = s;
                break;
            }
        }
        if (startSlot < 0)
            startSlot = 0;

        struct LoadedTrack
        {
            int slot;
            juce::File file;
            std::shared_ptr<AudioFileTrack> track;
        };
        std::vector<LoadedTrack> readyTracks;

        for (int i = 0; i < audioFiles.size() && (startSlot + i) < maxTracksAllowed; ++i)
        {
            const int slot = startSlot + i;
            const auto& file = audioFiles[i];

            auto track = AudioFileTrack::open (file, targetSr, eng.getFormatManager());
            if (track != nullptr)
                readyTracks.push_back ({ slot, file, std::move (track) });
        }

        juce::MessageManager::callAsync ([safeThis, readyTracks = std::move (readyTracks)] {
            if (safeThis == nullptr || readyTracks.empty()) return;

            std::vector<std::pair<int, std::shared_ptr<TrackReader>>> batchSlots;
            batchSlots.reserve (readyTracks.size());

            for (const auto& item : readyTracks)
            {
                batchSlots.emplace_back (item.slot, item.track);
                safeThis->project.setTrackFile (item.slot, item.file);

                auto& cp = safeThis->engine.getMixerParams().channels[(size_t) item.slot];
                if (cp.source.getInt() < 0)
                    cp.source.set ((float) item.slot);

                auto meta = safeThis->project.getChannelMetadata (item.slot);
                if (meta.name.startsWith ("CH ") || meta.name.isEmpty())
                    safeThis->project.setChannelName (item.slot, item.file.getFileNameWithoutExtension().substring (0, 8));
            }

            safeThis->engine.setTrackSlots (batchSlots);
            safeThis->project.setModified (true);
            safeThis->project.autoSaveSession();

            if (safeThis->project.onTracksChanged)
                safeThis->project.onTracksChanged();
            else
                safeThis->refreshStrips();
        });
    });
}

} // namespace mps
