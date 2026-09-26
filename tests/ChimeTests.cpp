// SPDX-License-Identifier: AGPL-3.0-or-later
#include "RackEngine.h"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace batchly;
using Audio = std::array<std::vector<float>, 2>;
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
Audio impulse(int sr, int seconds = 3) {
    Audio a { std::vector<float>(sr * seconds), std::vector<float>(sr * seconds) };
    a[0][0] = a[1][0] = .5f; return a;
}
Audio render(Audio audio, int sr, const ChimeParameters& p, int block = 257, int channels = 2) {
    ChimeEngine engine; engine.prepare(sr, p);
    for (int offset = 0; offset < static_cast<int>(audio[0].size()); offset += block) {
        float* data[] { audio[0].data() + offset, audio[1].data() + offset };
        engine.process(data, channels, std::min(block, static_cast<int>(audio[0].size()) - offset), p);
    }
    return audio;
}
double energy(const Audio& a, int from, int to) {
    double result = 0; for (int i = from; i < to; ++i) result += a[0][i] * a[0][i]; return result;
}
double magnitude(const Audio& a, double frequency, int sr) {
    double re = 0, im = 0;
    for (size_t i = 0; i < a[0].size(); ++i) {
        const double phase = 6.28318530718 * frequency * i / sr;
        re += a[0][i] * std::cos(phase); im += a[0][i] * std::sin(phase);
    }
    return std::sqrt(re * re + im * im);
}
int main() {
    try {
        ChimeParameters p; p.enabled = true; p.mix = 1; p.motion = p.detune = 0;
        for (int sr : { 8000, 44100, 48000, 96000, 192000 }) {
            const auto input = impulse(sr);
            auto off = p; off.enabled = false;
            require(render(input, sr, off) == input, "Disabled resonator altered audio");
            off = p; off.mix = 0;
            require(render(input, sr, off) == input, "Dry path altered audio");
            auto wet = render(input, sr, p);
            require(energy(wet, sr / 10, sr / 2) > 1e-7, "Resonator did not ring");
            require(energy(wet, sr * 2, sr * 3) < energy(wet, 0, sr) * 1e-6, "Resonator did not decay");
            require(wet == render(input, sr, p, 509), "Processing depends on block size");
            bool stereo = false;
            for (size_t i = 0; i < wet[0].size(); ++i) {
                require(std::isfinite(wet[0][i]) && std::abs(wet[0][i]) < 1, "Unbounded impulse response");
                stereo |= wet[0][i] != wet[1][i];
            }
            require(stereo, "Stereo spread missing");
            auto centered = p; centered.width = 0;
            wet = render(input, sr, centered);
            require(wet[0] == wet[1], "Zero width not centered");
            wet = render(input, sr, p, 257, 1);
            require(energy(wet, 1, sr) > 0 && wet[1] == input[1], "Mono processing touched another channel");
            auto silence = input; silence[0][0] = silence[1][0] = 0;
            require(render(silence, sr, p) == silence, "Silence generated sound");
        }
        p.spread = p.width = p.drive = 0; p.scale = 0;
        auto wet = render(impulse(48000), 48000, p);
        const double c3 = 130.81278265;
        require(magnitude(wet, c3, 48000) > magnitude(wet, c3 * std::pow(2.0, 1.0/12), 48000) * 8, "Root is not tuned to C3");
        p.root = 2; p.octave = 4;
        wet = render(impulse(48000), 48000, p);
        require(magnitude(wet, 293.6647679, 48000) > magnitude(wet, c3, 48000) * 8, "Root/octave change failed");
        p = {}; p.enabled = true; p.mix = 1; p.ringSeconds = 6;
        auto longRing = render(impulse(48000, 8), 48000, p);
        require(energy(longRing, 48000 * 7, 48000 * 8) < energy(longRing, 0, 48000) * 1e-6, "Long ring tail allowance insufficient");
        ChimeEngine engine; engine.prepare(48000, p);
        Audio block { std::vector<float>(257), std::vector<float>(257) };
        float* data[] { block[0].data(), block[1].data() };
        for (int step = 0; step < 1400; ++step) {
            p.root = step % 12; p.scale = step % 6; p.octave = step % 3 + 2;
            p.ringSeconds = step % 2 ? .08f : 6; p.colorHz = step % 2 ? 500 : 16000;
            p.width = p.spread = p.drive = p.detune = p.motion = step % 2 ? 0.f : 1.f;
            p.enabled = step % 11 != 0;
            for (int i = 0; i < 257; ++i) block[0][i] = block[1][i] = .8f * std::sin((step * 257 + i) * .071);
            engine.process(data, 2, 257, p);
            for (const auto& channel : block) for (float sample : channel)
                require(std::isfinite(sample) && std::abs(sample) < 2, "Automation made unstable audio");
        }
        p.ringSeconds = std::numeric_limits<float>::quiet_NaN(); p.colorHz = std::numeric_limits<float>::infinity();
        p.root = -100; p.scale = 300; p.octave = -50; p.enabled = true;
        block[0][0] = std::numeric_limits<float>::quiet_NaN(); block[1][0] = std::numeric_limits<float>::infinity();
        engine.process(data, 2, 257, p);
        for (const auto& channel : block) for (float sample : channel) require(std::isfinite(sample), "Invalid input escaped");
        RackParameters rackSettings; rackSettings.chime.enabled = true; rackSettings.atrium.enabled = true; rackSettings.drift.bypass = true;
        RackEngine rack; rack.prepare(48000, rackSettings);
        auto audio = impulse(48000, 1); const auto original = audio;
        float* rackData[] { audio[0].data(), audio[1].data() }; rack.process(rackData, 2, 48000, rackSettings);
        require(audio == original, "Whole-rack bypass changed input");
        require(RackEngine::tailSeconds(rackSettings) >= AtriumEngine::tailSeconds(rackSettings.atrium) + ChimeEngine::tailSeconds(rackSettings.chime), "Serial tails must add");
        std::cout << "PASS: tuned roots/octaves, ring decay, stereo/mono, width, dry/off, silence, sample rates, block invariance, automation, invalid input, rack bypass and tails\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
