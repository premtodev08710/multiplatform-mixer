#include "ScenesScreen.h"
#include "ConsoleLookAndFeel.h"

namespace mps
{

ScenesScreen::ScenesScreen (AudioEngine& eng, ProjectModel& proj)
    : engine (eng), project (proj)
{
    backButton.addListener (this);
    backButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff202630));
    backButton.setColour (juce::TextButton::textColourOffId, ConsoleLookAndFeel::accentCyan);
    addAndMakeVisible (backButton);

    saveProjectButton.addListener (this);
    addAndMakeVisible (saveProjectButton);

    loadProjectButton.addListener (this);
    addAndMakeVisible (loadProjectButton);

    copyAudioToggle.setToggleState (true, juce::dontSendNotification);
    addAndMakeVisible (copyAudioToggle);

    sceneNameEditor.setText ("Scene 1 - Rough Mix");
    addAndMakeVisible (sceneNameEditor);

    captureSceneButton.addListener (this);
    addAndMakeVisible (captureSceneButton);

    recallSceneButton.addListener (this);
    addAndMakeVisible (recallSceneButton);

    importScnButton.addListener (this);
    importScnButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff00838f));
    addAndMakeVisible (importScnButton);

    sceneList.setModel (this);
    addAndMakeVisible (sceneList);

    // Initial default scenes
    scenes.push_back ({ "01: Default Initial State", project.captureSceneSnapshot ("01: Default Initial State") });

    auto x32Scn = juce::File::getCurrentWorkingDirectory().getChildFile ("scene.scn");
    if (x32Scn.existsAsFile())
    {
        scenes.push_back ({ "02: X32 scene.scn (Thai Folk Band - Ponglang/Pin/Kan)", {} });
    }
    sceneList.updateContent();
}

int ScenesScreen::getNumRows()
{
    return (int) scenes.size();
}

void ScenesScreen::paintListBoxItem (int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll (ConsoleLookAndFeel::accentCyan.withAlpha (0.25f));
    else if (rowNumber % 2 == 1)
        g.fillAll (ConsoleLookAndFeel::bgSection.withAlpha (0.4f));

    g.setFont (juce::FontOptions (13.0f));
    g.setColour (ConsoleLookAndFeel::textPrimary);
    if (rowNumber >= 0 && rowNumber < (int) scenes.size())
        g.drawText (scenes[(size_t) rowNumber].name, 10, 0, width - 20, height, juce::Justification::centredLeft);
}

void ScenesScreen::selectedRowsChanged (int lastRowSelected)
{
    selectedSceneIndex = lastRowSelected;
}

void ScenesScreen::buttonClicked (juce::Button* b)
{
    if (b == &backButton)
    {
        if (onBack) onBack();
        return;
    }

    if (b == &captureSceneButton)
    {
        const auto name = sceneNameEditor.getText().trim();
        if (name.isNotEmpty())
        {
            scenes.push_back ({ name, project.captureSceneSnapshot (name) });
            sceneList.updateContent();
            sceneList.repaint();
        }
    }
    else if (b == &recallSceneButton)
    {
        if (selectedSceneIndex >= 0 && selectedSceneIndex < (int) scenes.size())
        {
            if (scenes[(size_t) selectedSceneIndex].data.isVoid())
            {
                auto x32Scn = juce::File::getCurrentWorkingDirectory().getChildFile ("scene.scn");
                if (x32Scn.existsAsFile())
                    project.loadX32SceneFile (x32Scn);
            }
            else
            {
                project.recallSceneSnapshot (scenes[(size_t) selectedSceneIndex].data);
            }
        }
    }
    else if (b == &importScnButton)
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Import Behringer X32 Scene",
            juce::File::getCurrentWorkingDirectory(),
            "*.scn;*.txt;*.*");

        const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser) {
            auto file = chooser.getResult();
            if (file.existsAsFile())
            {
                auto res = project.loadX32SceneFile (file);
                if (res.wasOk())
                {
                    scenes.push_back ({ "X32: " + file.getFileNameWithoutExtension(), project.captureSceneSnapshot (file.getFileNameWithoutExtension()) });
                    sceneList.updateContent();
                    sceneList.repaint();
                    juce::AlertWindow::showMessageBoxAsync (
                        juce::AlertWindow::InfoIcon, "X32 Scene Loaded",
                        "Successfully loaded X32 Scene:\n" + file.getFileName());
                }
                else
                {
                    juce::AlertWindow::showMessageBoxAsync (
                        juce::AlertWindow::WarningIcon, "Import Error",
                        "Failed to parse X32 Scene file:\n" + res.getErrorMessage());
                }
            }
        });
    }
    else if (b == &saveProjectButton)
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Save Mix Practice Project",
            juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
            "*.mpsproj;*.json");

        const auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting;
        fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser) {
            auto file = chooser.getResult();
            if (file != juce::File())
            {
                if (! file.hasFileExtension (".mpsproj"))
                    file = file.withFileExtension (".mpsproj");

                project.saveProject (file, copyAudioToggle.getToggleState());
            }
        });
    }
    else if (b == &loadProjectButton)
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Open Mix Practice Project",
            juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
            "*.mpsproj;*.json");

        const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser) {
            auto file = chooser.getResult();
            if (file.existsAsFile())
            {
                juce::StringArray missing;
                auto res = project.loadProject (file, missing);
                if (res.wasOk())
                {
                    if (! missing.isEmpty())
                    {
                        juce::AlertWindow::showMessageBoxAsync (
                            juce::AlertWindow::WarningIcon, "Missing Audio Files",
                            "Some audio files were not found:\n" + missing.joinIntoString ("\n"));
                    }
                }
                else
                {
                    juce::AlertWindow::showMessageBoxAsync (
                        juce::AlertWindow::WarningIcon, "Load Failed", res.getErrorMessage());
                }
            }
        });
    }
}

