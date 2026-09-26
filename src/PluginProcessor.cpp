// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace {
const std::array<const char*, 10> ids { "depth", "rate", "wander", "tone", "follow", "noise", "width", "mix", "output", "bypass" };
const std::array<const char*, 5> presetNames { "Soft focus", "Slow tide", "Wide room", "Worn motor", "Pure vibrato" };
const float presets[5][9] {
    { .35f, .45f, .65f, 7000, 0, 0, .75f, .5f, 0 },
    { .65f, .14f, .9f, 4800, .2f, 0, .55f, .65f, -1 },
    { .22f, .85f, .15f, 12000, 0, 0, 1, .45f, 0 },
    { .85f, 1.7f, .95f, 2300, .55f, .22f, .4f, .8f, -2 },
    { .35f, 2.1f, .15f, 14500, 0, 0, 0, 1, 0 }
};
}

juce::AudioProcessorValueTreeState::ParameterLayout BatchlyProcessor::makeLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    auto add = [&](const char* id, const char* label, float lo, float hi, float value, float skew = 1.0f) {
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { id, 1 }, label,
            juce::NormalisableRange<float>(lo, hi, 0.0f, skew), value));
    };
    add("depth", "Depth", 0, 1, .35f);
    add("rate", "Rate (Hz)", .05f, 8, .45f, .35f);
    add("wander", "Wander", 0, 1, .65f);
    add("tone", "Tone (Hz)", 250, 18000, 7000, .35f);
    add("follow", "Envelope follow", 0, 1, 0);
    add("noise", "Noise", 0, 1, 0);
    add("width", "Stereo width", 0, 1, .75f);
    add("mix", "Mix", 0, 1, .5f);
    add("output", "Output (dB)", -24, 12, 0);
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "bypass", 1 }, "Bypass", false));
    return layout;
}

BatchlyProcessor::BatchlyProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "BatchlyAudioState", makeLayout()) {
    for (size_t i = 0; i < ids.size(); ++i) parameterValues[i] = parameters.getRawParameterValue(ids[i]);
    formats.registerBasicFormats();
    if (isStandalone()) getBus(true, 0)->enable(false);
}
BatchlyProcessor::~BatchlyProcessor() {
    transport.setSource(nullptr);
    readThread.stopThread(5000);
}
void BatchlyProcessor::prepareToPlay(double sampleRate, int blockSize) {
    engine.prepare(sampleRate, readParameters());
    if (isStandalone()) {
        readThread.startThread();
        transport.prepareToPlay(blockSize, sampleRate);
    }
}
void BatchlyProcessor::releaseResources() { transport.releaseResources(); }
bool BatchlyProcessor::isBusesLayoutSupported(const BusesLayout& layout) const {
    auto out = layout.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
        && (isStandalone() ? layout.getMainInputChannelSet().isDisabled() : layout.getMainInputChannelSet() == out);
}
batchly::DriftParameters BatchlyProcessor::readParameters() const noexcept {
    batchly::DriftParameters p;
    p.depth = parameterValues[0]->load(); p.rateHz = parameterValues[1]->load();
    p.wander = parameterValues[2]->load(); p.toneHz = parameterValues[3]->load();
    p.follow = parameterValues[4]->load(); p.noise = parameterValues[5]->load();
    p.width = parameterValues[6]->load(); p.mix = parameterValues[7]->load();
    p.outputDb = parameterValues[8]->load(); p.bypass = parameterValues[9]->load() >= .5f;
    return p;
}
void BatchlyProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    if (buffer.getNumSamples() == 0) return;
    if (isStandalone()) {
        // The desktop version plays files; it never routes an open microphone to speakers.
        buffer.clear();
        if (demoPlaying.load()) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                const double beat = std::fmod(demoTime, 1.5);
                const double fade = (1.0 - std::exp(-beat * 100.0)) * std::exp(-beat * 2.8);
                float value = 0;
                for (double frequency : { 220.0, 261.625565, 329.627557, 391.995436 })
                    value += static_cast<float>(.035 * fade * (std::sin(juce::MathConstants<double>::twoPi * frequency * demoTime)
                        + .24 * std::sin(juce::MathConstants<double>::twoPi * 2.0 * frequency * demoTime)));
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch) buffer.setSample(ch, i, value);
                demoTime += 1.0 / getSampleRate();
            }
        } else {
            demoTime = 0;
            transport.getNextAudioBlock({ &buffer, 0, buffer.getNumSamples() });
        }
    }
    engine.process(buffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples(), readParameters());
    float maximum = 0;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        maximum = std::max(maximum, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
    peak.store(maximum);
    const auto motion = engine.getModulation();
    motionLeft.store(motion[0]); motionRight.store(motion[1]);
}
juce::AudioProcessorParameter* BatchlyProcessor::getBypassParameter() const { return parameters.getParameter("bypass"); }
juce::AudioProcessorEditor* BatchlyProcessor::createEditor() { return new BatchlyEditor(*this); }
const juce::String BatchlyProcessor::getProgramName(int index) { return presetNames[static_cast<size_t>(juce::jlimit(0, 4, index))]; }
void BatchlyProcessor::setCurrentProgram(int index) {
    index = juce::jlimit(0, 4, index);
    for (size_t i = 0; i < 9; ++i) {
        auto* parameter = parameters.getParameter(ids[i]);
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(presets[index][i]));
        parameter->endChangeGesture();
    }
    currentProgram.store(index);
}
bool BatchlyProcessor::isCurrentProgramModified() const {
    const auto index = static_cast<size_t>(juce::jlimit(0, 4, currentProgram.load()));
    for (size_t i = 0; i < 9; ++i) {
        const auto* parameter = parameters.getParameter(ids[i]);
        const float expected = parameter->convertTo0to1(presets[index][i]);
        if (std::abs(parameter->getValue() - expected) > .0001f) return true;
    }
    return false;
}
void BatchlyProcessor::getStateInformation(juce::MemoryBlock& data) {
    auto state = parameters.copyState();
    state.setProperty("schemaVersion", 1, nullptr);
    state.setProperty("program", currentProgram.load(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, data);
}
void BatchlyProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size)) {
        if (xml->hasTagName(parameters.state.getType())) {
            auto state = juce::ValueTree::fromXml(*xml);
            currentProgram.store(juce::jlimit(0, 4, static_cast<int>(state.getProperty("program", 0))));
            parameters.replaceState(state);
        }
    }
}
juce::Result BatchlyProcessor::loadAudioFile(const juce::File& file) {
    if (!isStandalone()) return juce::Result::fail("Load audio on your DAW track.");
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
    if (!reader || reader->lengthInSamples <= 0 || reader->sampleRate < 8000 || reader->sampleRate > 384000
        || reader->numChannels < 1 || reader->numChannels > 2)
        return juce::Result::fail("Choose a readable mono or stereo audio file (8-384 kHz).");
    const auto sampleRate = reader->sampleRate;
    demoPlaying.store(false);
    auto next = std::make_unique<juce::AudioFormatReaderSource>(reader.release(), true);
    transport.setSource(nullptr);
    readerSource = std::move(next);
    transport.setSource(readerSource.get(), 32768, &readThread, sampleRate);
    sourceFile = file;
    return juce::Result::ok();
}
void BatchlyProcessor::play() {
    if (!readerSource) return;
    if (transport.getCurrentPosition() >= transport.getLengthInSeconds() - .01) transport.setPosition(0);
    demoPlaying.store(false);
    transport.start();
}
void BatchlyProcessor::stop() { demoPlaying.store(false); transport.stop(); transport.setPosition(0); }

