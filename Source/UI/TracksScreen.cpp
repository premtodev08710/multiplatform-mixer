// Mix Practice Studio - Multitrack Management & File Import Screen
#include "TracksScreen.h"
#include "ConsoleLookAndFeel.h"
#include <algorithm>

namespace mps
{

// Custom component for Column 1: Order / Move handle & Up/Down buttons
class TrackMoveCellComponent : public juce::Component
{
public:
    TrackMoveCellComponent (TracksScreen& owner, int row)
        : screen (owner), rowNumber (row)
    {
        upButton.setTooltip ("Move track up");
        upButton.onClick = [this] {
            if (rowNumber > 0)
                screen.moveTrackRow (rowNumber, rowNumber - 1);
        };
        addAndMakeVisible (upButton);

        downButton.setTooltip ("Move track down");
        downButton.onClick = [this] {
            if (rowNumber < kMaxTracks - 1)
                screen.moveTrackRow (rowNumber, rowNumber + 1);
        };
        addAndMakeVisible (downButton);

        dragLabel.setText (juce::CharPointer_UTF8 ("\xe2\xa0\xbf"), juce::dontSendNotification); // Braille ⠿
        dragLabel.setFont (juce::FontOptions (14.0f));
        dragLabel.setJustificationType (juce::Justification::centred);
        dragLabel.setColour (juce::Label::textColourId, ConsoleLookAndFeel::accentCyan);
        dragLabel.setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        dragLabel.setTooltip ("Drag row to reorder track position");
        addAndMakeVisible (dragLabel);
    }

    void setRow (int r)
    {
        rowNumber = r;
        upButton.setEnabled (rowNumber > 0);
        downButton.setEnabled (rowNumber < kMaxTracks - 1);
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        screen.startDragging ("track_row:" + juce::String (rowNumber), &screen);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (1);
        upButton.setBounds (area.removeFromLeft (18).reduced (1));
        dragLabel.setBounds (area.removeFromLeft (20));
        downButton.setBounds (area.removeFromLeft (18).reduced (1));
    }

private:
    TracksScreen& screen;
    int rowNumber = 0;
    juce::TextButton upButton { juce::CharPointer_UTF8 ("\xe2\x96\xb2") };   // ▲
    juce::TextButton downButton { juce::CharPointer_UTF8 ("\xe2\x96\xbc") }; // ▼
    juce::Label dragLabel;
};

// Custom component for Column 8: Delete / Clear button
class TrackDeleteCellComponent : public juce::Component
{
public:
    TrackDeleteCellComponent (TracksScreen& owner, int row)
        : screen (owner), rowNumber (row)
    {
        delButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff3a1f22));
        delButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffff5252));
        delButton.setTooltip ("Clear this track slot");
        delButton.onClick = [this] {
            screen.removeTrackSlot (rowNumber);
        };
        addAndMakeVisible (delButton);
    }

    void setRow (int r) { rowNumber = r; }

    void resized() override
    {
        delButton.setBounds (getLocalBounds().reduced (3, 2));
    }

private:
    TracksScreen& screen;
    int rowNumber = 0;
    juce::TextButton delButton { juce::CharPointer_UTF8 ("\xe2\x9c\x95") }; // ✕
};