void ScenesScreen::paint (juce::Graphics& g)
{
    g.fillAll (ConsoleLookAndFeel::bgDark);

    auto bounds = getLocalBounds().reduced (12);
    auto leftBox = bounds.removeFromLeft (320);
    bounds.removeFromLeft (12);
    auto rightBox = bounds;

    // Left Panel (Project file save/load)
    g.setColour (ConsoleLookAndFeel::bgPanel);
    g.fillRoundedRectangle (leftBox.toFloat(), 4.0f);
    g.setColour (ConsoleLookAndFeel::borderSubtle);
    g.drawRoundedRectangle (leftBox.toFloat(), 4.0f, 1.0f);

    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.setColour (ConsoleLookAndFeel::textPrimary);
    g.drawText ("Project File Storage", leftBox.getX() + 16, leftBox.getY() + 14, 200, 20, juce::Justification::left);

    // Right Panel (Mixer Scenes)
    g.setColour (ConsoleLookAndFeel::bgPanel);
    g.fillRoundedRectangle (rightBox.toFloat(), 4.0f);
    g.setColour (ConsoleLookAndFeel::borderSubtle);
    g.drawRoundedRectangle (rightBox.toFloat(), 4.0f, 1.0f);

    g.drawText ("Console Scene Snapshots", rightBox.getX() + 16, rightBox.getY() + 14, 240, 20, juce::Justification::left);
}

void ScenesScreen::resized()
{
    auto bounds = getLocalBounds().reduced (12);

    if (getWidth() < 900)
    {
        // Stack project actions above the scene list on phones instead of
        // squeezing the list into a narrow side column.
        auto projectRow = bounds.removeFromTop (30);
        backButton.setBounds (projectRow.removeFromLeft (86));
        projectRow.removeFromLeft (6);
        const int actionW = (projectRow.getWidth() - 8) / 2;
        saveProjectButton.setBounds (projectRow.removeFromLeft (actionW));
        projectRow.removeFromLeft (8);
        loadProjectButton.setBounds (projectRow);
        bounds.removeFromTop (4);
        copyAudioToggle.setBounds (bounds.removeFromTop (24));
        bounds.removeFromTop (6);

        auto topRow = bounds.removeFromTop (30);
        const int gap = 4;
        const int editorW = std::min (160, topRow.getWidth() / 3);
        sceneNameEditor.setBounds (topRow.removeFromLeft (editorW));
        topRow.removeFromLeft (gap);
        const int buttonW = std::max (70, (topRow.getWidth() - 2 * gap) / 3);
        captureSceneButton.setBounds (topRow.removeFromLeft (buttonW));
        topRow.removeFromLeft (gap);
        recallSceneButton.setBounds (topRow.removeFromLeft (buttonW));
        topRow.removeFromLeft (gap);
        importScnButton.setBounds (topRow);
        bounds.removeFromTop (6);
        sceneList.setBounds (bounds);
        return;
    }

    // Left Box
    auto leftBox = bounds.removeFromLeft (320).reduced (16, 44);
    backButton.setBounds (leftBox.removeFromTop (30));
    leftBox.removeFromTop (8);
    saveProjectButton.setBounds (leftBox.removeFromTop (30));
    leftBox.removeFromTop (8);
    loadProjectButton.setBounds (leftBox.removeFromTop (30));
    leftBox.removeFromTop (10);
    copyAudioToggle.setBounds (leftBox.removeFromTop (26));

    bounds.removeFromLeft (12);

    // Right Box (Scenes)
    auto rightBox = bounds.reduced (16, 44);
    auto topRow = rightBox.removeFromTop (32);
    sceneNameEditor.setBounds (topRow.removeFromLeft (220));
    topRow.removeFromLeft (8);
    captureSceneButton.setBounds (topRow.removeFromLeft (110));
    topRow.removeFromLeft (8);
    recallSceneButton.setBounds (topRow.removeFromLeft (110));
    topRow.removeFromLeft (8);
    importScnButton.setBounds (topRow.removeFromLeft (170));

    rightBox.removeFromTop (12);
    sceneList.setBounds (rightBox);
}

} // namespace mps
