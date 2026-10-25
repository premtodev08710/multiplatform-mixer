#include "../Source/UI/MainComponent.h"
#include "../Source/Model/ConsoleStateFields.h"
#include <iostream>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI guiInit;

    if (argc > 1 && juce::String (argv[1]) == "--bus-display-ui-tests")
    {
        mps::AudioEngine host;
        mps::ProjectModel project (host);
        mps::DetailScreen detail (host, project);
        detail.setSize (1280, 800); detail.setVisible (false); detail.setVisible (true);
        detail.setSelectedStrip (mps::ChannelStripComponent::StripType::Bus, 0);
        auto tab = [&] (const char* name) {
            for (auto* child : detail.getChildren())
                if (auto* button = dynamic_cast<juce::TextButton*> (child))
                    if (button->getButtonText() == name) detail.buttonClicked (button);
        };
        auto differs = [] (const juce::Image& a, const juce::Image& b) {
            for (int y = 0; y < a.getHeight(); ++y)
                for (int x = 0; x < a.getWidth(); ++x)
                    if (a.getPixelAt (x, y) != b.getPixelAt (x, y)) return true;
            return false;
        };
        int failures = host.getMixEngine().getRtaChannel() == (int) mps::kMaxInputChannels ? 0 : 1;
        tab ("EQ");
        for (auto* child : detail.getChildren())
            if (auto* graph = dynamic_cast<mps::InteractiveEqGraph*> (child)) graph->setRtaEnabled (true);
        auto beforeEq = detail.createComponentSnapshot (detail.getLocalBounds());
        for (auto& band : host.getMixerMeters().busDetails[0].rtaBands) band.update (0.2f);
        detail.refreshLiveMeters();
        auto afterEq = detail.createComponentSnapshot (detail.getLocalBounds());
        if (!differs (beforeEq, afterEq)) ++failures;
        tab ("DYN");
        auto beforeDyn = detail.createComponentSnapshot (detail.getLocalBounds());
        host.getMixerMeters().busDetails[0].dsp.compGainReductionDb.store (18.0f);
        detail.refreshLiveMeters();
        auto afterDyn = detail.createComponentSnapshot (detail.getLocalBounds());
        if (!differs (beforeDyn, afterDyn)) ++failures;
        detail.setSelectedStrip (mps::ChannelStripComponent::StripType::Bus, 1);
        if (host.getMixEngine().getRtaChannel() != (int) mps::kMaxInputChannels + 1) ++failures;
        std::cout << "Bus EQ spectrum and compressor display: " << failures << " failures\n";
        return failures == 0 ? 0 : 1;
    }

    if (argc > 1 && juce::String (argv[1]) == "--bus-send-ui-tests")
    {
        mps::AudioEngine host;
        mps::ProjectModel project (host);
        mps::DetailScreen detail (host, project);
        detail.setSize (1280, 800);
        detail.setSelectedStrip (mps::ChannelStripComponent::StripType::Bus, 3);
        for (auto* child : detail.getChildren())
            if (auto* button = dynamic_cast<juce::TextButton*> (child))
                if (button->getButtonText() == "SENDS") detail.buttonClicked (button);
        int failures = 0, tested = 0;
        for (auto* child : detail.getChildren())
            if (auto* slider = dynamic_cast<juce::Slider*> (child))
                if (slider->getName().startsWith ("Send "))
                {
                    const int index = slider->getName().substring (5).getIntValue() - 1;
                    if (slider->isVisible() != (index < 6)) ++failures;
                    if (index < 6)
                    {
                        slider->setValue (mps::faderDbToPos (-12.0f), juce::sendNotificationSync);
                        if (std::abs (host.getMixerParams().buses[3].matrixSends[(size_t) index].get() + 12.0f) > 0.2f) ++failures;
                        if (host.getMixerParams().channels[0].sends[(size_t) index].get() > -89.0f) ++failures;
                        ++tested;
                    }
                }
        if (tested != 6) ++failures;
        std::cout << "Bus send UI: " << tested << " matrix sends, " << failures << " failures\n";
        return failures == 0 ? 0 : 1;
    }

    if (argc > 1 && juce::String (argv[1]) == "--console-state-tests")
    {
        mps::AudioEngine host;
        mps::ProjectModel project (host);
        auto& mixer = host.getMixerParams();
        mixer.buses[3].matrixSends[2].set (-7.5f);
        mixer.buses[3].dsp.compThreshDb.set (-31.0f);
        mixer.matrices[2].polarity.setBool (true);
        mixer.fxSlots[6].insertTargets[1].set (204);
        mixer.channels[2].insertFxSlot.set (6);
        mixer.auxes[1].faderDb.set (-12);
        mixer.monitorMode.set (2);
        mixer.outputSource.set (19);
        mixer.muteGroupActive[4].store (true);
        const auto snapshot = juce::JSON::parse (juce::JSON::toString (project.captureSceneSnapshot ("Round trip")));
        std::vector<float> expected;
        mps::visitConsoleState (mixer, [&] (const juce::String&, mps::Param& p) { expected.push_back (p.get()); p.resetToDefault(); });
        mixer.muteGroupActive[4].store (false);
        project.recallSceneSnapshot (snapshot);
        size_t index = 0; int failures = 0;
        mps::visitConsoleState (mixer, [&] (const juce::String& key, mps::Param& p) {
            if (std::abs (p.get() - expected[index++]) > 0.0001f) { std::cerr << key << " did not round trip\n"; ++failures; }
        });
        if (!mixer.muteGroupActive[4].load()) ++failures;
        const auto recordingFile = juce::File::createTempFile (".wav");
        recordingFile.deleteFile();
        host.preparePlayback (48000, 256);
        if (host.startRecording (recordingFile).failed()) ++failures;
        std::array<float, 256> left {}, right {};
        float* outputs[] = { left.data(), right.data() };
        juce::AudioIODeviceCallbackContext context;
        for (int block = 0; block < 8; ++block)
            host.audioDeviceIOCallbackWithContext (nullptr, 0, outputs, 2, 256, context);
        host.stopRecording();
        auto reader = std::unique_ptr<juce::AudioFormatReader> (host.getFormatManager().createReaderFor (recordingFile));
        if (!reader || reader->numChannels != 2 || reader->sampleRate != 48000 || reader->lengthInSamples != 2048) ++failures;
        if (host.startRecording (recordingFile).wasOk()) { ++failures; host.stopRecording(); }
        reader.reset(); recordingFile.deleteFile();
        // Legacy snapshots without consoleState must still be accepted.
        auto legacy = project.captureSceneSnapshot ("Legacy");
        legacy.getDynamicObject()->getProperty ("mixer").getDynamicObject()->removeProperty ("consoleState");
        project.recallSceneSnapshot (legacy);
        // Offline export uses fresh readers/DSP and excludes audition controls.
        mps::AudioEngine exportHost;
        const auto inputA = juce::File::createTempFile (".wav");
        const auto inputB = juce::File::createTempFile (".wav");
        auto makeInput = [&] (const juce::File& file, int frames, float value) {
            auto output = std::unique_ptr<juce::OutputStream>(file.createOutputStream());
            juce::WavAudioFormat wav;
            auto writer = wav.createWriterFor(output, juce::AudioFormatWriterOptions().withSampleRate(48000).withNumChannels(1).withBitsPerSample(24));
            std::vector<float> samples((size_t)frames,value); const float* data[] = { samples.data() };
            if (!writer || !writer->writeFromFloatArrays(data,1,frames)) ++failures;
        };
        makeInput(inputA,2400,0.2f); makeInput(inputB,4800,0.1f);
        exportHost.setTrackSlot(0,mps::AudioFileTrack::open(inputA,48000,exportHost.getFormatManager()));
        exportHost.setTrackSlot(1,mps::AudioFileTrack::open(inputB,48000,exportHost.getFormatManager()));
        auto& ep = exportHost.getMixerParams();
        for (int c = 0; c < 2; ++c) { ep.channels[(size_t)c].source.set((float)c); ep.channels[(size_t)c].pan.set(-1); ep.channels[(size_t)c].dsp.eqBypass.setBool(true); }
        ep.channels[0].solo.setBool(true); ep.monitorMute.setBool(true); ep.monitorLevelDb.set(-90); ep.outputSource.set(17);
        const auto playhead = exportHost.getStreamer().getPlayhead();
        const auto wavExport = juce::File::createTempFile(".wav"); wavExport.deleteFile();
        mps::MixdownExporter bounce(exportHost,wavExport,false,0.1);
        // Capture settings now, then alter live controls: export must keep its snapshot.
        ep.channels[0].faderDb.set(-30);
        bounce.startThread(); bounce.waitForThreadToExit(30000);
        if (!bounce.isFinished() || bounce.getResult().failed()) ++failures;
        auto exported = std::unique_ptr<juce::AudioFormatReader>(exportHost.getFormatManager().createReaderFor(wavExport));
        if (!exported || exported->lengthInSamples != 9600 || exported->numChannels != 2 || exported->sampleRate != 48000) ++failures;
        else {
            juce::AudioBuffer<float> audio(2,9600); exported->read(&audio,0,9600,0,true,true);
            if (std::abs(audio.getSample(0,0)-0.3f)>1e-4 || std::abs(audio.getSample(0,3000)-0.1f)>1e-4 || std::abs(audio.getSample(0,9500))>1e-5 || std::abs(audio.getSample(1,1000))>1e-5) ++failures;
        }
        if (!ep.monitorMute.getBool() || !ep.channels[0].solo.getBool() || ep.outputSource.getInt()!=17 || exportHost.getStreamer().getPlayhead()!=playhead) ++failures;
        exported.reset();
        const auto originalSize = wavExport.getSize();
        mps::MixdownExporter noOverwrite(exportHost,wavExport,false,0);
        noOverwrite.run();
        if (noOverwrite.getResult().wasOk() || wavExport.getSize()!=originalSize) ++failures;
        const auto cancelledFile = juce::File::createTempFile(".wav"); cancelledFile.deleteFile();
        mps::MixdownExporter cancelled(exportHost,cancelledFile,false,0);
        cancelled.signalThreadShouldExit(); cancelled.run();
        if (cancelled.getResult().wasOk() || cancelledFile.exists()) ++failures;
        if (mps::MixdownExporter::findMp3Encoder().existsAsFile()) {
            const auto mp3File = juce::File::createTempFile(".mp3"); mp3File.deleteFile();
            mps::MixdownExporter mp3(exportHost,mp3File,true,0);
            mp3.startThread(); mp3.waitForThreadToExit(30000);
            if (!mp3.isFinished() || mp3.getResult().failed() || mp3File.getSize()<100) ++failures;
            auto mp3Reader = std::unique_ptr<juce::AudioFormatReader>(exportHost.getFormatManager().createReaderFor(mp3File));
            if (!mp3Reader || mp3Reader->numChannels != 2 || mp3Reader->lengthInSamples < 4000) {
                std::cerr << "MP3 decode: " << (mp3Reader ? juce::String(mp3Reader->lengthInSamples) + " frames, " + juce::String(mp3Reader->numChannels) + " channels" : juce::String("no decoder")) << "\n";
                ++failures;
            }
            std::cout << "MP3 export bytes: " << mp3File.getSize() << "\n";
            mp3File.deleteFile();
        }
        wavExport.deleteFile(); inputA.deleteFile(); inputB.deleteFile();
        mps::ConsoleToolsScreen tools (host);
        for (int width : { 640, 1024 })
            for (int mode = 0; mode < 4; ++mode)
            {
                tools.setSize (width, 420); tools.setMode (mode);
                auto image = tools.createComponentSnapshot (tools.getLocalBounds());
                juce::File target ("/tmp/mps-console-" + juce::String(width) + "-" + juce::String(mode) + ".png");
                target.deleteFile(); juce::FileOutputStream stream (target);
                juce::PNGImageFormat png; if (!stream.openedOk() || !png.writeImageToStream (image, stream)) ++failures;
            }
        std::cout << "Console state: " << index << " parameters, " << failures << " failures\n";
        return failures == 0 ? 0 : 1;
    }

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
