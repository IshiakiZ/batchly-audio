// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginProcessor.h"
#include <iostream>
#include <stdexcept>

void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void descendants(juce::Component& parent, std::vector<juce::Component*>& result) {
    for (auto* child : parent.getChildren()) { result.push_back(child); descendants(*child, result); }
}
int main(int argc, char** argv) {
    // Construct the real JUCE components without adding a native desktop window.
    // This checks/render them without mouse input, focus changes or a sound device.
    juce::ScopedJuceInitialiser_GUI gui;
    try {
        const auto destination = juce::File::getCurrentWorkingDirectory().getChildFile(argc > 1 ? argv[1] : "editor-previews");
        require(destination.createDirectory().wasOk(), "Cannot create preview directory");
        for (bool standalone : {false, true}) {
            juce::AudioProcessor::setTypeOfNextNewPlugin(standalone ? juce::AudioProcessor::wrapperType_Standalone : juce::AudioProcessor::wrapperType_VST3);
            BatchlyProcessor processor;
            for (int module = 0; module < batchly::moduleCount; ++module) {
                processor.setCurrentProgram(module * batchly::presetsPerModule + 2);
                std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
                std::vector<juce::Component*> children; descendants(*editor, children);
                int knobs = 0, tabs = 0;
                juce::ComboBox* presetMenu = nullptr;
                juce::Viewport* collection = nullptr;
                juce::TextButton* selectedTab = nullptr;
                for (auto* child : children) {
                    if (auto* slider = dynamic_cast<juce::Slider*>(child)) {
                        ++knobs;
                        require(editor->getLocalBounds().contains(editor->getLocalArea(slider,slider->getLocalBounds())), "A control is outside the editor");
                        const auto formatted = slider->getTextFromValue(slider->getValue());
                        require(formatted.isNotEmpty(), "A control has no formatted value");
                        if (module == 6 && slider->getName() == "TIME") {
                            require(formatted.contains("ms"), "Relay time has the wrong unit");
                            slider->setValue(slider->getValue()+37,juce::sendNotificationSync);
                            require(std::abs(processor.readRackParameters().relay.timeMs-slider->getValue()) < .01, "Relay time control is disconnected");
                        }
                        if (module == 7 && slider->getName() == "PUNCH") {
                            slider->setValue(slider->getValueFromText("-63 %"),juce::sendNotificationSync);
                            require(std::abs(processor.readRackParameters().forge.punch+.63f)<.0001f,"Signed Forge Punch control is disconnected");
                        }
                        if (module == 7 && slider->getName() == "CEILING") {
                            require(formatted.contains("dB"),"Forge ceiling has the wrong unit");
                            slider->setValue(slider->getValueFromText("-4.7 dB"),juce::sendNotificationSync);
                            require(std::abs(processor.readRackParameters().forge.ceilingDb+4.7f)<.0001f,"Forge Ceiling control is disconnected");
                        }
                    }
                    if (auto* combo = dynamic_cast<juce::ComboBox*>(child))
                        if (combo->getNumItems() == batchly::presetsPerModule && combo->getName().isEmpty()) presetMenu = combo;
                    if (auto* viewport = dynamic_cast<juce::Viewport*>(child)) collection = viewport;
                    if (auto* button = dynamic_cast<juce::TextButton*>(child)) {
                        if (button->getName() == "collection") {
                            ++tabs;
                            require(static_cast<bool>(button->getProperties()["effectEnabled"]) == button->getToggleState(), "Initial sidebar status disagrees with this single-effect preset");
                            if (button->getButtonText() == batchly::moduleNames[module]) selectedTab = button;
                        }
                    }
                }
                require(knobs == 9 && tabs == batchly::moduleCount, "Missing controls or collection tabs");
                require(presetMenu && collection && selectedTab, "Missing preset menu or selected effect");
                require(collection->getViewArea().contains(selectedTab->getBounds()), "Selected effect is hidden in the sidebar");
                presetMenu->setSelectedId(5,juce::sendNotificationSync);
                require(processor.getDisplayedProgram() == module*5+4, "Preset menu did not select the requested program");
                require(!processor.isCurrentProgramModified(), "Factory preset did not reach all controls");
                const auto image = editor->createComponentSnapshot(editor->getLocalBounds());
                require(image.isValid() && image.getWidth() == 1000 && image.getHeight() == 650, "Invalid editor rendering");
                auto stream = destination.getChildFile(juce::String(batchly::moduleNames[module]) + (standalone ? "-desktop.png" : "-vst3.png")).createOutputStream();
                require(stream != nullptr, "Cannot write editor preview");
                require(stream->setPosition(0) && stream->truncate().wasOk(), "Cannot replace editor preview");
                juce::PNGImageFormat png; require(png.writeImageToStream(image,*stream), "Cannot encode editor preview");
                if (standalone && module == batchly::moduleCount-1) {
                    juce::MemoryBlock state; processor.getStateInformation(state);
                    require(destination.getChildFile(juce::String(batchly::moduleNames[module])+"-native.bapreset").replaceWithData(state.getData(),state.getSize()), "Cannot write native preset");
                    const auto input = destination.getChildFile("original-probe.wav");
                    const auto output = destination.getChildFile(juce::String(batchly::moduleNames[module])+"-export.wav");
                    juce::AudioBuffer<float> probe(2,96000);
                    for (int i=0;i<probe.getNumSamples();++i) {
                        const double time=i/48000.0, hit=std::fmod(time,.25);
                        const float sample=static_cast<float>(.08*std::exp(-hit*18)*(std::sin(time*6.28318530718*220)+.4*std::sin(time*6.28318530718*1777)));
                        probe.setSample(0,i,sample);probe.setSample(1,i,sample*.8f);
                    }
                    auto fileStream=input.createOutputStream();
                    require(fileStream!=nullptr,"Cannot create export probe");
                    fileStream->setPosition(0); require(fileStream->truncate().wasOk(),"Cannot reset export probe");
                    std::unique_ptr<juce::OutputStream> audioStream=std::move(fileStream);
                    juce::WavAudioFormat wav;
                    auto writer=wav.createWriterFor(audioStream,juce::AudioFormatWriterOptions().withSampleRate(48000).withNumChannels(2).withBitsPerSample(24));
                    require(writer && writer->writeFromAudioSampleBuffer(probe,0,probe.getNumSamples()),"Cannot write export probe");
                    writer.reset();
                    require(BatchlyProcessor::exportAudio(input,output,processor.readRackParameters()).wasOk(),"Native export failed");
                    require(BatchlyProcessor::exportAudio(input,input,processor.readRackParameters()).failed(),"Export can overwrite its original");
                }
            }
        }
        std::cout << "PASS: native pages render; controls fit; selected icons are visible; preset callbacks, time, signed Punch and Ceiling attachments work\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
