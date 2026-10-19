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

    // Snapshot EffectsScreen (NavEffects = 3)
    comp.showScreen (3); // NavEffects
    auto effectsImage = comp.createComponentSnapshot (comp.getLocalBounds());
    juce::File effectsOutFile = juce::File::getCurrentWorkingDirectory().getChildFile ("effects_lexicon_larc_1024x471.png");
    effectsOutFile.deleteFile();
    juce::FileOutputStream effectsStream (effectsOutFile);
    if (effectsStream.openedOk())
    {
        juce::PNGImageFormat png;
        png.writeImageToStream (effectsImage, effectsStream);
    }

    comp.setSize (1024, 642);

    // Snapshot Page 0: Reverb Time
    comp.getEffectsScreen().getFaceplate().setLarcPageIndex (0);
    auto page0Img = comp.createComponentSnapshot (comp.getLocalBounds());
    juce::File page0File = juce::File::getCurrentWorkingDirectory().getChildFile ("effects_larc_page0_1024x642.png");
    page0File.deleteFile();
    juce::FileOutputStream page0Stream (page0File);
    if (page0Stream.openedOk()) { juce::PNGImageFormat png; png.writeImageToStream (page0Img, page0Stream); }

    // Snapshot Page 1: Tone & Modulation
    comp.getEffectsScreen().getFaceplate().setLarcPageIndex (1);
    auto page1Img = comp.createComponentSnapshot (comp.getLocalBounds());
    juce::File page1File = juce::File::getCurrentWorkingDirectory().getChildFile ("effects_larc_page1_1024x642.png");
    page1File.deleteFile();
    juce::FileOutputStream page1Stream (page1File);
    if (page1Stream.openedOk()) { juce::PNGImageFormat png; png.writeImageToStream (page1Img, page1Stream); }

    // Snapshot Page 2: Echoes
    comp.getEffectsScreen().getFaceplate().setLarcPageIndex (2);
    auto page2Img = comp.createComponentSnapshot (comp.getLocalBounds());
    juce::File page2File = juce::File::getCurrentWorkingDirectory().getChildFile ("effects_larc_page2_1024x642.png");
    page2File.deleteFile();
    juce::FileOutputStream page2Stream (page2File);
    if (page2Stream.openedOk()) { juce::PNGImageFormat png; png.writeImageToStream (page2Img, page2Stream); }

    // Snapshot FX 5-8 Group
    comp.getAudioEngine().getMixerParams().channels[0].insertFxSlot.set (5.0f); // CH 01 inserts FX 5
    comp.getAudioEngine().getMixerParams().buses[1].insertFxSlot.set (6.0f);    // Bus 02 inserts FX 6
    comp.getAudioEngine().getMixerParams().matrices[0].insertFxSlot.set (7.0f); // Mtx 1 inserts FX 7
    comp.getAudioEngine().getMixerParams().main.insertFxSlot.set (8.0f);        // Main LR inserts FX 8
    comp.getEffectsScreen().refreshFromModel();

    // Trigger tab FX 5-8 button
    for (auto* c : comp.getEffectsScreen().getChildren())
    {
        if (auto* btn = dynamic_cast<juce::TextButton*> (c))
        {
            if (btn->getButtonText().contains ("FX 5"))
            {
                comp.getEffectsScreen().buttonClicked (btn);
                break;
            }
        }
    }
    auto fx5to8Img = comp.createComponentSnapshot (comp.getLocalBounds());
    juce::File fx5to8File = juce::File::getCurrentWorkingDirectory().getChildFile ("effects_group5to8_1024x642.png");
    fx5to8File.deleteFile();
    juce::FileOutputStream fx5to8Stream (fx5to8File);
    if (fx5to8Stream.openedOk()) { juce::PNGImageFormat png; png.writeImageToStream (fx5to8Img, fx5to8Stream); }

    // Select Slot 5 (FX 6 - Stereo Xtec EQ1)
    comp.getEffectsScreen().getFaceplate().setSlot (5);
    auto fx6Img = comp.createComponentSnapshot (comp.getLocalBounds());
    juce::File fx6File = juce::File::getCurrentWorkingDirectory().getChildFile ("effects_slot6_1024x642.png");
    fx6File.deleteFile();
    juce::FileOutputStream fx6Stream (fx6File);
    if (fx6Stream.openedOk()) { juce::PNGImageFormat png; png.writeImageToStream (fx6Img, fx6Stream); }

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
