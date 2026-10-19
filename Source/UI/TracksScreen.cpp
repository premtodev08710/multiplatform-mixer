// Mix Practice Studio - Multitrack Management & File Import Screen
#include "TracksScreen.h"
#include "ConsoleLookAndFeel.h"
#include <algorithm>

#if JUCE_ANDROID
#include <jni.h>
#endif

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
    backButton.addListener (this);
    backButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff202630));
    backButton.setColour (juce::TextButton::textColourOffId, ConsoleLookAndFeel::accentCyan);
    addAndMakeVisible (backButton);

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
    if (b == &backButton)
    {
        if (onBack) onBack();
        return;
    }

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

        project.setModified (true);
        project.autoSaveSession();

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

static juce::String sanitizeFileName (const juce::String& name)
{
    return name.replaceCharacters ("/\\:*?\"<>|", "________").trim();
}

static bool isSupportedAudioExtension (const juce::String& fileName)
{
    const auto lower = fileName.toLowerCase();
    return lower.endsWith (".wav")
        || lower.endsWith (".aif")
        || lower.endsWith (".aiff")
        || lower.endsWith (".flac")
        || lower.endsWith (".mp3")
        || lower.endsWith (".ogg");
}

static juce::String ensureAudioExtension (const juce::String& name, const juce::String& mimeType)
{
    if (isSupportedAudioExtension (name))
        return name;

    const auto lowerMime = mimeType.toLowerCase();
    if (lowerMime.contains ("wav"))                                return name + ".wav";
    if (lowerMime.contains ("mp3") || lowerMime.contains ("mpeg")) return name + ".mp3";
    if (lowerMime.contains ("flac"))                               return name + ".flac";
    if (lowerMime.contains ("ogg"))                                return name + ".ogg";
    if (lowerMime.contains ("aiff") || lowerMime.contains ("aif")) return name + ".aiff";

    return name;
}

static bool isSupportedAudioFile (const juce::File& f)
{
    return isSupportedAudioExtension (f.getFileName());
}

static void collectAudioFilesRecursively (const juce::File& f, juce::Array<juce::File>& result)
{
    if (f.isDirectory())
    {
        auto children = f.findChildFiles (juce::File::findFiles, true, "*");
        for (const auto& child : children)
        {
            if (isSupportedAudioFile (child))
                result.add (child);
        }
    }
    else if (isSupportedAudioFile (f))
    {
        result.add (f);
    }
}

//==============================================================================
// File Drag & Drop (External files / folders from desktop / Finder)
bool TracksScreen::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& path : files)
    {
        const juce::File f (path);
        if (f.isDirectory() || isSupportedAudioFile (f))
            return true;
    }
    return false;
}

void TracksScreen::filesDropped (const juce::StringArray& files, int /*x*/, int /*y*/)
{
    juce::Array<juce::File> audioFiles;
    for (const auto& path : files)
        collectAudioFilesRecursively (juce::File (path), audioFiles);

    if (! audioFiles.isEmpty())
    {
        setLastImportDir (audioFiles[0]);
        assignFilesToSlots (audioFiles);
    }
}

//==============================================================================
#if JUCE_ANDROID
static jclass g_folderImporterClass = nullptr;

static void ensureFolderImporterClassLoaded()
{
    if (g_folderImporterClass == nullptr)
    {
        auto* env = juce::getEnv();
        if (env != nullptr)
        {
            jclass localCls = env->FindClass ("com/mixpractice/studio/FolderImporter");
            if (localCls != nullptr)
            {
                g_folderImporterClass = (jclass) env->NewGlobalRef (localCls);
                env->DeleteLocalRef (localCls);
            }
            else
            {
                env->ExceptionClear();
            }
        }
    }
}

