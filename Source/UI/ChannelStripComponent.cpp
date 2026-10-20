#include "ChannelStripComponent.h"

namespace mps
{

ChannelStripComponent::ChannelStripComponent (int ch, AudioEngine& eng, ProjectModel& proj, StripType type)
    : channelIndex (ch), engine (eng), project (proj), stripType (type),
      meter (type == StripType::Master ? LedMeter::Mode::Stereo : LedMeter::Mode::Mono)
{
    // Scribble Strip Labels
    scribbleName.setJustificationType (juce::Justification::centred);
    scribbleName.setFont (juce::FontOptions (12.0f, juce::Font::bold));

    const bool canOpenDetail = (stripType == StripType::InputChannel
                             || stripType == StripType::Aux
                             || stripType == StripType::FxReturn
                             || stripType == StripType::Bus
                             || stripType == StripType::Matrix);

    if (canOpenDetail)
    {
        scribbleName.setTooltip ("Click to open Detail Screen");
        scribbleName.setMouseCursor (juce::MouseCursor::PointingHandCursor);
        scribbleName.addMouseListener (this, false);
    }
    addAndMakeVisible (scribbleName);

    scribbleSource.setJustificationType (juce::Justification::centred);
    scribbleSource.setFont (juce::FontOptions (10.0f));
    scribbleSource.setColour (juce::Label::textColourId, juce::Colour (0xffb0bec5));
    if (canOpenDetail)
    {
        scribbleSource.setTooltip ("Click to open Detail Screen");
        scribbleSource.setMouseCursor (juce::MouseCursor::PointingHandCursor);
        scribbleSource.addMouseListener (this, false);
    }
    addAndMakeVisible (scribbleSource);

    // Gain Display Label (Digital Preamp Trim readout / quick edit)
    if (stripType == StripType::InputChannel)
    {
        gainLabel.setJustificationType (juce::Justification::centred);
        gainLabel.setFont (juce::FontOptions (9.5f, juce::Font::bold));
        gainLabel.setColour (juce::Label::backgroundColourId, juce::Colour (0xff14171d));
        gainLabel.setColour (juce::Label::textColourId, juce::Colour (0xffffb74d)); // Amber console readout
        gainLabel.setColour (juce::Label::outlineColourId, juce::Colour (0xff323a48));
        gainLabel.setTooltip ("Digital Preamp Trim (-12 dB to +60 dB). Click to edit.");
        gainLabel.setEditable (true, false, false);
        gainLabel.onTextChange = [this] {
            if (ignoreCallbacks || stripType != StripType::InputChannel) return;
            const float db = juce::jlimit (-12.0f, 60.0f, gainLabel.getText().getFloatValue());
            auto& cp = engine.getMixerParams().channels[(size_t) channelIndex];
            cp.trimDb.set (db);
            gainLabel.setText ((db >= 0.0f ? "+" : "") + juce::String (db, 1) + " dB", juce::dontSendNotification);
        };
        addAndMakeVisible (gainLabel);
    }

    const bool hasPanAndSolo = (stripType != StripType::Master && stripType != StripType::MainC);
    if (hasPanAndSolo)
    {
        panSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        panSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        panSlider.setRange (-1.0, 1.0, 0.01);
        panSlider.setValue (0.0);
        panSlider.addListener (this);
        addAndMakeVisible (panSlider);

        soloButton.setName ("solo");
        soloButton.setClickingTogglesState (true);
        soloButton.addListener (this);
        addAndMakeVisible (soloButton);
    }

    // Mute Button
    muteButton.setName ("mute");
    muteButton.setClickingTogglesState (true);
    muteButton.addListener (this);
    addAndMakeVisible (muteButton);

    // Fader
    faderSlider.setSliderStyle (juce::Slider::LinearVertical);
    faderSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    faderSlider.setRange (0.0, 1.0, 0.001);
    faderSlider.setValue (faderDbToPos (0.0f));
    faderSlider.addListener (this);
    addAndMakeVisible (faderSlider);

    // Fader Value Label (dB)
    faderValueLabel.setJustificationType (juce::Justification::centred);
    faderValueLabel.setFont (juce::FontOptions (11.0f));
    faderValueLabel.setColour (juce::Label::backgroundColourId, juce::Colour (0xff14171c));
    faderValueLabel.setColour (juce::Label::textColourId, ConsoleLookAndFeel::textPrimary);
    faderValueLabel.setEditable (true, false, false);
    faderValueLabel.onTextChange = [this] {
        const float db = faderValueLabel.getText().getFloatValue();
        faderSlider.setValue (faderDbToPos (db));
    };
    addAndMakeVisible (faderValueLabel);

    // Meters setup
    if (stripType == StripType::InputChannel)
    {
        auto& cm = engine.getMixerMeters().channels[(size_t) channelIndex];
        meter.setMeters (&cm.preL, &cm.preR, &cm.clip);
    }
    else if (stripType == StripType::FxReturn)
    {
        auto& retM = engine.getMixerMeters().fxReturns[(size_t) channelIndex];
        meter.setMeters (&retM, nullptr, nullptr);
    }
    else if (stripType == StripType::Aux)
    {
        auto& auxM = engine.getMixerMeters().auxes[(size_t) channelIndex];
        meter.setMeters (&auxM, nullptr, nullptr);
    }
    else if (stripType == StripType::Bus)
    {
        auto& busM = engine.getMixerMeters().buses[(size_t) channelIndex];
        meter.setMeters (&busM, nullptr, nullptr);
    }
    else if (stripType == StripType::Matrix)
    {
        auto& mtxM = engine.getMixerMeters().matrices[(size_t) channelIndex];
        meter.setMeters (&mtxM, nullptr, nullptr);
    }
    else if (stripType == StripType::Dca)
    {
        auto& dcaM = engine.getMixerMeters().dcas[(size_t) channelIndex];
        meter.setMeters (&dcaM, nullptr, nullptr);
    }
    else if (stripType == StripType::MainC)
    {
        meter.setMeters (&engine.getMixerMeters().mainC, nullptr, nullptr);
    }
    else // Master
    {
        auto& mm = engine.getMixerMeters().main;
        meter.setMeters (&mm.l, &mm.r, &mm.clip);
    }
    addAndMakeVisible (meter);

    updateFromModel();
    startTimerHz (10); // Polling sync for model updates if changed externally
}

ChannelStripComponent::~ChannelStripComponent()
{
    stopTimer();
}

void ChannelStripComponent::setSelected (bool s)
{
    if (selected != s)
    {
        selected = s;
        repaint();
    }
}

void ChannelStripComponent::setSendsOnFader (int targetFx)
{
    if (sendsOnFaderTarget != targetFx)
    {
        sendsOnFaderTarget = targetFx;
        updateFromModel();
        repaint();
    }
}

juce::MouseCursor ChannelStripComponent::getMouseCursor()
{
    const bool canOpenDetail = (stripType == StripType::InputChannel
                             || stripType == StripType::Aux
                             || stripType == StripType::FxReturn
                             || stripType == StripType::Bus
                             || stripType == StripType::Matrix);

    if (canOpenDetail)
    {
        const auto mousePos = getMouseXYRelative();
        if (mousePos.y <= 60)
            return juce::MouseCursor::PointingHandCursor;
    }
    return juce::MouseCursor::NormalCursor;
}

void ChannelStripComponent::mouseDown (const juce::MouseEvent& e)
{
    const bool canOpenDetail = (stripType == StripType::InputChannel
                             || stripType == StripType::Aux
                             || stripType == StripType::FxReturn
                             || stripType == StripType::Bus
                             || stripType == StripType::Matrix);

    if (canOpenDetail)
    {
        if (onSelected)
            onSelected (channelIndex);

        const auto localPos = e.getEventRelativeTo (this).getPosition();
        const bool onScribble = (e.eventComponent == &scribbleName
                              || e.eventComponent == &scribbleSource
                              || localPos.y <= 60);

        if (onScribble && onOpenDetail)
        {
            onOpenDetail (channelIndex);
        }
    }
}

void ChannelStripComponent::mouseDoubleClick (const juce::MouseEvent&)
{
    const bool canOpenDetail = (stripType == StripType::InputChannel
                             || stripType == StripType::Aux
                             || stripType == StripType::FxReturn
                             || stripType == StripType::Bus
                             || stripType == StripType::Matrix);

    if (canOpenDetail && onOpenDetail)
    {
        onOpenDetail (channelIndex);
    }
}

void ChannelStripComponent::updateFromModel()
{
    ignoreCallbacks = true;

    if (stripType == StripType::InputChannel)
    {
        auto& cp = engine.getMixerParams().channels[(size_t) channelIndex];
        auto& meta = project.getChannelMetadata (channelIndex);

        scribbleName.setText (meta.name, juce::dontSendNotification);
        const int src = cp.source.getInt();
        scribbleSource.setText (src >= 0 ? "IN " + juce::String (src + 1) : "OFF", juce::dontSendNotification);

        const float gDb = cp.trimDb.get();
        gainLabel.setText ((gDb >= 0.0f ? "+" : "") + juce::String (gDb, 1) + " dB", juce::dontSendNotification);

        panSlider.setValue (cp.pan.get(), juce::dontSendNotification);
        soloButton.setToggleState (cp.solo.getBool(), juce::dontSendNotification);

        bool isEffectiveMuted = cp.mute.getBool();
        const int mgMask = cp.muteGroups.getInt();
        if (mgMask > 0)
        {
            for (int mg = 0; mg < 6; ++mg)
                if ((mgMask & (1 << mg)) && engine.getMixerParams().muteGroupActive[(size_t) mg].load (std::memory_order_relaxed))
                {
                    isEffectiveMuted = true;
                    break;
                }
        }
        const int dcaMask = cp.dcaGroups.getInt();
        if (dcaMask > 0)
        {
            for (int d = 0; d < 8; ++d)
                if ((dcaMask & (1 << d)) && engine.getMixerParams().dcas[(size_t) d].mute.getBool())
                {
                    isEffectiveMuted = true;
                    break;
                }
        }
        muteButton.setToggleState (isEffectiveMuted, juce::dontSendNotification);

        if (sendsOnFaderTarget >= 0 && sendsOnFaderTarget < 16)
        {
            scribbleSource.setText ("SEND B" + juce::String (sendsOnFaderTarget + 1), juce::dontSendNotification);
            const float sDb = cp.sends[(size_t) sendsOnFaderTarget].get();
            faderSlider.setValue (faderDbToPos (sDb), juce::dontSendNotification);
            faderValueLabel.setText (sDb <= kFaderMinDb ? "-inf" : juce::String (sDb, 1) + " dB", juce::dontSendNotification);
        }
        else
        {
            if (engine.getMixerParams().isChannelLinked (channelIndex))
                scribbleSource.setText ("LINK " + juce::String (channelIndex % 2 == 0 ? "L" : "R"), juce::dontSendNotification);
            const float db = cp.faderDb.get();
            faderSlider.setValue (faderDbToPos (db), juce::dontSendNotification);
            faderValueLabel.setText (db <= kFaderMinDb ? "-inf" : juce::String (db, 1) + " dB", juce::dontSendNotification);
        }
    }
    else if (stripType == StripType::FxReturn)
    {
        auto& rp = engine.getMixerParams().fxReturns[(size_t) channelIndex];
        const int fxSlot = channelIndex / 2; // 0..3
        const char* slotSuffix = (channelIndex % 2 == 0) ? "L" : "R";
        scribbleName.setText ("FX" + juce::String (fxSlot + 1) + slotSuffix, juce::dontSendNotification);
        scribbleSource.setText ("FX RACK", juce::dontSendNotification);

        panSlider.setValue (rp.pan.get(), juce::dontSendNotification);
        soloButton.setToggleState (rp.solo.getBool(), juce::dontSendNotification);
        muteButton.setToggleState (rp.mute.getBool(), juce::dontSendNotification);

        const float db = rp.faderDb.get();
        faderSlider.setValue (faderDbToPos (db), juce::dontSendNotification);
        faderValueLabel.setText (db <= kFaderMinDb ? "-inf" : juce::String (db, 1) + " dB", juce::dontSendNotification);
    }
    else if (stripType == StripType::Aux)
    {
        auto& ap = engine.getMixerParams().auxes[(size_t) channelIndex];
        scribbleName.setText ("Aux" + juce::String (channelIndex + 1), juce::dontSendNotification);
        scribbleSource.setText ("AUX IN", juce::dontSendNotification);

        panSlider.setValue (ap.pan.get(), juce::dontSendNotification);
        soloButton.setToggleState (false, juce::dontSendNotification);
        muteButton.setToggleState (ap.mute.getBool(), juce::dontSendNotification);

        const float db = ap.faderDb.get();
        faderSlider.setValue (faderDbToPos (db), juce::dontSendNotification);
        faderValueLabel.setText (db <= kFaderMinDb ? "-inf" : juce::String (db, 1) + " dB", juce::dontSendNotification);
    }
    else if (stripType == StripType::Bus)
    {
        auto& bp = engine.getMixerParams().buses[(size_t) channelIndex];
        if (channelIndex >= 12 && channelIndex < 16)
        {
            const int fxIdx = channelIndex - 12 + 1;
            scribbleName.setText ("FX " + juce::String (fxIdx) + " Send", juce::dontSendNotification);
            scribbleSource.setText ("BUS " + juce::String (channelIndex + 1), juce::dontSendNotification);
        }
        else
        {
            scribbleName.setText ("Bus" + juce::String (channelIndex + 1), juce::dontSendNotification);
            scribbleSource.setText ("BUS OUT", juce::dontSendNotification);
        }

        panSlider.setValue (bp.pan.get(), juce::dontSendNotification);
        soloButton.setToggleState (false, juce::dontSendNotification);
        muteButton.setToggleState (bp.mute.getBool(), juce::dontSendNotification);

        const float db = bp.faderDb.get();
        faderSlider.setValue (faderDbToPos (db), juce::dontSendNotification);
        faderValueLabel.setText (db <= kFaderMinDb ? "-inf" : juce::String (db, 1) + " dB", juce::dontSendNotification);
    }
    else if (stripType == StripType::Matrix)
    {
        auto& mp = engine.getMixerParams().matrices[(size_t) channelIndex];
        scribbleName.setText ("Mtx" + juce::String (channelIndex + 1), juce::dontSendNotification);
        scribbleSource.setText ("MATRIX", juce::dontSendNotification);

        panSlider.setValue (mp.pan.get(), juce::dontSendNotification);
        soloButton.setToggleState (false, juce::dontSendNotification);
        muteButton.setToggleState (mp.mute.getBool(), juce::dontSendNotification);

        const float db = mp.faderDb.get();
        faderSlider.setValue (faderDbToPos (db), juce::dontSendNotification);
        faderValueLabel.setText (db <= kFaderMinDb ? "-inf" : juce::String (db, 1) + " dB", juce::dontSendNotification);
    }
    else if (stripType == StripType::Dca)
    {
        auto& dp = engine.getMixerParams().dcas[(size_t) channelIndex];
        scribbleName.setText ("DCA " + juce::String (channelIndex + 1), juce::dontSendNotification);
        scribbleSource.setText ("DCA GRP", juce::dontSendNotification);

        soloButton.setToggleState (false, juce::dontSendNotification);
        muteButton.setToggleState (dp.mute.getBool(), juce::dontSendNotification);

        const float db = dp.faderDb.get();
        faderSlider.setValue (faderDbToPos (db), juce::dontSendNotification);
        faderValueLabel.setText (db <= kFaderMinDb ? "-inf" : juce::String (db, 1) + " dB", juce::dontSendNotification);
    }
    else if (stripType == StripType::MainC)
    {
        scribbleName.setText ("MainC", juce::dontSendNotification);
        scribbleSource.setText ("CENTER", juce::dontSendNotification);
        muteButton.setToggleState (false, juce::dontSendNotification);

        faderSlider.setValue (faderDbToPos (0.0f), juce::dontSendNotification);
        faderValueLabel.setText ("0.0 dB", juce::dontSendNotification);
    }
    else // Master
    {
        auto& mp = engine.getMixerParams().main;
        scribbleName.setText ("MainLR", juce::dontSendNotification);
        scribbleSource.setText ("STEREO", juce::dontSendNotification);
        muteButton.setToggleState (mp.mute.getBool(), juce::dontSendNotification);

        const float db = mp.faderDb.get();
        faderSlider.setValue (faderDbToPos (db), juce::dontSendNotification);
        faderValueLabel.setText (db <= kFaderMinDb ? "-inf" : juce::String (db, 1) + " dB", juce::dontSendNotification);
    }

    ignoreCallbacks = false;
    repaint();
}

void ChannelStripComponent::timerCallback()
{
    if (! isShowing())
        return;

    // Keep UI synchronized if scene recalled or modified
    if (stripType == StripType::InputChannel)
    {
        auto& cp = engine.getMixerParams().channels[(size_t) channelIndex];
        const float targetDb = (sendsOnFaderTarget >= 0 && sendsOnFaderTarget < 16)
            ? cp.sends[(size_t) sendsOnFaderTarget].get()
            : cp.faderDb.get();
        const float modelFaderPos = faderDbToPos (targetDb);
        if (std::abs ((float) faderSlider.getValue() - modelFaderPos) > 0.005f)
            updateFromModel();

        const float gDb = cp.trimDb.get();
        const juce::String gTxt = (gDb >= 0.0f ? "+" : "") + juce::String (gDb, 1) + " dB";
        if (! gainLabel.isBeingEdited() && gainLabel.getText() != gTxt)
            gainLabel.setText (gTxt, juce::dontSendNotification);

        const auto& meta = project.getChannelMetadata (channelIndex);
        if (! scribbleName.isBeingEdited() && scribbleName.getText() != meta.name)
        {
            scribbleName.setText (meta.name, juce::dontSendNotification);
            repaint();
        }
    }
    else if (stripType == StripType::FxReturn)
    {
        auto& rp = engine.getMixerParams().fxReturns[(size_t) channelIndex];
        const float modelFaderPos = faderDbToPos (rp.faderDb.get());
        if (std::abs ((float) faderSlider.getValue() - modelFaderPos) > 0.005f)
            updateFromModel();
    }
    else if (stripType == StripType::Master)
    {
        auto& mp = engine.getMixerParams().main;
        const float modelFaderPos = faderDbToPos (mp.faderDb.get());
        if (std::abs ((float) faderSlider.getValue() - modelFaderPos) > 0.005f)
            updateFromModel();
    }
}

void ChannelStripComponent::sliderValueChanged (juce::Slider* slider)
{
    if (ignoreCallbacks) return;

    if (slider == &panSlider)
    {
        if (stripType == StripType::InputChannel)
            engine.getMixerParams().channels[(size_t) channelIndex].pan.set ((float) panSlider.getValue());
        else if (stripType == StripType::FxReturn)
            engine.getMixerParams().fxReturns[(size_t) channelIndex].pan.set ((float) panSlider.getValue());
        else if (stripType == StripType::Aux)
            engine.getMixerParams().auxes[(size_t) channelIndex].pan.set ((float) panSlider.getValue());
        else if (stripType == StripType::Bus)
            engine.getMixerParams().buses[(size_t) channelIndex].pan.set ((float) panSlider.getValue());
        else if (stripType == StripType::Matrix)
            engine.getMixerParams().matrices[(size_t) channelIndex].pan.set ((float) panSlider.getValue());
    }
    else if (slider == &faderSlider)
    {
        const float db = faderPosToDb ((float) faderSlider.getValue());
        faderValueLabel.setText (db <= kFaderMinDb ? "-inf" : juce::String (db, 1) + " dB", juce::dontSendNotification);

        if (stripType == StripType::InputChannel)
        {
            if (sendsOnFaderTarget >= 0 && sendsOnFaderTarget < 16)
            {
                engine.getMixerParams().channels[(size_t) channelIndex].sends[(size_t) sendsOnFaderTarget].set (db);
                if (engine.getMixerParams().isChannelLinked (channelIndex))
                {
                    const int otherCh = engine.getMixerParams().getLinkedChannel (channelIndex);
                    engine.getMixerParams().channels[(size_t) otherCh].sends[(size_t) sendsOnFaderTarget].set (db);
                }
            }
            else
            {
                engine.getMixerParams().channels[(size_t) channelIndex].faderDb.set (db);
                if (engine.getMixerParams().isChannelLinked (channelIndex))
                {
                    const int otherCh = engine.getMixerParams().getLinkedChannel (channelIndex);
                    engine.getMixerParams().channels[(size_t) otherCh].faderDb.set (db);
                }
            }
        }
        else if (stripType == StripType::FxReturn)
            engine.getMixerParams().fxReturns[(size_t) channelIndex].faderDb.set (db);
        else if (stripType == StripType::Aux)
            engine.getMixerParams().auxes[(size_t) channelIndex].faderDb.set (db);
        else if (stripType == StripType::Bus)
            engine.getMixerParams().buses[(size_t) channelIndex].faderDb.set (db);
        else if (stripType == StripType::Matrix)
            engine.getMixerParams().matrices[(size_t) channelIndex].faderDb.set (db);
        else if (stripType == StripType::Dca)
            engine.getMixerParams().dcas[(size_t) channelIndex].faderDb.set (db);
        else if (stripType == StripType::Master)
            engine.getMixerParams().main.faderDb.set (db);
    }
}

void ChannelStripComponent::buttonClicked (juce::Button* button)
{
    if (ignoreCallbacks) return;

    if (button == &soloButton)
    {
        if (stripType == StripType::InputChannel)
        {
            const bool s = soloButton.getToggleState();
            engine.getMixerParams().channels[(size_t) channelIndex].solo.setBool (s);
            if (engine.getMixerParams().isChannelLinked (channelIndex))
            {
                const int otherCh = engine.getMixerParams().getLinkedChannel (channelIndex);
                engine.getMixerParams().channels[(size_t) otherCh].solo.setBool (s);
            }
        }
        else if (stripType == StripType::FxReturn)
            engine.getMixerParams().fxReturns[(size_t) channelIndex].solo.setBool (soloButton.getToggleState());
    }
    else if (button == &muteButton)
    {
        if (stripType == StripType::InputChannel)
        {
            const bool m = muteButton.getToggleState();
            engine.getMixerParams().channels[(size_t) channelIndex].mute.setBool (m);
            if (engine.getMixerParams().isChannelLinked (channelIndex))
            {
                const int otherCh = engine.getMixerParams().getLinkedChannel (channelIndex);
                engine.getMixerParams().channels[(size_t) otherCh].mute.setBool (m);
            }
        }
        else if (stripType == StripType::FxReturn)
            engine.getMixerParams().fxReturns[(size_t) channelIndex].mute.setBool (muteButton.getToggleState());
        else if (stripType == StripType::Aux)
            engine.getMixerParams().auxes[(size_t) channelIndex].mute.setBool (muteButton.getToggleState());
        else if (stripType == StripType::Bus)
            engine.getMixerParams().buses[(size_t) channelIndex].mute.setBool (muteButton.getToggleState());
        else if (stripType == StripType::Matrix)
            engine.getMixerParams().matrices[(size_t) channelIndex].mute.setBool (muteButton.getToggleState());
        else if (stripType == StripType::Dca)
            engine.getMixerParams().dcas[(size_t) channelIndex].mute.setBool (muteButton.getToggleState());
        else if (stripType == StripType::Master)
            engine.getMixerParams().main.mute.setBool (muteButton.getToggleState());
    }
}

void ChannelStripComponent::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // 1. Strip background
    g.setColour (ConsoleLookAndFeel::bgPanel);
    g.fillRoundedRectangle (bounds, 4.0f);

