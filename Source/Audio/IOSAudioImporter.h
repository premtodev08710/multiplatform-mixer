#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace mps
{
// Completion runs on the message thread. Files are permanent, app-owned copies.
using IOSAudioImportCompletion = std::function<void (juce::Array<juce::File>, juce::String)>;
void showIOSAudioImportPicker (juce::Component& parent, bool selectFolder,
                              IOSAudioImportCompletion completion);
}
