// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace {
const std::array<const char*, 10> ids { "depth", "rate", "wander", "tone", "follow", "noise", "width", "mix", "output", "bypass" };
const auto& tapeIds = batchly::factoryIds[1];
const auto& reverbIds = batchly::factoryIds[2];
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
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "chime_enabled", 1 }, "Chime enabled", false));
    add("chime_ring", "Chime ring (s)", .08f, 6, 1.6f, .5f);
    add("chime_color", "Chime color (Hz)", 500, 16000, 8000, .35f);
    add("chime_drive", "Chime drive", 0, 1, .12f);
    add("chime_spread", "Chime spread", 0, 1, .5f);
    add("chime_detune", "Chime detune", 0, 1, .25f);
    add("chime_motion", "Chime motion", 0, 1, .15f);
    add("chime_width", "Chime width", 0, 1, .85f);
    add("chime_mix", "Chime mix", 0, 1, .4f);
    juce::StringArray notes, scales;
    for (const auto* note : batchly::ChimeEngine::noteNames) notes.add(note);
    for (const auto* scale : batchly::ChimeEngine::scaleNames) scales.add(scale);
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "chime_root", 1 }, "Chime root", notes, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "chime_scale", 1 }, "Chime scale", scales, 1));
    layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID { "chime_octave", 1 }, "Chime octave", 2, 4, 3));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "helix_enabled", 1 }, "Helix enabled", false));
    add("helix_rate", "Helix rate (Hz)", .03f, 8, .24f, .35f);
    add("helix_depth", "Helix depth", 0, 1, .65f);
    add("helix_feedback", "Helix feedback", -.85f, .85f, .35f);
    add("helix_center", "Helix center (Hz)", 100, 6000, 650, .35f);
    add("helix_tone", "Helix tone (Hz)", 500, 18000, 11000, .35f);
    add("helix_drive", "Helix drive", 0, 1, .1f);
    add("helix_width", "Helix width", 0, 1, .75f);
    add("helix_mix", "Helix mix", 0, 1, .5f);
    return layout;
}

