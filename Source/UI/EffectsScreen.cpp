// Mix Practice Studio - Effects Rack UI Screen Implementation
#include "EffectsScreen.h"

namespace mps
{

EffectsScreen::EffectsScreen (AudioEngine& eng, ProjectModel& proj)
    : engine (eng), project (proj)
{
    // 1. Back to Mixer button
    backButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff27272a));
    backButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    backButton.addListener (this);
    addAndMakeVisible (backButton);

    // 2. Top-Right FX Library Button (Sky Blue as in simulator)
    libraryHeaderButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff0284c7));
    libraryHeaderButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    libraryHeaderButton.addListener (this);
    addAndMakeVisible (libraryHeaderButton);

    // 3. 8 Slot Tabs: FX1 .. FX8
    for (int i = 0; i < 8; ++i)
    {
        slotTabs[(size_t) i].setButtonText ("FX" + juce::String (i + 1));
        slotTabs[(size_t) i].addListener (this);
        addAndMakeVisible (slotTabs[(size_t) i]);
    }

    // Legacy group buttons kept for test compatibility
    const char* groupNames[NumGroups] = { "FX 1 - 4", "FX 5 - 8" };
    for (int i = 0; i < NumGroups; ++i)
    {
        groupButtons[i].setButtonText (groupNames[i]);
        groupButtons[i].addListener (this);
        addChildComponent (groupButtons[i]);
    }

    // 4. Hardware Simulated Front Panel Faceplate
    addAndMakeVisible (faceplate);
    faceplate.setSlot (selectedSlot);
    faceplate.onTypeChangeRequested = [this] (FxType) {
        refreshFromModel();
        project.setModified();
    };
    faceplate.onParametersChanged = [this] {
        project.setModified();
    };

    refreshFromModel();
}

EffectsScreen::~EffectsScreen() = default;

void EffectsScreen::refreshFromModel()
{
    ignoreCallbacks = true;

    for (int i = 0; i < 8; ++i)
    {
        const bool active = (i == selectedSlot);
        slotTabs[(size_t) i].setColour (juce::TextButton::buttonColourId,
                                        active ? juce::Colour (0xff3c3c3e) : juce::Colour (0xff202022));
        slotTabs[(size_t) i].setColour (juce::TextButton::textColourOffId,
                                        active ? juce::Colours::white : juce::Colour (0xffa1a1aa));
    }

    const bool is58 = (selectedSlot >= 4);
    libraryHeaderButton.setButtonText (is58 ? juce::CharPointer_UTF8 ("\xe2\x9a\xa1 FX5-8 LIBRARY (5 PAGES)")
                                            : juce::CharPointer_UTF8 ("\xe2\x9a\xa1 FX1-4 LIBRARY (8 PAGES)"));

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

    if (b == &libraryHeaderButton)
    {
        faceplate.openCarousel();
        return;
    }

    for (int i = 0; i < 8; ++i)
    {
        if (b == &slotTabs[(size_t) i])
        {
            selectedSlot = i;
            faceplate.setSlot (selectedSlot);
            refreshFromModel();
            return;
        }
    }

    for (int g = 0; g < NumGroups; ++g)
    {
        if (b == &groupButtons[g])
        {
            selectedSlot = (g == 0) ? 0 : 4;
            faceplate.setSlot (selectedSlot);
            refreshFromModel();
            return;
        }
    }
}

