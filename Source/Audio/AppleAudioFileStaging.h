#pragma once

#include <juce_core/juce_core.h>
#import <Foundation/Foundation.h>

namespace mps
{
struct AppleAudioImportResult
{
    juce::Array<juce::File> files;
    juce::String error;
};

AppleAudioImportResult stageAppleAudioDocuments (NSArray<NSURL*>* urls, bool selectFolder,
                                               const juce::File& directory);
}
