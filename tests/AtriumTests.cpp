// SPDX-License-Identifier: AGPL-3.0-or-later
#include "RackEngine.h"
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace batchly;
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
using Audio = std::array<std::vector<float>, 2>;
Audio impulse(double sr, double seconds) {
    Audio audio;
    for (auto& channel : audio) { channel.assign(static_cast<size_t>(sr * seconds), 0); channel[0] = .25f; }
    return audio;
}
void render(AtriumEngine& engine, Audio& audio, const AtriumParameters& p, int block = 257, int channels = 2) {
    for (int start = 0; start < static_cast<int>(audio[0].size()); start += block) {
        float* pointers[] { audio[0].data() + start, audio[1].data() + start };
        engine.process(pointers, channels, std::min(block, static_cast<int>(audio[0].size()) - start), p);
    }
}
double energy(const std::vector<float>& audio, int first, int last) {
    double sum = 0;
    for (int i = first; i < last; ++i) sum += static_cast<double>(audio[i]) * audio[i];
    return sum;
}
int main() {
    try {
        for (double sr : { 8000., 44100., 48000., 96000., 192000., 384000. }) {
            AtriumEngine engine; AtriumParameters p;
            auto audio = impulse(sr, 1); const auto original = audio;
            engine.prepare(sr, p); render(engine, audio, p);
            require(audio == original, "Disabled reverb changed audio");
            p.enabled = true; p.mix = 0;
            engine.prepare(sr, p); render(engine, audio, p);
            require(audio == original, "Dry mix changed audio");
            p.mix = 1;
            engine.prepare(sr, p); render(engine, audio, p);
            require(audio[0][0] == 0, "Wet reverb leaked the dry transient");
            require(energy(audio[0], static_cast<int>(sr / 10), static_cast<int>(sr)) > 1e-6, "Reverb has no tail");
            require(audio[0] != audio[1], "Mono input did not produce a stereo room");
            for (auto& channel : audio)
                for (float value : channel) require(std::isfinite(value) && std::abs(value) < 1, "Reverb is unstable");
            p.width = 0; audio = original;
            engine.prepare(sr, p); render(engine, audio, p);
            require(audio[0] == audio[1], "Width zero did not center the tail");
            for (auto& channel : audio) std::fill(channel.begin(), channel.end(), 0.f);
            engine.prepare(sr, p); render(engine, audio, p);
            require(energy(audio[0], 0, static_cast<int>(sr)) == 0, "Reverb generated sound from silence");
        }
        AtriumParameters p; p.enabled = true; p.mix = 1; p.motion = 0; p.preDelayMs = 0;
        AtriumEngine a, b;
        auto immediate = impulse(48000, 2), delayed = immediate;
        a.prepare(48000, p); render(a, immediate, p);
        p.preDelayMs = 73; b.prepare(48000, p); render(b, delayed, p);
        for (int i = 0; i < 3504; ++i) require(delayed[0][i] == 0, "Pre-delay starts too soon");
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 3504; i < 96000; ++i)
                require(std::abs(delayed[ch][i] - immediate[ch][i - 3504]) < 1e-6, "Pre-delay did not shift the response accurately");

        p.preDelayMs = 0; p.decaySeconds = .4f;
        auto shortRoom = impulse(48000, 5), longRoom = shortRoom;
        a.prepare(48000, p); render(a, shortRoom, p);
        p.decaySeconds = 4; b.prepare(48000, p); render(b, longRoom, p);
        const double early = energy(longRoom[0], 24000, 48000);
        const double late = energy(longRoom[0], 144000, 192000);
        require(late > energy(shortRoom[0], 144000, 192000) * 1000, "Longer decay did not sustain the room");
        require(late < early * .01, "Reverb failed to decay");
        std::cout << "4-second room energy, 0.5-1s: " << early << ", 3-4s: " << late << '\n';

        p.motion = .8f; p.size = .95f; p.decaySeconds = 12;
        auto whole = impulse(48000, 3), chunked = whole;
        a.prepare(48000, p); b.prepare(48000, p);
        render(a, whole, p, 144000); render(b, chunked, p, 137);
        require(whole == chunked, "Reverb depends on host block size");
        p.width = 0;
        a.prepare(48000, p); b.prepare(48000, p); whole = impulse(48000, 1); chunked = whole;
        render(a, whole, p, 257, 1); render(b, chunked, p);
        require(whole[0] == chunked[0], "Mono room differs from summed stereo");

        auto probeTone = [&](float cutoff) {
            AtriumParameters settings; settings.enabled = true; settings.mix = 1; settings.motion = 0; settings.lowCutHz = cutoff;
            auto audio = impulse(48000, 2);
            for (int i = 0; i < 96000; ++i) audio[0][i] = audio[1][i] = .1f * std::sin(i * 6.28318530718f * 60 / 48000);
            a.prepare(48000, settings); render(a, audio, settings);
            return energy(audio[0], 48000, 96000);
        };
        require(probeTone(1500) < probeTone(20) * .02, "Low cut failed to remove bass from the room");

        p = {}; p.enabled = true; p.mix = 1;
        a.prepare(48000, p);
        auto automation = impulse(48000, 4);
        for (int i = 0; i < 192000; i += 64) {
            p.decaySeconds = (i / 64) % 2 ? 12.f : .2f; p.size = (i / 64) % 2 ? 1.f : 0.f;
            p.motion = 1; p.preDelayMs = (i / 64) % 2 ? 250.f : 0.f;
            p.dampingHz = (i / 64) % 2 ? 18000.f : 500.f;
            float* block[] { automation[0].data() + i, automation[1].data() + i };
            a.process(block, 2, 64, p);
        }
        for (const auto& channel : automation)
            for (float value : channel) require(std::isfinite(value) && std::abs(value) < 2, "Extreme automation destabilized the room");
        p.decaySeconds = std::numeric_limits<float>::quiet_NaN(); p.size = std::numeric_limits<float>::infinity();
        automation[0][0] = p.decaySeconds; render(a, automation, p);
        for (float value : automation[0]) require(std::isfinite(value), "Invalid input poisoned reverb state");

        // Full-rack bypass must preserve the original input even with a long tail.
        RackParameters settings; settings.atrium.enabled = true; settings.patina.enabled = true; settings.drift.bypass = true;
        RackEngine rack; rack.prepare(48000, settings);
        auto rackAudio = impulse(48000, 1); const auto original = rackAudio;
        float* block[] { rackAudio[0].data(), rackAudio[1].data() };
        rack.process(block, 2, 48000, settings);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 48000; ++i) require(std::abs(rackAudio[ch][i] - original[ch][i]) < 1e-7, "Three-module bypass changed audio");
        require(AtriumEngine::tailSeconds(settings.atrium) > settings.atrium.decaySeconds * 2, "Export tail is too short");
        std::cout << "PASS: reverb off/dry, stereo/mono/width, silence, pre-delay, decay, low cut, block invariance, automation, invalid input, rack bypass\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
