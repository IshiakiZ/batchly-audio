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
const std::array<const char*, 9> tapeIds { "patina_sample", "patina_drive", "patina_wear", "patina_flutter",
    "patina_hiss", "patina_chorus", "patina_tone", "patina_mix", "output" };
const std::array<const char*, 9> tapeTitles { "SAMPLE RATE", "DRIVE", "WEAR", "FLUTTER", "HISS", "CHORUS", "TONE", "MIX", "OUTPUT" };
const auto& moduleNames = batchly::moduleNames;
const auto& enabledIds = batchly::enabledIds;
const std::array<const char*, 9> chimeIds { "chime_ring", "chime_color", "chime_drive", "chime_spread", "chime_detune", "chime_motion", "chime_width", "chime_mix", "output" };
const std::array<const char*, 9> chimeTitles { "RING", "COLOR", "DRIVE", "SPREAD", "DETUNE", "MOTION", "WIDTH", "MIX", "OUTPUT" };
const std::array<const char*, 9> chimeHints {
    "How long the tuned strings ring. The input excites notes from the selected scale.",
    "Roll off high frequencies from the ringing layer.",
    "Add harmonics before the strings to excite more of their notes.",
    "Blend a second octave into the ringing layer.",
    "Slightly separate the left and right tunings. Width zero removes this separation.",
    "Slow, subtle pitch movement in the tuned strings.",
    "Stereo spread of the ringing layer. Zero centers it; mono tracks remain mono.",
    "Blend the dry input with the tuned ringing layer. Fully wet works well on a send.",
    "Final gain after all effects. Keep the output meter below 0 dB."
};
const std::array<const char*, 9> helixIds { "helix_rate", "helix_depth", "helix_feedback", "helix_center", "helix_tone", "helix_drive", "helix_width", "helix_mix", "output" };
const std::array<const char*, 9> helixTitles { "RATE", "DEPTH", "FEEDBACK", "CENTER", "TONE", "DRIVE", "WIDTH", "MIX", "OUTPUT" };
const std::array<const char*, 9> helixHints {
    "Speed of the phase sweep, in cycles per second. This rate runs freely without tempo sync.",
    "Width of the sweep around the Center frequency. Zero holds the phaser in one place.",
    "Resonance from the phased signal. Negative values give a different, hollow character.",
    "Center of the moving phase filters. Lower values emphasize a deeper sweep.",
    "Soften the top end of the wet signal. The dry signal retains its original tone.",
    "Add soft saturation before the phase filters.",
    "Offset between the left and right sweeps. Zero aligns them; mono tracks stay mono.",
    "Blend dry and phased audio. Near half gives strong moving notches; fully wet gives phase rotation.",
    "Final gain after all effects. Keep the output meter below 0 dB."
};
const std::array<const char*, 9> gleamIds { "gleam_presence", "gleam_air", "gleam_focus", "gleam_excite", "gleam_tame", "gleam_width", "gleam_trim", "gleam_mix", "output" };
const std::array<const char*, 9> gleamTitles { "PRESENCE", "AIR", "FOCUS", "EXCITE", "TAME", "WIDTH", "TRIM", "MIX", "OUTPUT" };
const std::array<const char*, 9> gleamHints {
    "Broad upper-mid lift. The dB amount sets the added brightness gain, not a sharp EQ band.",
    "Add high-end lift above Focus. The transition is gentle, not a brick-wall split.",
    "Starting region of the Air and Excite layers. Limited by the host's sample rate.",
    "Add soft harmonic color from the high frequencies. Zero adds no nonlinear color.",
    "Reduce added brightness when high-frequency peaks become strong. The original signal remains intact before Trim.",
    "Stereo width of the added brightness. It does not widen mono audio or change the dry stereo image.",
    "Compensate for added level inside Gleam. Shared Output still follows the whole rack.",
    "Blend the original with the enhanced and trimmed signal.",
    "Final gain after all effects. Keep the output meter below 0 dB."
};
const std::array<const char*, 9> relayIds { "relay_time", "relay_feedback", "relay_tone", "relay_motion", "relay_rate", "relay_bounce", "relay_glide", "relay_mix", "output" };
const std::array<const char*, 9> relayTitles { "TIME", "FEEDBACK", "TONE", "MOTION", "RATE", "BOUNCE", "GLIDE", "MIX", "OUTPUT" };
const std::array<const char*, 9> relayHints {
    "Echo spacing in milliseconds. Changing Time bends pitch; Glide controls how gradually it moves.",
    "Amount of each echo sent back into the delay. Higher values create longer repeat trails.",
    "Soften the echoes on each pass. Bass is gently filtered from the feedback path.",
    "Depth of delay-time movement. Higher settings make pitch movement more obvious.",
    "Speed of the delay movement in cycles per second. Time and Rate are free-running without tempo sync.",
    "Send echoes between speakers. Full bounce centers the input into the first left echo, then alternates sides. Mono stays mono.",
    "How slowly delay time follows a new setting. Longer glides stretch the pitch transition.",
    "Blend dry sound and echoes. Fully wet is useful on a send/return track.",
    "Final gain after all effects. Keep the output meter below 0 dB."
};
const std::array<const char*, 9> forgeIds { "forge_punch", "forge_body", "forge_weight", "forge_edge", "forge_drive", "forge_ceiling", "forge_width", "forge_mix", "output" };
const std::array<const char*, 9> forgeTitles { "PUNCH", "BODY", "WEIGHT", "EDGE", "DRIVE", "CEILING", "WIDTH", "MIX", "OUTPUT" };
const std::array<const char*, 9> forgeHints {
    "Boost new hits, or turn below zero to soften their attack. Stereo channels share one detector.",
    "Blend in compression with makeup gain to bring up the body and sustain of each hit.",
    "Broad low-end lift around 120 Hz. The dB amount is the low-frequency shelf gain.",
    "Broad brightness lift above 3.2 kHz. The transition is gentle and follows the host sample rate.",
    "Blend in driven soft saturation. Zero leaves this stage clean.",
    "Peak ceiling of the fully wet drum signal, with a smooth knee. Dry Mix and shared Output can exceed it. This is not a true-peak limiter.",
    "Stereo width before the clipper. Zero centers the wet signal; one preserves its width. Mono remains mono.",
    "Blend the original with the shaped drums. Parallel blends retain some original attack and peaks.",
    "Final gain after all effects. Keep the output meter below 0 dB."
};
const std::array<const char*, 9> cinderIds { "cinder_grit", "cinder_noise", "cinder_tone", "cinder_texture", "cinder_drive", "cinder_decay", "cinder_width", "cinder_mix", "output" };
const std::array<const char*, 9> cinderTitles { "GRIT", "NOISE", "TONE FOCUS", "NOISE FOCUS", "DRIVE", "DECAY", "WIDTH", "MIX", "OUTPUT" };
const std::array<const char*, 9> cinderHints {
    "Add a filtered layer of tonal grit around Tone Focus. The original signal remains underneath.",
    "Add generated noise that follows incoming sound. It fades after each sound according to Decay.",
    "Choose the center region for the driven tonal layer. The broad filters are not a surgical band.",
    "Choose the center region for the generated texture. Limited by the host sample rate.",
    "Push the tonal band's nonlinear coloration. Zero generates no tonal grit.",
    "Noise envelope release time. Higher values let the texture linger after the source stops.",
    "Spread only the added layers. Zero centers them; the dry stereo image stays intact.",
    "Scale both added layers together. Zero returns the original sound exactly.",
    "Final gain after all effects. Keep the output meter below 0 dB."
};
const std::array<const char*, 9> emberIds { "ember_drive", "ember_shape", "ember_color", "ember_filter", "ember_anchor", "ember_bias", "ember_trim", "ember_mix", "output" };
const std::array<const char*, 9> emberTitles { "DRIVE", "SHAPE", "COLOR", "FILTER", "ANCHOR", "BIAS", "TRIM", "MIX", "OUTPUT" };
const std::array<const char*, 9> emberHints {
    "Gain into the saturation curves. Trim can compensate for the added level.",
    "Morph through four original curves: Round at 0%, Edge at 33%, Dense at 67%, Fold at 100%.",
    "Brighten or soften the signal before saturation. Negative values favor a warmer sound.",
    "Low-pass filter after saturation. Lower it to soften generated harmonics.",
    "Restore more of the original low-frequency signal around 150 Hz while retaining upper harmonics.",
    "Shift the nonlinear curve to add asymmetric coloration and even harmonics. DC is filtered afterward.",
    "Level compensation inside Ember, before its dry/wet mix.",
    "Blend the original with saturated bass. Zero returns the original signal exactly.",
    "Final gain after all effects. Keep the output meter below 0 dB."
};
const std::array<const char*, 9> vistaIds { "vista_low", "vista_mid", "vista_high", "vista_low_split", "vista_high_split", "vista_spread", "vista_delay", "vista_mix", "output" };
const std::array<const char*, 9> vistaTitles { "LOW WIDTH", "MID WIDTH", "HIGH WIDTH", "LOW SPLIT", "HIGH SPLIT", "SPREAD", "DELAY", "MIX", "OUTPUT" };
const std::array<const char*, 9> vistaHints {
    "Width of existing low-frequency stereo content. Zero centers the low region.",
    "Width of existing middle-frequency stereo content. 100% keeps its original width.",
    "Width of existing high-frequency stereo content. The broad bands overlap gently.",
    "Transition between low and middle width regions. This is a gentle crossover.",
    "Transition between middle and high width regions, limited by the sample rate.",
    "Generate stereo difference from the center above Low Split. The added difference cancels in mono.",
    "Delay in the generated stereo layer. Changing it can bend that layer's pitch.",
    "Blend the original stereo image with Vista. Mono host tracks remain unchanged.",
    "Final gain after all effects. Widening can raise individual channel peaks."
};
const std::array<const char*, 9> quartzIds { "quartz_input", "quartz_low", "quartz_mid", "quartz_high", "quartz_character", "quartz_ceiling", "quartz_release", "quartz_mix", "output" };
const std::array<const char*, 9> quartzTitles { "INPUT", "LOW", "MID", "HIGH", "CHARACTER", "CEILING", "RELEASE", "MIX", "OUTPUT" };
const std::array<const char*, 9> quartzHints {
    "Gain before broad tone shaping, soft character and sample-peak limiting.",
    "Broad low-frequency gain below roughly 180 Hz. The tone bands overlap gently.",
    "Broad middle-frequency gain between roughly 180 Hz and 3.5 kHz.",
    "Broad high-frequency gain above roughly 3.5 kHz. This is not a surgical equalizer.",
    "Blend in original soft saturation. Zero keeps the tone path linear.",
    "Maximum fully wet sample level after the limiter settles. Dry blend and final Output can exceed it; not a true-peak limit.",
    "How quickly linked limiter gain returns after a peak. Longer times reduce fast pumping.",
    "Blend the original with Quartz. Use 100% when relying on its sample ceiling.",
    "Final gain after all effects, including Quartz. This can raise peaks above Quartz's ceiling."
};
const std::array<const char*, 9> reverbIds { "atrium_decay", "atrium_size", "atrium_predelay", "atrium_damping",
    "atrium_lowcut", "atrium_motion", "atrium_width", "atrium_mix", "output" };
