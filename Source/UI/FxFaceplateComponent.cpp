#include "FxFaceplateComponent.h"
#include <cstdio>
#include <cmath>

namespace mps
{

FxFaceplateComponent::FxFaceplateComponent (AudioEngine& eng)
    : engine (eng)
{
    setLookAndFeel (&rackLnf);

    // 1. Dual Independent Insert Buttons
    for (int i = 0; i < 2; ++i)
    {
        insertButtons[i].setButtonText ("Insert");
        insertButtons[i].getProperties().set ("rackBtn", true);
        insertButtons[i].addListener (this);
        addAndMakeVisible (insertButtons[i]);

        muteButtons[i].setButtonText ("MUTE");
        muteButtons[i].getProperties().set ("muteBtn", true);
        muteButtons[i].setClickingTogglesState (true);
        muteButtons[i].addListener (this);
        addAndMakeVisible (muteButtons[i]);
    }

    // 2. EFFECTS and PRESETS Buttons
    effectsButton.getProperties().set ("rackBtn", true);
    effectsButton.addListener (this);
    addAndMakeVisible (effectsButton);

    presetsButton.getProperties().set ("rackBtn", true);
    presetsButton.addListener (this);
    addAndMakeVisible (presetsButton);

    // 2b. Bottom Rack Bar Controls (Copy, Paste, Layer/Engine 1 & 2)
    copyButton.getProperties().set ("rackBtn", true);
    copyButton.addListener (this);
    addAndMakeVisible (copyButton);

    pasteButton.getProperties().set ("rackBtn", true);
    pasteButton.addListener (this);
    addAndMakeVisible (pasteButton);

    for (int i = 0; i < 2; ++i)
    {
        engineButtons[i].getProperties().set ("engineBtn", true);
        engineButtons[i].setClickingTogglesState (true);
        engineButtons[i].setToggleState (i == activeEngineLayer, juce::dontSendNotification);
        engineButtons[i].addListener (this);
        addAndMakeVisible (engineButtons[i]);
    }

    // 3. Hardware Unit Canvas
    addAndMakeVisible (unitCanvas);

    unitCanvas.onParameterChanged = [this] (int paramIdx, float normVal) {
        if (ignoreCallbacks) return;
        auto& p = engine.getMixerParams().fxSlots[(size_t) currentSlot];
        const auto& info = unitCanvas.getCurrentModel();

        if (info.family == X32Fx::GuiFamily::GraphicEq)
        {
            if (paramIdx >= 0 && paramIdx < 31)
            {
                if (auto* c = unitCanvas.getControl (paramIdx))
                {
                    const float db = (float) c->getValue();
                    const auto curType = (FxType) (int) p.fxType.getInt();
                    const bool isDualGeq = (curType == FxType::DualGEQ || curType == FxType::DualTruEQ);
                    if (! isDualGeq || activeEngineLayer == 0)
                        p.geqGainsL[(size_t) paramIdx].set (db);
                    if (! isDualGeq || activeEngineLayer == 1)
                        p.geqGainsR[(size_t) paramIdx].set (db);
                }
            }
        }
        else
        {
            p.parameter (paramIdx).set (normVal);
        }

        if (onParametersChanged) onParametersChanged();
    };

    unitCanvas.onSubProgramSelected = [this] (int subIdx) {
        if (ignoreCallbacks) return;
        FxType targetType = FxType::HallReverb;
        switch (subIdx)
        {
            case 0: targetType = FxType::HallReverb; break;
            case 1: targetType = FxType::AmbienceReverb; break;
            case 2: targetType = FxType::RichPlateReverb; break;
            case 3: targetType = FxType::RoomReverb; break;
            case 4: targetType = FxType::ChamberReverb; break;
            case 5: targetType = FxType::HallReverb; break;
        }

        auto& p = engine.getMixerParams().fxSlots[(size_t) currentSlot];
        const auto curType = (FxType) (int) p.fxType.getInt();
        if (curType == targetType)
        {
            larcPageIndex = (larcPageIndex + 1) % 3;
            refreshFromSlot();
        }
        else
        {
            p.setDefaultsForType (targetType);
            setSlot (currentSlot);
            if (onTypeChangeRequested) onTypeChangeRequested (targetType);
            if (onParametersChanged) onParametersChanged();
        }
    };

    unitCanvas.onPageChanged = [this] (int page) {
        if (ignoreCallbacks) return;
        larcPageIndex = page;
        refreshFromSlot();
        if (onParametersChanged) onParametersChanged();
    };

    // 4. Carousel Overlay
    addChildComponent (carousel);
    carousel.onEffectChosen = [this] (const X32Fx::EffectModelInfo& chosen) {
        if (ignoreCallbacks) return;
        const auto newType = (FxType) chosen.index14;
        auto& p = engine.getMixerParams().fxSlots[(size_t) currentSlot];
        p.setDefaultsForType (newType);
        setSlot (currentSlot);
        if (onTypeChangeRequested) onTypeChangeRequested (newType);
        if (onParametersChanged) onParametersChanged();
    };
    carousel.onClose = [this] {
        carousel.setVisible (false);
    };

    setSlot (0);
}

FxFaceplateComponent::~FxFaceplateComponent()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void FxFaceplateComponent::setSlot (int slotIndex)
{
    currentSlot = juce::jlimit (0, 7, slotIndex);
    auto& p = engine.getMixerParams().fxSlots[(size_t) currentSlot];
    const auto type = (FxType) (int) p.fxType.getInt();

    const auto& catalog = X32Fx::getCatalog();
    const int modelIdx = (int) type;
    if (modelIdx >= 0 && modelIdx < (int) catalog.size())
    {
        unitCanvas.loadEffectModel (catalog[(size_t) modelIdx]);
    }

    refreshFromSlot();
}

void FxFaceplateComponent::setLarcPageIndex (int page)
{
    larcPageIndex = std::max (0, std::min (2, page));
    refreshFromSlot();
    repaint();
}

void FxFaceplateComponent::refreshFromSlot()
{
    ignoreCallbacks = true;
    auto& p = engine.getMixerParams().fxSlots[(size_t) currentSlot];

    // 1. Update Rack Control Panel Buttons
    for (int i = 0; i < 2; ++i)
    {
        insertButtons[i].setButtonText (getInsertTargetText (currentSlot, i));
        muteButtons[i].setToggleState (p.bypass.getBool(), juce::dontSendNotification);
    }

    // Helper to convert stored parameter value (either normalized 0..1 or legacy raw default) to 0..100 slider scale
    auto getSliderPercent = [&p] (const X32Fx::EffectModelInfo& model, int paramIdx) -> float {
        const float raw = p.parameter (paramIdx).get();
        if (raw >= 0.0f && raw <= 1.0f)
        {
            // Check if raw is an exact normalized value (or 0.0 / 1.0)
            // Note: For HallReverb, p6 is initialized to 0.0f in setDefaultsForType, whereas p1..p5 are > 1.0f.
            const bool slotHasLegacyRaw = (p.parameter (0).get() > 1.001f || p.parameter (1).get() > 1.001f || p.parameter (2).get() > 1.001f);
            if (! slotHasLegacyRaw)
                return raw * 100.0f;
        }
        if (paramIdx >= 0 && paramIdx < (int) model.defaultValues.size())
            return model.defaultValues[(size_t) paramIdx];
        return juce::jlimit (0.0f, 100.0f, raw);
    };

    // 2. Populate Canvas Controls from AudioEngine Parameters
    const auto& info = unitCanvas.getCurrentModel();
    if (info.family == X32Fx::GuiFamily::LarcReverb)
    {
        unitCanvas.setLarcPage (larcPageIndex, juce::dontSendNotification);
        for (int i = 0; i < 6; ++i)
        {
            const int pIdx = larcPageIndex * 6 + i;
            if (auto* c = unitCanvas.getControl (i))
            {
                c->setValue (getSliderPercent (info, pIdx), juce::dontSendNotification);
            }
        }
    }
    else if (info.family == X32Fx::GuiFamily::GraphicEq)
    {
        const auto curType = (FxType) (int) p.fxType.getInt();
        const bool useRight = (curType == FxType::DualGEQ || curType == FxType::DualTruEQ) && (activeEngineLayer == 1);
        for (int i = 0; i < 31; ++i)
        {
            if (auto* c = unitCanvas.getControl (i))
            {
                const float gVal = useRight ? p.geqGainsR[(size_t) i].get() : p.geqGainsL[(size_t) i].get();
                c->setValue (gVal, juce::dontSendNotification);
            }
        }
    }
    else if (info.family == X32Fx::GuiFamily::PrecisionLimiter)
    {
        for (int i = 0; i < 6; ++i)
        {
            if (auto* c = unitCanvas.getControl (i))
            {
                c->setValue (getSliderPercent (info, i), juce::dontSendNotification);
            }
        }
    }
    else
    {
        const int cnt = juce::jmin ((int) info.paramNames.size(), 12);
        for (int i = 0; i < cnt; ++i)
        {
            if (auto* c = unitCanvas.getControl (i))
            {
                c->setValue (getSliderPercent (info, i), juce::dontSendNotification);
            }
        }
    }

    unitCanvas.updateAllControlStatuses();
    syncMixerEntityInserts();
    ignoreCallbacks = false;
    repaint();
}

juce::String FxFaceplateComponent::getInsertTargetText (int slotIdx, int insertPointIndex) const
{
    if (slotIdx < 0 || slotIdx >= 8 || insertPointIndex < 0 || insertPointIndex >= 2)
        return "Insert";

    const auto& mp = engine.getMixerParams();
    const float rawTarget = mp.fxSlots[(size_t) slotIdx].insertTargets[(size_t) insertPointIndex].get();
    const int targetCode = juce::roundToInt (rawTarget);

    if (targetCode >= 1 && targetCode <= 32)
    {
        char buf[16];
        std::snprintf (buf, sizeof (buf), "Ch %02d", targetCode);
        return juce::String (buf);
    }
    else if (targetCode >= 101 && targetCode <= 116)
    {
        char buf[16];
        std::snprintf (buf, sizeof (buf), "Bus %02d", targetCode - 100);
        return juce::String (buf);
    }
    else if (targetCode >= 201 && targetCode <= 206)
    {
        return "Mtx " + juce::String (targetCode - 200);
    }
    else if (targetCode == 301)
    {
        return "Main LR";
    }

    if (slotIdx >= 0 && slotIdx < 4)
    {
        char buf[16];
        std::snprintf (buf, sizeof (buf), "Bus %02d", 13 + slotIdx);
        return juce::String (buf);
    }

    return "Insert";
}

juce::String FxFaceplateComponent::getInsertTargetTextForSlot (int slotIdx) const
{
    const auto t0 = getInsertTargetText (slotIdx, 0);
    const auto t1 = getInsertTargetText (slotIdx, 1);
    if (t0 == t1)
        return t0;
    return t0 + " / " + t1;
}

void FxFaceplateComponent::syncMixerEntityInserts()
{
    auto& mp = engine.getMixerParams();

    for (int ch = 0; ch < kMaxInputChannels; ++ch)
        mp.channels[(size_t) ch].insertFxSlot.set (0.0f);
    for (int b = 0; b < 16; ++b)
        mp.buses[(size_t) b].insertFxSlot.set (0.0f);
    for (int m = 0; m < 6; ++m)
        mp.matrices[(size_t) m].insertFxSlot.set (0.0f);
    mp.main.insertFxSlot.set (0.0f);

    for (int s = 0; s < 8; ++s)
    {
        auto& slot = mp.fxSlots[(size_t) s];
        const bool isIns = (s >= 4) || slot.insertMode.getBool();
        if (! isIns)
            continue;

        const int slotNum = s + 1; // 1-based (1..8)
        for (int p = 0; p < 2; ++p)
        {
            const int targetCode = juce::roundToInt (slot.insertTargets[(size_t) p].get());
            if (targetCode >= 1 && targetCode <= 32)
            {
                mp.channels[(size_t) (targetCode - 1)].insertFxSlot.set ((float) slotNum);
            }
            else if (targetCode >= 101 && targetCode <= 116)
            {
                mp.buses[(size_t) (targetCode - 101)].insertFxSlot.set ((float) slotNum);
            }
            else if (targetCode >= 201 && targetCode <= 206)
            {
                mp.matrices[(size_t) (targetCode - 201)].insertFxSlot.set ((float) slotNum);
            }
            else if (targetCode == 301)
            {
                mp.main.insertFxSlot.set ((float) slotNum);
            }
        }
    }
}

void FxFaceplateComponent::showInsertMenu (juce::Component* target, int insertPointIndex)
{
    if (insertPointIndex < 0 || insertPointIndex >= 2)
        insertPointIndex = 0;

    auto& mp = engine.getMixerParams();
    auto& slotParams = mp.fxSlots[(size_t) currentSlot];
    const int curTargetCode = juce::roundToInt (slotParams.insertTargets[(size_t) insertPointIndex].get());

    juce::PopupMenu menu;

    if (currentSlot >= 0 && currentSlot < 4)
    {
        const int defaultBus = 13 + currentSlot;
        char defLabel[48];
        std::snprintf (defLabel, sizeof (defLabel), "Bus %02d (Default Send/Return)", defaultBus);

        const bool isDefault = (! slotParams.insertMode.getBool()) || (curTargetCode == 100 + defaultBus) || (curTargetCode == 0);
        menu.addItem (1000, defLabel, true, isDefault);
        menu.addSeparator();

        for (int b = 0; b < 16; ++b)
        {
            char itemLabel[24];
            std::snprintf (itemLabel, sizeof (itemLabel), "Bus %02d", b + 1);
            const bool isCur = (curTargetCode == 101 + b);
            menu.addItem (1001 + b, itemLabel, true, isCur);
        }

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target != nullptr ? target : &insertButtons[insertPointIndex]),
            [this, insertPointIndex, defaultBus] (int res) {
                if (res <= 0) return;
                auto& mixerParams = engine.getMixerParams();
                auto& slot = mixerParams.fxSlots[(size_t) currentSlot];

                if (res >= 1001 && res <= 1016)
                {
                    const int busNum = res - 1000;
                    slot.insertTargets[(size_t) insertPointIndex].set ((float) (100 + busNum));
                    slot.insertMode.setBool (true);
                }
                else if (res == 1000)
                {
                    slot.insertTargets[(size_t) insertPointIndex].set ((float) (100 + defaultBus));
                    slot.insertMode.setBool (false);
                }

                syncMixerEntityInserts();
                refreshFromSlot();
                if (onParametersChanged) onParametersChanged();
            });
    }
    else
    {
        menu.addItem (2000, "Unassigned (OFF)", true, curTargetCode == 0);
        menu.addSeparator();

        juce::PopupMenu chMenu;
        for (int ch = 0; ch < kMaxInputChannels; ++ch)
        {
            char buf[24];
            std::snprintf (buf, sizeof (buf), "CH %02d", ch + 1);
            chMenu.addItem (2100 + ch, buf, true, curTargetCode == (ch + 1));
        }
        menu.addSubMenu ("Input Channels (CH 01-32)", chMenu);

        juce::PopupMenu busMenu;
        for (int b = 0; b < 16; ++b)
        {
            char buf[24];
            std::snprintf (buf, sizeof (buf), "Bus %02d", b + 1);
            busMenu.addItem (2200 + b, buf, true, curTargetCode == (101 + b));
        }
        menu.addSubMenu ("Mix Buses (Bus 01-16)", busMenu);

        juce::PopupMenu mtxMenu;
        for (int m = 0; m < 6; ++m)
        {
            mtxMenu.addItem (2300 + m, "Matrix " + juce::String (m + 1), true, curTargetCode == (201 + m));
        }
        menu.addSubMenu ("Matrices (Matrix 1-6)", mtxMenu);

        menu.addItem (2400, "Main LR", true, curTargetCode == 301);

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target != nullptr ? target : &insertButtons[insertPointIndex]),
            [this, insertPointIndex] (int res) {
                if (res <= 0) return;
                auto& mixerParams = engine.getMixerParams();
                auto& slot = mixerParams.fxSlots[(size_t) currentSlot];

                if (res == 2000)
                {
                    slot.insertTargets[(size_t) insertPointIndex].set (0.0f);
                }
                else if (res >= 2100 && res < 2100 + kMaxInputChannels)
                {
                    const int chIdx = res - 2100;
                    slot.insertTargets[(size_t) insertPointIndex].set ((float) (chIdx + 1));
                }
                else if (res >= 2200 && res < 2216)
                {
                    const int busIdx = res - 2200;
                    slot.insertTargets[(size_t) insertPointIndex].set ((float) (101 + busIdx));
                }
                else if (res >= 2300 && res < 2306)
                {
                    const int mtxIdx = res - 2300;
                    slot.insertTargets[(size_t) insertPointIndex].set ((float) (201 + mtxIdx));
                }
                else if (res == 2400)
                {
                    slot.insertTargets[(size_t) insertPointIndex].set (301.0f);
                }

                syncMixerEntityInserts();
                refreshFromSlot();
                if (onParametersChanged) onParametersChanged();
            });
    }
}

