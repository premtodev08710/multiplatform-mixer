#include "../Source/Audio/AppleAudioFileStaging.h"
#include <iostream>

static NSURL* urlFor (const juce::File& file)
{
    return [NSURL fileURLWithPath: [NSString stringWithUTF8String: file.getFullPathName().toRawUTF8()]];
}

int main()
{
    @autoreleasepool
    {
        const auto root = juce::File::getSpecialLocation (juce::File::tempDirectory)
            .getNonexistentChildFile ("mps-apple-import-tests", "", false);
        root.createDirectory();
        struct Cleanup { juce::File root; ~Cleanup() { root.deleteRecursively(); } } cleanup { root };
        int failures = 0;
        auto check = [&] (bool ok, const char* name) {
            std::cout << (ok ? "PASS " : "FAIL ") << name << std::endl;
            if (! ok) ++failures;
        };
        const auto source = root.getChildFile ("source");
        const auto output = root.getChildFile ("imported");
        source.createDirectory();
        auto first = source.getChildFile (juce::String::fromUTF8 ("เสียง กลอง.WAV"));
        const unsigned char bytes[] { 0, 1, 2, 3, 255 };
        first.replaceWithData (bytes, sizeof (bytes));

        auto imported = mps::stageAppleAudioDocuments (@[urlFor (first)], false, output);
        check (imported.error.isEmpty() && imported.files.size() == 1, "import audio with a Thai filename and uppercase extension");
        if (imported.files.size() == 1)
        {
            juce::MemoryBlock originalData, importedData;
            first.loadFileAsData (originalData);
            imported.files[0].loadFileAsData (importedData);
            check (originalData == importedData, "preserve every byte of audio");
            auto duplicate = mps::stageAppleAudioDocuments (@[urlFor (first)], false, output);
            check (duplicate.files.size() == 1 && duplicate.files[0] != imported.files[0], "duplicate names create independent files");
            check (imported.files[0].getSize() == sizeof (bytes), "existing imported audio is not overwritten");
            auto selfImport = mps::stageAppleAudioDocuments (@[urlFor (imported.files[0])], false, output);
            check (selfImport.files.size() == 1 && imported.files[0].getSize() == sizeof (bytes), "reimport from app storage preserves the source");
        }

        const auto nested = source.getChildFile ("nested");
        nested.createDirectory();
        nested.getChildFile ("bass.flac").replaceWithData (bytes, sizeof (bytes));
        source.getChildFile ("notes.txt").replaceWithText ("not audio");
        auto folder = mps::stageAppleAudioDocuments (@[urlFor (source)], true, output);
        check (folder.files.size() == 2, "recursively import audio and ignore unrelated documents");

        const auto insideOutput = source.getChildFile ("Imported Audio");
        insideOutput.createDirectory();
        insideOutput.getChildFile ("previous.wav").replaceWithData (bytes, sizeof (bytes));
        auto rootFolder = mps::stageAppleAudioDocuments (@[urlFor (source)], true, insideOutput);
        check (rootFolder.files.size() == 2, "skip the destination folder when it is inside the selected folder");

        const auto empty = source.getChildFile ("empty.wav");
        empty.replaceWithText ("");
        auto rejected = mps::stageAppleAudioDocuments (@[urlFor (empty)], false, output);
        check (rejected.files.isEmpty() && rejected.error.isNotEmpty(), "report empty files instead of silently failing");
        const auto blocked = root.getChildFile ("not-a-directory");
        blocked.replaceWithText ("blocked");
        auto unwritable = mps::stageAppleAudioDocuments (@[urlFor (first)], false, blocked.getChildFile ("audio"));
        check (unwritable.files.isEmpty() && unwritable.error.isNotEmpty(), "report destination creation errors");
        return failures == 0 ? 0 : 1;
    }
}
