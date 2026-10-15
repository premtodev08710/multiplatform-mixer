// Mix Practice Studio - Multitrack Management & File Import Screen
#pragma once

#include "../Audio/AudioEngine.h"
#include "../Model/ProjectModel.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace mps
{

class TracksScreen : public juce::Component,
                     public juce::Button::Listener,
                     public juce::TableListBoxModel,
                     public juce::DragAndDropContainer,
                     public juce::DragAndDropTarget,
                     public juce::FileDragAndDropTarget
{
public:
    TracksScreen (AudioEngine& engine, ProjectModel& project);
    ~TracksScreen() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void buttonClicked (juce::Button* button) override;

    // TableListBoxModel implementation
    int getNumRows() override;
    void paintRowBackground (juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell (juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    juce::Component* refreshComponentForCell (int rowNumber, int columnId, bool isRowSelected, juce::Component* existingComponentToUpdate) override;

    // Drag and Drop (Track Row Reordering)
    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override;
    void itemDropped (const SourceDetails& dragSourceDetails) override;

    // File Drag and Drop (External Files from Finder / Desktop)
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    void assignFilesToSlots (const juce::Array<juce::File>& files);
    void moveTrackRow (int fromSlot, int toSlot);
    void removeTrackSlot (int slot);

    std::function<void()> onBack;

private:
    juce::TextButton backButton { "< Mixer" };
    void importAudioFiles();
    void importAudioFilesInternal();
    void importFolderStems();
    void importFolderStemsInternal();
    void processLoadedFilesInBackground (const juce::Array<juce::File>& rawFiles);

    AudioEngine& engine;
    ProjectModel& project;

    juce::TextButton importButton { "IMPORT AUDIO FILES (MULTI)" };
    juce::TextButton importFolderButton { "IMPORT STEMS FOLDER" };
    juce::TextButton clearButton { "CLEAR ALL TRACKS" };
    juce::Label dragHintLabel;

    juce::TableListBox trackTable;

    std::unique_ptr<juce::FileChooser> fileChooser;
};

} // namespace mps