void FxFaceplateComponent::showInsertMenu (juce::Component* target)
{
    juce::PopupMenu choiceMenu;
    choiceMenu.addItem (1, "Insert 1 (Left / In A): " + getInsertTargetText (currentSlot, 0));
    choiceMenu.addItem (2, "Insert 2 (Right / In B): " + getInsertTargetText (currentSlot, 1));
    choiceMenu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target),
        [this, target] (int res) {
            if (res == 1) showInsertMenu (target, 0);
            else if (res == 2) showInsertMenu (target, 1);
        });
}

void FxFaceplateComponent::openCarousel()
{
    carousel.openForSlot (currentSlot + 1, unitCanvas.getCurrentModel().id);
}

void FxFaceplateComponent::showPresetsMenu (juce::Component* target)
{
    const auto& info = unitCanvas.getCurrentModel();
    juce::PopupMenu menu;
    menu.addSectionHeader (info.name.toUpperCase() + " FACTORY PRESETS");
    menu.addItem (1, "1. Factory Default (" + info.name + ")");
    menu.addItem (2, "2. Warm Studio Vocal /Lush");
    menu.addItem (3, "3. Tight Drum / Percussion");
    menu.addItem (4, "4. Wide Arena / Live Mix");
    menu.addItem (5, "5. Subtle Analog Glue");

    if (info.family == X32Fx::GuiFamily::LarcReverb)
    {
        menu.addSeparator();
        menu.addItem (10, "Cycle LARC Parameter Page (Page " + juce::String (larcPageIndex + 1) + "/3)");
    }

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target != nullptr ? target : &presetsButton),
        [this, &info] (int res) {
            if (res <= 0) return;
            if (res == 10)
            {
                setLarcPageIndex ((larcPageIndex + 1) % 3);
                return;
            }

            auto& p = engine.getMixerParams().fxSlots[(size_t) currentSlot];
            const auto curType = (FxType) (int) p.fxType.getInt();
            p.setDefaultsForType (curType);

            const float scale = (res == 1) ? 1.0f : (res == 2 ? 1.18f : (res == 3 ? 0.72f : (res == 4 ? 1.32f : 0.88f)));
            for (int i = 0; i < 16; ++i)
            {
                const float baseDef = (i < (int) info.defaultValues.size()) ? (info.defaultValues[(size_t) i] / 100.0f) : 0.5f;
                p.parameter (i).set (juce::jlimit (0.05f, 0.95f, baseDef * scale));
            }
            refreshFromSlot();
            if (onParametersChanged) onParametersChanged();
        });
}

