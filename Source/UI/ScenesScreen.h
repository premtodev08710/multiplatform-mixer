// Mix Practice Studio - Scene Snapshots and Project Management Screen
#pragma once

#include "../Audio/AudioEngine.h"
#include "../Model/ProjectModel.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace mps
{

class ScenesScreen : public juce::Component,
                     public juce::Button::Listener,
                     public juce::ListBoxModel
{
public:
    ScenesScreen (AudioEngine& engine, ProjectModel& project);
    ~ScenesScreen() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void buttonClicked (juce::Button* button) override;

    // ListBoxModel
    int getNumRows() override;
    void paintListBoxItem (int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void selectedRowsChanged (int lastRowSelected) override;

private:
    AudioEngine& engine;
    ProjectModel& project;

    juce::TextButton saveProjectButton { "SAVE PROJECT" };
    juce::TextButton loadProjectButton { "LOAD PROJECT" };
    juce::ToggleButton copyAudioToggle { "Copy audio into project folder" };

    juce::TextButton captureSceneButton { "STORE SCENE" };
    juce::TextButton recallSceneButton { "RECALL SCENE" };
    juce::TextEditor sceneNameEditor;
    juce::ListBox sceneList;

    struct StoredScene { juce::String name; juce::var data; };
    std::vector<StoredScene> scenes;
    int selectedSceneIndex = -1;

    std::unique_ptr<juce::FileChooser> fileChooser;
};

} // namespace mps
