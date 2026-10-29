// Mix Practice Studio - Multitrack Management & File Import Screen
#pragma once

#include "../Audio/AudioEngine.h"
#include "../Model/ProjectModel.h"
#include "TimelineRangeComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace mps
{

class TracksScreen : public juce::Component,
                     public juce::Button::Listener,
                     public juce::TableListBoxModel,
                     public juce::DragAndDropContainer,
                     public juce::DragAndDropTarget,
                     public juce::FileDragAndDropTarget,
                     private juce::Timer
{
public:
    TracksScreen (AudioEngine& engine, ProjectModel& project);
    ~TracksScreen() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

    void buttonClicked (juce::Button* button) override;

    // TableListBoxModel implementation
    int getNumRows() override;
    void paintRowBackground (juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell (juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    juce::Component* refreshComponentForCell (int rowNumber, int columnId, bool isRowSelected, juce::Component* existingComponentToUpdate) override;
    juce::var getDragSourceDescription (const juce::SparseSet<int>& currentlySelectedRows) override;

    // Drag and Drop (Track Row Reordering)
    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override;
    void itemDragEnter (const SourceDetails& dragSourceDetails) override;
    void itemDragMove (const SourceDetails& dragSourceDetails) override;
    void itemDragExit (const SourceDetails& dragSourceDetails) override;
    void itemDropped (const SourceDetails& dragSourceDetails) override;
    void startRowDrag (int rowNumber, const juce::MouseEvent& e);

    // File Drag and Drop (External Files from Finder / Desktop)
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    void assignFilesToSlots (const juce::Array<juce::File>& files);
    void moveTrackRow (int fromSlot, int toSlot);
    void removeTrackSlot (int slot);

    std::function<void()> onBack;

private:
    void timerCallback() override;

    juce::TextButton backButton { "< Mixer" };
    void importAudioFiles();
    void importAudioFilesInternal();
    void importFolderStems();
    void importFolderStemsInternal();
   #if JUCE_IOS
    void importIOSAudio (bool selectFolder);
   #endif
    void processLoadedFilesInBackground (const juce::Array<juce::File>& rawFiles);

    AudioEngine& engine;
    ProjectModel& project;

    juce::TextButton importButton { "IMPORT AUDIO FILES (MULTI)" };
    juce::TextButton importFolderButton { "IMPORT STEMS FOLDER" };
    juce::TextButton clearButton { "CLEAR ALL TRACKS" };
    juce::Label dragHintLabel;

    // Dedicated Master Multitrack Timeline & Zoom Workspace
    TimelineRangeComponent masterTimeline;
    juce::TextButton trackPlayButton { "▶ PLAY" };
    juce::TextButton trackStopButton { "■ STOP" };
    juce::TextButton trackLoopButton { "🔁 LOOP" };
    juce::TextButton trackSetAButton { "[ A ]" };
    juce::TextButton trackSetBButton { "[ B ]" };
    juce::TextButton trackAllButton  { "ALL" };
    juce::Label trackTimeLabel;
    juce::Label trackLoopLabel;

    juce::TableListBox trackTable;
    int dragSourceRow = -1;
    int dragTargetRow = -1;

    std::unique_ptr<juce::FileChooser> fileChooser;
};

} // namespace mps