void FxFaceplateComponent::buttonClicked (juce::Button* b)
{
    if (b == &muteButtons[0] || b == &muteButtons[1])
    {
        auto& p = engine.getMixerParams().fxSlots[(size_t) currentSlot];
        p.bypass.toggle();
        refreshFromSlot();
        if (onParametersChanged) onParametersChanged();
        return;
    }

    if (b == &insertButtons[0])
    {
        showInsertMenu (b, 0);
        return;
    }

    if (b == &insertButtons[1])
    {
        showInsertMenu (b, 1);
        return;
    }

    if (b == &effectsButton)
    {
        openCarousel();
        return;
    }

    if (b == &presetsButton)
    {
        showPresetsMenu (b);
        return;
    }

    if (b == &copyButton)
    {
        const auto& p = engine.getMixerParams().fxSlots[(size_t) currentSlot];
        clipboardType = (FxType) (int) p.fxType.getInt();
        for (int i = 0; i < 18; ++i)
            clipboardParams[(size_t) i] = p.parameter (i).get();
        for (int i = 0; i < 31; ++i)
            clipboardGeq[(size_t) i] = p.geqGainsL[(size_t) i].get();
        hasClipboard = true;
        return;
    }

    if (b == &pasteButton)
    {
        if (! hasClipboard) return;
        auto& p = engine.getMixerParams().fxSlots[(size_t) currentSlot];
        p.fxType.set ((float) (int) clipboardType);
        for (int i = 0; i < 18; ++i)
            p.parameter (i).set (clipboardParams[(size_t) i]);
        for (int i = 0; i < 31; ++i)
        {
            p.geqGainsL[(size_t) i].set (clipboardGeq[(size_t) i]);
            p.geqGainsR[(size_t) i].set (clipboardGeq[(size_t) i]);
        }
        setSlot (currentSlot);
        if (onTypeChangeRequested) onTypeChangeRequested (clipboardType);
        if (onParametersChanged) onParametersChanged();
        return;
    }

    for (int i = 0; i < 2; ++i)
    {
        if (b == &engineButtons[i])
        {
            activeEngineLayer = i;
            engineButtons[0].setToggleState (activeEngineLayer == 0, juce::dontSendNotification);
            engineButtons[1].setToggleState (activeEngineLayer == 1, juce::dontSendNotification);
            if (unitCanvas.getCurrentModel().family == X32Fx::GuiFamily::LarcReverb)
                setLarcPageIndex (i);
            repaint();
            return;
        }
    }
}

