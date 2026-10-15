#include "../Source/UI/MainComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

int main (int, char*[])
{
    juce::ScopedJuceInitialiser_GUI guiInit;

    mps::MainComponent comp;
    comp.setSize (1024, 471);

    // 1. Snapshot HomeScreen (Mixer Console with longer sliders & top-right gear icon)
    comp.showScreen (0); // NavHome
    auto homeImage = comp.createComponentSnapshot (comp.getLocalBounds());
    juce::File homeOutFile = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("home_1024x471.png");
    homeOutFile.deleteFile();
    juce::FileOutputStream homeStream (homeOutFile);
    if (homeStream.openedOk())
    {
        juce::PNGImageFormat png;
        png.writeImageToStream (homeImage, homeStream);
    }

    comp.showScreen (1); // NavDetail
    auto image = comp.createComponentSnapshot (comp.getLocalBounds());
    juce::File outFile = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("detail_1024x471.png");
    outFile.deleteFile();
    juce::FileOutputStream stream (outFile);
    if (stream.openedOk())
    {
        juce::PNGImageFormat png;
        png.writeImageToStream (image, stream);
    }

    // Also snapshot SetupScreen
    comp.showScreen (9); // NavSetup
    auto setupImage = comp.createComponentSnapshot (comp.getLocalBounds());
    juce::File setupOutFile = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("setup_1024x471.png");
    setupOutFile.deleteFile();
    juce::FileOutputStream setupStream (setupOutFile);
    if (setupStream.openedOk())
    {
        juce::PNGImageFormat png;
        png.writeImageToStream (setupImage, setupStream);
    }

    comp.showScreen (1); // Back to NavDetail
    mps::DetailScreen* detail = nullptr;
    for (auto* child : comp.getChildren())
    {
        if (auto* d = dynamic_cast<mps::DetailScreen*> (child))
        {
            detail = d;
            break;
        }
    }

    if (detail != nullptr)
    {
        for (const char* tabName : { "GATE", "DYN", "EQ", "SENDS" })
        {
            for (auto* c : detail->getChildren())
        {
            if (auto* btn = dynamic_cast<juce::TextButton*> (c))
            {
                if (btn->getButtonText() == tabName)
                {
                    detail->buttonClicked (btn);
                    break;
                }
            }
        }
        auto img = comp.createComponentSnapshot (comp.getLocalBounds());
        juce::File f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("tab_" + juce::String (tabName).toLowerCase() + ".png");
        f.deleteFile();
        juce::FileOutputStream s (f);
        if (s.openedOk())
        {
            juce::PNGImageFormat png;
            png.writeImageToStream (img, s);
        }
        }
    }

    return 0;
}
