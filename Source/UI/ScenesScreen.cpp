#include "ScenesScreen.h"
#include "ConsoleLookAndFeel.h"
#include "../Core/AppEdition.h"

namespace mps
{

ScenesScreen::ScenesScreen (AudioEngine& eng, ProjectModel& proj)
    : engine (eng), project (proj)
{
    backButton.addListener (this);
    backButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff202630));
    backButton.setColour (juce::TextButton::textColourOffId, ConsoleLookAndFeel::accentCyan);
    addAndMakeVisible (backButton);

    newSceneButton.addListener (this);
    newSceneButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff059669)); // Emerald Green
    newSceneButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible (newSceneButton);

    saveProjectButton.setButtonText ("SAVE PROJECT FILE");
    saveProjectButton.addListener (this);
    addAndMakeVisible (saveProjectButton);

    loadProjectButton.addListener (this);
    addAndMakeVisible (loadProjectButton);

    copyAudioToggle.setToggleState (true, juce::dontSendNotification);
    addAndMakeVisible (copyAudioToggle);

    sceneNameEditor.setText ("Scene 1 - Rough Mix");
    addAndMakeVisible (sceneNameEditor);

    captureSceneButton.setButtonText ("SAVE SCENE (บันทึกลงเครื่อง)");
    captureSceneButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff0284c7)); // Sky Blue
    captureSceneButton.addListener (this);
    addAndMakeVisible (captureSceneButton);

    recallSceneButton.setButtonText ("RECALL SCENE (เปิดซีนเดิม)");
    recallSceneButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xffd97706)); // Amber
    recallSceneButton.addListener (this);
    addAndMakeVisible (recallSceneButton);

    deleteSceneButton.setButtonText ("DELETE (ลบ)");
    deleteSceneButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff991b1b)); // Crimson
    deleteSceneButton.addListener (this);
    addAndMakeVisible (deleteSceneButton);

    importScnButton.addListener (this);
    importScnButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff00838f));
    addAndMakeVisible (importScnButton);

    sceneList.setModel (this);
    addAndMakeVisible (sceneList);

    loadPersistedScenes();
}

void ScenesScreen::persistScenes()
{
    juce::Array<juce::var> arr;
    for (const auto& s : scenes)
    {
        auto obj = new juce::DynamicObject();
        obj->setProperty ("name", s.name);
        obj->setProperty ("data", s.data);
        arr.add (juce::var (obj));
    }
    project.saveStoredScenes (juce::var (arr));
}