void FxFaceplateComponent::paint (juce::Graphics& g)
{
    const int w = getWidth();
    const int h = getHeight();

    // 1. Top Control Strip Background (compact on mobile so center faceplate gets maximum height)
    const int topStripH = juce::jlimit (44, 62, juce::roundToInt ((float) h * 0.165f));
    g.setColour (juce::Colour (0xff2b2b2e));
    g.fillRect (0, 0, w, topStripH);
    g.setColour (juce::Colour (0xff18181b));
    g.drawHorizontalLine (topStripH - 1, 0.0f, (float) w);

    // Routing Box on Left
    const float topBarY = 4.0f;
    const float topBarH = (float) topStripH - 8.0f;
    const float routingBoxW = juce::jmin ((float) w - 230.0f, 500.0f);
    auto routingBox = juce::Rectangle<float> (8.0f, topBarY, routingBoxW, topBarH);

    g.setColour (juce::Colour (0xff303033));
    g.fillRoundedRectangle (routingBox, 3.0f);
    g.setColour (juce::Colour (0xff8a8a93));
    g.drawRoundedRectangle (routingBox, 3.0f, 1.0f);

    // Stereo Infinity Loop Diagram -> [ L oo R ] -> (distinct top/middle/bottom bands so ∞ never overlaps R)
    auto loopBox = juce::Rectangle<float> (routingBox.getCentreX() - 60.0f, topBarY + 3.0f, 120.0f, topBarH - 6.0f);
    g.setColour (juce::Colour (0xff252528));
    g.fillRoundedRectangle (loopBox, 2.0f);
    g.setColour (juce::Colour (0xff9ca3af));
    g.drawRoundedRectangle (loopBox, 2.0f, 1.0f);

    const float thirdH = loopBox.getHeight() / 3.0f;
    g.setColour (juce::Colour (0xffd4d4d8));
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    g.drawText (juce::CharPointer_UTF8 ("\xe2\x86\x92   L   \xe2\x86\x92"),
                loopBox.withHeight (thirdH + 1.0f), juce::Justification::centred, false);
    g.setColour (juce::Colour (0xff38bdf8));
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText (juce::CharPointer_UTF8 ("\xe2\x88\x9e"),
                loopBox.withY (loopBox.getY() + thirdH - 1.0f).withHeight (thirdH + 2.0f), juce::Justification::centred, false);
    g.setColour (juce::Colour (0xffd4d4d8));
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    g.drawText (juce::CharPointer_UTF8 ("\xe2\x86\x92   R   \xe2\x86\x92"),
                loopBox.withY (loopBox.getBottom() - thirdH - 1.0f).withHeight (thirdH + 1.0f), juce::Justification::centred, false);

    // LED Meters (Input & Output pairs)
    const float meterInX = loopBox.getX() - 90.0f;
    const float meterOutX = loopBox.getRight() + 8.0f;
    const float mRow1Y = topBarY + topBarH * 0.16f;
    const float mRow2Y = topBarY + topBarH * 0.56f;
    drawLedMeter (g, meterInX, mRow1Y, peakInL);
    drawLedMeter (g, meterInX, mRow2Y, peakInR);
    drawLedMeter (g, meterOutX, mRow1Y, peakOutL);
    drawLedMeter (g, meterOutX, mRow2Y, peakOutR);

    // Current Effect Title Badge on Right
    g.setColour (juce::Colour (0xfffbbf24));
    g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 11.5f, juce::Font::bold));
    const auto title = "FX" + juce::String (currentSlot + 1) + ": " + unitCanvas.getCurrentModel().name.toUpperCase();
    const int badgeX = (int) (routingBox.getRight() + 8.0f);
    const int badgeW = std::max (70, w - badgeX - 104);
    g.drawFittedText (title, badgeX, (int) topBarY, badgeW, (int) topBarH, juce::Justification::centredRight, 2, 0.85f);

    // 2. Bottom Rack Bar (compact on mobile so center faceplate gets more room)
    const int botBarH = juce::jlimit (34, 46, juce::roundToInt ((float) h * 0.125f));
    const int botBarY = h - botBarH;
    g.setColour (juce::Colour (0xff27272a));
    g.fillRect (0, botBarY, w, botBarH);
    g.setColour (juce::Colour (0xff141416));
    g.drawHorizontalLine (botBarY, 0.0f, (float) w);

    // Center Hardware Text
    g.setColour (juce::Colour (0xffa1a1aa));
    g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 10.5f, juce::Font::plain));
    const juce::String engineStr = "BEHRINGER X32 VIRTUAL FX RACK  |  ENGINE " + juce::String (activeEngineLayer + 1) + " ACTIVE";
    g.drawText (engineStr, 96, botBarY, w - 296, botBarH, juce::Justification::centred, false);

    // Right "LAYER / ENGINE" Label
    g.setColour (juce::Colour (0xffa1a1aa));
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("LAYER / ENGINE", w - 208, botBarY, 106, botBarH, juce::Justification::centredRight, false);
}