juce::Result BatchlyProcessor::exportAudio(const juce::File& source, const juce::File& destination,
                                          const batchly::DriftParameters& settings) {
    if (source == destination) return juce::Result::fail("Choose a new filename to preserve your original audio.");
    juce::AudioFormatManager manager; manager.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(manager.createReaderFor(source));
    if (!reader || reader->numChannels < 1 || reader->numChannels > 2 || reader->sampleRate < 8000
        || reader->sampleRate > 384000 || reader->lengthInSamples <= 0)
        return juce::Result::fail("The source audio could not be read.");
    juce::TemporaryFile temporary(destination);
    std::unique_ptr<juce::OutputStream> output(temporary.getFile().createOutputStream());
    if (!output) return juce::Result::fail("The destination folder could not be written.");
    juce::WavAudioFormat wav;
    auto writer = wav.createWriterFor(output, juce::AudioFormatWriterOptions().withSampleRate(reader->sampleRate)
        .withNumChannels(2).withBitsPerSample(24));
    if (!writer) return juce::Result::fail("The WAV writer could not be created.");
    batchly::DriftEngine renderEngine; renderEngine.prepare(reader->sampleRate, settings);
    juce::AudioBuffer<float> buffer(2, 4096);
    const auto total = reader->lengthInSamples + static_cast<juce::int64>(reader->sampleRate * .08);
    bool clipped = false;
    for (juce::int64 position = 0; position < total; position += 4096) {
        const auto length = static_cast<int>(std::min<juce::int64>(4096, total - position));
        buffer.clear();
        const int readable = static_cast<int>(std::min<juce::int64>(length, std::max<juce::int64>(0, reader->lengthInSamples - position)));
        if (readable > 0 && !reader->read(&buffer, 0, readable, position, true, true))
            return juce::Result::fail("Reading the source failed. No final WAV was saved.");
        renderEngine.process(buffer.getArrayOfWritePointers(), 2, length, settings);
        clipped = clipped || buffer.getMagnitude(0, length) > 1.0f;
        if (!writer->writeFromAudioSampleBuffer(buffer, 0, length))
            return juce::Result::fail("Writing the WAV failed. Check free disk space.");
    }
    writer.reset();
    if (clipped) return juce::Result::fail("The output would clip. Lower Output and export again. No final WAV was saved.");
    if (!temporary.overwriteTargetFileWithTemporary()) return juce::Result::fail("Could not save the finished WAV.");
    return juce::Result::ok();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BatchlyProcessor(); }
