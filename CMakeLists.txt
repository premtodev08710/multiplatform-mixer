cmake_minimum_required(VERSION 3.22)

project(MixPracticeStudio VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add local JUCE 8 framework dependency
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
    add_subdirectory(external/JUCE)
else()
    message(FATAL_ERROR "JUCE not found in external/JUCE. Please clone JUCE 8.0.15 into external/JUCE.")
endif()

#==============================================================================
# Main GUI Application
juce_add_gui_app(MixPracticeStudio
    PRODUCT_NAME "Mix Practice Studio"
    COMPANY_NAME "Mix Practice Studio"
    BUNDLE_ID "com.mixpractice.studio"
    VERSION 1.0.0
    ICON_BIG Resources/app_icon.png
)

target_sources(MixPracticeStudio PRIVATE
    # Core DSP & Transport (Pure C++ / cross-platform)
    Source/Core/DspMath.h
    Source/Core/ChannelDsp.h
    Source/Core/EffectsDsp.h
    Source/Core/EffectsDsp.cpp
    Source/Core/MixerParams.h
    Source/Core/MixEngine.h
    Source/Core/MixEngine.cpp
    Source/Core/TimelineStreamer.h
    Source/Core/TimelineStreamer.cpp
    Source/Core/SincResampler.h
    Source/Core/SincResampler.cpp

    # Audio IO & File Streaming
    Source/Audio/AudioFileTrack.h
    Source/Audio/AudioFileTrack.cpp
    Source/Audio/AudioEngine.h
    Source/Audio/AudioEngine.cpp

    # Model & Project Storage
    Source/Model/ProjectModel.h
    Source/Model/ProjectModel.cpp

    # Network Remote Control (Behringer X32 OSC Protocol)
    Source/Network/X32OscClient.h
    Source/Network/X32OscClient.cpp

    # UI Components
    Source/UI/ConsoleLookAndFeel.h
    Source/UI/ConsoleLookAndFeel.cpp
    Source/UI/ConsoleDial.h
    Source/UI/ConsoleDial.cpp
    Source/UI/TrapezoidPanIndicator.h
    Source/UI/TrapezoidPanIndicator.cpp
    Source/UI/TransferCurveGraph.h
    Source/UI/TransferCurveGraph.cpp
    Source/UI/GainReductionMeter.h
    Source/UI/GainReductionMeter.cpp
    Source/UI/BankSelectorBar.h
    Source/UI/BankSelectorBar.cpp
    Source/UI/SignalFlowDiagram.h
    Source/UI/SignalFlowDiagram.cpp
    Source/UI/LedMeter.h
    Source/UI/LedMeter.cpp
    Source/UI/InteractiveEqGraph.h
    Source/UI/InteractiveEqGraph.cpp
    Source/UI/ChannelStripComponent.h
    Source/UI/ChannelStripComponent.cpp
    Source/UI/TimelineRangeComponent.h
    Source/UI/TimelineRangeComponent.cpp
    Source/UI/TransportBar.h
    Source/UI/TransportBar.cpp
    Source/UI/HomeScreen.h
    Source/UI/HomeScreen.cpp
    Source/UI/DetailScreen.h
    Source/UI/DetailScreen.cpp
    Source/UI/EffectsScreen.h
    Source/UI/EffectsScreen.cpp
    Source/UI/FxFaceplateComponent.h
    Source/UI/FxFaceplateComponent.cpp
    Source/UI/TracksScreen.h
    Source/UI/TracksScreen.cpp
    Source/UI/ScenesScreen.h
    Source/UI/ScenesScreen.cpp
    Source/UI/SetupScreen.h
    Source/UI/SetupScreen.cpp
    Source/UI/MainComponent.h
    Source/UI/MainComponent.cpp

    # Application Entry
    Source/Main.cpp
)

target_compile_definitions(MixPracticeStudio PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_SILENCE_XCODE_15_LINKER_WARNING=1
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:MixPracticeStudio,JUCE_PROJECT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:MixPracticeStudio,JUCE_VERSION>"
)

target_link_libraries(MixPracticeStudio PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
)

#==============================================================================
# Automated Core Unit Tests
add_executable(MixPracticeCoreTests
    Tests/CoreTests.cpp
    Source/Core/MixEngine.cpp
    Source/Core/EffectsDsp.cpp
    Source/Core/TimelineStreamer.cpp
    Source/Core/SincResampler.cpp
)

target_include_directories(MixPracticeCoreTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

enable_testing()
add_test(NAME CoreDspAndTransportTests COMMAND MixPracticeCoreTests)
