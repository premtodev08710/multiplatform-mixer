#include "AppleAudioFileStaging.h"

namespace mps
{
namespace
{
juce::String text (NSString* value)
{
    return value != nil ? juce::String::fromUTF8 ([value UTF8String]) : juce::String();
}

bool isAudioFile (NSURL* url)
{
    return juce::StringArray { "wav", "aif", "aiff", "flac", "mp3", "ogg", "m4a", "aac", "caf" }
        .contains (text ([url pathExtension]).toLowerCase());
}

void copyAudioFile (NSURL* source, const juce::File& destinationDirectory, AppleAudioImportResult& result)
{
    if (! isAudioFile (source)) return;

    auto name = juce::File::createLegalFileName (text ([source lastPathComponent]));
    if (name.isEmpty()) name = "track.wav";
    const auto candidate = destinationDirectory.getChildFile (name);
    const auto destination = destinationDirectory.getNonexistentChildFile (
        candidate.getFileNameWithoutExtension(), candidate.getFileExtension());
    NSURL* target = [NSURL fileURLWithPath: [NSString stringWithUTF8String: destination.getFullPathName().toRawUTF8()]];

    // Coordinating the read also lets iCloud/Files providers download the content.
    NSFileCoordinator* coordinator = [[NSFileCoordinator alloc] initWithFilePresenter: nil];
    NSError* coordinationError = nil;
    __block NSError* copyError = nil;
    __block BOOL copied = NO;
    [coordinator coordinateReadingItemAtURL: source options: 0 error: &coordinationError
        byAccessor: ^(NSURL* readableURL) {
            copied = [[NSFileManager defaultManager] copyItemAtURL: readableURL toURL: target error: &copyError];
        }];
    [coordinator release];

    if (copied && destination.getSize() > 0)
        result.files.add (destination);
    else
    {
        destination.deleteFile();
        if (result.error.isEmpty())
            result.error = name + ": " + text ([(coordinationError ?: copyError) localizedDescription]);
    }
}

}

AppleAudioImportResult stageAppleAudioDocuments (NSArray<NSURL*>* urls, bool selectFolder,
                                                   const juce::File& directory)
{
    AppleAudioImportResult result;
    const auto created = directory.createDirectory();
    if (created.failed())
    {
        result.error = created.getErrorMessage();
        return result;
    }

    NSURL* destinationURL = [[NSURL fileURLWithPath: [NSString stringWithUTF8String: directory.getFullPathName().toRawUTF8()]] URLByResolvingSymlinksInPath];
    NSString* destPath = [destinationURL path];
    NSString* destPrefix = [destPath stringByAppendingString: @"/"];

    for (NSURL* url in urls)
    {
        // Keep the selected folder's security scope alive throughout enumeration
        // and copying; a plain juce::File loses the Files provider permission.
        const BOOL scoped = [url startAccessingSecurityScopedResource];
        if (selectFolder)
        {
            __block NSString* enumerationError = nil;
            NSDirectoryEnumerator* enumerator = [[NSFileManager defaultManager]
                enumeratorAtURL: url
                includingPropertiesForKeys: @[NSURLIsRegularFileKey, NSURLIsSymbolicLinkKey]
                options: NSDirectoryEnumerationSkipsHiddenFiles | NSDirectoryEnumerationSkipsPackageDescendants
                errorHandler: ^BOOL (NSURL* failedURL, NSError* error) {
                    (void) failedURL;
                    if (enumerationError == nil) enumerationError = [[error localizedDescription] copy];
                    return YES;
                }];
            for (NSURL* child in enumerator)
            {
                NSString* childPath = [[child URLByResolvingSymlinksInPath] path];
                if ([childPath isEqualToString: destPath] || [childPath hasPrefix: destPrefix])
                {
                    [enumerator skipDescendants];
                    continue;
                }
                NSNumber* regular = nil;
                NSNumber* symbolicLink = nil;
                [child getResourceValue: &regular forKey: NSURLIsRegularFileKey error: nil];
                [child getResourceValue: &symbolicLink forKey: NSURLIsSymbolicLinkKey error: nil];
                if ([regular boolValue] && ! [symbolicLink boolValue])
                    copyAudioFile (child, directory, result);
            }
            if (result.error.isEmpty()) result.error = text (enumerationError);
            [enumerationError release];
        }
        else
            copyAudioFile (url, directory, result);

        if (scoped) [url stopAccessingSecurityScopedResource];
    }

    if (result.files.isEmpty() && result.error.isEmpty())
        result.error = "No supported audio files were found. Select WAV, AIFF, FLAC, MP3, OGG, M4A, AAC or CAF files.";
    return result;
}
}
