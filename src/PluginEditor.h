// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "PluginProcessor.h"
#include "UpdateService.h"

class DeckLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    DeckLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    juce::Font getTextButtonFont(juce::TextButton&, int) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    juce::Font getLabelFont(juce::Label&) override;
};

class BatchlyEditor final : public juce::AudioProcessorEditor, private juce::Timer,
                            public juce::FileDragAndDropTarget {
public:
    explicit BatchlyEditor(BatchlyProcessor&);
    ~BatchlyEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray&, int, int) override;
private:
    void timerCallback() override;
    void loadFile(const juce::File&);
    void chooseAudio();
    void chooseExport();
    void choosePreset(bool save);
    void checkUpdates();
    void installUpdate(const batchly::UpdateRelease&);
    void report(const juce::Result&, const juce::String& success);
    BatchlyProcessor& processor;
    DeckLookAndFeel look;
    std::array<juce::Slider, 9> knobs;
    std::array<juce::Label, 9> labels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 9> attachments;
    juce::TextButton bypass { "BYPASS" }, open { "Open audio" }, play { "Play" }, stop { "Stop" },
        demo { "Demo" }, exportButton { "Export WAV" }, savePreset { "Save" }, loadPreset { "Load" }, updates { "Updates" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    juce::ComboBox presets;
    juce::Label status;
    juce::TooltipWindow tooltips { this, 700 };
    std::unique_ptr<juce::FileChooser> chooser;
    juce::ThreadPool exportPool { 1 };
    bool exporting = false;
    std::array<float, 200> leftHistory {}, rightHistory {};
    size_t historyPosition = 0;
    float meter = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BatchlyEditor)
};
