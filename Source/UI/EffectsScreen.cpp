// Mix Practice Studio - Effects Rack UI Screen Implementation
#include "EffectsScreen.h"

namespace mps
{

EffectsScreen::EffectsScreen (AudioEngine& eng, ProjectModel& proj)
    : engine (eng), project (proj)
{
    // 1. Group Selector Buttons: FX 1-4 vs FX 5-8
    const char* groupNames[NumGroups] = { "FX 1 - 4 (SEND / RETURN OR INSERT)", "FX 5 - 8 (INSERT ONLY)" };
    for (int i = 0; i < NumGroups; ++i)
    {
        groupButtons[i].setButtonText (groupNames[i]);
        groupButtons[i].setName ("fx_group");
        groupButtons[i].setColour (juce::TextButton::buttonColourId, ConsoleLookAndFeel::bgSection);
        groupButtons[i].setColour (juce::TextButton::textColourOffId, ConsoleLookAndFeel::textMuted);
        groupButtons[i].setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        groupButtons[i].setClickingTogglesState (true);
        groupButtons[i].setRadioGroupId (5001);
        groupButtons[i].addListener (this);
        addAndMakeVisible (groupButtons[i]);
    }
    groupButtons[0].setToggleState (true, juce::dontSendNotification);

    backButton.addListener (this);
    backButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff202630));
    backButton.setColour (juce::TextButton::textColourOffId, ConsoleLookAndFeel::accentCyan);
    addAndMakeVisible (backButton);

    setupSlots();

    // Hardware Simulated Front Panel Faceplate
    addAndMakeVisible (faceplate);
    faceplate.setSlot (selectedSlot);
    faceplate.onTypeChangeRequested = [this] (FxType newType) {
        auto& p = engine.getMixerParams().fxSlots[(size_t) selectedSlot];
        p.setDefaultsForType (newType);
        slotUis[(size_t) selectedSlot].typeCombo.setSelectedId ((int) newType + 1, juce::dontSendNotification);
        faceplate.setSlot (selectedSlot);
        refreshFromModel();
        project.setModified();
    };
    faceplate.onParametersChanged = [this] { project.setModified(); };

    updateGroupVisibility();
    refreshFromModel();
}

EffectsScreen::~EffectsScreen()
{
}

void EffectsScreen::setupSlots()
{
    for (int i = 0; i < 8; ++i)
    {
        auto& slot = slotUis[(size_t) i];

        slot.slotTitle.setText ("FX " + juce::String (i + 1), juce::dontSendNotification);
        slot.slotTitle.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        slot.slotTitle.setColour (juce::Label::textColourId, ConsoleLookAndFeel::accentCyan);
        addAndMakeVisible (slot.slotTitle);

        // Match the installed X32-Mix FX picker order and names.
        for (int t = 0; t < getNumFxTypes(); ++t)
            slot.typeCombo.addItem (juce::String (t + 1) + ". " + getFxMetadata ((FxType) t).name, t + 1);

        slot.typeCombo.addListener (this);
        slot.typeCombo.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff101317));
        slot.typeCombo.setColour (juce::ComboBox::outlineColourId, ConsoleLookAndFeel::borderSubtle);
        slot.typeCombo.setColour (juce::ComboBox::textColourId, ConsoleLookAndFeel::textPrimary);
        slot.typeCombo.setColour (juce::ComboBox::arrowColourId, ConsoleLookAndFeel::accentCyan);
        addAndMakeVisible (slot.typeCombo);

        slot.bypassButton.setClickingTogglesState (true);
        slot.bypassButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2b3038));
        slot.bypassButton.addListener (this);
        addAndMakeVisible (slot.bypassButton);

        slot.modeButton.setClickingTogglesState (true);
        slot.modeButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2b3038));
        slot.modeButton.addListener (this);
        addAndMakeVisible (slot.modeButton);

        slot.mixDial.onValueChanged = [this, i] (double val) {
            if (ignoreCallbacks) return;
            auto& p = engine.getMixerParams().fxSlots[(size_t) i];
            p.mix.set ((float) (val * 0.01));
            if (selectedSlot == i) faceplate.refreshFromSlot();
            project.setModified();
        };
        addAndMakeVisible (slot.mixDial);

        auto& fxSlotDsp = engine.getMixEngine().getFxRack().getSlot (i);
        slot.inMeter.setMeters (fxSlotDsp.getInPeakLPtr(), fxSlotDsp.getInPeakRPtr(), nullptr);
        slot.outMeter.setMeters (fxSlotDsp.getOutPeakLPtr(), fxSlotDsp.getOutPeakRPtr(), nullptr);
        addAndMakeVisible (slot.inMeter);
        addAndMakeVisible (slot.outMeter);

        slot.selectButton.addListener (this);
        slot.selectButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2b3038));
        addAndMakeVisible (slot.selectButton);
    }
}