    // Selected highlight border (Yellow or Cyan glow)
    if (selected)
    {
        g.setColour (ConsoleLookAndFeel::accentYellow);
        g.drawRoundedRectangle (bounds.reduced (1.0f), 4.0f, 2.0f);
    }
    else
    {
        g.setColour (ConsoleLookAndFeel::borderSubtle);
        g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);
    }

    // 2. Scribble Strip Banner Box (colored badge at top)
    const float scribbleH = 38.0f;
    juce::Rectangle<float> scribbleRect (bounds.getX() + 3.0f, bounds.getY() + 3.0f, bounds.getWidth() - 6.0f, scribbleH);

    juce::Colour bannerColour = juce::Colour (0xff37474f);
    if (stripType == StripType::Master)
        bannerColour = juce::Colour (0xffe53935);
    else if (stripType == StripType::FxReturn)
        bannerColour = juce::Colour (0xffd81b60); // Magenta for FX Returns
    else if (stripType == StripType::Aux)
        bannerColour = juce::Colour (0xff43a047); // Green for Aux
    else if (stripType == StripType::Bus)
        bannerColour = juce::Colour (0xff00bcd4); // Cyan for Buses
    else if (stripType == StripType::Matrix)
        bannerColour = juce::Colour (0xff8e24aa); // Purple for Matrix
    else if (stripType == StripType::Dca)
        bannerColour = juce::Colour (0xff0288d1); // Vibrant Blue for DCAs
    else if (stripType == StripType::MainC)
        bannerColour = juce::Colour (0xff78909c);
    else // InputChannel
    {
        if (sendsOnFaderTarget >= 0)
            bannerColour = juce::Colour (0xffffa000); // Amber for Sends On Faders
        else
            bannerColour = project.getChannelMetadata (channelIndex).colour;
    }

    g.setColour (bannerColour.withAlpha (0.25f));
    g.fillRoundedRectangle (scribbleRect, 3.0f);

    g.setColour (bannerColour);
    g.fillRect (scribbleRect.getX(), scribbleRect.getY(), scribbleRect.getWidth(), 3.0f); // Top accent stripe
    g.drawRoundedRectangle (scribbleRect, 3.0f, 1.0f);

    // 3. Fader Scale Markings (+10, 0, -5, -10, -20, -30, -40, -60, -inf)
    const float faderTop = (float) faderSlider.getY() + 10.0f;
    const float faderBottom = (float) faderSlider.getBottom() - 10.0f;
    const float faderRangeH = faderBottom - faderTop;

    const struct { float db; const char* txt; } marks[] = {
        { 10.0f, "+10" }, { 0.0f, "0" }, { -5.0f, "-5" }, { -10.0f, "-10" },
        { -20.0f, "-20" }, { -30.0f, "-30" }, { -40.0f, "-40" }, { -60.0f, "-60" }, { -90.0f, "-inf" }
    };

    g.setFont (juce::FontOptions (8.0f));
    g.setColour (juce::Colour (0xff606874));
    for (const auto& m : marks)
    {
        const float pos = faderDbToPos (m.db);
        const float y = faderBottom - pos * faderRangeH;
        g.drawHorizontalLine ((int) y, bounds.getX() + 6.0f, bounds.getX() + 10.0f);
    }
}

