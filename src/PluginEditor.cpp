// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginEditor.h"
#include "BinaryData.h"

namespace {
// These colors and fonts follow Batchly's current site theme; the controls remain tactile.
const juce::Colour background(0xfff3f2f2), panel(0xffeae9e9), ink(0xff201e1d), muted(0xff646262),
    accent(0xffec3013), accentText(0xffae1800), rule(0xff9e9d9d);
juce::Font brandFont(float size, bool bold = false) {
    static const auto registered = juce::Typeface::createSystemTypefaceFor(BinaryData::Archivo_ttf, BinaryData::Archivo_ttfSize);
    return juce::Font(juce::FontOptions(registered->getName(), size, bold ? juce::Font::bold : juce::Font::plain));
}
juce::Font monoFont(float size) {
    static const auto face = juce::Typeface::createSystemTypefaceFor(BinaryData::JetBrainsMono_ttf, BinaryData::JetBrainsMono_ttfSize);
    static const auto medium = face->cloneWithVariableSettings(face->getNamedInstanceConfiguration("Medium"));
    return juce::Font(juce::FontOptions(medium != nullptr ? medium : face).withHeight(size * 1.23f));
}
const std::array<const char*, 9> ids { "depth", "rate", "wander", "tone", "follow", "noise", "width", "mix", "output" };
const std::array<const char*, 9> titles { "DEPTH", "RATE", "WANDER", "TONE", "FOLLOW", "NOISE", "WIDTH", "MIX", "OUTPUT" };
const std::array<const char*, 9> hints {
    "Amount of pitch movement. Small amounts thicken; larger amounts bend the pitch.",
    "Speed of the pitch movement in cycles per second.",
    "Blend a repeating sweep into smooth, unpredictable movement.",
    "Top-end cutoff of the wet signal. Turn left for a darker sound.",
    "Louder notes open the tone filter. Set to zero for a fixed tone.",
    "Add quiet generated hiss to the wet signal. Zero is completely silent.",
    "Difference between left and right movement. Mono tracks remain mono.",
    "Dry/wet balance. Near half is chorus; fully wet is vibrato.",
    "Final gain. Keep the output meter below 0 dB to avoid clipping."
};
void text(juce::Graphics& g, const juce::String& value, juce::Rectangle<int> area,
          float size, juce::Colour colour, bool bold = false, juce::Justification align = juce::Justification::centredLeft) {
    g.setColour(colour);
    g.setFont(size <= 11 ? monoFont(size) : brandFont(size, bold));
    g.drawText(value, area, align);
}
void screw(juce::Graphics& g, float x, float y) {
    g.setColour(juce::Colours::black.withAlpha(.4f)); g.fillEllipse(x - 4, y - 3, 9, 9);
    g.setColour(juce::Colour(0xff83837c)); g.fillEllipse(x - 4, y - 4, 8, 8);
    g.setColour(juce::Colour(0xff393b38)); g.drawLine(x - 2, y + 1, x + 2, y - 1, 1.3f);
}
}