//==============================================================================
TracksScreen::TracksScreen (AudioEngine& eng, ProjectModel& proj)
    : engine (eng), project (proj)
{
    importButton.addListener (this);
    importButton.setTooltip ("Select multiple audio files (WAV, AIFF, FLAC, MP3)");
    addAndMakeVisible (importButton);

    importFolderButton.addListener (this);
    importFolderButton.setTooltip ("Import all stem audio files from a folder");
    addAndMakeVisible (importFolderButton);

    clearButton.addListener (this);
    clearButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff4a1a1c));
    addAndMakeVisible (clearButton);

    dragHintLabel.setText ("Tip: Drag & Drop audio files here or drag rows to reorder tracks", juce::dontSendNotification);
    dragHintLabel.setFont (juce::FontOptions (11.0f));
    dragHintLabel.setColour (juce::Label::textColourId, ConsoleLookAndFeel::textMuted);
    dragHintLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (dragHintLabel);

    // Setup table columns
    trackTable.setModel (this);
    trackTable.setRowHeight (26);
    trackTable.getHeader().addColumn ("Move", 1, 62, 55, 70);
    trackTable.getHeader().addColumn ("Slot", 2, 60, 40, 80);
    trackTable.getHeader().addColumn ("File Name", 3, 260, 150, 450);
    trackTable.getHeader().addColumn ("Channels", 4, 90, 70, 120);
    trackTable.getHeader().addColumn ("Duration", 5, 95, 75, 140);
    trackTable.getHeader().addColumn ("Sample Rate", 6, 95, 75, 140);
    trackTable.getHeader().addColumn ("Assigned To", 7, 110, 80, 180);
    trackTable.getHeader().addColumn ("Clear", 8, 48, 40, 60);
    addAndMakeVisible (trackTable);
}

int TracksScreen::getNumRows()
{
    return kMaxTracks;
}

void TracksScreen::paintRowBackground (juce::Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll (ConsoleLookAndFeel::bgSection.brighter (0.15f));
    else if (rowNumber % 2 == 1)
        g.fillAll (ConsoleLookAndFeel::bgSection.withAlpha (0.45f));
    else
        g.fillAll (ConsoleLookAndFeel::bgSection.withAlpha (0.2f));

    // Subtle bottom row separator
    g.setColour (juce::Colour (0xff1f242d));
    g.drawHorizontalLine (25, 0.0f, (float) getWidth());
}

void TracksScreen::paintCell (juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/)
{
    g.setFont (juce::FontOptions (12.0f));
    g.setColour (ConsoleLookAndFeel::textPrimary);

    const int slot = rowNumber;
    auto reader = engine.getTrackSlot (slot);
    auto file = project.getTrackFile (slot);

    juce::String text;

    if (columnId == 2) // Slot
    {
        text = "Track " + juce::String (slot + 1);
    }
    else if (columnId == 3) // File Name
    {
        text = file.existsAsFile() ? file.getFileName() : "(empty)";
        if (! file.existsAsFile()) g.setColour (ConsoleLookAndFeel::textMuted);
    }
    else if (columnId == 4) // Channels
    {
        if (reader != nullptr)
            text = (reader->getNumChannels() == 2) ? "Stereo (2ch)" : "Mono (1ch)";
        else
            text = "-";
    }
    else if (columnId == 5) // Duration
    {
        if (reader != nullptr && engine.getSampleRate() > 0.0)
        {
            const double secs = (double) reader->getLengthInFrames() / engine.getSampleRate();
            const int mins = (int) (secs / 60.0);
            text = juce::String::formatted ("%02d:%05.2f", mins, std::fmod (secs, 60.0));
        }
        else text = "-";
    }
    else if (columnId == 6) // Sample Rate
    {
        if (auto* aft = dynamic_cast<AudioFileTrack*> (reader.get()))
            text = juce::String (aft->getOriginalSampleRate() * 0.001, 1) + " kHz";
        else text = "-";
    }
    else if (columnId == 7) // Assigned To
    {
        juce::String assigned;
        auto& params = engine.getMixerParams();
        for (int c = 0; c < kMaxInputChannels; ++c)
        {
            if (params.channels[(size_t) c].source.getInt() == slot)
            {
                if (assigned.isNotEmpty()) assigned += ", ";
                assigned += "CH " + juce::String (c + 1);
            }
        }
        text = assigned.isNotEmpty() ? assigned : "Unassigned";
        if (assigned.isEmpty()) g.setColour (ConsoleLookAndFeel::textMuted);
    }

    if (columnId >= 2 && columnId <= 7)
    {
        g.drawText (text, 6, 0, width - 12, height, juce::Justification::centredLeft);
    }
}

