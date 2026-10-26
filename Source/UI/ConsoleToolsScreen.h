#pragma once
#include "../Audio/AudioEngine.h"
#include "../Audio/MixdownExporter.h"
#include "ConsoleLookAndFeel.h"

namespace mps {
// Routing controls and meter bridge use the same atomic model as the channel strips.
class ConsoleToolsScreen : public juce::Component, private juce::Timer
{
public:
    explicit ConsoleToolsScreen (AudioEngine& host) : engine (host)
    {
        source.addItem ("Main LR", 1);
        for (int i = 0; i < 16; ++i) source.addItem ("Bus " + juce::String (i + 1), i + 2);
        source.setSelectedId (1);
        source.onChange = [this] { refreshSends(); };
        addAndMakeVisible (source);
        output.addItem ("Main LR", 1);
        for (int i = 0; i < 16; ++i) output.addItem ("Bus " + juce::String (i + 1), i + 2);
        for (int i = 0; i < 6; ++i) output.addItem ("Matrix " + juce::String (i + 1), i + 18);
        output.addItem ("Mono / Center", 24);
        output.onChange = [this] { engine.getMixerParams().outputSource.set ((float) (output.getSelectedId() - 1)); };
        addAndMakeVisible (output);
        for (int i = 0; i < 6; ++i)
        {
            auto& slider = sends[(size_t) i];
            slider.setSliderStyle (juce::Slider::LinearVertical);
            slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 24);
            slider.setRange (-90.0, 10.0, 0.1);
            slider.setTextValueSuffix (" dB");
            slider.onValueChange = [this, i] {
                auto& p = engine.getMixerParams();
                const int bus = source.getSelectedId() - 2;
                auto& value = bus < 0 ? p.main.matrixSends[(size_t) i] : p.buses[(size_t) bus].matrixSends[(size_t) i];
                value.set ((float) sends[(size_t) i].getValue());
            };
            addAndMakeVisible (slider);
        }
        monitorMode.addItem ("Solo in place (Main)", 1);
        monitorMode.addItem ("Solo PFL", 2);
        monitorMode.addItem ("Solo AFL", 3);
        monitorMode.onChange = [this] { engine.getMixerParams().monitorMode.set ((float) (monitorMode.getSelectedId()-1)); };
        addAndMakeVisible (monitorMode);
        level.setRange (-90, 10, 0.1); level.setTextValueSuffix (" dB");
        level.setSliderStyle (juce::Slider::LinearHorizontal);
        level.setTextBoxStyle (juce::Slider::TextBoxRight, false, 80, 24);
        level.onValueChange = [this] { engine.getMixerParams().monitorLevelDb.set ((float) level.getValue()); };
        addAndMakeVisible (level);
        dim.setButtonText ("DIM (-20 dB)"); mute.setButtonText ("MUTE MONITOR"); mono.setButtonText ("MONO");
        for (auto* button : { &dim, &mute, &mono })
        {
            button->setClickingTogglesState (true); addAndMakeVisible (*button);
        }
        dim.onClick = [this] { engine.getMixerParams().monitorDim.setBool (dim.getToggleState()); };
        mute.onClick = [this] { engine.getMixerParams().monitorMute.setBool (mute.getToggleState()); };
        mono.onClick = [this] { engine.getMixerParams().monitorMono.setBool (mono.getToggleState()); };
        record.setButtonText ("RECORD STEREO WAV..."); stop.setButtonText ("STOP & SAVE");
        record.onClick = [this] {
            chooser = std::make_unique<juce::FileChooser> ("New stereo WAV recording", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Mix.wav"), "*.wav");
            juce::Component::SafePointer<ConsoleToolsScreen> safe (this);
            chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                [safe] (const juce::FileChooser& selected) {
                    if (!safe || selected.getResult() == juce::File()) return;
                    auto result = safe->engine.startRecording (selected.getResult().withFileExtension ("wav"));
                    safe->recordStatus = result.wasOk() ? "Recording: " + selected.getResult().getFileName() : result.getErrorMessage();
                    safe->repaint();
                });
        };
        stop.onClick = [this] { engine.stopRecording(); recordStatus = "Saved. Dropped blocks: " + juce::String(engine.getRecordingOverruns()); repaint(); };
        addAndMakeVisible (record); addAndMakeVisible (stop);
        exportFormat.addItem ("WAV 24-bit", 1);
        exportFormat.addItem ("MP3 320 kbps", 2);
        exportFormat.setSelectedId (1);
        addAndMakeVisible (exportFormat);
        tailSeconds.setRange (0, 30, 0.5); tailSeconds.setValue (3);
        tailSeconds.setSliderStyle (juce::Slider::LinearHorizontal);
        tailSeconds.setTextBoxStyle (juce::Slider::TextBoxRight, false, 70, 24);
        tailSeconds.setTextValueSuffix (" sec");
        addAndMakeVisible (tailSeconds);
        exportMix.setButtonText ("EXPORT FULL MIX...");
        cancelExport.setButtonText ("CANCEL EXPORT");
        cancelExport.onClick = [this] { if (exporter) exporter->signalThreadShouldExit(); };
        exportMix.onClick = [this] {
            if (exporter && !exporter->isFinished()) return;
            const bool mp3 = exportFormat.getSelectedId() == 2;
            if (mp3 && !MixdownExporter::findMp3Encoder().existsAsFile())
            { exportStatus = "MP3 requires LAME on this device. WAV export is available."; repaint(); return; }
            chooser = std::make_unique<juce::FileChooser> ("Export full Main LR mix", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile (mp3 ? "Mix.mp3" : "Mix.wav"), mp3 ? "*.mp3" : "*.wav");
            juce::Component::SafePointer<ConsoleToolsScreen> safe (this);
            chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                [safe, mp3] (const juce::FileChooser& selected) {
                    if (!safe || selected.getResult() == juce::File()) return;
                    safe->exporter = std::make_unique<MixdownExporter> (safe->engine, selected.getResult().withFileExtension(mp3 ? "mp3" : "wav"), mp3, safe->tailSeconds.getValue());
                    safe->exportStatus = "Exporting full mix...";
                    safe->exportReported = false;
                    safe->exporter->startThread();
                });
        };
        addAndMakeVisible (exportMix); addAndMakeVisible (cancelExport);
        startTimerHz (20);
    }

    void setMode (int value)
    {
        mode = value;
        source.setVisible (mode == 1);
        output.setVisible (mode == 1);
        output.setSelectedId (engine.getMixerParams().outputSource.getInt() + 1, juce::dontSendNotification);
        for (auto& slider : sends) slider.setVisible (mode == 1);
        record.setVisible (mode == 3); stop.setVisible (mode == 3);
        exportMix.setVisible(mode == 3); cancelExport.setVisible(mode == 3);
        exportFormat.setVisible(mode == 3); tailSeconds.setVisible(mode == 3);
        monitorMode.setVisible (mode == 2); level.setVisible (mode == 2);
        dim.setVisible (mode == 2); mute.setVisible (mode == 2); mono.setVisible (mode == 2);
        auto& p = engine.getMixerParams();
        monitorMode.setSelectedId (p.monitorMode.getInt()+1, juce::dontSendNotification);
        level.setValue (p.monitorLevelDb.get(), juce::dontSendNotification);
        dim.setToggleState (p.monitorDim.getBool(), juce::dontSendNotification);
        mute.setToggleState (p.monitorMute.getBool(), juce::dontSendNotification);
        mono.setToggleState (p.monitorMono.getBool(), juce::dontSendNotification);
        refreshSends();
        repaint();
    }

    void resized() override
    {
        const int half = (getWidth()-60)/2;
        exportFormat.setBounds (24, 85, half, 30);
        tailSeconds.setBounds (36+half, 85, half, 30);
        exportMix.setBounds (24, 125, half, 36); cancelExport.setBounds (36+half, 125, half, 36);
        record.setBounds (24, 270, half, 34); stop.setBounds (36+half, 270, half, 34);
        monitorMode.setBounds (24, 90, juce::jmin(300, getWidth()-48), 36);
        level.setBounds (24, 170, getWidth()-48, 40);
        dim.setBounds (24, 245, 180, 40); mute.setBounds (214, 245, 180, 40); mono.setBounds (404, 245, 100, 40);
        source.setBounds (24, 78, juce::jmin (220, getWidth() / 3), 34);
        output.setBounds (getWidth() / 2, 78, juce::jmin (260, getWidth() / 2 - 24), 34);
        const int w = juce::jmax (1, (getWidth() - 48) / 6);
        for (int i = 0; i < 6; ++i) sends[(size_t) i].setBounds (24 + i * w, 158, w - 10, juce::jmax (80, getHeight() - 190));
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff171c22));
        g.setColour (juce::Colours::white);
        g.setFont (22.0f);
        g.drawText (mode == 0 ? "Meter Bridge" : mode == 1 ? "Matrix Routing" : mode == 2 ? "Monitor & Solo" : "Stereo Recorder", 24, 12, getWidth() - 48, 30, juce::Justification::centredLeft);
        g.setFont (14.0f);
        if (mode == 3)
        {
            g.drawFittedText ("Export the entire Main LR mix with FX. Solo and Monitor controls are excluded. Extra FX tail:", 24, 45, getWidth()-48, 34, juce::Justification::centredLeft, 2);
            const auto status = exporter && !exporter->isFinished() ? "Exporting " + juce::String((int)(exporter->getProgress()*100)) + "%" : exportStatus;
            g.drawFittedText (status, 24, 170, getWidth()-48, 45, juce::Justification::centredLeft, 2);
            g.drawText ("Or record the monitored output in real time:", 24, 230, getWidth()-48, 24, juce::Justification::centredLeft);
            g.drawFittedText (recordStatus, 24, 315, getWidth()-48, 36, juce::Justification::centredLeft, 2);
            g.drawText ((engine.isRecording() ? juce::String("RECORDING - ") : juce::String("Stopped - ")) + "Dropped blocks: " + juce::String(engine.getRecordingOverruns()), 24, 360, getWidth()-48, 24, juce::Justification::centredLeft);
            return;
        }
        if (mode == 2)
        {
            g.drawText ("Solo mode", 24, 56, getWidth()-48, 24, juce::Justification::centredLeft);
            g.drawText ("Monitor level", 24, 140, getWidth()-48, 24, juce::Justification::centredLeft);
            g.drawFittedText ("PFL / AFL auditions soloed channels without cutting the Main mix. With no Solo, you hear the selected device output source.", 24, 310, getWidth()-48, 65, juce::Justification::centredLeft, 3);
            return;
        }
        if (mode == 1)
        {
            g.drawText ("Send source (post fader)", 24, 50, 260, 24, juce::Justification::centredLeft);
            g.drawText ("Device output pair", getWidth() / 2, 50, getWidth() / 2 - 24, 24, juce::Justification::centredLeft);
            const int w = juce::jmax (1, (getWidth() - 48) / 6);
            for (int i = 0; i < 6; ++i) g.drawText ("Matrix " + juce::String (i + 1), 24 + i*w, 126, w-10, 28, juce::Justification::centred);
            return;
        }
        auto& m = engine.getMixerMeters();
        constexpr int meterCount = 73;
        const int availableRows = juce::jmax (1, (getHeight() - 55) / 30);
        const int columns = juce::jmax (juce::jmax (1, getWidth() / 110), (meterCount + availableRows - 1) / availableRows);
        const int rows = (meterCount + columns - 1) / columns;
        const int cellW = getWidth() / columns;
        const int cellH = juce::jmax (25, (getHeight() - 55) / rows);
        for (int i = 0; i < meterCount; ++i)
        {
            float peak = 0.0f; juce::String name;
            if (i < 32) { name = "CH " + juce::String(i+1); peak = juce::jmax(m.channels[(size_t)i].postL.read(), m.channels[(size_t)i].postR.read()); }
            else if (i < 48) { name = "Bus " + juce::String(i-31); peak = m.buses[(size_t)(i-32)].read(); }
            else if (i < 54) { name = "Mtx " + juce::String(i-47); peak = m.matrices[(size_t)(i-48)].read(); }
            else if (i < 62) { name = "FX" + juce::String((i-54)/2+1) + ((i-54)%2 == 0 ? "L" : "R"); peak = m.fxReturns[(size_t)(i-54)].read(); }
            else if (i < 70) { name = "Aux " + juce::String(i-61); peak = m.auxes[(size_t)(i-62)].read(); }
            else if (i == 70) { name = "Main L"; peak = m.main.l.read(); }
            else if (i == 71) { name = "Main R"; peak = m.main.r.read(); }
            else { name = "M/C"; peak = m.mainC.read(); }
            const int x = (i % columns)*cellW + 8, y = 52 + (i / columns)*cellH;
            const float db = gainToDb (peak);
            g.setColour (juce::Colours::lightgrey);
            g.drawText (name + "  " + (db < -90 ? juce::String("-inf") : juce::String(db, 1)), x, y, cellW-16, 20, juce::Justification::centredLeft);
            g.setColour (juce::Colour (0xff303943)); g.fillRect (x, y+21, cellW-16, 5);
            g.setColour (peak >= 1.0f ? juce::Colours::red : juce::Colours::turquoise);
            g.fillRect (x, y+21, (int) ((cellW-16)*juce::jlimit(0.0f,1.0f,(db+60.0f)/60.0f)), 5);
        }
    }