void EffectsScreen::updateGroupVisibility()
{
    const int startSlot = (currentGroup == Group1To4) ? 0 : 4;
    const int endSlot = startSlot + 4;

    for (int i = 0; i < 8; ++i)
    {
        const bool visible = (i >= startSlot && i < endSlot);
        auto& s = slotUis[(size_t) i];
        s.slotTitle.setVisible (visible);
        s.typeCombo.setVisible (visible);
        s.bypassButton.setVisible (visible);
        s.modeButton.setVisible (visible);
        s.modeButton.setEnabled (i < 4);
        s.mixDial.setVisible (visible);
        s.inMeter.setVisible (visible);
        s.outMeter.setVisible (visible);
        s.selectButton.setVisible (visible);
    }

    if (selectedSlot < startSlot || selectedSlot >= endSlot)
    {
        selectedSlot = startSlot;
        faceplate.setSlot (selectedSlot);
    }

    resized();
    repaint();
}

void EffectsScreen::refreshFromModel()
{
    ignoreCallbacks = true;
    for (int i = 0; i < 8; ++i)
    {
        auto& p = engine.getMixerParams().fxSlots[(size_t) i];
        auto& s = slotUis[(size_t) i];

        const int typeId = (int) p.fxType.getInt() + 1;
        s.typeCombo.setSelectedId (typeId, juce::dontSendNotification);

        const bool bypassed = p.bypass.getBool();
        s.bypassButton.setToggleState (! bypassed, juce::dontSendNotification);
        s.bypassButton.setButtonText (bypassed ? "BYPASS" : "IN");

        const bool isInsert = i >= 4 || p.insertMode.getBool();
        if (i >= 4)
            p.insertMode.setBool (true);
        s.modeButton.setToggleState (isInsert, juce::dontSendNotification);
        s.modeButton.setButtonText (i >= 4 ? "INSERT ONLY" : (isInsert ? "INSERT" : "SEND/RET"));

        s.mixDial.setValue (p.mix.get() * 100.0, juce::dontSendNotification);
    }

    faceplate.refreshFromSlot();
    ignoreCallbacks = false;
    repaint();
}

void EffectsScreen::buttonClicked (juce::Button* b)
{
    if (b == &backButton)
    {
        if (onBack) onBack();
        return;
    }

    for (int g = 0; g < NumGroups; ++g)
    {
        if (b == &groupButtons[g])
        {
            currentGroup = g;
            updateGroupVisibility();
            return;
        }
    }

    for (int i = 0; i < 8; ++i)
    {
        auto& s = slotUis[(size_t) i];
        auto& p = engine.getMixerParams().fxSlots[(size_t) i];

        if (b == &s.selectButton)
        {
            selectedSlot = i;
            faceplate.setSlot (selectedSlot);
            repaint();
            return;
        }

        if (b == &s.bypassButton)
        {
            const bool active = s.bypassButton.getToggleState();
            p.bypass.setBool (! active);
            s.bypassButton.setButtonText (active ? "IN" : "BYPASS");
            if (selectedSlot == i) faceplate.refreshFromSlot();
            project.setModified();
            return;
        }

        if (b == &s.modeButton)
        {
            if (i >= 4) return;
            const bool isInsert = s.modeButton.getToggleState();
            p.insertMode.setBool (isInsert);
            s.modeButton.setButtonText (isInsert ? "INSERT" : "SEND/RET");
            project.setModified();
            return;
        }
    }
}

void EffectsScreen::comboBoxChanged (juce::ComboBox* combo)
{
    if (ignoreCallbacks) return;

    for (int i = 0; i < 8; ++i)
    {
        if (combo == &slotUis[(size_t) i].typeCombo)
        {
            const int selectedId = combo->getSelectedId();
            if (selectedId >= 1 && selectedId <= getNumFxTypes())
            {
                auto& p = engine.getMixerParams().fxSlots[(size_t) i];
                p.setDefaultsForType ((FxType) (selectedId - 1));
                if (selectedSlot == i)
                    faceplate.setSlot (selectedSlot);
                refreshFromModel();
                project.setModified();
            }
            return;
        }
    }
}