BatchlyProcessor::BatchlyProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "BatchlyAudioState", makeLayout()) {
    for (size_t i = 0; i < ids.size(); ++i) parameterValues[i] = parameters.getRawParameterValue(ids[i]);
    for (size_t i = 0; i < tapeValues.size(); ++i) tapeValues[i] = parameters.getRawParameterValue(tapeIds[i]);
    for (size_t i = 0; i < reverbValues.size(); ++i) reverbValues[i] = parameters.getRawParameterValue(reverbIds[i]);
    driftEnabled = parameters.getRawParameterValue("drift_enabled");
    patinaEnabled = parameters.getRawParameterValue("patina_enabled");
    atriumEnabled = parameters.getRawParameterValue("atrium_enabled");
    chimeEnabled = parameters.getRawParameterValue("chime_enabled");
    for (size_t i = 0; i < chimeValues.size(); ++i) chimeValues[i] = parameters.getRawParameterValue(batchly::factoryIds[3][i]);
    helixEnabled = parameters.getRawParameterValue("helix_enabled");
    for (size_t i = 0; i < helixValues.size(); ++i) helixValues[i] = parameters.getRawParameterValue(batchly::factoryIds[4][i]);
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
    p.chime.ringSeconds = chimeValues[0]->load(); p.chime.colorHz = chimeValues[1]->load();
    p.chime.drive = chimeValues[2]->load(); p.chime.spread = chimeValues[3]->load();
    p.chime.detune = chimeValues[4]->load(); p.chime.motion = chimeValues[5]->load();
    p.chime.width = chimeValues[6]->load(); p.chime.mix = chimeValues[7]->load();
    p.chime.root = static_cast<int>(chimeValues[8]->load()); p.chime.scale = static_cast<int>(chimeValues[9]->load());
    p.chime.octave = static_cast<int>(chimeValues[10]->load()); p.chime.enabled = chimeEnabled->load() >= .5f;
    p.helix.enabled = helixEnabled->load() > .5f;
    p.helix.rateHz = helixValues[0]->load(); p.helix.depth = helixValues[1]->load();
    p.helix.feedback = helixValues[2]->load(); p.helix.centerHz = helixValues[3]->load();
    p.helix.toneHz = helixValues[4]->load(); p.helix.drive = helixValues[5]->load();
    p.helix.width = helixValues[6]->load(); p.helix.mix = helixValues[7]->load();
    return p;
}
double BatchlyProcessor::getTailLengthSeconds() const {
    const auto settings = readRackParameters();
    return batchly::RackEngine::tailSeconds(settings);
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
    const auto sweep = engine.helixSweep(); phaserLeft.store(sweep[0]); phaserRight.store(sweep[1]);
    const auto levels = engine.chimeLevels();
    for (size_t i = 0; i < levels.size(); ++i) resonatorLevels[i].store(levels[i]);
}
juce::AudioProcessorParameter* BatchlyProcessor::getBypassParameter() const { return parameters.getParameter("bypass"); }
juce::AudioProcessorEditor* BatchlyProcessor::createEditor() { return new BatchlyEditor(*this); }
const juce::String BatchlyProcessor::getProgramName(int index) {
    index = juce::jlimit(0, getNumPrograms() - 1, index);
    return batchly::presetNames[index / 5][index % 5];
}
void BatchlyProcessor::setCurrentProgram(int index) {
    index = juce::jlimit(0, getNumPrograms() - 1, index);
    setModuleProgram(index / 5, index % 5);
    // DAW program selection recalls a single-effect starting point. The on-screen
    // preset menu only edits its selected module and preserves the other effects.
    for (int module = 0; module < batchly::moduleCount; ++module)
        if (module != index / 5) parameters.getParameter(batchly::enabledIds[module])->setValueNotifyingHost(0);
    selectModule(index / 5);
}
void BatchlyProcessor::setModuleProgram(int module, int index) {
    module = juce::jlimit(0, batchly::moduleCount - 1, module);
    index = juce::jlimit(0, 4, index);
    for (int control = 0; control < batchly::factoryControlCount[module]; ++control) {
        auto* parameter = parameters.getParameter(batchly::factoryIds[module][control]);
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(batchly::factoryValues[module][index][control]));
        parameter->endChangeGesture();
    }
    auto* enabled = parameters.getParameter(batchly::enabledIds[module]);
    enabled->beginChangeGesture(); enabled->setValueNotifyingHost(1); enabled->endChangeGesture();
    modulePrograms[module].store(index);
    currentProgram.store(module * 5 + index);
}
int BatchlyProcessor::getDisplayedProgram() const {
    const int module = selectedModule();
    return module * 5 + modulePrograms[module].load();
}
bool BatchlyProcessor::isCurrentProgramModified() const {
    const int module = selectedModule(), program = modulePrograms[module].load();
    for (int control = 0; control < batchly::factoryControlCount[module]; ++control) {
        const auto* parameter = parameters.getParameter(batchly::factoryIds[module][control]);
        const float expected = parameter->convertTo0to1(batchly::factoryValues[module][program][control]);
        if (std::abs(parameter->getValue() - expected) > .0001f) return true;
    }
    return false;
}
void BatchlyProcessor::getStateInformation(juce::MemoryBlock& data) {
    auto state = parameters.copyState();
    state.setProperty("schemaVersion", 5, nullptr);
    state.setProperty("program", currentProgram.load(), nullptr);
    for (int module = 0; module < batchly::moduleCount; ++module)
        state.setProperty(batchly::programKeys[module], modulePrograms[module].load(), nullptr);
    state.setProperty("editorModule", editorModule.load(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, data);
}
void BatchlyProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size)) {
        if (xml->hasTagName(parameters.state.getType())) {
            auto state = juce::ValueTree::fromXml(*xml);
            currentProgram.store(juce::jlimit(0, getNumPrograms() - 1, static_cast<int>(state.getProperty("program", 0))));
            for (int module = 0; module < batchly::moduleCount; ++module) {
                const int fallback = module == 0 ? currentProgram.load() % 5 : 0;
                modulePrograms[module].store(juce::jlimit(0, 4, static_cast<int>(state.getProperty(batchly::programKeys[module], fallback))));
            }
            editorModule.store(juce::jlimit(0, batchly::moduleCount - 1, static_cast<int>(state.getProperty("editorModule", 0))));
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
    const double tail = batchly::RackEngine::tailSeconds(settings);
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