const std::array<const char*, 9> reverbTitles { "DECAY", "SIZE", "PRE-DELAY", "DAMPING", "LOW CUT", "MOTION", "WIDTH", "MIX", "OUTPUT" };
const std::array<const char*, 9> reverbHints {
    "Nominal low-frequency decay time. Damping makes the high frequencies fade faster.",
    "Length of the room's reflection paths. Moving Size while audio plays can bend the tail's pitch.",
    "Wait before the room responds. Keeps the start of each note clear. Moving this can bend pitch.",
    "High-frequency cutoff inside the room. Lower values make a darker, softer tail.",
    "Remove bass from the reverb input. The dry signal keeps its original bass.",
    "Gently move the room's reflection paths for a softer, evolving tail.",
    "Spread of the wet room. Zero makes a centered tail; mono tracks remain mono.",
    "Dry/wet balance. Use fully wet on a send, or a little reverb directly on a track.",
    "Final gain after all effects. Keep the output meter below 0 dB."
};
const std::array<const char*, 9> tapeHints {
    "Lower the internal sample rate for a softer, submerged sound. Limited by your host's sample rate.",
    "Push the original soft saturation curve. Higher values add harmonics and compression.",
    "Slow tape-speed variation. Adds a wandering pitch bend.",
    "Faster tape-speed ripple. Small values add gentle instability.",
    "Generated tape-like hiss. Zero adds no noise; higher values can be heard during silence.",
    "Blend in a stereo modulated voice for a wider, softer sound.",
    "Additional top-end rolloff on the tape signal.",
    "Blend the tape signal with its input. Fully wet avoids parallel delay coloration.",
    "Final gain after all effects. Keep the output meter below 0 dB."
};
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
juce::Font DeckLookAndFeel::getTextButtonFont(juce::TextButton& button, int) {
    return brandFont(button.getName() == "collection" ? 20.f : 13.f, true);
}
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
void DeckLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button, bool hover, bool down) {
    if (button.getName() != "collection") {
        juce::LookAndFeel_V4::drawButtonText(g, button, hover, down);
        return;
    }
    const auto colour = button.getToggleState() ? background : ink;
    const float y = button.getHeight() * .5f;
    if (button.getButtonText() == "Drift") {
        for (int voice = 0; voice < 2; ++voice) {
            juce::Path wave;
            for (int point = 0; point <= 28; ++point) {
                const float x = 12.f + point;
                const float value = y + (voice == 0 ? -3.f : 4.f)
                    + 5.f * std::sin(point * .24f + voice * .8f);
                if (point == 0) wave.startNewSubPath(x, value); else wave.lineTo(x, value);
            }
            g.setColour(voice == 0 ? colour : accent);
            g.strokePath(wave, juce::PathStrokeType(2));
        }
    } else if (button.getButtonText() == "Patina") {
        for (float x : { 19.f, 37.f }) {
            g.setColour(colour); g.drawEllipse(x - 7, y - 9, 14, 14, 1.8f);
            g.fillEllipse(x - 2, y - 4, 4, 4);
            for (int spoke = 0; spoke < 3; ++spoke) {
                const auto tip = juce::Point<float>(x, y - 2).getPointOnCircumference(5, spoke * 2.0944f);
                g.drawLine(x, y - 2, tip.x, tip.y, 1.2f);
            }
        }
        g.setColour(accent); g.drawLine(19, y + 7, 37, y + 7, 2);
    } else if (button.getButtonText() == "Atrium") {
        // Nested arches give the room processor its own original collection mark.
        for (int layer = 0; layer < 3; ++layer) {
            const float inset = layer * 4.f;
            juce::Path arch;
            arch.startNewSubPath(13 + inset, y + 12);
            arch.lineTo(13 + inset, y - 1);
            arch.cubicTo(13 + inset, y - 18 + inset, 43 - inset, y - 18 + inset, 43 - inset, y - 1);
            arch.lineTo(43 - inset, y + 12);
            g.setColour(layer == 1 ? accent : colour);
            g.strokePath(arch, juce::PathStrokeType(1.8f));
        }
    }
    if (button.getButtonText() == "Chime") {
        g.setColour(colour); g.drawLine(13, y - 14, 43, y - 14, 1.8f);
        for (int bar = 0; bar < 4; ++bar) {
            const float x = 16.f + bar * 8;
            g.setColour(bar == 1 ? accent : colour);
            g.drawLine(x, y - 10, x, y + 13 - bar * 4, 3);
            g.drawLine(x, y - 14, x, y - 10, 1);
        }
    }
    if (button.getButtonText() == "Helix") {
        for (int loop = 0; loop < 2; ++loop) {
            g.setColour(loop == 0 ? colour : accent);
            g.drawEllipse(12.f + loop * 12, y - 12, 20, 24, 1.8f);
        }
    }
    if (button.getButtonText() == "Gleam") {
        juce::Path diamond; diamond.startNewSubPath(27, y - 13); diamond.lineTo(37, y);
        diamond.lineTo(27, y + 13); diamond.lineTo(17, y); diamond.closeSubPath();
        g.setColour(colour); g.strokePath(diamond, juce::PathStrokeType(1.8f));
        g.setColour(accent); g.drawLine(37, y - 8, 45, y - 14, 1.8f); g.drawLine(39, y, 48, y, 1.8f);
        g.drawLine(37, y + 8, 45, y + 14, 1.8f);
    }
    if (button.getButtonText() == "Relay") {
        for (int echo = 0; echo < 3; ++echo) {
            g.setColour(echo == 1 ? accent : colour);
            g.drawRect(13.f + echo * 10, y - 12 + echo * 5, 10.f, 17.f, 1.8f);
        }
    }
    if (button.getButtonText() == "Forge") {
        juce::Path strike; strike.startNewSubPath(14,y+8); strike.lineTo(22,y+8);
        strike.lineTo(25,y-12); strike.lineTo(30,y+12); strike.lineTo(34,y+4); strike.lineTo(42,y+4);
        g.setColour(colour); g.strokePath(strike,juce::PathStrokeType(2));
        g.setColour(accent); g.drawLine(12,y+17,44,y+17,2);
    }
    if (button.getButtonText() == "Cinder") {
        for(int n=0;n<9;++n) {
            const float x=15.f+(n%3)*10,yOffset=-11.f+(n/3)*10;
            g.setColour(n%3==1?accent:colour);
            g.fillRect(x,y+yOffset,3.f+(n%2)*2,3.f+(n%2)*2);
        }
    }
    if (button.getButtonText() == "Ember") {
        juce::Path flame;flame.startNewSubPath(28,y-15);flame.cubicTo(44,y,43,y+13,29,y+15);
        flame.cubicTo(10,y+14,12,y,21,y-5);flame.lineTo(23,y+4);flame.closeSubPath();
        g.setColour(colour);g.strokePath(flame,juce::PathStrokeType(1.8f));
        g.setColour(accent);g.drawLine(25,y+9,30,y-2,2);
    }
    if (button.getButtonText() == "Vista") {
        g.setColour(colour);g.drawLine(17,y,39,y,1.8f);
        g.drawLine(17,y,23,y-7,1.8f);g.drawLine(17,y,23,y+7,1.8f);
        g.drawLine(39,y,33,y-7,1.8f);g.drawLine(39,y,33,y+7,1.8f);
        g.setColour(accent);g.drawLine(28,y-14,28,y+14,2);
    }
    if (button.getButtonText() == "Quartz") {
        juce::Path crystal;crystal.startNewSubPath(19,y+12);crystal.lineTo(16,y-5);
        crystal.lineTo(27,y-16);crystal.lineTo(38,y-5);crystal.lineTo(35,y+12);crystal.closeSubPath();
        g.setColour(colour);g.strokePath(crystal,juce::PathStrokeType(1.8f));
        g.setColour(accent);g.drawLine(27,y-16,27,y+12,1.7f);
        g.drawLine(16,y-5,38,y-5,1.7f);
    }
    g.setColour(colour); g.setFont(brandFont(18, true));
    g.drawText(button.getButtonText(), 53, 10, button.getWidth() - 58, 27, juce::Justification::centredLeft);
    const bool enabled = static_cast<bool>(button.getProperties()["effectEnabled"]);
    g.setFont(monoFont(9));
    g.setColour(enabled ? (button.getToggleState() ? juce::Colour(0xffff7964) : accentText) : colour.withAlpha(.65f));
    g.drawText(enabled ? "ON" : "OFF", 54, 37, button.getWidth() - 58, 16, juce::Justification::centredLeft);
    if (button.getToggleState()) { g.setColour(accent); g.fillRect(0, 0, 3, button.getHeight()); }
}

