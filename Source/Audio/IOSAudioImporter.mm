#include "IOSAudioImporter.h"
#include "AppleAudioFileStaging.h"

#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <objc/runtime.h>

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
            const auto directory = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                .getChildFile ("Imported Audio");
            auto result = mps::stageAppleAudioDocuments (selected, folder, directory);
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
    UIViewController* presenter = view != nil ? view.window.rootViewController : nil;

    if (presenter == nil)
    {
        for (UIWindow* window in [UIApplication sharedApplication].windows)
        {
            if (window.rootViewController != nil)
            {
                presenter = window.rootViewController;
                if (window.isKeyWindow) break;
            }
        }
    }

    if (presenter == nil)
    {
        for (UIScene* scene in [UIApplication sharedApplication].connectedScenes)
        {
            if ([scene isKindOfClass: [UIWindowScene class]])
            {
                for (UIWindow* window in ((UIWindowScene*) scene).windows)
                {
                    if (window.rootViewController != nil)
                    {
                        presenter = window.rootViewController;
                        break;
                    }
                }
            }
            if (presenter != nil) break;
        }
    }

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
