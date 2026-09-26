// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace {
const std::array<const char*, 10> ids { "depth", "rate", "wander", "tone", "follow", "noise", "width", "mix", "output", "bypass" };
const std::array<const char*, 15> presetNames { "Soft focus", "Slow tide", "Wide room", "Worn motor", "Pure vibrato",
    "Fresh spool", "Pocket cassette", "Submerged", "Sun-bleached", "Midnight dub",
    "Open atrium", "Close walls", "Velvet hall", "Glass canopy", "After hours" };
const std::array<const char*, 3> enabledIds { "drift_enabled", "patina_enabled", "atrium_enabled" };
const std::array<const char*, 8> reverbIds { "atrium_decay", "atrium_size", "atrium_predelay", "atrium_damping",
    "atrium_lowcut", "atrium_motion", "atrium_width", "atrium_mix" };
const float reverbPresets[5][8] {
    { 2.4f, .55f, 24, 6500, 120, .2f, 1, .25f },
    { .55f, .15f, 3, 9500, 180, .05f, .65f, .2f },
    { 4.8f, .75f, 42, 2200, 180, .3f, .9f, .32f },
    { 3.2f, .65f, 35, 14000, 220, .55f, 1, .3f },
    { 9, .95f, 80, 4200, 280, .8f, 1, .45f }
};
const std::array<const char*, 8> tapeIds { "patina_sample", "patina_drive", "patina_wear", "patina_flutter",
    "patina_hiss", "patina_chorus", "patina_tone", "patina_mix" };
const float tapePresets[5][8] {
    { 16000, .25f, .2f, .12f, 0, .18f, 11000, 1 },
    { 12500, .42f, .42f, .34f, .14f, .1f, 5600, 1 },
    { 4500, .18f, .1f, .04f, 0, .08f, 1800, 1 },
    { 22000, .38f, .3f, .2f, 0, .62f, 9000, 1 },
    { 8500, .7f, .52f, .25f, .1f, .32f, 3400, 1 }
};
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
    // Append parameters so the original Drift parameter order and IDs stay stable.
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "drift_enabled", 1 }, "Drift enabled", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "patina_enabled", 1 }, "Patina enabled", false));
    add("patina_sample", "Patina sample rate (Hz)", 2000, 48000, 16000, .35f);
    add("patina_drive", "Patina drive", 0, 1, .25f);
    add("patina_wear", "Patina wear", 0, 1, .2f);
    add("patina_flutter", "Patina flutter", 0, 1, .12f);
    add("patina_hiss", "Patina hiss", 0, 1, 0);
    add("patina_chorus", "Patina chorus", 0, 1, .18f);
    add("patina_tone", "Patina tone (Hz)", 400, 18000, 11000, .35f);
    add("patina_mix", "Patina mix", 0, 1, 1);
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "atrium_enabled", 1 }, "Atrium enabled", false));
    add("atrium_decay", "Atrium decay (s)", .2f, 12, 2.4f, .5f);
    add("atrium_size", "Atrium size", 0, 1, .55f);
    add("atrium_predelay", "Atrium pre-delay (ms)", 0, 250, 24, .6f);
    add("atrium_damping", "Atrium damping (Hz)", 500, 18000, 6500, .35f);
    add("atrium_lowcut", "Atrium low cut (Hz)", 20, 2000, 120, .35f);
    add("atrium_motion", "Atrium motion", 0, 1, .2f);
    add("atrium_width", "Atrium width", 0, 1, 1);
    add("atrium_mix", "Atrium mix", 0, 1, .25f);
    return layout;
}