private:
    void refreshSends()
    {
        auto& p = engine.getMixerParams();
        const int bus = source.getSelectedId() - 2;
        for (int i = 0; i < 6; ++i)
            sends[(size_t)i].setValue (bus < 0 ? p.main.matrixSends[(size_t)i].get() : p.buses[(size_t)bus].matrixSends[(size_t)i].get(), juce::dontSendNotification);
    }
    void timerCallback() override
    {
        const bool busy = exporter && !exporter->isFinished();
        exportMix.setEnabled(!busy); exportFormat.setEnabled(!busy); tailSeconds.setEnabled(!busy);
        cancelExport.setEnabled(busy);
        if (exporter && exporter->isFinished() && !exportReported)
        {
            exportStatus = exporter->getResult().wasOk() ? (exporter->clipped() ? "Export complete. Peaks reached clipping; lower the mix level for a clean export." : "Export complete.") : exporter->getResult().getErrorMessage();
            exportReported = true;
        }
        if (isShowing()) repaint();
    }
    AudioEngine& engine;
    int mode = 0;
    juce::ComboBox source, output, monitorMode, exportFormat;
    juce::Slider tailSeconds;
    juce::TextButton exportMix, cancelExport;
    std::unique_ptr<MixdownExporter> exporter;
    juce::String exportStatus = "Choose WAV or MP3, then Export Full Mix.";
    bool exportReported = false;
    juce::Slider level;
    juce::TextButton dim, mute, mono, record, stop;
    juce::String recordStatus = "Choose a new WAV file to begin.";
    std::unique_ptr<juce::FileChooser> chooser;
    std::array<juce::Slider, 6> sends;
};
} // namespace mps