DeckLookAndFeel::DeckLookAndFeel() {
    setColour(juce::Slider::textBoxTextColourId, ink);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setDefaultSansSerifTypefaceName(brandFont(14).getTypefaceName());
    setColour(juce::Slider::trackColourId, accent);
    setColour(juce::Slider::thumbColourId, accent);
    setColour(juce::ComboBox::backgroundColourId, background);
    setColour(juce::ComboBox::textColourId, ink);
    setColour(juce::ComboBox::outlineColourId, rule);
    setColour(juce::ComboBox::arrowColourId, ink);
    setColour(juce::PopupMenu::backgroundColourId, background);
    setColour(juce::PopupMenu::textColourId, ink);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, ink);
    setColour(juce::PopupMenu::highlightedTextColourId, background);
    setColour(juce::TextButton::textColourOffId, ink);
    setColour(juce::TextButton::textColourOnId, background);
    setColour(juce::TooltipWindow::backgroundColourId, ink);
    setColour(juce::TooltipWindow::textColourId, background);
}
juce::Font DeckLookAndFeel::getTextButtonFont(juce::TextButton&, int) { return brandFont(13, true); }
juce::Font DeckLookAndFeel::getComboBoxFont(juce::ComboBox&) { return brandFont(14); }
juce::Font DeckLookAndFeel::getLabelFont(juce::Label& label) {
    return dynamic_cast<juce::Slider*>(label.getParentComponent()) != nullptr ? monoFont(13) : label.getFont();
}
void DeckLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                     float position, float start, float end, juce::Slider&) {
    const auto area = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
        static_cast<float>(width), static_cast<float>(height)).reduced(12);
    const auto radius = std::min(area.getWidth(), area.getHeight()) * .5f;
    const auto centre = area.getCentre();
    const auto angle = start + position * (end - start);
    juce::Path track, active;
    track.addCentredArc(centre.x, centre.y, radius, radius, 0, start, end, true);
    active.addCentredArc(centre.x, centre.y, radius, radius, 0, start, angle, true);
    g.setColour(rule); g.strokePath(track, juce::PathStrokeType(2.5f));
    g.setColour(accent); g.strokePath(active, juce::PathStrokeType(3.0f));
    for (int tick = 0; tick <= 10; ++tick) {
        const float a = start + (end - start) * static_cast<float>(tick) / 10;
        const auto p1 = centre.getPointOnCircumference(radius + 5, a);
        const auto p2 = centre.getPointOnCircumference(radius + 8, a);
        g.setColour(muted); g.drawLine({ p1, p2 }, 1);
    }
    auto body = juce::Rectangle<float>(radius * 1.58f, radius * 1.58f).withCentre(centre);
    g.setColour(juce::Colours::black.withAlpha(.16f)); g.fillEllipse(body.translated(1, 5).expanded(2));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xfffafafa), body.getX(), body.getY(),
        juce::Colour(0xff8e8b89), body.getRight(), body.getBottom(), false));
    g.fillEllipse(body);
    g.setColour(juce::Colours::white.withAlpha(.7f)); g.drawEllipse(body.reduced(.8f), 1);
    const auto cap = body.reduced(body.getWidth() * .17f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff494645), cap.getX(), cap.getY(),
        juce::Colour(0xff201e1d), cap.getRight(), cap.getBottom(), false));
    g.fillEllipse(cap);
    const auto point = centre.getPointOnCircumference(radius * .61f, angle);
    const auto inner = centre.getPointOnCircumference(radius * .43f, angle);
    g.setColour(juce::Colour(0xffff6249)); g.drawLine({ inner, point }, 3);
}
void DeckLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                         const juce::Colour&, bool hover, bool down) {
    auto colour = button.getToggleState() ? ink : panel;
    if (hover) colour = colour.darker(.08f);
    if (down) colour = colour.darker(.15f);
    if (!button.isEnabled()) colour = colour.withAlpha(.3f);
    const auto bounds = button.getLocalBounds().toFloat().reduced(.5f);
    g.setColour(colour); g.fillRect(bounds);
    g.setColour(button.hasKeyboardFocus(true) ? accent : rule); g.drawRect(bounds, 1);
}