void ChannelStripComponent::resized()
{
    auto area = getLocalBounds().reduced (3);

    // Scribble strip
    auto scribbleArea = area.removeFromTop (38);
    scribbleName.setBounds (scribbleArea.removeFromTop (22));
    scribbleSource.setBounds (scribbleArea);

    area.removeFromTop (2);

    const bool hasPanAndSolo = (stripType != StripType::Master && stripType != StripType::MainC);

    if (stripType == StripType::InputChannel)
    {
        gainLabel.setBounds (area.removeFromTop (14).reduced (2, 0));
        area.removeFromTop (1);
    }

    if (hasPanAndSolo)
    {
        panSlider.setBounds (area.removeFromTop (36).reduced (3, 1));
        area.removeFromTop (2);

        // Solo & Mute buttons
        auto btnArea = area.removeFromTop (22);
        soloButton.setBounds (btnArea.removeFromLeft (btnArea.getWidth() / 2).reduced (2, 0));
        muteButton.setBounds (btnArea.reduced (2, 0));
    }
    else
    {
        // Master Mute
        area.removeFromTop (6);
        muteButton.setBounds (area.removeFromTop (24).reduced (4, 0));
    }

    area.removeFromTop (3);

    // Bottom dB readout
    faderValueLabel.setBounds (area.removeFromBottom (18).reduced (2, 0));
    area.removeFromBottom (2);

    // Fader & Meter columns side by side
    auto faderMeterArea = area;
    const int meterW = (stripType == StripType::Master) ? 18 : 12;
    meter.setBounds (faderMeterArea.removeFromRight (meterW));
    faderMeterArea.removeFromRight (2);
    faderSlider.setBounds (faderMeterArea);
}

} // namespace mps