static juce::StringArray runAndroidFolderImporter (const juce::String& treeUriStr, const juce::File& destDir)
{
    juce::StringArray result;
    auto* env = juce::getEnv();
    if (env == nullptr || g_folderImporterClass == nullptr) return result;

    jmethodID mid = env->GetStaticMethodID (g_folderImporterClass, "importAudioFromTree",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)[Ljava/lang/String;");
    if (mid == nullptr)
    {
        env->ExceptionClear();
        return result;
    }

    auto jContext = juce::getAppContext();
    jstring jTreeUri = env->NewStringUTF (treeUriStr.toUTF8());
    jstring jDestDir = env->NewStringUTF (destDir.getFullPathName().toUTF8());

    auto jArray = (jobjectArray) env->CallStaticObjectMethod (g_folderImporterClass, mid, jContext.get(), jTreeUri, jDestDir);
    if (env->ExceptionCheck())
    {
        env->ExceptionClear();
    }
    else if (jArray != nullptr)
    {
        const int count = env->GetArrayLength (jArray);
        for (int i = 0; i < count; ++i)
        {
            auto jStr = (jstring) env->GetObjectArrayElement (jArray, i);
            if (jStr != nullptr)
            {
                const char* utf = env->GetStringUTFChars (jStr, nullptr);
                if (utf != nullptr)
                {
                    result.add (juce::String::fromUTF8 (utf));
                    env->ReleaseStringUTFChars (jStr, utf);
                }
                env->DeleteLocalRef (jStr);
            }
        }
        env->DeleteLocalRef (jArray);
    }

    env->DeleteLocalRef (jTreeUri);
    env->DeleteLocalRef (jDestDir);
    return result;
}

static juce::StringArray runAndroidDocumentImporter (const juce::StringArray& uriStrings, const juce::File& destDir)
{
    juce::StringArray result;
    auto* env = juce::getEnv();
    if (env == nullptr || g_folderImporterClass == nullptr || uriStrings.isEmpty()) return result;

    jmethodID mid = env->GetStaticMethodID (g_folderImporterClass, "importAudioDocuments",
        "(Landroid/content/Context;[Ljava/lang/String;Ljava/lang/String;)[Ljava/lang/String;");
    if (mid == nullptr)
    {
        env->ExceptionClear();
        return result;
    }

    auto jContext = juce::getAppContext();
    jclass stringCls = env->FindClass ("java/lang/String");
    jobjectArray jUris = env->NewObjectArray (uriStrings.size(), stringCls, nullptr);
    for (int i = 0; i < uriStrings.size(); ++i)
    {
        jstring s = env->NewStringUTF (uriStrings[i].toUTF8());
        env->SetObjectArrayElement (jUris, i, s);
        env->DeleteLocalRef (s);
    }
    jstring jDestDir = env->NewStringUTF (destDir.getFullPathName().toUTF8());

    auto jArray = (jobjectArray) env->CallStaticObjectMethod (g_folderImporterClass, mid, jContext.get(), jUris, jDestDir);
    if (env->ExceptionCheck())
    {
        env->ExceptionClear();
    }
    else if (jArray != nullptr)
    {
        const int count = env->GetArrayLength (jArray);
        for (int i = 0; i < count; ++i)
        {
            auto jStr = (jstring) env->GetObjectArrayElement (jArray, i);
            if (jStr != nullptr)
            {
                const char* utf = env->GetStringUTFChars (jStr, nullptr);
                if (utf != nullptr)
                {
                    result.add (juce::String::fromUTF8 (utf));
                    env->ReleaseStringUTFChars (jStr, utf);
                }
                env->DeleteLocalRef (jStr);
            }
        }
        env->DeleteLocalRef (jArray);
    }

    env->DeleteLocalRef (jDestDir);
    env->DeleteLocalRef (jUris);
    env->DeleteLocalRef (stringCls);
    return result;
}
#endif

//==============================================================================
// File Choosers
void TracksScreen::importAudioFiles()
{
   #if JUCE_ANDROID
    ensureFolderImporterClassLoaded();
    juce::RuntimePermissions::request (juce::RuntimePermissions::readMediaAudio, [this] (bool) {
        juce::RuntimePermissions::request (juce::RuntimePermissions::readExternalStorage, [this] (bool) {
            importAudioFilesInternal();
        });
    });
   #else
    importAudioFilesInternal();
   #endif
}

void TracksScreen::importAudioFilesInternal()
{
   #if JUCE_ANDROID
    ensureFolderImporterClassLoaded();
   #endif

    fileChooser = std::make_unique<juce::FileChooser> (
        "Select Multitrack Audio Files (WAV, AIFF, FLAC, MP3, OGG)",
        getLastImportDir(),
        "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");

    const auto flags = juce::FileBrowserComponent::openMode
                     | juce::FileBrowserComponent::canSelectFiles
                     | juce::FileBrowserComponent::canSelectMultipleItems;

    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser) {
       #if JUCE_ANDROID
        juce::StringArray uriStrings;
        for (const auto& u : chooser.getURLResults())
            uriStrings.add (u.toString (true));
        if (uriStrings.isEmpty() && ! chooser.getURLResult().isEmpty())
            uriStrings.add (chooser.getURLResult().toString (true));

        if (uriStrings.isEmpty() && chooser.getResults().isEmpty())
            return;

        dragHintLabel.setText ("Importing audio files in background...", juce::dontSendNotification);
        importButton.setEnabled (false);
        importFolderButton.setEnabled (false);

        auto importDirectory = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                                   .getChildFile ("Imported Audio");
        importDirectory.createDirectory();

        juce::Thread::launch ([this, uriStrings, importDirectory] {
            juce::Array<juce::File> filesToLoad;
            if (! uriStrings.isEmpty())
            {
                auto stagedPaths = runAndroidDocumentImporter (uriStrings, importDirectory);
                for (const auto& p : stagedPaths)
                    filesToLoad.add (juce::File (p));
            }
            processLoadedFilesInBackground (filesToLoad);
        });
       #else
        auto filesToLoad = chooser.getResults();
        if (! filesToLoad.isEmpty())
        {
            setLastImportDir (filesToLoad[0]);
            assignFilesToSlots (filesToLoad);
        }
       #endif
    });
}