void EffectsScreen::paint (juce::Graphics& g)
{
    g.fillAll (ConsoleLookAndFeel::bgDark);

    const auto bounds = getLocalBounds();
    const int totalW = bounds.getWidth();

    // Group Tab Bar Outline
    g.setColour (juce::Colour (0xff20242b));
    g.fillRect (0, 0, totalW, 46);
    g.setColour (ConsoleLookAndFeel::borderSubtle);
    g.drawHorizontalLine (46, 0.0f, (float) totalW);

    // Outline around Selected Slot in Rack
    const int startSlot = (currentGroup == Group1To4) ? 0 : 4;
    const int slotW = (totalW - 24) / 4;
    const int cardY = 54;
    const int cardH = 166;

    for (int i = 0; i < 4; ++i)
    {
        const int sIdx = startSlot + i;
        const int cardX = 12 + i * slotW;
        const juce::Rectangle<float> rect ((float) cardX + 2.0f, (float) cardY, (float) slotW - 4.0f, (float) cardH);

        const bool isSelected = (selectedSlot == sIdx);
        g.setColour (isSelected ? juce::Colour (0xff242b36) : juce::Colour (0xff1a1e24));
        g.fillRoundedRectangle (rect, 4.0f);

        g.setColour (isSelected ? ConsoleLookAndFeel::accentCyan : juce::Colour (0xff323a46));
        g.drawRoundedRectangle (rect, 4.0f, isSelected ? 1.8f : 1.0f);
        g.setColour (isSelected ? ConsoleLookAndFeel::accentCyan.withAlpha (0.10f)
                                : juce::Colour (0xff252a31));
        g.fillRect (cardX + 3, cardY + 3, slotW - 6, 3);
    }
}

void EffectsScreen::resized()
{
    const int totalW = getWidth();
    const int totalH = getHeight();

    // 1. Group Selector Tabs & Back Button
    backButton.setBounds (8, 9, 84, 29);
    const int tabW = std::min (260, std::max (180, (totalW - 120) / 3));
    groupButtons[0].setBounds (100, 9, tabW, 29);
    groupButtons[1].setBounds (108 + tabW, 9, tabW, 29);

    // 2. Upper 4 Modular Rack Cards
    const int startSlot = (currentGroup == Group1To4) ? 0 : 4;
    const int slotW = (totalW - 24) / 4;
    const int cardY = 54;

    for (int i = 0; i < 4; ++i)
    {
        const int sIdx = startSlot + i;
        const int cardX = 12 + i * slotW;
        auto& s = slotUis[(size_t) sIdx];

        s.slotTitle.setBounds (cardX + 10, cardY + 8, 80, 20);
        s.bypassButton.setBounds (cardX + slotW - 68, cardY + 8, 56, 22);
        s.typeCombo.setBounds (cardX + 10, cardY + 34, slotW - 24, 26);
        s.modeButton.setBounds (cardX + 10, cardY + 66, 85, 22);
        s.mixDial.setBounds (cardX + 10, cardY + 92, 70, 56);

        // LED Meters
        const int meterX = cardX + 90;
        const int meterW = 30;
        s.inMeter.setBounds (meterX, cardY + 66, meterW, 76);
        s.outMeter.setBounds (meterX + meterW + 6, cardY + 66, meterW, 76);

        s.selectButton.setBounds (cardX + slotW - 74, cardY + 136, 64, 24);
    }

    // 3. Lower Hardware Faceplate Area
    const int faceplateY = 232;
    const int faceplateVisibleH = std::max (1, totalH - faceplateY - 10);
    // FX faceplates have detailed control layouts designed for a taller rack.
    // Scale the whole faceplate uniformly on short phone screens so every
    // switch and parameter remains visible and touchable.
    const float faceplateScale = std::min (1.0f, faceplateVisibleH / 220.0f);
    faceplate.setTransform (juce::AffineTransform::scale (faceplateScale));
    faceplate.setBounds (12, faceplateY,
                         juce::roundToInt ((totalW - 24) / faceplateScale), 220);
}

} // namespace mps