juce::Component* TracksScreen::refreshComponentForCell (int rowNumber, int columnId, bool /*isRowSelected*/, juce::Component* existing)
{
    if (columnId == 1) // Move Handle / Up-Down
    {
        auto* comp = dynamic_cast<TrackMoveCellComponent*> (existing);
        if (comp == nullptr)
            comp = new TrackMoveCellComponent (*this, rowNumber);
        comp->setRow (rowNumber);
        return comp;
    }
    else if (columnId == 8) // Delete Button
    {
        auto* comp = dynamic_cast<TrackDeleteCellComponent*> (existing);
        if (comp == nullptr)
            comp = new TrackDeleteCellComponent (*this, rowNumber);
        comp->setRow (rowNumber);
        return comp;
    }

    return existing;
}

void TracksScreen::buttonClicked (juce::Button* b)
{
    if (b == &importButton)
    {
        importAudioFiles();
    }
    else if (b == &importFolderButton)
    {
        importFolderStems();
    }
    else if (b == &clearButton)
    {
        engine.clearAllTracks();
        for (int s = 0; s < kMaxTracks; ++s)
            project.setTrackFile (s, juce::File());

        if (project.onTracksChanged)
            project.onTracksChanged();

        trackTable.updateContent();
        trackTable.repaint();
    }
}

void TracksScreen::moveTrackRow (int fromSlot, int toSlot)
{
    if (fromSlot >= 0 && fromSlot < kMaxTracks && toSlot >= 0 && toSlot < kMaxTracks && fromSlot != toSlot)
    {
        project.moveTrack (fromSlot, toSlot);
        trackTable.updateContent();
        trackTable.repaint();
    }
}

void TracksScreen::removeTrackSlot (int slot)
{
    if (slot >= 0 && slot < kMaxTracks)
    {
        engine.setTrackSlot (slot, nullptr);
        project.setTrackFile (slot, juce::File());

        if (project.onTracksChanged)
            project.onTracksChanged();

        trackTable.updateContent();
        trackTable.repaint();
    }
}

//==============================================================================
// Drag & Drop Track Rows
bool TracksScreen::isInterestedInDragSource (const SourceDetails& details)
{
    return details.description.toString().startsWith ("track_row:");
}

void TracksScreen::itemDropped (const SourceDetails& details)
{
    const juce::String desc = details.description.toString();
    if (desc.startsWith ("track_row:"))
    {
        const int fromSlot = desc.substring (10).getIntValue();
        const auto tablePoint = trackTable.getLocalPoint (this, details.localPosition);
        const int toSlot = trackTable.getRowContainingPosition (tablePoint.x, tablePoint.y);

        if (toSlot >= 0 && toSlot < kMaxTracks && fromSlot != toSlot)
        {
            moveTrackRow (fromSlot, toSlot);
        }
    }
}

static juce::File getLastImportDir()
{
    static juce::File sLastDir;
    if (sLastDir.isDirectory())
        return sLastDir;
    auto music = juce::File::getSpecialLocation (juce::File::userMusicDirectory);
    if (music.isDirectory()) return music;
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
}

static void setLastImportDir (const juce::File& f)
{
    static juce::File sLastDir;
    sLastDir = f.isDirectory() ? f : f.getParentDirectory();
}

//==============================================================================
// File Drag & Drop (External files from desktop / Finder)
bool TracksScreen::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& path : files)
    {
        const auto ext = juce::File (path).getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".aif" || ext == ".aiff" || ext == ".flac" || ext == ".mp3" || ext == ".ogg")
            return true;
    }
    return false;
}

void TracksScreen::filesDropped (const juce::StringArray& files, int x, int y)
{
    juce::Array<juce::File> audioFiles;
    for (const auto& path : files)
    {
        const juce::File f (path);
        const auto ext = f.getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".aif" || ext == ".aiff" || ext == ".flac" || ext == ".mp3" || ext == ".ogg")
            audioFiles.add (f);
    }

    if (! audioFiles.isEmpty())
    {
        setLastImportDir (audioFiles[0]);
        assignFilesToSlots (audioFiles);
    }
}