void TracksScreen::importFolderStems()
{
   #if JUCE_ANDROID
    ensureFolderImporterClassLoaded();
    juce::RuntimePermissions::request (juce::RuntimePermissions::readMediaAudio, [this] (bool) {
        juce::RuntimePermissions::request (juce::RuntimePermissions::readExternalStorage, [this] (bool) {
            importFolderStemsInternal();
        });
    });
   #else
    importFolderStemsInternal();
   #endif
}

void TracksScreen::importFolderStemsInternal()
{
   #if JUCE_ANDROID
    ensureFolderImporterClassLoaded();
   #endif

    fileChooser = std::make_unique<juce::FileChooser> (
        "Select Folder containing Multitrack Stems",
        getLastImportDir());

    const auto flags = juce::FileBrowserComponent::openMode
                     | juce::FileBrowserComponent::canSelectDirectories;

    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser) {
       #if JUCE_ANDROID
        auto urlResults = chooser.getURLResults();
        auto treeUrl = urlResults.isEmpty() ? chooser.getURLResult() : urlResults[0];
        const auto treeUrlStr = treeUrl.toString (true);
        const auto posixPath = chooser.getResult().getFullPathName();

        if (treeUrlStr.isEmpty() && posixPath.isEmpty())
            return;

        dragHintLabel.setText ("Importing multitrack stems in background...", juce::dontSendNotification);
        importButton.setEnabled (false);
        importFolderButton.setEnabled (false);

        auto importDirectory = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                                   .getChildFile ("Imported Audio");
        importDirectory.createDirectory();

        juce::Thread::launch ([this, treeUrlStr, posixPath, importDirectory] {
            juce::Array<juce::File> filesToLoad;

            if (treeUrlStr.isNotEmpty())
            {
                auto stagedPaths = runAndroidFolderImporter (treeUrlStr, importDirectory);
                for (const auto& p : stagedPaths)
                    filesToLoad.add (juce::File (p));
            }

            if (filesToLoad.isEmpty() && posixPath.isNotEmpty())
            {
                juce::File folder (posixPath);
                if (folder.isDirectory())
                {
                    juce::Array<juce::File> found;
                    collectAudioFilesRecursively (folder, found);
                    for (const auto& f : found)
                    {
                        auto in = f.createInputStream();
                        if (in != nullptr)
                        {
                            auto destFile = importDirectory.getChildFile (f.getFileName());
                            destFile.deleteFile();
                            auto out = destFile.createOutputStream();
                            if (out != nullptr)
                            {
                                out->writeFromInputStream (*in, -1);
                                out->flush();
                                out.reset();
                                if (destFile.existsAsFile() && destFile.getSize() > 0)
                                    filesToLoad.add (destFile);
                            }
                        }
                    }
                }
            }

            processLoadedFilesInBackground (filesToLoad);
        });
       #else
        const auto folder = chooser.getResult();
        if (folder.isDirectory())
        {
            setLastImportDir (folder);
            juce::Array<juce::File> filesToLoad;
            collectAudioFilesRecursively (folder, filesToLoad);
            if (! filesToLoad.isEmpty())
                assignFilesToSlots (filesToLoad);
        }
       #endif
    });
}

void TracksScreen::assignFilesToSlots (const juce::Array<juce::File>& rawFiles)
{
    dragHintLabel.setText ("Loading tracks in background...", juce::dontSendNotification);
    importButton.setEnabled (false);
    importFolderButton.setEnabled (false);

    juce::Thread::launch ([this, rawFiles] {
        processLoadedFilesInBackground (rawFiles);
    });
}

