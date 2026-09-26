// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "RackEngine.h"

class BatchlyProcessor final : public juce::AudioProcessor {
public:
    BatchlyProcessor();
    ~BatchlyProcessor() override;
    void prepareToPlay(double sampleRate, int blockSize) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Batchly Audio"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;
    int getNumPrograms() override { return 15; }
    int getCurrentProgram() override { return currentProgram.load(); }
    void setCurrentProgram(int index) override;
    bool isCurrentProgramModified() const;
    int getDisplayedProgram() const;
    void setModuleProgram(int module, int index);
    int selectedModule() const { return editorModule.load(); }
    void selectModule(int module) { editorModule.store(juce::jlimit(0, 2, module)); }
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorParameter* getBypassParameter() const override;
    batchly::DriftParameters readParameters() const noexcept;
    batchly::RackParameters readRackParameters() const noexcept;

    bool isStandalone() const { return wrapperType == wrapperType_Standalone; }
    juce::Result loadAudioFile(const juce::File& file);
    void play();
    void stop();
    void playDemo() { transport.stop(); demoPlaying.store(true); }
    bool isPlaying() const { return transport.isPlaying() || demoPlaying.load(); }
    double playbackPosition() const { return transport.getCurrentPosition(); }
    double duration() const { return transport.getLengthInSeconds(); }
    const juce::File& loadedFile() const { return sourceFile; }
    static juce::Result exportAudio(const juce::File&, const juce::File&, const batchly::RackParameters&);
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float> peak { 0 }, motionLeft { 0 }, motionRight { 0 };
    std::atomic<float> tapeMovement { 0 };
    std::atomic<float> reverbLevel { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();
    std::array<std::atomic<float>*, 10> parameterValues {};
    std::array<std::atomic<float>*, 8> tapeValues {};
    std::array<std::atomic<float>*, 8> reverbValues {};
    std::atomic<float>* driftEnabled = nullptr;
    std::atomic<float>* patinaEnabled = nullptr;
    std::atomic<float>* atriumEnabled = nullptr;
    batchly::RackEngine engine;
    std::atomic<int> currentProgram { 0 };
    std::atomic<int> driftProgram { 0 }, patinaProgram { 0 }, atriumProgram { 0 }, editorModule { 0 };
    juce::AudioFormatManager formats;
    juce::TimeSliceThread readThread { "Audio file read-ahead" };
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transport;
    juce::File sourceFile;
    std::atomic<bool> demoPlaying { false };
    double demoTime = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BatchlyProcessor)
};