void FxFaceplateComponent::drawLedMeter (juce::Graphics& g, float x, float y, float peak01)
{
    // Background recessed box for 11-segment LED meter
    g.setColour (juce::Colour (0xff141416));
    g.fillRoundedRectangle (x - 3.0f, y - 2.0f, 84.0f, 13.0f, 2.0f);
    g.setColour (juce::Colour (0xff3f3f46));
    g.drawRoundedRectangle (x - 3.0f, y - 2.0f, 84.0f, 13.0f, 2.0f, 0.8f);

    const int activeLeds = (int) std::round (juce::jlimit (0.0f, 1.0f, peak01) * 11.0f);
    for (int i = 0; i < 11; ++i)
    {
        if (i < activeLeds)
        {
            if (i >= 9) g.setColour (juce::Colour (0xffef4444)); // red clip
            else if (i >= 7) g.setColour (juce::Colour (0xffeab308)); // yellow
            else g.setColour (juce::Colour (0xff22c55e)); // green
        }
        else
        {
            g.setColour (juce::Colour (0xff1f1f23));
        }
        g.fillRect (x + (float) i * 7.2f, y, 5.2f, 9.0f);
    }
}

void FxFaceplateComponent::resized()
{
    if (getWidth() <= 50 || getHeight() <= 50)
        return;

    const int w = getWidth();
    const int h = getHeight();

    const int topStripH = juce::jlimit (44, 62, juce::roundToInt ((float) h * 0.165f));
    const float topBarY = 4.0f;
    const float topBarH = (float) topStripH - 8.0f;
    const float routingBoxW = juce::jmin ((float) w - 230.0f, 500.0f);
    auto routingBox = juce::Rectangle<float> (8.0f, topBarY, routingBoxW, topBarH);

    const int btnH = juce::jlimit (15, 21, (int) (topBarH * 0.42f));
    const int row1Y = (int) (topBarY + topBarH * 0.06f);
    const int row2Y = (int) (topBarY + topBarH * 0.52f);

    insertButtons[0].setBounds ((int) (routingBox.getX() + 6.0f), row1Y, 74, btnH);
    insertButtons[1].setBounds ((int) (routingBox.getX() + 6.0f), row2Y, 74, btnH);

    muteButtons[0].setBounds ((int) (routingBox.getRight() - 78.0f), row1Y, 72, btnH);
    muteButtons[1].setBounds ((int) (routingBox.getRight() - 78.0f), row2Y, 72, btnH);

    effectsButton.setBounds (w - 98, row1Y, 90, btnH);
    presetsButton.setBounds (w - 98, row2Y, 90, btnH);

    const int botBarH = juce::jlimit (34, 46, juce::roundToInt ((float) h * 0.125f));
    const int botBarY = h - botBarH;
    const int copyBtnH = juce::jlimit (14, 18, (botBarH - 8) / 2);

    // Bottom Rack Bar Buttons
    copyButton.setBounds (10, botBarY + 3, 72, copyBtnH);
    pasteButton.setBounds (10, botBarY + 5 + copyBtnH, 72, copyBtnH);

    const int engBtnH = juce::jlimit (24, 32, botBarH - 8);
    const int engBtnY = botBarY + (botBarH - engBtnH) / 2;
    engineButtons[0].setBounds (w - 90, engBtnY, 36, engBtnH);
    engineButtons[1].setBounds (w - 48, engBtnY, 36, engBtnH);

    // Center Hardware Unit Canvas
    unitCanvas.setBounds (0, topStripH, w, std::max (80, h - topStripH - botBarH));
    carousel.setBounds (getLocalBounds());
}

void FxFaceplateComponent::visibilityChanged()
{
    if (isShowing())
        startTimerHz (15);
    else
        stopTimer();
}

void FxFaceplateComponent::timerCallback()
{
    if (! isShowing())
    {
        stopTimer();
        return;
    }

    auto& slotDsp = engine.getMixEngine().getFxRack().getSlot (currentSlot);
    const float inL = slotDsp.getInPeakL();
    const float inR = slotDsp.getInPeakR();
    const float outL = slotDsp.getOutPeakL();
    const float outR = slotDsp.getOutPeakR();

    peakInL = std::max (inL, peakInL * 0.82f);
    peakInR = std::max (inR, peakInR * 0.82f);
    peakOutL = std::max (outL, peakOutL * 0.82f);
    peakOutR = std::max (outR, peakOutR * 0.82f);

    unitCanvas.setMeterPeaks (std::max (peakInL, peakOutL), std::max (peakInR, peakOutR));
    repaint (8, 6, 520, 60);
}

} // namespace mps