void TracksScreen::processLoadedFilesInBackground (const juce::Array<juce::File>& rawFiles)
{
    juce::Array<juce::File> files = rawFiles;
    files.sort();

    const double targetSr = (engine.getSampleRate() > 1000.0) ? engine.getSampleRate() : 48000.0;

    int startSlot = -1;
    for (int s = 0; s < kMaxTracks; ++s)
    {
        if (engine.getTrackSlot (s) == nullptr)
        {
            startSlot = s;
            break;
        }
    }
    if (startSlot < 0) startSlot = 0;

    struct LoadedTrack
    {
        int slot;
        juce::File file;
        std::shared_ptr<AudioFileTrack> track;
    };
    std::vector<LoadedTrack> readyTracks;

    for (int i = 0; i < files.size() && (startSlot + i) < kMaxTracks; ++i)
    {
        const int slot = startSlot + i;
        const auto& file = files[i];

        auto track = AudioFileTrack::open (file, targetSr, engine.getFormatManager());
        if (track != nullptr)
        {
            readyTracks.push_back ({ slot, file, std::move (track) });
        }
    }

    juce::MessageManager::callAsync ([this, readyTracks = std::move (readyTracks), totalFiles = files.size(), startSlot] {
        importButton.setEnabled (true);
        importFolderButton.setEnabled (true);
        dragHintLabel.setText ("Tip: Drag & Drop audio files here or drag rows to reorder tracks", juce::dontSendNotification);

        for (const auto& item : readyTracks)
        {
            engine.setTrackSlot (item.slot, item.track);
            project.setTrackFile (item.slot, item.file);

            auto& cp = engine.getMixerParams().channels[(size_t) item.slot];
            if (cp.source.getInt() < 0)
                cp.source.set ((float) item.slot);

            auto meta = project.getChannelMetadata (item.slot);
            if (meta.name.startsWith ("CH ") || meta.name.isEmpty())
                project.setChannelName (item.slot, item.file.getFileNameWithoutExtension().substring (0, 8));
        }

        if (project.onTracksChanged)
            project.onTracksChanged();

        project.setModified (true);
        project.autoSaveSession();

        trackTable.updateContent();
        trackTable.repaint();

       #if JUCE_ANDROID
        if (! readyTracks.empty())
        {
            juce::AlertWindow::showMessageBoxAsync (
                juce::MessageBoxIconType::InfoIcon,
                "Tracks Imported",
                "Successfully imported " + juce::String ((int) readyTracks.size()) + " track(s) into mixer!\n"
                "Assigned to Channel " + juce::String (startSlot + 1) + " - " + juce::String (startSlot + (int) readyTracks.size()) + ".");
        }
        else if (totalFiles > 0)
        {
            juce::AlertWindow::showMessageBoxAsync (
                juce::MessageBoxIconType::WarningIcon,
                "Import Failed",
                "Could not decode audio files. Supported formats: WAV, AIFF, FLAC, MP3, OGG.");
        }
        else
        {
            juce::AlertWindow::showMessageBoxAsync (
                juce::MessageBoxIconType::InfoIcon,
                "Import Stems",
                "No audio files found in the selected folder.\n\n"
                "Tip: Please select a folder containing .wav, .mp3, or .flac stem files.");
        }
       #endif
    });
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
    if (getWidth() < 900)
    {
        auto firstRow = area.removeFromTop (32);
        backButton.setBounds (firstRow.removeFromLeft (86));
        firstRow.removeFromLeft (6);
        const int gap = 6;
        const int buttonW = (firstRow.getWidth() - gap) / 2;
        importButton.setBounds (firstRow.removeFromLeft (buttonW));
        firstRow.removeFromLeft (gap);
        importFolderButton.setBounds (firstRow);
        area.removeFromTop (4);
        auto secondRow = area.removeFromTop (30);
        clearButton.setBounds (secondRow.removeFromLeft (140));
        secondRow.removeFromLeft (6);
        dragHintLabel.setBounds (secondRow);
        area.removeFromTop (6);
    }
    else
    {
        auto topRow = area.removeFromTop (36);
        backButton.setBounds (topRow.removeFromLeft (96));
        topRow.removeFromLeft (8);
        importButton.setBounds (topRow.removeFromLeft (210));
        topRow.removeFromLeft (8);
        importFolderButton.setBounds (topRow.removeFromLeft (170));
        topRow.removeFromLeft (8);
        clearButton.setBounds (topRow.removeFromLeft (140));
        dragHintLabel.setBounds (topRow);
        area.removeFromTop (8);
    }
    trackTable.setBounds (area);
}

} // namespace mps