BatchlyEditor::BatchlyEditor(BatchlyProcessor& p) : AudioProcessorEditor(p), processor(p) {
    setLookAndFeel(&look);
    setSize(1000, 650);
    for (size_t i = 0; i < knobs.size(); ++i) {
        auto& knob = knobs[i];
        knob.setSliderStyle(i == 8 ? juce::Slider::LinearHorizontal : juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 94, 24);
        knob.setRotaryParameters(juce::MathConstants<float>::pi * 1.22f, juce::MathConstants<float>::pi * 2.78f, true);
        knob.setTooltip(hints[i]); knob.setName(titles[i]);
        attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, ids[i], knob);
        knob.textFromValueFunction = [i](double value) {
            if (i == 1) return juce::String(value, 2) + " Hz";
            if (i == 3) return value >= 1000 ? juce::String(value / 1000, 2) + " kHz" : juce::String(value, 0) + " Hz";
            if (i == 8) return juce::String(value, 1) + " dB";
            return juce::String(value * 100, 0) + " %";
        };
        knob.valueFromTextFunction = [i](const juce::String& value) {
            auto number = value.getDoubleValue();
            if (i == 3 && value.containsIgnoreCase("k")) return number * 1000;
            return (i == 1 || i == 3 || i == 8) ? number : number / 100;
        };
        addAndMakeVisible(knob);
        labels[i].setText(titles[i], juce::dontSendNotification);
        labels[i].setJustificationType(juce::Justification::centred);
        labels[i].setColour(juce::Label::textColourId, muted);
        labels[i].setFont(monoFont(11));
        addAndMakeVisible(labels[i]);
        knob.updateText();
        knob.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        knob.setDoubleClickReturnValue(true, processor.parameters.getParameter(ids[i])->convertFrom0to1(
            processor.parameters.getParameter(ids[i])->getDefaultValue()));
    }
    for (auto* button : { &bypass, &open, &play, &stop, &demo, &exportButton, &savePreset, &loadPreset, &updates }) addAndMakeVisible(*button);
    bypass.setClickingTogglesState(true);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters, "bypass", bypass);
    bypass.setTooltip("Compare with the original signal. The change is faded to prevent clicks.");
    for (int i = 0; i < processor.getNumPrograms(); ++i) presets.addItem(processor.getProgramName(i), i + 1);
    presets.setSelectedId(processor.getCurrentProgram() + 1, juce::dontSendNotification);
    presets.onChange = [this] { processor.setCurrentProgram(presets.getSelectedId() - 1); };
    presets.setTooltip("Choose a starting point, then adjust any control.");
    addAndMakeVisible(presets);
    open.onClick = [this] { chooseAudio(); };
    play.onClick = [this] { processor.play(); };
    stop.onClick = [this] { processor.stop(); };
    demo.onClick = [this] { processor.playDemo(); status.setText("Original synthesized demo. Adjust Mix to compare chorus and vibrato.", juce::dontSendNotification); };
    exportButton.onClick = [this] { chooseExport(); };
    savePreset.onClick = [this] { choosePreset(true); };
    loadPreset.onClick = [this] { choosePreset(false); };
    updates.onClick = [this] { checkUpdates(); };
    updates.setTooltip("Check GitHub for a new version. Downloads only begin when you choose Install.");
    status.setColour(juce::Label::textColourId, muted);
    status.setFont(brandFont(13));
    status.setText(processor.isStandalone() ? "Drop an audio file here, or try the built-in demo." : "Audio comes from your DAW track. Double-click a knob to reset it.", juce::dontSendNotification);
    addAndMakeVisible(status);
    for (auto* button : { &open, &play, &stop, &demo, &exportButton }) button->setVisible(processor.isStandalone());
    startTimerHz(30);
}
BatchlyEditor::~BatchlyEditor() {
    stopTimer();
    exportPool.removeAllJobs(true, -1);
    setLookAndFeel(nullptr);
}
void BatchlyEditor::resized() {
    presets.setBounds(565, 19, 210, 32); savePreset.setBounds(786, 19, 52, 32);
    loadPreset.setBounds(845, 19, 52, 32); bypass.setBounds(910, 19, 72, 32);
    knobs[0].setBounds(628, 181, 152, 163); labels[0].setBounds(628, 344, 152, 24);
    knobs[1].setBounds(804, 181, 152, 163); labels[1].setBounds(804, 344, 152, 24);
    for (size_t i = 2; i < 8; ++i) {
        const int x = 194 + static_cast<int>(i - 2) * 129;
        knobs[i].setBounds(x, 404, 112, 118); labels[i].setBounds(x, 525, 112, 20);
    }
    labels[8].setBounds(807, 560, 70, 20); knobs[8].setBounds(866, 546, 112, 54);
    open.setBounds(195, 560, 104, 32); demo.setBounds(307, 560, 62, 32);
    play.setBounds(377, 560, 56, 32); stop.setBounds(441, 560, 56, 32);
    exportButton.setBounds(507, 560, 105, 32);
    status.setBounds(191, 608, 780, 24);
    updates.setBounds(24, 516, 124, 28);
}
void BatchlyEditor::paint(juce::Graphics& g) {
    g.fillAll(background);
    g.setColour(panel); g.fillRect(0, 0, 172, getHeight());
    g.setColour(rule); g.drawVerticalLine(171, 0, 650);
    g.setColour(background); g.fillRect(172, 0, 828, 70);
    g.setColour(ink); g.fillRect(191, 69, 791, 2);
    text(g, "BATCHLY", { 24, 22, 135, 25 }, 23, ink, true);
    g.setColour(accent); g.fillRect(128, 40, 5, 5);
    text(g, "A U D I O", { 26, 47, 130, 18 }, 11, muted);
    text(g, "THE COLLECTION", { 24, 104, 136, 20 }, 10, muted, true);
    g.setColour(ink); g.fillRect(12, 139, 146, 74);
    g.setColour(accent); g.fillRect(12, 139, 4, 74);
    text(g, "01", { 26, 151, 30, 19 }, 11, background, true);
    text(g, "Drift", { 26, 172, 125, 26 }, 22, background, true);
    text(g, "CHORUS / VIBRATO", { 24, 229, 142, 18 }, 10, muted);
    text(g, "LOCAL AUDIO", { 24, 561, 132, 20 }, 10, muted, true);
    text(g, "Open source", { 24, 589, 125, 20 }, 13, ink);
    text(g, "v" + juce::String(batchly::UpdateService::currentVersion), { 24, 610, 140, 18 }, 9, muted);
    text(g, "AUDIO TOOLS / 001", { 196, 20, 340, 32 }, 11, muted);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffeeeceb), 192, 84, juce::Colour(0xffd4d2d0), 980, 159, false));
    g.fillRect(190, 87, 792, 70);
    for (int y = 90; y < 153; y += 3) {
        g.setColour(juce::Colours::white.withAlpha(.045f)); g.drawHorizontalLine(y, 198, 975);
    }
    text(g, "DRIFT", { 217, 91, 190, 61 }, 48, ink, true);
    g.setColour(accent); g.fillRect(396, 106, 3, 33);
    text(g, "RANDOM-MOTION CHORUS & VIBRATO", { 420, 107, 473, 20 }, 13, ink, true);
    text(g, "Slow movement. Soft edges. A little room to wander.", { 420, 129, 496, 18 }, 12, muted);
    screw(g, 203, 101); screw(g, 969, 101); screw(g, 203, 144); screw(g, 969, 144);
    const juce::Rectangle<float> scope(196, 184, 398, 182);
    g.setColour(ink); g.fillRect(scope);
    g.setColour(rule); g.drawRect(scope, 1);
    text(g, "PITCH MOVEMENT", { 213, 195, 210, 20 }, 10, juce::Colour(0xffb9b6b4), true);
    text(g, "L / R", { 540, 195, 43, 20 }, 10, background, true);
    g.saveState(); g.reduceClipRegion(207, 222, 376, 129);
    for (int x = 211; x < 584; x += 37) { g.setColour(juce::Colour(0xff383432)); g.drawVerticalLine(x, 222, 350); }
    for (int y = 230; y < 351; y += 30) { g.setColour(juce::Colour(0xff383432)); g.drawHorizontalLine(y, 207, 583); }
    auto drawHistory = [&](const std::array<float, 200>& history, juce::Colour colour) {
        juce::Path path;
        for (size_t i = 0; i < history.size(); ++i) {
            const float x = 208.0f + 375.0f * static_cast<float>(i) / 199;
            const float y = 286.0f - history[(historyPosition + i) % history.size()] * 56.0f;
            if (i == 0) path.startNewSubPath(x, y); else path.lineTo(x, y);
        }
        g.setColour(colour.withAlpha(.1f)); g.strokePath(path, juce::PathStrokeType(8));
        g.setColour(colour); g.strokePath(path, juce::PathStrokeType(1.6f));
    };
    drawHistory(rightHistory, background); drawHistory(leftHistory, accent); g.restoreState();
    text(g, "MOVEMENT", { 202, 380, 125, 18 }, 10, accentText, true);
    text(g, "CHARACTER", { 337, 380, 370, 18 }, 10, accentText, true);
    text(g, "IMAGE / BLEND", { 719, 380, 254, 18 }, 10, accentText, true);
    g.setColour(rule); g.drawHorizontalLine(552, 195, 977);
    g.setColour(panel); g.fillRect(636, 568, 142, 12);
    const float decibels = juce::Decibels::gainToDecibels(meter, -60.0f);
    g.setColour(meter >= 1 ? accent : ink);
    g.fillRect(636.0f, 568.0f, juce::jlimit(0.0f, 142.0f, (decibels + 60) / 60 * 142), 12.0f);
    text(g, meter < .001f ? "OUTPUT / SILENT" : "OUTPUT / " + juce::String(decibels, 1) + " dB",
         { 636, 585, 163, 16 }, 9, muted);
}
void BatchlyEditor::timerCallback() {
    leftHistory[historyPosition] = processor.motionLeft.load();
    rightHistory[historyPosition] = processor.motionRight.load();
    historyPosition = (historyPosition + 1) % leftHistory.size();
    meter = std::max(processor.peak.load(), meter * .9f);
    play.setEnabled(processor.loadedFile().existsAsFile() && !processor.isPlaying());
    stop.setEnabled(processor.isPlaying());
    exportButton.setEnabled(processor.loadedFile().existsAsFile() && !exporting);
    const auto startingPoint = processor.getProgramName(processor.getCurrentProgram())
        + (processor.isCurrentProgramModified() ? " *" : "");
    if (presets.getText() != startingPoint) presets.setText(startingPoint, juce::dontSendNotification);
    repaint();
}
bool BatchlyEditor::isInterestedInFileDrag(const juce::StringArray& files) {
    return processor.isStandalone() && files.size() == 1;
}
void BatchlyEditor::filesDropped(const juce::StringArray& files, int, int) { if (files.size() == 1) loadFile(juce::File(files[0])); }
void BatchlyEditor::report(const juce::Result& result, const juce::String& success) {
    status.setText(result.wasOk() ? success : result.getErrorMessage(), juce::dontSendNotification);
    if (result.failed()) juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Batchly Audio", result.getErrorMessage());
}
void BatchlyEditor::loadFile(const juce::File& file) { report(processor.loadAudioFile(file), "Loaded: " + file.getFileName() + "  |  Press Play to listen."); }
void BatchlyEditor::chooseAudio() {
    chooser = std::make_unique<juce::FileChooser>("Open audio", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
    auto safe = juce::Component::SafePointer<BatchlyEditor>(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safe](const juce::FileChooser& dialog) { if (safe && dialog.getResult().existsAsFile()) safe->loadFile(dialog.getResult()); });
}
void BatchlyEditor::chooseExport() {
    const auto source = processor.loadedFile();
    if (!source.existsAsFile()) return;
    chooser = std::make_unique<juce::FileChooser>("Export processed audio", source.getSiblingFile(source.getFileNameWithoutExtension() + " - Drift.wav"), "*.wav");
    auto safe = juce::Component::SafePointer<BatchlyEditor>(this);
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
        [safe, source](const juce::FileChooser& dialog) {
            if (!safe || dialog.getResult() == juce::File()) return;
            const auto destination = dialog.getResult().withFileExtension("wav");
            if (destination != dialog.getResult() && destination.existsAsFile()) {
                safe->report(juce::Result::fail("Choose the exact .wav filename so the file dialog can confirm replacement."), "");
                return;
            }
            const auto settings = safe->processor.readParameters();
            safe->exporting = true; safe->status.setText("Rendering the current settings...", juce::dontSendNotification);
            // Rendering has its own engine so exporting never blocks or changes live playback.
            safe->exportPool.addJob([safe, source, destination, settings] {
                const auto result = BatchlyProcessor::exportAudio(source, destination, settings);
                juce::MessageManager::callAsync([safe, result, destination] {
                    if (!safe) return;
                    safe->exporting = false;
                    safe->report(result, "Saved: " + destination.getFileName());
                });
            });
        });
}
void BatchlyEditor::choosePreset(bool save) {
    chooser = std::make_unique<juce::FileChooser>(save ? "Save your preset" : "Load a preset", juce::File(), "*.bapreset");
    auto safe = juce::Component::SafePointer<BatchlyEditor>(this);
    chooser->launchAsync((save ? juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting : juce::FileBrowserComponent::openMode)
                         | juce::FileBrowserComponent::canSelectFiles,
        [safe, save](const juce::FileChooser& dialog) {
            if (!safe || dialog.getResult() == juce::File()) return;
            if (save) {
                const auto destination = dialog.getResult().withFileExtension("bapreset");
                if (destination != dialog.getResult() && destination.existsAsFile()) {
                    safe->report(juce::Result::fail("Choose the exact .bapreset filename so the file dialog can confirm replacement."), "");
                    return;
                }
                juce::MemoryBlock state; safe->processor.getStateInformation(state);
                const bool ok = destination.replaceWithData(state.getData(), state.getSize());
                safe->report(ok ? juce::Result::ok() : juce::Result::fail("Could not write the preset."), "Preset saved.");
            } else {
                juce::MemoryBlock state;
                if (dialog.getResult().getSize() > 1024 * 1024 || !dialog.getResult().loadFileAsData(state)) {
                    safe->report(juce::Result::fail("This preset could not be read."), ""); return;
                }
                auto xml = juce::AudioProcessor::getXmlFromBinary(state.getData(), static_cast<int>(state.getSize()));
                if (!xml || !xml->hasTagName("BatchlyAudioState")) {
                    safe->report(juce::Result::fail("Choose a valid Batchly Audio preset."), ""); return;
                }
                safe->processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
                safe->presets.setText("User preset", juce::dontSendNotification);
                safe->report(juce::Result::ok(), "Preset loaded.");
            }
        });
}