void EffectsScreen::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0d0d0f));

    const int totalW = getWidth();
    const int totalH = getHeight();
    if (totalW <= 50 || totalH <= 50)
        return;

    // Enforce strict 2.4 : 6 (height : width) aspect ratio across all platforms (Mobile, Windows, macOS)
    // Ratio: height / width = 2.4 / 6.0 = 0.40  (or width / height = 6.0 / 2.4 = 2.50)
    const int availW = totalW - 16;
    const int availH = totalH - 12;

    int modW = availW;
    int totalBlockH = juce::roundToInt ((float) modW * (2.4f / 6.0f));
    if (totalBlockH > availH)
    {
        totalBlockH = availH;
        modW = juce::roundToInt ((float) totalBlockH * (6.0f / 2.4f));
    }

    const int modX = (totalW - modW) / 2;
    const int startY = (totalH - totalBlockH) / 2;

    const int headerH = juce::jlimit (26, 38, juce::roundToInt ((float) totalBlockH * 0.085f));
    const int gapH = juce::jlimit (4, 8, juce::roundToInt ((float) totalBlockH * 0.018f));
    const int chassisH = totalBlockH - headerH - gapH;
    const int tabStripH = juce::jlimit (24, 30, juce::roundToInt ((float) chassisH * 0.072f));

    // 1. Top Navigation Header Bar
    juce::Rectangle<float> hdrBar ((float) modX, (float) startY, (float) modW, (float) headerH);
    g.setColour (juce::Colour (0xff1a1a1e));
    g.fillRoundedRectangle (hdrBar, 6.0f);
    g.setColour (juce::Colour (0xff27272a));
    g.drawRoundedRectangle (hdrBar, 6.0f, 1.0f);

    // Red glowing status dot
    const float dotX = (float) modX + 88.0f;
    const float dotY = hdrBar.getCentreY() - 5.0f;
    g.setColour (juce::Colour (0xffef4444).withAlpha (0.35f));
    g.fillEllipse (dotX - 3.0f, dotY - 3.0f, 16.0f, 16.0f);
    g.setColour (juce::Colour (0xffef4444));
    g.fillEllipse (dotX, dotY, 10.0f, 10.0f);

    // BEHRINGER | X32 EDIT text
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (13.5f, juce::Font::bold));
    g.drawText ("BEHRINGER", (int) (dotX + 18.0f), startY, 92, headerH, juce::Justification::centredLeft, false);

    g.setColour (juce::Colour (0xff71717a));
    g.setFont (juce::FontOptions (12.5f, juce::Font::plain));
    g.drawText ("|  X32 EDIT  (2.4 : 6)", (int) (dotX + 108.0f), startY, 150, headerH, juce::Justification::centredLeft, false);

    const int rightClearance = (startY < 38 && (modX + modW > totalW - 44)) ? 36 : 0;

    // Mode Badge
    if (modW >= 680)
    {
        const bool is58 = (selectedSlot >= 4);
        const juce::String modeTxt = is58 ? "FX5-8: 28 INSERT MODELS (5 PAGES)" : "FX1-4: 61 MODELS (8 PAGES)";
        juce::Rectangle<float> modeBadge ((float) (modX + modW - 430 - rightClearance), (float) (startY + (headerH - 24) / 2), 215.0f, 24.0f);
        g.setColour (juce::Colour (0xff27272a));
        g.fillRoundedRectangle (modeBadge, 4.0f);
        g.setColour (juce::Colour (0xff3f3f46));
        g.drawRoundedRectangle (modeBadge, 4.0f, 1.0f);
        g.setColour (juce::Colour (0xff38bdf8));
        g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 10.5f, juce::Font::bold));
        g.drawText (modeTxt, modeBadge, juce::Justification::centred, false);
    }

    // 2. Main Rack Chassis Outer Frame & Drop Shadow
    const int chassisY = startY + headerH + gapH;
    juce::Rectangle<float> chassis ((float) modX, (float) chassisY, (float) modW, (float) chassisH);

    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.fillRoundedRectangle (chassis.translated (0.0f, 6.0f), 6.0f);

    g.setColour (juce::Colour (0xff1c1c1e));
    g.fillRoundedRectangle (chassis, 6.0f);

    // Top FX1..FX8 Tab Strip Background
    juce::Rectangle<float> tabStrip ((float) modX, (float) chassisY, (float) modW, (float) tabStripH);
    g.setColour (juce::Colour (0xff141416));
    g.fillRoundedRectangle (tabStrip, 6.0f);
    g.setColour (juce::Colour (0xff3f3f46));
    g.drawHorizontalLine (chassisY + tabStripH, (float) modX, (float) (modX + modW));

    // Outer border
    g.setColour (juce::Colour (0xff38383c));
    g.drawRoundedRectangle (chassis, 6.0f, 2.0f);
}

void EffectsScreen::resized()
{
    const int totalW = getWidth();
    const int totalH = getHeight();
    if (totalW <= 50 || totalH <= 50)
        return;

    // Enforce strict 2.4 : 6 (height : width) aspect ratio across all platforms (Mobile, Windows, macOS)
    const int availW = totalW - 16;
    const int availH = totalH - 12;

    int modW = availW;
    int totalBlockH = juce::roundToInt ((float) modW * (2.4f / 6.0f));
    if (totalBlockH > availH)
    {
        totalBlockH = availH;
        modW = juce::roundToInt ((float) totalBlockH * (6.0f / 2.4f));
    }

    const int modX = (totalW - modW) / 2;
    const int startY = (totalH - totalBlockH) / 2;

    const int headerH = juce::jlimit (26, 38, juce::roundToInt ((float) totalBlockH * 0.085f));
    const int gapH = juce::jlimit (4, 8, juce::roundToInt ((float) totalBlockH * 0.018f));
    const int chassisH = totalBlockH - headerH - gapH;
    const int tabStripH = juce::jlimit (24, 30, juce::roundToInt ((float) chassisH * 0.072f));

    const int rightClearance = (startY < 38 && (modX + modW > totalW - 44)) ? 36 : 0;

    // 1. Top Header Bar Buttons
    backButton.setBounds (modX + 8, startY + 5, 70, headerH - 10);
    libraryHeaderButton.setBounds (modX + modW - 205 - rightClearance, startY + 5, 195, headerH - 10);

    // 2. FX1..FX8 Tabs inside top of rack chassis
    const int chassisY = startY + headerH + gapH;
    const int tabY = chassisY + 3;
    const int tabH = tabStripH - 4;
    const int tabW = juce::jmin (54, (modW - 24) / 8 - 2);
    int tabX = modX + 8;

    for (int i = 0; i < 8; ++i)
    {
        slotTabs[(size_t) i].setBounds (tabX, tabY, tabW, tabH);
        tabX += tabW + 2;
    }

    // 3. Faceplate Module Area
    faceplate.setBounds (modX + 2, chassisY + tabStripH + 1, modW - 4, chassisH - tabStripH - 3);
}

} // namespace mps
