#include "IOSAudioImporter.h"

#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <objc/runtime.h>

namespace
{
struct ImportResult
{
    juce::Array<juce::File> files;
    juce::String error;
};

juce::String text (NSString* value)
{
    return value != nil ? juce::String::fromUTF8 ([value UTF8String]) : juce::String();
}

bool isAudioFile (NSURL* url)
{
    return juce::StringArray { "wav", "aif", "aiff", "flac", "mp3", "ogg", "m4a", "aac", "caf" }
        .contains (text ([url pathExtension]).toLowerCase());
}

void copyAudioFile (NSURL* source, const juce::File& destinationDirectory, ImportResult& result)
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

ImportResult stageAudio (NSArray<NSURL*>* urls, bool selectFolder)
{
    ImportResult result;
    const auto directory = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
        .getChildFile ("Imported Audio");
    const auto created = directory.createDirectory();
    if (created.failed())
    {
        result.error = created.getErrorMessage();
        return result;
    }

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

@interface MPSAudioImportDelegate : NSObject<UIDocumentPickerDelegate>
{
@public
    mps::IOSAudioImportCompletion completion;
    bool selectFolder;
}
@end

@implementation MPSAudioImportDelegate
- (void) documentPicker: (UIDocumentPickerViewController*) controller didPickDocumentsAtURLs: (NSArray<NSURL*>*) urls
{
    auto callback = std::move (completion);
    const bool folder = selectFolder;
    NSArray<NSURL*>* selected = [urls copy];
    dispatch_async (dispatch_get_global_queue (QOS_CLASS_USER_INITIATED, 0), ^{
        @autoreleasepool
        {
            auto result = stageAudio (selected, folder);
            [selected release];
            juce::MessageManager::callAsync ([callback, result] {
                if (callback) callback (result.files, result.error);
            });
        }
    });
}

- (void) documentPickerWasCancelled: (UIDocumentPickerViewController*) controller
{
    auto callback = std::move (completion);
    if (callback) callback ({}, {});
}
@end

namespace mps
{
void showIOSAudioImportPicker (juce::Component& parent, bool selectFolder,
                              IOSAudioImportCompletion completion)
{
    auto* peer = parent.getPeer();
    UIView* view = peer != nullptr ? (UIView*) peer->getNativeHandle() : nil;
    UIViewController* presenter = view.window.rootViewController;
    while (presenter.presentedViewController != nil)
        presenter = presenter.presentedViewController;

    if (presenter == nil)
    {
        completion ({}, "Unable to open Files. Return to Tracks and try again.");
        return;
    }

    // Import files as copies, avoiding temporary external URLs once Files closes.
    // public.data also permits FLAC/OGG from providers that omit audio UTType tags.
    NSArray<UTType*>* types = selectFolder ? @[UTTypeFolder] : @[UTTypeData];
    UIDocumentPickerViewController* picker = [[UIDocumentPickerViewController alloc]
        initForOpeningContentTypes: types asCopy: ! selectFolder];
    MPSAudioImportDelegate* delegate = [[MPSAudioImportDelegate alloc] init];
    delegate->completion = std::move (completion);
    delegate->selectFolder = selectFolder;
    picker.delegate = delegate;
    picker.allowsMultipleSelection = ! selectFolder;
    picker.modalPresentationStyle = UIModalPresentationFullScreen;
    // UIDocumentPicker's delegate is weak. Retain it for the picker's lifetime.
    static char delegateKey;
    objc_setAssociatedObject (picker, &delegateKey, delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [delegate release];
    [presenter presentViewController: picker animated: YES completion: nil];
    [picker release];
}
}