BatchlyEditor::BatchlyEditor(BatchlyProcessor& p) : AudioProcessorEditor(p), processor(p) {
    setLookAndFeel(&look);
    setSize(1000, 650);
    for (size_t i = 0; i < knobs.size(); ++i) {
        auto& knob = knobs[i];
        knob.setSliderStyle(i == 8 ? juce::Slider::LinearHorizontal : juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 94, 24);
        knob.setRotaryParameters(juce::MathConstants<float>::pi * 1.22f, juce::MathConstants<float>::pi * 2.78f, true);
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
    for (auto* button : { &bypass, &open, &play, &stop, &demo, &exportButton, &savePreset, &loadPreset, &updates,
                         &moduleEnabled }) addAndMakeVisible(*button);
    bypass.setClickingTogglesState(true);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters, "bypass", bypass);
    bypass.setTooltip("Bypass the whole rack and Output to compare with the original signal. Changes are faded to prevent clicks.");
    presets.onChange = [this] { if (presets.getSelectedId() > 0) processor.setModuleProgram(shownModule, presets.getSelectedId() - 1); };
    presets.setTooltip("Choose a starting point, then adjust any control.");
    addAndMakeVisible(presets);
    collectionViewport.setViewedComponent(&collectionContent, false);
    collectionViewport.setScrollBarsShown(true, false);
    collectionViewport.setScrollBarThickness(6);
    addAndMakeVisible(collectionViewport);
    for (int module = 0; module < batchly::moduleCount; ++module) {
        auto& tab = collectionTabs[module];
        tab.setName("collection"); tab.setButtonText(moduleNames[module]);
        tab.setTooltip("Show " + juce::String(moduleNames[module]) + " controls. Audio runs through the collection from top to bottom.");
        tab.onClick = [this, module] { showModule(module); };
        collectionContent.addAndMakeVisible(tab);
    }
    for (size_t i = 0; i < tuning.size(); ++i) addChildComponent(tuning[i]);
    for (int i = 0; i < 12; ++i) tuning[0].addItem(batchly::ChimeEngine::noteNames[i], i + 1);
    for (int i = 0; i < 6; ++i) tuning[1].addItem(batchly::ChimeEngine::scaleNames[i], i + 1);
    for (int i = 0; i < 3; ++i) tuning[2].addItem("Oct " + juce::String(i + 2), i + 1);
    const char* tuningIds[] { "chime_root", "chime_scale", "chime_octave" };
    const char* tuningLabels[] { "Root note", "Scale", "Base octave" };
    for (size_t i = 0; i < tuning.size(); ++i) {
        tuning[i].setName(tuningLabels[i]); tuning[i].setTooltip(tuningLabels[i]);
        tuningAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.parameters, tuningIds[i], tuning[i]);
    }
    moduleEnabled.setClickingTogglesState(true);
    moduleEnabled.setTooltip("Switch this effect on or off. The other effects keep their own settings.");
    open.onClick = [this] { chooseAudio(); };
    play.onClick = [this] { processor.play(); };
    stop.onClick = [this] { processor.stop(); };
    demo.onClick = [this] { processor.playDemo(); status.setText("Original synthesized demo. Adjust the controls or toggle BYPASS to compare.", juce::dontSendNotification); };
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
    showModule(processor.selectedModule());
    startTimerHz(30);
}
void BatchlyEditor::showModule(int module) {
    shownModule = juce::jlimit(0, batchly::moduleCount - 1, module);
    processor.selectModule(shownModule);
    configureKnobs();
    presets.clear(juce::dontSendNotification);
    for (int i = 0; i < 5; ++i) presets.addItem(processor.getProgramName(shownModule * 5 + i), i + 1);
    presets.setSelectedId(processor.getDisplayedProgram() % 5 + 1, juce::dontSendNotification);
    displayedProgram = -1;
    moduleAttachment.reset();
    moduleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters,
        enabledIds[shownModule], moduleEnabled);
    for (int i = 0; i < batchly::moduleCount; ++i) {
        collectionTabs[i].setToggleState(shownModule == i, juce::dontSendNotification);
        // Set the first frame correctly before the periodic meter refresh runs.
        collectionTabs[i].getProperties().set("effectEnabled", processor.parameters.getRawParameterValue(enabledIds[i])->load() >= .5f);
    }
    const auto selectedBounds = collectionTabs[shownModule].getBounds();
    const int viewY = collectionViewport.getViewPositionY();
    if (selectedBounds.getY() < viewY) collectionViewport.setViewPosition(0, selectedBounds.getY());
    else if (selectedBounds.getBottom() > viewY + collectionViewport.getViewHeight())
        collectionViewport.setViewPosition(0, selectedBounds.getBottom() - collectionViewport.getViewHeight());
    for (auto& control : tuning) control.setVisible(shownModule == 3);
    moduleEnabled.setButtonText(juce::String(moduleNames[shownModule]).toUpperCase() + (moduleEnabled.getToggleState() ? " ON" : " OFF"));
    repaint();
}
void BatchlyEditor::configureKnobs() {
    const bool tape = shownModule == 1;
    const bool reverb = shownModule == 2;
    const bool chime = shownModule == 3;
    const bool helix = shownModule == 4;
    const bool gleam = shownModule == 5;
    const bool relay = shownModule == 6;
    const bool forge = shownModule == 7;
    const bool cinder = shownModule == 8;
    const bool ember = shownModule == 9;
    const bool vista = shownModule == 10;
    const bool quartz = shownModule == 11;
    const std::array<const char*, 9>* controlIds[] { &ids, &tapeIds, &reverbIds, &chimeIds, &helixIds, &gleamIds, &relayIds, &forgeIds, &cinderIds, &emberIds, &vistaIds, &quartzIds };
    const std::array<const char*, 9>* controlTitles[] { &titles, &tapeTitles, &reverbTitles, &chimeTitles, &helixTitles, &gleamTitles, &relayTitles, &forgeTitles, &cinderTitles, &emberTitles, &vistaTitles, &quartzTitles };
    const std::array<const char*, 9>* controlHints[] { &hints, &tapeHints, &reverbHints, &chimeHints, &helixHints, &gleamHints, &relayHints, &forgeHints, &cinderHints, &emberHints, &vistaHints, &quartzHints };
    for (size_t i = 0; i < knobs.size(); ++i) {
        attachments[i].reset();
        auto& knob = knobs[i];
        const auto id = (*controlIds[shownModule])[i];
        attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, id, knob);
        const bool frequency = quartz ? false : vista ? (i == 3 || i == 4) : ember ? i == 3 : cinder ? (i == 2 || i == 3) : forge ? false : relay ? (i == 2 || i == 4) : gleam ? i == 2 : helix ? (i == 0 || i == 3 || i == 4) : chime ? i == 1 : reverb ? (i == 3 || i == 4) : tape ? (i == 0 || i == 6) : (i == 1 || i == 3);
        knob.textFromValueFunction = [i, frequency, reverb, chime, gleam, relay, forge, cinder, ember, vista, quartz](double value) {
            if (((reverb || chime) && i == 0) || (cinder && i == 5)) return juce::String(value, 2) + " s";
            if ((reverb && i == 2) || (relay && i == 0) || (vista && i == 6) || (quartz && i == 6)) return juce::String(value, 1) + " ms";
            if (frequency) return value >= 1000 ? juce::String(value / 1000, 2) + " kHz" : juce::String(value, 2) + " Hz";
            if (i == 8 || (gleam && (i == 0 || i == 1 || i == 6)) || (forge && (i == 2 || i == 3 || i == 5)) || (ember && (i == 0 || i == 6)) || (quartz && (i < 4 || i == 5))) return juce::String(value, 1) + " dB";
            return juce::String(value * 100, 0) + " %";
        };
        knob.valueFromTextFunction = [i, frequency, reverb, chime, gleam, relay, forge, cinder, ember, vista, quartz](const juce::String& value) {
            const auto number = value.getDoubleValue();
            if ((reverb && (i == 0 || i == 2)) || (chime && i == 0) || (relay && i == 0) || (cinder && i == 5) || (vista && i == 6) || (quartz && i == 6)) return number;
            if (frequency) return number * (value.containsIgnoreCase("k") ? 1000 : 1);
            return i == 8 || (gleam && (i == 0 || i == 1 || i == 6)) || (forge && (i == 2 || i == 3 || i == 5)) || (ember && (i == 0 || i == 6)) || (quartz && (i < 4 || i == 5)) ? number : number / 100;
        };
        knob.setName((*controlTitles[shownModule])[i]);
        knob.setTooltip((*controlHints[shownModule])[i]);
        labels[i].setText((*controlTitles[shownModule])[i], juce::dontSendNotification);
        auto* parameter = processor.parameters.getParameter(id);
        knob.setDoubleClickReturnValue(true, parameter->convertFrom0to1(parameter->getDefaultValue()));
        knob.updateText();
    }
}
BatchlyEditor::~BatchlyEditor() {
    stopTimer();
    exportPool.removeAllJobs(true, -1);
    setLookAndFeel(nullptr);
}
void BatchlyEditor::resized() {
    collectionViewport.setBounds(12, 136, 154, 354);
    collectionContent.setSize(146, batchly::moduleCount * 72 - 8);
    for (int i = 0; i < batchly::moduleCount; ++i) collectionTabs[i].setBounds(0, i * 72, 146, 64);
    tuning[0].setBounds(211, 329, 74, 25);
    tuning[1].setBounds(293, 329, 193, 25);
    tuning[2].setBounds(494, 329, 85, 25);
    moduleEnabled.setBounds(414, 19, 139, 32);
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
    const auto rack = processor.readRackParameters();
    text(g, rack.drift.bypass ? "RACK BYPASSED" : "CHAIN / TOP TO BOTTOM", { 15, 492, 150, 18 }, 8, accentText);
    text(g, "LOCAL AUDIO", { 24, 561, 132, 20 }, 10, muted, true);
    text(g, "Open source", { 24, 589, 125, 20 }, 13, ink);
    text(g, "v" + juce::String(batchly::UpdateService::currentVersion), { 24, 610, 140, 18 }, 9, muted);
    text(g, "AUDIO TOOLS / " + juce::String(shownModule + 1).paddedLeft('0', 3), { 196, 20, 200, 32 }, 11, muted);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffeeeceb), 192, 84, juce::Colour(0xffd4d2d0), 980, 159, false));
    g.fillRect(190, 87, 792, 70);
    for (int y = 90; y < 153; y += 3) {
        g.setColour(juce::Colours::white.withAlpha(.045f)); g.drawHorizontalLine(y, 198, 975);
    }
    text(g, juce::String(moduleNames[shownModule]).toUpperCase(), { 211, 91, 185, 61 }, shownModule == 0 ? 48.f : 42.f, ink, true);
    g.setColour(accent); g.fillRect(396, 106, 3, 33);
    const std::array<const char*, 12> subtitles { "RANDOM-MOTION CHORUS & VIBRATO", "TAPE COLOR & PITCH WEAR", "SPACIOUS ROOMS & MOVING TAILS", "TUNED STRINGS & HARMONIC COLOR", "STEREO SWEEPS & PHASE ROTATION", "PRESENCE, AIR & SOFT BRILLIANCE", "MOVING ECHOES & STEREO RETURNS", "DRUM ATTACK, BODY & SOFT CLIPPING", "TONAL GRIT & MOVING TEXTURE", "BASS SATURATION & HARMONIC WEIGHT", "THREE-BAND WIDTH & STEREO SPACE", "BROAD TONE & LINKED PEAK CONTROL" };
    const std::array<const char*, 12> descriptions { "Slow movement. Soft edges. A little room to wander.",
        "Soft edges. Warm reels. A little history in every note.", "Close walls. Open halls. Give each note a place to linger.", "Strike a note. Find its colors. Let the strings answer.", "Slow circles. Deep notches. Keep the sound in motion.", "Open the top. Keep the body. Let the detail shine.", "Send it out. Let it wander. Hear it come back.", "Shape the strike. Add weight. Hold the peaks.", "Find the grain. Shape the dust. Leave a trace.", "Warm the core. Bend the curve. Keep the foundation.", "Open the sides. Keep the center. Find your space.", "Shape the balance. Round the edges. Set the finish." };
    text(g, subtitles[shownModule], { 420, 107, 473, 20 }, 13, ink, true);
    text(g, descriptions[shownModule], { 420, 129, 496, 18 }, 12, muted);
    screw(g, 203, 101); screw(g, 969, 101); screw(g, 203, 144); screw(g, 969, 144);
    const juce::Rectangle<float> scope(196, 184, 398, 182);
    if (shownModule == 1) drawTapeDeck(g);
    else if (shownModule == 2) drawReverbRoom(g);
    else if (shownModule == 3) drawResonator(g);
    else if (shownModule == 4) drawPhaser(g);
    else if (shownModule == 5) drawEnhancer(g);
    else if (shownModule == 6) drawDelay(g);
    else if (shownModule == 7) drawDrums(g);
    else if (shownModule == 8) drawTexture(g);
    else if (shownModule == 9) drawBass(g);
    else if (shownModule == 10) drawStereo(g);
    else if (shownModule == 11) drawMastering(g);
    else {
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
    }
    text(g, shownModule == 11 ? "BALANCE" : shownModule == 10 ? "UPPER IMAGE" : shownModule == 9 ? "TONE" : shownModule == 8 ? "BAND FOCUS" : shownModule == 7 ? "TONE" : shownModule == 6 ? "ECHO COLOR" : shownModule == 5 ? "BRIGHTNESS" : shownModule == 4 ? "RESONANCE" : shownModule == 3 ? "EXCITATION" : shownModule == 2 ? "ARRIVAL" : "MOVEMENT", { 202, 380, 125, 18 }, 10, accentText, true);
    text(g, shownModule == 11 ? "COLOR / PEAKS" : shownModule == 10 ? "BANDS / SPACE" : shownModule == 9 ? "BODY / COLOR" : shownModule == 8 ? "COLOR / RELEASE" : shownModule == 7 ? "COLOR / PEAKS" : shownModule == 6 ? "PITCH / MOVEMENT" : shownModule == 5 ? "COLOR / CONTROL" : shownModule == 4 ? "FILTER / DRIVE" : shownModule == 3 ? "TUNING / MOTION" : shownModule == 2 ? "TONE / MOTION" : "CHARACTER", { 337, 380, 370, 18 }, 10, accentText, true);
    text(g, shownModule == 11 ? "RECOVERY / BLEND" : "IMAGE / BLEND", { 719, 380, 254, 18 }, 10, accentText, true);
    g.setColour(rule); g.drawHorizontalLine(552, 195, 977);
    g.setColour(panel); g.fillRect(636, 568, 142, 12);
    const float decibels = juce::Decibels::gainToDecibels(meter, -60.0f);
    g.setColour(meter >= 1 ? accent : ink);
    g.fillRect(636.0f, 568.0f, juce::jlimit(0.0f, 142.0f, (decibels + 60) / 60 * 142), 12.0f);
    text(g, meter < .001f ? "OUTPUT / SILENT" : "OUTPUT / " + juce::String(decibels, 1) + " dB",
         { 636, 585, 163, 16 }, 9, muted);
}
void BatchlyEditor::drawTapeDeck(juce::Graphics& g) {
    g.setColour(ink); g.fillRect(196, 184, 398, 182);
    g.setColour(rule); g.drawRect(196, 184, 398, 182);
    text(g, "TAPE TRANSPORT", { 211, 193, 220, 20 }, 10, background);
    const auto settings = processor.readRackParameters().patina;
    const double effectiveRate = std::min(static_cast<double>(settings.sampleHz), processor.getSampleRate());
    text(g, juce::String(effectiveRate / 1000, 1) + " kHz", { 501, 193, 80, 20 }, 10, juce::Colour(0xffe6b591));
    g.setColour(juce::Colour(0xff725343));
    g.drawLine(286, 321, 505, 321, 3); g.drawLine(286, 235, 505, 235, 2);
    for (int reel = 0; reel < 2; ++reel) {
        const juce::Point<float> centre(reel == 0 ? 293.f : 495.f, 279.f);
        g.setColour(juce::Colour(0xff45342d)); g.fillEllipse(centre.x - 51, centre.y - 51, 102, 102);
        for (int ring = 18; ring < 49; ring += 3) {
            g.setColour(juce::Colour(0xff977460).withAlpha(.36f));
            g.drawEllipse(centre.x - ring, centre.y - ring, ring * 2.f, ring * 2.f, 1);
        }
        g.setColour(juce::Colour(0xffc9c2b9)); g.drawEllipse(centre.x - 53, centre.y - 53, 106, 106, 2);
        for (int spoke = 0; spoke < 3; ++spoke) {
            const float angle = reelAngle + spoke * juce::MathConstants<float>::twoPi / 3 + reel * .35f;
            const auto inner = centre.getPointOnCircumference(15, angle);
            const auto outer = centre.getPointOnCircumference(43, angle);
            g.setColour(juce::Colour(0xffd3cdc7)); g.drawLine({ inner, outer }, 13);
        }
        g.setColour(juce::Colour(0xffc9c2b9)); g.fillEllipse(centre.x - 13, centre.y - 13, 26, 26);
        g.setColour(ink); g.fillEllipse(centre.x - 5, centre.y - 5, 10, 10);
    }
    g.setColour(juce::Colour(0xffa29a93)); g.fillRect(375, 301, 36, 28);
    g.setColour(accent); g.fillRect(377, 306, 32, 3);
    text(g, settings.enabled ? "TAPE / ENGAGED" : "TAPE / OFF", { 211, 337, 190, 19 }, 10,
        settings.enabled ? juce::Colour(0xffe6b591) : background);
    text(g, "STEREO COLOR", { 459, 337, 125, 19 }, 10, background);
}
void BatchlyEditor::drawReverbRoom(juce::Graphics& g) {
    const auto room = processor.readRackParameters().atrium;
    g.setColour(ink); g.fillRect(196, 184, 398, 182);
    g.setColour(rule); g.drawRect(196, 184, 398, 182);
    text(g, "ROOM DEPTH", { 211, 193, 195, 20 }, 10, background);
    text(g, juce::String(room.decaySeconds, 2) + " s", { 501, 193, 80, 20 }, 10, background);
    const juce::Rectangle<float> outer(216, 221, 358, 104);
    const juce::Rectangle<float> inner = outer.reduced(92 + room.size * 34, 22 + room.size * 15);
    g.setColour(juce::Colour(0xff625653));
    g.drawLine({ outer.getTopLeft(), inner.getTopLeft() }, 1);
    g.drawLine({ outer.getTopRight(), inner.getTopRight() }, 1);
    g.drawLine({ outer.getBottomLeft(), inner.getBottomLeft() }, 1);
    g.drawLine({ outer.getBottomRight(), inner.getBottomRight() }, 1);
    for (int layer = 0; layer < 7; ++layer) {
        const float depth = static_cast<float>(layer) / 6;
        const auto frame = outer.reduced((outer.getWidth() - inner.getWidth()) * depth * .5f,
            (outer.getHeight() - inner.getHeight()) * depth * .5f);
        g.setColour(layer == 6 ? accent : juce::Colour(0xffa39995).withAlpha(.7f - .07f * layer));
        g.drawRect(frame, layer == 6 ? 2.f : 1.f);
    }
    // This is a live wet-level indicator within a room illustration, not a
    // measured room response or a claim of a physical acoustic simulation.
    const float glow = juce::jlimit(0.f, 1.f, reverbMeter * 8);
    g.setColour(accent.withAlpha(.08f + .45f * glow)); g.fillRect(inner.reduced(3));
    g.setColour(accent); g.fillRect(218.f, 319.f, 352.f * glow, 3.f);
    text(g, room.enabled ? "ROOM / ENGAGED" : "ROOM / OFF", { 211, 337, 170, 19 }, 10, background);
    text(g, "WET LEVEL", { 489, 337, 94, 19 }, 10, background);
}
void BatchlyEditor::drawResonator(juce::Graphics& g) {
    const auto settings = processor.readRackParameters().chime;
    g.setColour(ink); g.fillRect(196, 184, 398, 182);
    g.setColour(rule); g.drawRect(196, 184, 398, 182);
    text(g, "RESONANT STRINGS", { 211, 193, 220, 20 }, 10, background);
    text(g, "2 OCTAVES", { 481, 193, 102, 20 }, 10, background);
    const int count = batchly::ChimeEngine::noteCount(settings.scale);
    g.setColour(muted); g.drawLine(214, 223, 576, 223, 1);
    for (int n = 0; n < count; ++n) {
        const float x = 237.f + n * 316.f / (count - 1);
        const float height = 68.f - n * 5;
        const float glow = juce::jlimit(0.f, 1.f, resonatorMeters[n] * 12);
        g.setColour(muted); g.drawLine(x, 223, x, 232, 1);
        g.setGradientFill(juce::ColourGradient(background, x - 8, 232, juce::Colour(0xff736762), x + 9, 232 + height, false));
        g.fillRoundedRectangle(x - 7, 232, 14, height, 2);
        g.setColour(accent.withAlpha(.15f + .7f * glow));
        g.fillRoundedRectangle(x - 7, 232, 14, height, 2);
        g.setColour(background); g.fillEllipse(x - 1.5f, 237, 3, 3);
        const int note = (settings.root + batchly::ChimeEngine::semitone(settings.scale, n)) % 12;
        text(g, batchly::ChimeEngine::noteNames[note], { static_cast<int>(x) - 18, 304, 36, 18 }, 10, background, false, juce::Justification::centred);
    }
}
void BatchlyEditor::drawPhaser(juce::Graphics& g) {
    const auto settings = processor.readRackParameters().helix;
    g.setColour(ink); g.fillRect(196, 184, 398, 182);
    g.setColour(rule); g.drawRect(196, 184, 398, 182);
    text(g, "PHASE ROTATION", { 211, 193, 220, 20 }, 10, background);
    text(g, "8 STAGES", { 493, 193, 90, 20 }, 10, background);
    const float sweep[] { processor.phaserLeft.load(), processor.phaserRight.load() };
    for (int ch = 0; ch < 2; ++ch) {
        const float cx = 341.f + ch * 107, cy = 274;
        for (int ring = 0; ring < 4; ++ring) {
            const float r = 48.f - ring * 10;
            g.setColour(juce::Colour(0xff63524c)); g.drawEllipse(cx - r, cy - r, r * 2, r * 2, 1);
            const float angle = sweep[ch] * 2.2f + ring * .6f;
            const auto dot = juce::Point<float>(cx, cy).getPointOnCircumference(r, angle);
            g.setColour(ch == 0 ? accent : background); g.fillEllipse(dot.x - 3, dot.y - 3, 6, 6);
        }
        text(g, ch == 0 ? "L" : "R", { static_cast<int>(cx) - 8, 263, 16, 20 }, 11, background, true, juce::Justification::centred);
    }
    text(g, settings.enabled ? "PHASER / ENGAGED" : "PHASER / OFF", { 211, 337, 200, 19 }, 10, background);
    text(g, juce::String(settings.rateHz, 2) + " Hz", { 486, 337, 96, 19 }, 10, background);
}
void BatchlyEditor::drawEnhancer(juce::Graphics& g) {
    const auto settings = processor.readRackParameters().gleam;
    const float level = juce::jlimit(0.f, 1.f, processor.brightnessLevel.load() * 8);
    const float reduction = processor.brightnessReduction.load();
    g.setColour(ink); g.fillRect(196, 184, 398, 182);
    g.setColour(rule); g.drawRect(196, 184, 398, 182);
    text(g, "HIGH-END DETAIL", { 211, 193, 220, 20 }, 10, background);
    text(g, "TAME " + juce::String(reduction * 100, 0) + "%", { 482, 193, 103, 20 }, 10, background);
    juce::Path prism; prism.startNewSubPath(350, 223); prism.lineTo(390, 280);
    prism.lineTo(350, 323); prism.lineTo(310, 280); prism.closeSubPath();
    g.setColour(background.withAlpha(.08f + level * .15f)); g.fillPath(prism);
    g.setColour(background); g.strokePath(prism, juce::PathStrokeType(1.5f));
    g.setColour(background.withAlpha(.7f)); g.drawLine(216, 280, 310, 280, 2);
    for (int ray = 0; ray < 5; ++ray) {
        const float y = 238.f + ray * 21;
        g.setColour(accent.withAlpha(.28f + level * .65f));
        g.drawLine(390, 280, 568, y, 1.3f + .15f * settings.airDb);
    }
    text(g, settings.enabled ? "ENHANCER / ENGAGED" : "ENHANCER / OFF", { 211, 337, 210, 19 }, 10, background);
    text(g, "HIGH LEVEL", { 476, 337, 108, 19 }, 10, background);
}
void BatchlyEditor::drawDelay(juce::Graphics& g) {
    const auto settings = processor.readRackParameters().relay;
    g.setColour(ink); g.fillRect(196, 184, 398, 182);
    g.setColour(rule); g.drawRect(196, 184, 398, 182);
    text(g, "SIGNAL / RETURNS", { 211, 193, 220, 20 }, 10, background);
    text(g, juce::String(settings.timeMs, 0) + " ms", { 488, 193, 95, 20 }, 10, background);
    const float left = juce::jlimit(0.f, 1.f, processor.echoLeft.load() * 8);
    const float right = juce::jlimit(0.f, 1.f, processor.echoRight.load() * 8);
    g.setColour(rule.withAlpha(.5f)); g.drawLine(217, 277, 578, 277, 1);
    for (int repeat = 0; repeat < 6; ++repeat) {
        const float x = 223.f + repeat * 60;
        const float height = 65.f * std::pow(settings.feedback, repeat) + 8;
        const bool otherSide = repeat % 2 != 0;
        const float center = 277.f + (otherSide ? 20 : -20) * settings.bounce;
        const float level = otherSide ? right : left;
        const auto colour = otherSide ? background : accent;
        g.setColour(colour.withAlpha(.1f + level * .6f)); g.fillRect(x, center - height / 2, 30.f, height);
        g.setColour(colour.withAlpha(.8f)); g.drawRect(x, center - height / 2, 30.f, height, 1.3f);
    }
    text(g, settings.enabled ? "DELAY / ENGAGED" : "DELAY / OFF", { 211, 337, 210, 19 }, 10, background);
    text(g, "L / R ECHO", { 476, 337, 108, 19 }, 10, background);
}
void BatchlyEditor::drawDrums(juce::Graphics& g) {
    const auto settings = processor.readRackParameters().forge;
    g.setColour(ink); g.fillRect(196,184,398,182);
    g.setColour(rule); g.drawRect(196,184,398,182);
    text(g,"STRIKE / SHAPE",{211,193,210,20},10,background);
    text(g,juce::String(settings.ceilingDb,1)+" dB",{490,193,93,20},10,background);
    // The envelope is a control illustration. The two bars below show measured activity.
    juce::Path original, shaped;
    for (int i=0;i<330;++i) {
        const float x=224.f+i, time=i/330.f;
        const float base=(1-std::exp(-time*110))*std::exp(-time*5);
        const float shape=std::min(.96f,base*(1+settings.punch*std::exp(-time*22))*(1+settings.body*time));
        const float y1=317-base*84,y2=317-shape*84;
        if(i==0){original.startNewSubPath(x,y1);shaped.startNewSubPath(x,y2);}
        else {original.lineTo(x,y1);shaped.lineTo(x,y2);}
    }
    g.setColour(rule);g.strokePath(original,juce::PathStrokeType(1));
    g.setColour(accent);g.strokePath(shaped,juce::PathStrokeType(2));
    text(g,"ATTACK",{213,337,52,19},9,background);
    text(g,"CLIP",{406,337,44,19},9,background);
    g.setColour(muted);g.fillRect(272,344,104,4);g.fillRect(460,344,104,4);
    g.setColour(accent);
    g.fillRect(272.f,344.f,104*juce::jlimit(0.f,1.f,processor.drumAttack.load()),4.f);
    g.fillRect(460.f,344.f,104*juce::jlimit(0.f,1.f,processor.drumClipping.load()),4.f);
}
void BatchlyEditor::drawTexture(juce::Graphics& g) {
    const auto settings=processor.readRackParameters().cinder;
    g.setColour(ink);g.fillRect(196,184,398,182);g.setColour(rule);g.drawRect(196,184,398,182);
    text(g,"GRAIN / TEXTURE",{211,193,220,20},10,background);
    text(g,juce::String(settings.decaySeconds,2)+" s",{492,193,91,20},10,background);
    const float grit=juce::jlimit(0.f,1.f,processor.gritLevel.load()*8);
    const float noise=juce::jlimit(0.f,1.f,processor.textureLevel.load()*20);
    const float toneX=226+325*static_cast<float>(std::log(settings.toneHz/80)/std::log(150.f));
    const float noiseX=226+325*static_cast<float>(std::log(settings.noiseHz/200)/std::log(80.f));
    for(int row=0;row<7;++row)for(int column=0;column<30;++column){
        const float x=224.f+column*12,y=231.f+row*13;
        const float density=std::exp(-std::abs(x-noiseX)/75);
        g.setColour(background.withAlpha(.05f+density*(.12f+noise*.65f)));
        g.fillRect(x+(row%2)*3,y,2.f+noise*2,2.f+noise*2);
    }
    juce::Path ridge;
    for(int point=0;point<350;++point){
        const float x=220.f+point,y=320-70*std::exp(-std::pow((x-toneX)/52,2.f))*(.35f+settings.grit*.4f+grit*.25f);
        if(point==0)ridge.startNewSubPath(x,y);else ridge.lineTo(x,y);
    }
    g.setColour(accent);g.strokePath(ridge,juce::PathStrokeType(2));
    text(g,settings.enabled?"TEXTURE / ENGAGED":"TEXTURE / OFF",{211,337,220,19},10,background);
    text(g,"TONE + NOISE",{468,337,116,19},10,background);
}
void BatchlyEditor::drawBass(juce::Graphics& g) {
    const auto settings=processor.readRackParameters().ember;
    g.setColour(ink);g.fillRect(196,184,398,182);g.setColour(rule);g.drawRect(196,184,398,182);
    text(g,"CURVE / HARMONICS",{211,193,230,20},10,background);
    const char* names[]{"ROUND","EDGE","DENSE","FOLD"};
    text(g,names[juce::jlimit(0,3,static_cast<int>(settings.shape*3+.5f))],{500,193,83,20},10,background);
    g.setColour(muted);g.drawLine(219,275,570,275,1);g.drawLine(395,222,395,329,1);
    juce::Path curve;
    const double gain=std::pow(10.0,settings.driveDb/20),bias=settings.bias*.6;
    for(int n=0;n<=350;++n){
        const double x=(n/350.0*2-1)*gain;
        const double y=batchly::EmberEngine::averagedCurve(x+bias,x+bias,settings.shape)-batchly::EmberEngine::averagedCurve(bias,bias,settings.shape);
        const float px=220.f+n,py=275.f-static_cast<float>(y)*30;
        if(n==0)curve.startNewSubPath(px,py);else curve.lineTo(px,py);
    }
    g.setColour(accent);g.strokePath(curve,juce::PathStrokeType(2));
    text(g,"INPUT",{213,337,52,19},9,background);text(g,"WET",{406,337,44,19},9,background);
    g.setColour(muted);g.fillRect(272,344,104,4);g.fillRect(460,344,104,4);
    g.setColour(accent);g.fillRect(272.f,344.f,104*juce::jlimit(0.f,1.f,processor.bassInput.load()),4.f);
    g.fillRect(460.f,344.f,104*juce::jlimit(0.f,1.f,processor.bassWet.load()),4.f);
}
void BatchlyEditor::drawStereo(juce::Graphics& g) {
    const auto settings=processor.readRackParameters().vista;
    g.setColour(ink);g.fillRect(196,184,398,182);g.setColour(rule);g.drawRect(196,184,398,182);
    text(g,"STEREO / FIELD",{211,193,230,20},10,background);
    const float widths[]{settings.lowWidth,settings.midWidth,settings.highWidth};
    const char* names[]{"LOW","MID","HIGH"};
    for(int n=0;n<3;++n){
        const float y=237.f+n*31,extent=widths[n]*64;
        text(g,names[n],{215,static_cast<int>(y)-8,48,17},9,background);
        g.setColour(muted);g.drawLine(270,y,560,y,1);
        g.setColour(accent);g.fillRect(415-extent,y-4,extent*2,8.f);
        g.setColour(background);g.drawLine(415,y-9,415,y+9,1);
    }
    text(g,"MID",{213,337,52,19},9,background);text(g,"SIDE",{406,337,44,19},9,background);
    g.setColour(muted);g.fillRect(272,344,104,4);g.fillRect(460,344,104,4);
    g.setColour(accent);g.fillRect(272.f,344.f,104*juce::jlimit(0.f,1.f,processor.stereoMid.load()),4.f);
    g.fillRect(460.f,344.f,104*juce::jlimit(0.f,1.f,processor.stereoSide.load()),4.f);
}
void BatchlyEditor::drawMastering(juce::Graphics& g) {
    const auto settings=processor.readRackParameters().quartz;
    g.setColour(ink);g.fillRect(196,184,398,182);g.setColour(rule);g.drawRect(196,184,398,182);
    text(g,"TONE / BALANCE",{211,193,230,20},10,background);
    const float gains[]{settings.lowDb,settings.midDb,settings.highDb};
    const char* names[]{"LOW","MID","HIGH"};
    for(int n=0;n<3;++n){
        const float x=250.f+n*136;
        g.setColour(muted);g.drawLine(x-24,270,x+24,270,1);g.drawLine(x,229,x,311,1);
        const float top=270-std::max(0.f,gains[n])*4;
        g.setColour(accent);g.fillRect(x-14,top,28.f,std::max(2.f,std::abs(gains[n])*4));
        text(g,names[n],{static_cast<int>(x)-30,312,60,18},9,background);
    }
    const float reduction=processor.masterReduction.load();
    text(g,"REDUCTION",{212,337,90,19},9,background);
    g.setColour(muted);g.fillRect(303,344,190,4);
    g.setColour(accent);g.fillRect(303.f,344.f,190*juce::jlimit(0.f,1.f,reduction/18),4.f);
    text(g,juce::String(reduction,1)+" dB",{508,337, 70,19},9,background);
}
void BatchlyEditor::timerCallback() {
    if (shownModule != processor.selectedModule()) showModule(processor.selectedModule());
    leftHistory[historyPosition] = processor.motionLeft.load();
    rightHistory[historyPosition] = processor.motionRight.load();
    historyPosition = (historyPosition + 1) % leftHistory.size();
    meter = std::max(processor.peak.load(), meter * .9f);
    reverbMeter = std::max(processor.reverbLevel.load(), reverbMeter * .93f);
    for (size_t i = 0; i < resonatorMeters.size(); ++i) resonatorMeters[i] = std::max(processor.resonatorLevels[i].load(), resonatorMeters[i] * .9f);
    for (int i = 0; i < batchly::moduleCount; ++i) {
        collectionTabs[i].getProperties().set("effectEnabled", processor.parameters.getRawParameterValue(enabledIds[i])->load() > .5f);
        collectionTabs[i].repaint();
    }
    if (meter > .001f && moduleEnabled.getToggleState() && !bypass.getToggleState())
        reelAngle = std::fmod(reelAngle + .025f + .009f * processor.tapeMovement.load(), juce::MathConstants<float>::twoPi);
    moduleEnabled.setButtonText(juce::String(moduleNames[shownModule]).toUpperCase() + (moduleEnabled.getToggleState() ? " ON" : " OFF"));
    play.setEnabled(processor.loadedFile().existsAsFile() && !processor.isPlaying());
    stop.setEnabled(processor.isPlaying());
    exportButton.setEnabled(processor.loadedFile().existsAsFile() && !exporting);
    const int program = processor.getDisplayedProgram();
    const bool modified = processor.isCurrentProgramModified();
    // ComboBox delivers selection changes asynchronously. Only refresh when the
    // processor changes, or a timer tick can overwrite a pending user selection.
    if (!presets.isPopupActive() && (program != displayedProgram || modified != displayedModified)) {
        presets.setText(processor.getProgramName(program) + (modified ? " *" : ""), juce::dontSendNotification);
        displayedProgram = program; displayedModified = modified;
    }
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
    chooser = std::make_unique<juce::FileChooser>("Export processed audio", source.getSiblingFile(source.getFileNameWithoutExtension() + " - Batchly.wav"), "*.wav");
    auto safe = juce::Component::SafePointer<BatchlyEditor>(this);
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
        [safe, source](const juce::FileChooser& dialog) {
            if (!safe || dialog.getResult() == juce::File()) return;
            const auto destination = dialog.getResult().withFileExtension("wav");
            if (destination != dialog.getResult() && destination.existsAsFile()) {
                safe->report(juce::Result::fail("Choose the exact .wav filename so the file dialog can confirm replacement."), "");
                return;
            }
            const auto settings = safe->processor.readRackParameters();
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