//==============================================================================
// File Choosers
void TracksScreen::importAudioFiles()
{
    fileChooser = std::make_unique<juce::FileChooser> (
        "Select Multitrack Audio Files (WAV, AIFF, FLAC, MP3)",
        getLastImportDir(),
        "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");

    // Standard JUCE multi-file open flags: openMode | canSelectFiles | canSelectMultipleItems
    const auto flags = juce::FileBrowserComponent::openMode
                     | juce::FileBrowserComponent::canSelectFiles
                     | juce::FileBrowserComponent::canSelectMultipleItems;

    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser) {
        auto files = chooser.getResults();
        if (! files.isEmpty())
        {
            setLastImportDir (files[0]);
            assignFilesToSlots (files);
        }
    });
}

void TracksScreen::importFolderStems()
{
    fileChooser = std::make_unique<juce::FileChooser> (
        "Select Folder containing Multitrack Stems",
        getLastImportDir());

    const auto flags = juce::FileBrowserComponent::openMode
                     | juce::FileBrowserComponent::canSelectDirectories;

    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser) {
        const auto folder = chooser.getResult();
        if (folder.isDirectory())
        {
            setLastImportDir (folder);
            auto files = folder.findChildFiles (juce::File::findFiles, false, "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
            if (! files.isEmpty())
                assignFilesToSlots (files);
        }
    });
}

void TracksScreen::assignFilesToSlots (const juce::Array<juce::File>& rawFiles)
{
    // Natural alphanumeric sorting (e.g. 01_Kick, 02_Snare, ... 10_Vocal in order)
    juce::Array<juce::File> files = rawFiles;
    files.sort();

    const double targetSr = engine.getSampleRate();

    // Find first empty track slot
    int startSlot = 0;
    for (int s = 0; s < kMaxTracks; ++s)
    {
        if (engine.getTrackSlot (s) == nullptr)
        {
            startSlot = s;
            break;
        }
    }

    for (int i = 0; i < files.size() && (startSlot + i) < kMaxTracks; ++i)
    {
        const int slot = startSlot + i;
        const auto& file = files[i];

        auto track = AudioFileTrack::open (file, targetSr, engine.getFormatManager());
        if (track != nullptr)
        {
            engine.setTrackSlot (slot, track);
            project.setTrackFile (slot, file);

            // Auto-assign to corresponding channel strip if unassigned
            auto& cp = engine.getMixerParams().channels[(size_t) slot];
            if (cp.source.getInt() < 0)
                cp.source.set ((float) slot);

            // Name channel strip from file name if default or CH XX
            auto meta = project.getChannelMetadata (slot);
            if (meta.name.startsWith ("CH ") || meta.name.isEmpty())
                project.setChannelName (slot, file.getFileNameWithoutExtension().substring (0, 8));
        }
    }

    if (project.onTracksChanged)
        project.onTracksChanged();

    trackTable.updateContent();
    trackTable.repaint();
}

void TracksScreen::paint (juce::Graphics& g)
{
    g.fillAll (ConsoleLookAndFeel::bgDark);

    auto bounds = getLocalBounds().reduced (12);
    bounds.removeFromTop (44); // Button area

    g.setColour (ConsoleLookAndFeel::bgPanel);
    g.fillRoundedRectangle (bounds.toFloat(), 4.0f);
    g.setColour (ConsoleLookAndFeel::borderSubtle);
    g.drawRoundedRectangle (bounds.toFloat(), 4.0f, 1.0f);
}

void TracksScreen::resized()
{
    auto area = getLocalBounds().reduced (12);

    auto topRow = area.removeFromTop (36);
    importButton.setBounds (topRow.removeFromLeft (210));
    topRow.removeFromLeft (8);
    importFolderButton.setBounds (topRow.removeFromLeft (170));
    topRow.removeFromLeft (8);
    clearButton.setBounds (topRow.removeFromLeft (140));

    dragHintLabel.setBounds (topRow);

    area.removeFromTop (8);
    trackTable.setBounds (area);
}

} // namespace mps