void BatchlyEditor::checkUpdates() {
    updates.setEnabled(false); updates.setButtonText("Checking...");
    auto safe = juce::Component::SafePointer<BatchlyEditor>(this);
    exportPool.addJob([safe] {
        batchly::UpdateRelease release;
        const auto result = batchly::UpdateService::check(release);
        juce::MessageManager::callAsync([safe, result, release] {
            if (!safe) return;
            safe->updates.setEnabled(true); safe->updates.setButtonText("Updates");
            if (result.failed()) { safe->report(result, ""); return; }
            if (!release.available()) {
                safe->status.setText("You're up to date. Version " + juce::String(batchly::UpdateService::currentVersion), juce::dontSendNotification);
                return;
            }
            juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::InfoIcon, "Batchly Audio update",
                "Version " + release.version + " is available. Download and install it?\n\n"
                "Installation finishes after you close this app or DAW normally. Windows may ask for administrator approval.",
                "Install", "Later", safe.getComponent(), juce::ModalCallbackFunction::create([safe, release](int answer) {
                    if (safe && answer == 1) safe->installUpdate(release);
                }));
        });
    });
}

void BatchlyEditor::installUpdate(const batchly::UpdateRelease& release) {
    updates.setEnabled(false); updates.setButtonText("Downloading...");
    status.setText("Downloading and verifying the update...", juce::dontSendNotification);
    auto safe = juce::Component::SafePointer<BatchlyEditor>(this);
    const bool standalone = processor.isStandalone();
    exportPool.addJob([safe, release, standalone] {
        juce::File request;
        auto result = batchly::UpdateService::stage(release, request, standalone);
        if (result.wasOk()) result = batchly::UpdateService::launch(request);
        juce::MessageManager::callAsync([safe, result] {
            if (!safe) return;
            if (result.failed()) {
                safe->updates.setEnabled(true); safe->updates.setButtonText("Updates");
                safe->report(result, ""); return;
            }
            safe->updates.setButtonText("Ready on exit");
            safe->status.setText("Update ready. Save your work and close this app or DAW to finish installation.", juce::dontSendNotification);
        });
    });
}
