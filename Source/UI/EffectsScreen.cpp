// Mix Practice Studio - Effects Rack UI Screen Implementation
#include "EffectsScreen.h"

namespace mps
{

EffectsScreen::EffectsScreen (AudioEngine& eng, ProjectModel& proj)
    : engine (eng), project (proj)
{
    // 1. Group Selector Buttons: FX 1-4 vs FX 5-8
    const char* groupNames[NumGroups] = { "FX 1 - 4 (SEND / RETURN RACK)", "FX 5 - 8 (INSERT / OUTPUT RACK)" };
    for (int i = 0; i < NumGroups; ++i)
    {
        groupButtons[i].setButtonText (groupNames[i]);
        groupButtons[i].setClickingTogglesState (true);
        groupButtons[i].setRadioGroupId (5001);
        groupButtons[i].addListener (this);
        addAndMakeVisible (groupButtons[i]);
    }
    groupButtons[0].setToggleState (true, juce::dontSendNotification);

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
    };

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

        // Populate all 41 effects grouped by category
        slot.typeCombo.addItem ("-- 1. REVERB --", 9001);
        slot.typeCombo.setItemEnabled (9001, false);
        for (int t = (int) FxType::HallReverb; t <= (int) FxType::ReverseReverb; ++t)
            slot.typeCombo.addItem (juce::String (t + 1) + ". " + getFxMetadata ((FxType) t).name, t + 1);

        slot.typeCombo.addItem ("-- 2. DELAY --", 9002);
        slot.typeCombo.setItemEnabled (9002, false);
        for (int t = (int) FxType::StereoDelay; t <= (int) FxType::RhythmDelay; ++t)
            slot.typeCombo.addItem (juce::String (t + 1) + ". " + getFxMetadata ((FxType) t).name, t + 1);

        slot.typeCombo.addItem ("-- 3. MODULATION --", 9003);
        slot.typeCombo.setItemEnabled (9003, false);
        for (int t = (int) FxType::StereoChorus; t <= (int) FxType::TremoloPanner; ++t)
            slot.typeCombo.addItem (juce::String (t + 1) + ". " + getFxMetadata ((FxType) t).name, t + 1);

        slot.typeCombo.addItem ("-- 4. PITCH --", 9004);
        slot.typeCombo.setItemEnabled (9004, false);
        slot.typeCombo.addItem ("18. Pitch Shifter", (int) FxType::PitchShifter + 1);

        slot.typeCombo.addItem ("-- 5. GRAPHIC EQ (31-BAND) --", 9005);
        slot.typeCombo.setItemEnabled (9005, false);
        for (int t = (int) FxType::StereoGEQ; t <= (int) FxType::DualTruEQ; ++t)
            slot.typeCombo.addItem (juce::String (t + 1) + ". " + getFxMetadata ((FxType) t).name, t + 1);

        slot.typeCombo.addItem ("-- 6. PREMIUM DYNAMICS --", 9006);
        slot.typeCombo.setItemEnabled (9006, false);
        for (int t = (int) FxType::LeisureCompressor; t <= (int) FxType::DualDeEsser; ++t)
            slot.typeCombo.addItem (juce::String (t + 1) + ". " + getFxMetadata ((FxType) t).name, t + 1);

        slot.typeCombo.addItem ("-- 7. PREMIUM TUBE EQ / ENHANCER --", 9007);
        slot.typeCombo.setItemEnabled (9007, false);
        for (int t = (int) FxType::XtecEq1; t <= (int) FxType::DualEnhancer; ++t)
            slot.typeCombo.addItem (juce::String (t + 1) + ". " + getFxMetadata ((FxType) t).name, t + 1);

        slot.typeCombo.addItem ("-- 8. PREMIUM UTILITIES & GUITAR AMP --", 9008);
        slot.typeCombo.setItemEnabled (9008, false);
        for (int t = (int) FxType::EdisonEx1; t <= (int) FxType::SubOctaver; ++t)
            slot.typeCombo.addItem (juce::String (t + 1) + ". " + getFxMetadata ((FxType) t).name, t + 1);

        slot.typeCombo.addListener (this);
        addAndMakeVisible (slot.typeCombo);

        slot.bypassButton.setClickingTogglesState (true);
        slot.bypassButton.addListener (this);
        addAndMakeVisible (slot.bypassButton);

        slot.modeButton.setClickingTogglesState (true);
        slot.modeButton.addListener (this);
        addAndMakeVisible (slot.modeButton);

        slot.mixDial.onValueChanged = [this, i] (double val) {
            if (ignoreCallbacks) return;
            auto& p = engine.getMixerParams().fxSlots[(size_t) i];
            p.mix.set ((float) (val * 0.01));
            if (selectedSlot == i) faceplate.refreshFromSlot();
        };
        addAndMakeVisible (slot.mixDial);

        auto& fxSlotDsp = engine.getMixEngine().getFxRack().getSlot (i);
        slot.inMeter.setMeters (fxSlotDsp.getInPeakLPtr(), fxSlotDsp.getInPeakRPtr(), nullptr);
        slot.outMeter.setMeters (fxSlotDsp.getOutPeakLPtr(), fxSlotDsp.getOutPeakRPtr(), nullptr);
        addAndMakeVisible (slot.inMeter);
        addAndMakeVisible (slot.outMeter);

        slot.selectButton.addListener (this);
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

        const bool isInsert = p.insertMode.getBool();
        s.modeButton.setToggleState (isInsert, juce::dontSendNotification);
        s.modeButton.setButtonText (isInsert ? "INSERT" : "SEND/RET");

        s.mixDial.setValue (p.mix.get() * 100.0, juce::dontSendNotification);
    }

    faceplate.refreshFromSlot();
    ignoreCallbacks = false;
    repaint();
}

void EffectsScreen::buttonClicked (juce::Button* b)
{
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
            return;
        }

        if (b == &s.modeButton)
        {
            const bool isInsert = s.modeButton.getToggleState();
            p.insertMode.setBool (isInsert);
            s.modeButton.setButtonText (isInsert ? "INSERT" : "SEND/RET");
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
            if (selectedId >= 1 && selectedId <= 41)
            {
                auto& p = engine.getMixerParams().fxSlots[(size_t) i];
                p.setDefaultsForType ((FxType) (selectedId - 1));
                if (selectedSlot == i)
                    faceplate.setSlot (selectedSlot);
                refreshFromModel();
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
    const int cardH = 150;

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
    }
}

void EffectsScreen::resized()
{
    const int totalW = getWidth();
    const int totalH = getHeight();

    // 1. Group Selector Tabs
    const int tabW = 260;
    groupButtons[0].setBounds (12, 10, tabW, 28);
    groupButtons[1].setBounds (16 + tabW, 10, tabW, 28);

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

        s.selectButton.setBounds (cardX + slotW - 74, cardY + 116, 64, 24);
    }

    // 3. Lower Hardware Faceplate Area
    const int faceplateY = 212;
    faceplate.setBounds (12, faceplateY, totalW - 24, std::max (100, totalH - faceplateY - 10));
}

} // namespace mps