BatchlyProcessor::BatchlyProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "BatchlyAudioState", makeLayout()) {
    for (size_t i = 0; i < ids.size(); ++i) parameterValues[i] = parameters.getRawParameterValue(ids[i]);
    for (size_t i = 0; i < tapeIds.size(); ++i) tapeValues[i] = parameters.getRawParameterValue(tapeIds[i]);
    for (size_t i = 0; i < reverbIds.size(); ++i) reverbValues[i] = parameters.getRawParameterValue(reverbIds[i]);
    driftEnabled = parameters.getRawParameterValue("drift_enabled");
    patinaEnabled = parameters.getRawParameterValue("patina_enabled");
    atriumEnabled = parameters.getRawParameterValue("atrium_enabled");
    formats.registerBasicFormats();
    if (isStandalone()) getBus(true, 0)->enable(false);
}
BatchlyProcessor::~BatchlyProcessor() {
    transport.setSource(nullptr);
    readThread.stopThread(5000);
}
void BatchlyProcessor::prepareToPlay(double sampleRate, int blockSize) {
    engine.prepare(sampleRate, readRackParameters());
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
batchly::RackParameters BatchlyProcessor::readRackParameters() const noexcept {
    batchly::RackParameters p;
    p.drift = readParameters(); p.driftEnabled = driftEnabled->load() >= .5f;
    p.patina.sampleHz = tapeValues[0]->load(); p.patina.drive = tapeValues[1]->load();
    p.patina.wear = tapeValues[2]->load(); p.patina.flutter = tapeValues[3]->load();
    p.patina.hiss = tapeValues[4]->load(); p.patina.chorus = tapeValues[5]->load();
    p.patina.toneHz = tapeValues[6]->load(); p.patina.mix = tapeValues[7]->load();
    p.patina.enabled = patinaEnabled->load() >= .5f;
    p.atrium.decaySeconds = reverbValues[0]->load(); p.atrium.size = reverbValues[1]->load();
    p.atrium.preDelayMs = reverbValues[2]->load(); p.atrium.dampingHz = reverbValues[3]->load();
    p.atrium.lowCutHz = reverbValues[4]->load(); p.atrium.motion = reverbValues[5]->load();
    p.atrium.width = reverbValues[6]->load(); p.atrium.mix = reverbValues[7]->load();
    p.atrium.enabled = atriumEnabled->load() >= .5f;
    return p;
}
double BatchlyProcessor::getTailLengthSeconds() const {
    const auto settings = readRackParameters();
    return settings.atrium.enabled ? batchly::AtriumEngine::tailSeconds(settings.atrium) : .16;
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
    engine.process(buffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples(), readRackParameters());
    float maximum = 0;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        maximum = std::max(maximum, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
    peak.store(maximum);
    const auto motion = engine.driftMotion();
    motionLeft.store(motion[0]); motionRight.store(motion[1]);
    tapeMovement.store(engine.tapeMotion()[0]);
    reverbLevel.store(engine.reverbPeak());
}
juce::AudioProcessorParameter* BatchlyProcessor::getBypassParameter() const { return parameters.getParameter("bypass"); }
juce::AudioProcessorEditor* BatchlyProcessor::createEditor() { return new BatchlyEditor(*this); }
const juce::String BatchlyProcessor::getProgramName(int index) { return presetNames[static_cast<size_t>(juce::jlimit(0, 14, index))]; }
void BatchlyProcessor::setCurrentProgram(int index) {
    index = juce::jlimit(0, 14, index);
    setModuleProgram(index / 5, index % 5);
    // Host program selection recalls a complete single-effect starting point.
    for (int module = 0; module < 3; ++module)
        if (module != index / 5) parameters.getParameter(enabledIds[module])->setValueNotifyingHost(0);
    selectModule(index / 5);
}
void BatchlyProcessor::setModuleProgram(int module, int index) {
    index = juce::jlimit(0, 4, index);
    module = juce::jlimit(0, 2, module);
    const size_t count = module == 0 ? 9 : 8;
    for (size_t i = 0; i < count; ++i) {
        auto* parameter = parameters.getParameter(module == 0 ? ids[i] : module == 1 ? tapeIds[i] : reverbIds[i]);
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(module == 0 ? presets[index][i] : module == 1 ? tapePresets[index][i] : reverbPresets[index][i]));
        parameter->endChangeGesture();
    }
    auto* enabled = parameters.getParameter(enabledIds[module]);
    enabled->beginChangeGesture(); enabled->setValueNotifyingHost(1); enabled->endChangeGesture();
    (module == 0 ? driftProgram : module == 1 ? patinaProgram : atriumProgram).store(index);
    currentProgram.store(index + module * 5);
}
int BatchlyProcessor::getDisplayedProgram() const {
    const int module = selectedModule();
    return module * 5 + (module == 0 ? driftProgram.load() : module == 1 ? patinaProgram.load() : atriumProgram.load());
}
bool BatchlyProcessor::isCurrentProgramModified() const {
    const int module = selectedModule();
    const auto index = static_cast<size_t>(getDisplayedProgram() % 5);
    for (size_t i = 0; i < (module == 0 ? 9u : 8u); ++i) {
        const auto* parameter = parameters.getParameter(module == 0 ? ids[i] : module == 1 ? tapeIds[i] : reverbIds[i]);
        const float expected = parameter->convertTo0to1(module == 0 ? presets[index][i] : module == 1 ? tapePresets[index][i] : reverbPresets[index][i]);
        if (std::abs(parameter->getValue() - expected) > .0001f) return true;
    }
    return false;
}
void BatchlyProcessor::getStateInformation(juce::MemoryBlock& data) {
    auto state = parameters.copyState();
    state.setProperty("schemaVersion", 3, nullptr);
    state.setProperty("program", currentProgram.load(), nullptr);
    state.setProperty("driftProgram", driftProgram.load(), nullptr);
    state.setProperty("patinaProgram", patinaProgram.load(), nullptr);
    state.setProperty("atriumProgram", atriumProgram.load(), nullptr);
    state.setProperty("editorModule", editorModule.load(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, data);
}
void BatchlyProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size)) {
        if (xml->hasTagName(parameters.state.getType())) {
            auto state = juce::ValueTree::fromXml(*xml);
            currentProgram.store(juce::jlimit(0, 14, static_cast<int>(state.getProperty("program", 0))));
            driftProgram.store(juce::jlimit(0, 4, static_cast<int>(state.getProperty("driftProgram", currentProgram.load() % 5))));
            patinaProgram.store(juce::jlimit(0, 4, static_cast<int>(state.getProperty("patinaProgram", 0))));
            atriumProgram.store(juce::jlimit(0, 4, static_cast<int>(state.getProperty("atriumProgram", 0))));
            editorModule.store(juce::jlimit(0, 2, static_cast<int>(state.getProperty("editorModule", 0))));
            // Older projects have no new parameters. Restore their defaults, not the
            // previous instance's active effects, when loading a legacy state.
            for (auto* parameter : getParameters()) {
                if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter)) {
                    if (!state.getChildWithProperty("id", ranged->paramID).isValid()) {
                        juce::ValueTree child("PARAM");
                        child.setProperty("id", ranged->paramID, nullptr);
                        child.setProperty("value", ranged->convertFrom0to1(ranged->getDefaultValue()), nullptr);
                        state.appendChild(child, nullptr);
                    }
                }
            }
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
                                          const batchly::RackParameters& settings) {
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
    batchly::RackEngine renderEngine; renderEngine.prepare(reader->sampleRate, settings);
    juce::AudioBuffer<float> buffer(2, 4096);
    const double tail = settings.atrium.enabled ? batchly::AtriumEngine::tailSeconds(settings.atrium)
        : settings.patina.enabled ? .16 : .08;
    const auto total = reader->lengthInSamples + static_cast<juce::int64>(reader->sampleRate * tail);
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
