// Mix Practice Studio - Main Application Entry Point
#include "Core/AppEdition.h"
#include "UI/MainComponent.h"
#include <juce_gui_extra/juce_gui_extra.h>

namespace mps
{

class MixPracticeStudioApplication : public juce::JUCEApplication
{
public:
    MixPracticeStudioApplication() = default;

    const juce::String getApplicationName() override       { return AppEdition::isPro() ? "Mix Practice Studio Pro" : "Mix Practice Studio"; }
    const juce::String getApplicationVersion() override    { return "1.0.1"; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    void initialise (const juce::String& /*commandLine*/) override
    {
       #if JUCE_ANDROID
        juce::RuntimePermissions::request (juce::RuntimePermissions::readMediaAudio, [] (bool) {
            juce::RuntimePermissions::request (juce::RuntimePermissions::readExternalStorage, [] (bool) {});
        });
       #endif

        const juce::String title = getApplicationName() + " [" + AppEdition::getEditionName() + "]";
        mainWindow = std::make_unique<MainWindow> (title);
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const juce::String& /*commandLine*/) override
    {
    }

    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow (juce::String name)
            : DocumentWindow (name,
                              juce::Colour (0xff121417),
                              DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent(), true);

           #if JUCE_IOS || JUCE_ANDROID
            setFullScreen (true);
           #else
            setResizable (true, true);
            setResizeLimits (800, 500, 2560, 1600);
            centreWithSize (getWidth(), getHeight());
           #endif

            setVisible (true);

           #if JUCE_ANDROID
            // JUCE only hides Android's system bars in kiosk mode. Without this,
            // the edge-to-edge status bar overlays the top navigation buttons.
            juce::Desktop::getInstance().setKioskModeComponent (this);
           #endif
        }

        ~MainWindow() override
        {
           #if JUCE_ANDROID
            juce::Desktop::getInstance().setKioskModeComponent (nullptr);
           #endif
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

} // namespace mps

START_JUCE_APPLICATION (mps::MixPracticeStudioApplication)