void ScenesScreen::loadPersistedScenes()
{
    scenes.clear();
    auto jsonVar = project.loadStoredScenes();
    if (jsonVar.isArray())
    {
        if (auto* arr = jsonVar.getArray())
        {
            for (const auto& v : *arr)
            {
                if (auto* obj = v.getDynamicObject())
                {
                    const auto name = obj->getProperty ("name").toString();
                    auto data = obj->getProperty ("data");
                    if (name.isNotEmpty())
                        scenes.push_back ({ name, data });
                }
            }
        }
    }

    if (scenes.empty())
    {
        scenes.push_back ({ "01: Default Initial State", project.captureSceneSnapshot ("01: Default Initial State") });
        auto x32Scn = juce::File::getCurrentWorkingDirectory().getChildFile ("scene.scn");
        if (x32Scn.existsAsFile())
            scenes.push_back ({ "02: X32 scene.scn (Thai Folk Band - Ponglang/Pin/Kan)", {} });
        persistScenes();
    }
    sceneList.updateContent();
    sceneList.repaint();
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

    if (b == &newSceneButton)
    {
        juce::AlertWindow::showOkCancelBox (
            juce::AlertWindow::QuestionIcon,
            juce::CharPointer_UTF8 ("\xe0\xb8\xaa\xe0\xb8\xa3\xe0\xb9\x89\xe0\xb8\xb2\xe0\xb8\x87\xe0\xb8\x8b\xe0\xb8\xb5\xe0\xb8\x99\xe0\xb9\x83\xe0\xb8\xab\xe0\xb8\xa1\xe0\xb9\x88 (Create New Scene / Project)"),
            juce::CharPointer_UTF8 (
                "\xe0\xb8\x95\xe0\xb9\x89\xe0\xb8\xad\xe0\xb8\x87\xe0\xb8\x81\xe0\xb8\xb2\xe0\xb8\xa3\xe0\xb8\xaa\xe0\xb8\xa3\xe0\xb9\x89\xe0\xb8\xb2\xe0\xb8\x87\xe0\xb8\x8b\xe0\xb8\xb5\xe0\xb8\x99\xe0\xb9\x83\xe0\xb8\xab\xe0\xb8\xa1\xe0\xb9\x88\xe0\xb9\x83\xe0\xb8\x8a\xe0\xb9\x88\xe0\xb8\xab\xe0\xb8\xa3\xe0\xb8\xb7\xe0\xb8\xad\xe0\xb9\x84\xe0\xb8\xa1\xe0\xb9\x88?\n\n"
                "\xe2\x80\xa2 \xe0\xb8\xa3\xe0\xb8\xb0\xe0\xb8\x9a\xe0\xb8\x9a\xe0\xb8\x88\xe0\xb8\xb0\xe0\xb8\xa3\xe0\xb8\xb5\xe0\xb9\x80\xe0\xb8\x8b\xe0\xb9\x87\xe0\xb8\x95\xe0\xb8\x81\xe0\xb8\xb2\xe0\xb8\xa3\xe0\xb8\x95\xe0\xb8\xb1\xe0\xb9\x89\xe0\xb8\x87\xe0\xb8\x84\xe0\xb9\x88\xe0\xb8\xb2\xe0\xb8\xa1\xe0\xb8\xb4\xe0\xb8\x81\xe0\xb9\x80\xe0\xb8\x8b\xe0\xb8\xad\xe0\xb8\xa3\xe0\xb9\x8c\xe0\xb9\x83\xe0\xb8\xab\xe0\xb8\xa1\xe0\xb9\x88\xe0\xb8\x97\xe0\xb8\xb1\xe0\xb9\x89\xe0\xb8\x87\xe0\xb8\xab\xe0\xb8\xa1\xe0\xb8\x94\n"
                "\xe2\x80\xa2 \xe0\xb9\x80\xe0\xb8\x84\xe0\xb8\xa5\xe0\xb8\xb5\xe0\xb8\xa2\xe0\xb8\xa3\xe0\xb9\x8c\xe0\xb9\x84\xe0\xb8\x9f\xe0\xb8\xa5\xe0\xb9\x8c\xe0\xb8\xa1\xe0\xb8\xb1\xe0\xb8\xa5\xe0\xb8\x95\xe0\xb8\xb4\xe0\xb9\x81\xe0\xb8\x97\xe0\xb8\xa3\xe0\xb8\x81\xe0\xb9\x80\xe0\xb8\x94\xe0\xb8\xb4\xe0\xb8\xa1\xe0\xb8\xad\xe0\xb8\xad\xe0\xb8\x81 \xe0\xb9\x80\xe0\xb8\x9e\xe0\xb8\xb7\xe0\xb9\x88\xe0\xb8\xad\xe0\xb9\x83\xe0\xb8\xab\xe0\xb9\x89\xe0\xb8\x97\xe0\xb9\x88\xe0\xb8\xb2\xe0\xb8\x99\xe0\xb9\x80\xe0\xb8\x9e\xe0\xb8\xb4\xe0\xb9\x88\xe0\xb8\xa1\xe0\xb9\x80\xe0\xb8\x9e\xe0\xb8\xa5\xe0\xb8\x87\xe0\xb9\x83\xe0\xb8\xab\xe0\xb8\xa1\xe0\xb9\x88\xe0\xb9\x84\xe0\xb8\x94\xe0\xb9\x89\xe0\xb8\x97\xe0\xb8\xb1\xe0\xb8\x99\xe0\xb8\x97\xe0\xb8\xb5\n\n"
                "(All mixer faders, EQs, FX and multitracks will be reset to start a fresh project)"
            ),
            juce::CharPointer_UTF8 ("\xe0\xb8\xa2\xe0\xb8\xb7\xe0\xb8\x99\xe0\xb8\xa2\xe0\xb8\xb1\xe0\xb8\x99\xe0\xb8\xaa\xe0\xb8\xa3\xe0\xb9\x89\xe0\xb8\xb2\xe0\xb8\x87\xe0\xb8\x8b\xe0\xb8\xb5\xe0\xb8\x99\xe0\xb9\x83\xe0\xb8\xab\xe0\xb8\xa1\xe0\xb9\x88"),
            juce::CharPointer_UTF8 ("\xe0\xb8\xa2\xe0\xb8\x81\xe0\xb9\x80\xe0\xb8\xa5\xe0\xb8\xb4\xe0\xb8\x81"),
            nullptr,
            juce::ModalCallbackFunction::create ([this] (int res) {
                if (res != 0)
                {
                    project.createNewScene();
                    juce::AlertWindow::showMessageBoxAsync (
                        juce::AlertWindow::InfoIcon,
                        juce::CharPointer_UTF8 ("\xe0\xb8\xaa\xe0\xb8\xa3\xe0\xb9\x89\xe0\xb8\xb2\xe0\xb8\x87\xe0\xb8\x8b\xe0\xb8\xb5\xe0\xb8\x99\xe0\xb9\x83\xe0\xb8\xab\xe0\xb8\xa1\xe0\xb9\x88\xe0\xb9\x80\xe0\xb8\xa3\xe0\xb8\xb5\xe0\xb8\xa2\xe0\xb8\x9a\xe0\xb8\xa3\xe0\xb9\x89\xe0\xb8\xad\xe0\xb8\xa2"),
                        juce::CharPointer_UTF8 ("\xe0\xb8\xa3\xe0\xb8\xb5\xe0\xb9\x80\xe0\xb8\x8b\xe0\xb9\x87\xe0\xb8\x95\xe0\xb8\xa1\xe0\xb8\xb4\xe0\xb8\x81\xe0\xb9\x80\xe0\xb8\x8b\xe0\xb8\xad\xe0\xb8\xa3\xe0\xb9\x8c\xe0\xb9\x80\xe0\xb8\xa3\xe0\xb8\xb5\xe0\xb8\xa2\xe0\xb8\x9a\xe0\xb8\xa3\xe0\xb9\x89\xe0\xb8\xad\xe0\xb8\xa2\xe0\xb9\x81\xe0\xb8\xa5\xe0\xb9\x89\xe0\xb8\xa7 \xe0\xb8\xaa\xe0\xb8\xb2\xe0\xb8\xa1\xe0\xb8\xb2\xe0\xb8\xa3\xe0\xb8\x96\xe0\xb9\x80\xe0\xb8\x9e\xe0\xb8\xb4\xe0\xb9\x88\xe0\xb8\xa1\xe0\xb9\x84\xe0\xb8\x9f\xe0\xb8\xa5\xe0\xb9\x8c\xe0\xb8\xa1\xe0\xb8\xb1\xe0\xb8\xa5\xe0\xb8\x95\xe0\xb8\xb4\xe0\xb9\x81\xe0\xb8\x97\xe0\xb8\xa3\xe0\xb8\x81\xe0\xb9\x80\xe0\xb8\x9e\xe0\xb8\xa5\xe0\xb8\x87\xe0\xb9\x83\xe0\xb8\xab\xe0\xb8\xa1\xe0\xb9\x88\xe0\xb9\x84\xe0\xb8\x94\xe0\xb9\x89\xe0\xb8\x97\xe0\xb8\xb1\xe0\xb8\x99\xe0\xb8\x97\xe0\xb8\xb5\xe0\xb8\x84\xe0\xb8\xa3\xe0\xb8\xb1\xe0\xb8\x9a")
                    );
                }
            })
        );
        return;
    }

    if (b == &captureSceneButton)
    {
        const auto name = sceneNameEditor.getText().trim();
        if (name.isNotEmpty())
        {
            scenes.push_back ({ name, project.captureSceneSnapshot (name) });
            persistScenes();
            sceneList.updateContent();
            sceneList.repaint();
            juce::AlertWindow::showMessageBoxAsync (
                juce::AlertWindow::InfoIcon,
                juce::String::fromUTF8 ("บันทึกซีนลงเครื่องเรียบร้อย"),
                juce::String::fromUTF8 ("บันทึกซีน: \"") + name + juce::String::fromUTF8 ("\"\nพร้อมการตั้งค่ามิกเซอร์, EQ, FX และไฟล์เพลงมัลติแทรกเรียบร้อยแล้ว")
            );
        }
    }
    else if (b == &recallSceneButton)
    {
        if (selectedSceneIndex >= 0 && selectedSceneIndex < (int) scenes.size())
        {
            const auto& item = scenes[(size_t) selectedSceneIndex];
            if (item.data.isVoid())
            {
                auto x32Scn = juce::File::getCurrentWorkingDirectory().getChildFile ("scene.scn");
                if (x32Scn.existsAsFile())
                    project.loadX32SceneFile (x32Scn);
            }
            else
            {
                project.recallSceneSnapshot (item.data);
                juce::AlertWindow::showMessageBoxAsync (
                    juce::AlertWindow::InfoIcon,
                    juce::String::fromUTF8 ("เปิดซีนเรียบร้อย"),
                    juce::String::fromUTF8 ("โหลดซีน: \"") + item.name + juce::String::fromUTF8 ("\"\nพร้อมดึงการตั้งค่าและไฟล์มัลติแทรกเดิมกลับมาเรียบร้อย")
                );
            }
        }
        else
        {
            juce::AlertWindow::showMessageBoxAsync (
                juce::AlertWindow::WarningIcon,
                "No Scene Selected",
                "Please select a scene from the list to recall."
            );
        }
    }
    else if (b == &deleteSceneButton)
    {
        if (selectedSceneIndex >= 0 && selectedSceneIndex < (int) scenes.size())
        {
            scenes.erase (scenes.begin() + selectedSceneIndex);
            selectedSceneIndex = -1;
            persistScenes();
            sceneList.updateContent();
            sceneList.repaint();
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
                    persistScenes();
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
        // Mobile layout: Stack actions neatly
        auto topActions = bounds.removeFromTop (32);
        backButton.setBounds (topActions.removeFromLeft (76));
        topActions.removeFromLeft (6);
        newSceneButton.setBounds (topActions);

        bounds.removeFromTop (6);
        auto projectRow = bounds.removeFromTop (30);
        const int actionW = (projectRow.getWidth() - 8) / 2;
        saveProjectButton.setBounds (projectRow.removeFromLeft (actionW));
        projectRow.removeFromLeft (8);
        loadProjectButton.setBounds (projectRow);
        bounds.removeFromTop (4);
        copyAudioToggle.setBounds (bounds.removeFromTop (22));
        bounds.removeFromTop (6);

        auto sceneEditRow = bounds.removeFromTop (30);
        sceneNameEditor.setBounds (sceneEditRow.removeFromLeft (std::min (200, sceneEditRow.getWidth() / 2)));
        sceneEditRow.removeFromLeft (6);
        captureSceneButton.setBounds (sceneEditRow);
        bounds.removeFromTop (4);

        auto sceneBtnRow = bounds.removeFromTop (30);
        const int sBtnW = (sceneBtnRow.getWidth() - 8) / 3;
        recallSceneButton.setBounds (sceneBtnRow.removeFromLeft (sBtnW));
        sceneBtnRow.removeFromLeft (4);
        deleteSceneButton.setBounds (sceneBtnRow.removeFromLeft (sBtnW));
        sceneBtnRow.removeFromLeft (4);
        importScnButton.setBounds (sceneBtnRow);

        bounds.removeFromTop (6);
        sceneList.setBounds (bounds);
        return;
    }

    // Left Box (Project file actions)
    auto leftBox = bounds.removeFromLeft (320).reduced (16, 44);
    backButton.setBounds (leftBox.removeFromTop (30));
    leftBox.removeFromTop (12);
    newSceneButton.setBounds (leftBox.removeFromTop (34));
    leftBox.removeFromTop (12);
    saveProjectButton.setBounds (leftBox.removeFromTop (30));
    leftBox.removeFromTop (8);
    loadProjectButton.setBounds (leftBox.removeFromTop (30));
    leftBox.removeFromTop (10);
    copyAudioToggle.setBounds (leftBox.removeFromTop (26));

    bounds.removeFromLeft (12);

    // Right Box (Scenes & Snapshots)
    auto rightBox = bounds.reduced (16, 44);
    auto topRow = rightBox.removeFromTop (32);
    sceneNameEditor.setBounds (topRow.removeFromLeft (200));
    topRow.removeFromLeft (8);
    captureSceneButton.setBounds (topRow.removeFromLeft (130));
    topRow.removeFromLeft (8);
    recallSceneButton.setBounds (topRow.removeFromLeft (130));
    topRow.removeFromLeft (8);
    deleteSceneButton.setBounds (topRow.removeFromLeft (80));
    topRow.removeFromLeft (8);
    importScnButton.setBounds (topRow.removeFromLeft (150));

    rightBox.removeFromTop (12);
    sceneList.setBounds (rightBox);
}

} // namespace mps
