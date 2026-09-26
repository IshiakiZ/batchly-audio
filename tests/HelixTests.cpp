// SPDX-License-Identifier: AGPL-3.0-or-later
#include "RackEngine.h"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace batchly;
using Audio = std::array<std::vector<float>, 2>;
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
Audio tone(int sr, double hz = 443) {
    Audio a { std::vector<float>(sr * 2), std::vector<float>(sr * 2) };
    for (int i = 0; i < sr * 2; ++i) a[0][i] = a[1][i] = .15f * static_cast<float>(std::sin(i * 6.28318530718 * hz / sr));
    return a;
}
Audio render(Audio audio, int sr, const HelixParameters& p, int block = 257, int channels = 2) {
    HelixEngine engine; engine.prepare(sr, p);
    for (int offset = 0; offset < static_cast<int>(audio[0].size()); offset += block) {
        float* data[] { audio[0].data() + offset, audio[1].data() + offset };
        engine.process(data, channels, std::min(block, static_cast<int>(audio[0].size()) - offset), p);
    }
    return audio;
}
double energy(const std::vector<float>& a, int from = 0) {
    double e = 0; for (size_t i = from; i < a.size(); ++i) e += a[i] * a[i]; return e;
}
double difference(const Audio& a, const Audio& b) {
    double sum = 0; for (size_t i = 0; i < a[0].size(); ++i) sum += std::pow(a[0][i] - b[0][i], 2); return sum;
}
int main() {
    try {
        HelixParameters p; p.enabled = true;
        for (int sr : {8000, 44100, 48000, 96000, 192000, 384000}) {
            const auto input = tone(sr); auto off = p; off.enabled = false;
            require(render(input, sr, off) == input, "Disabled phaser changed audio");
            off = p; off.mix = 0; require(render(input, sr, off) == input, "Dry path changed audio");
            const auto wet = render(input, sr, p);
            require(difference(wet, input) > 1, "No audible phase movement");
            require(wet == render(input, sr, p, 509), "Phaser depends on block size");
            require(wet[0] != wet[1], "Stereo offset missing");
            auto centered = p; centered.width = 0; const auto monoWidth = render(input, sr, centered);
            require(monoWidth[0] == monoWidth[1], "Zero width did not align channels");
            const auto mono = render(input, sr, p, 257, 1); require(mono[1] == input[1], "Mono changed other channel");
            auto silence = input; for (auto& ch : silence) std::fill(ch.begin(), ch.end(), 0.f);
            require(render(silence, sr, p) == silence, "Phaser generated audio from silence");
            for (const auto& ch : wet) for (float sample : ch) require(std::isfinite(sample) && std::abs(sample) < 1, "Unbounded normal output");
        }
        // With fixed stages and no feedback/drive, fully wet should mainly change phase.
        // Blending dry audio should create deep cancellations at some frequencies.
        p.depth = p.feedback = p.drive = p.width = 0; p.toneHz = 18000;
        double smallestBlend = 1;
        for (double hz : {90, 160, 280, 440, 660, 930, 1300, 1900, 2700}) {
            const auto input = tone(48000, hz); p.mix = 1;
            const auto wet = render(input, 48000, p);
            const double ratio = energy(wet[0], 24000) / energy(input[0], 24000);
            require(ratio > .9 && ratio < 1.01, "Static all-pass altered low/mid frequency magnitude");
            p.mix = .5; const auto blended = render(input, 48000, p);
            smallestBlend = std::min(smallestBlend, energy(blended[0], 24000) / energy(input[0], 24000));
        }
        require(smallestBlend < .08, "Dry/wet blend did not create phase notches");
        p.enabled = true; p.depth = 1; p.rateHz = 8; p.centerHz = 100; p.feedback = .85f; p.mix = 1;
        Audio impulse { std::vector<float>(48000 * 3), std::vector<float>(48000 * 3) }; impulse[0][0] = impulse[1][0] = .5f;
        const auto tail = render(impulse, 48000, p); require(energy(tail[0], 48000 * 2) < 1e-12, "Tail did not settle within export allowance");
        HelixEngine engine; engine.prepare(48000, p); Audio block { std::vector<float>(257), std::vector<float>(257) };
        float* data[] {block[0].data(), block[1].data()};
        for (int step = 0; step < 1500; ++step) {
            p.rateHz = step % 2 ? .03f : 8; p.depth = step % 3 ? 1.f : 0.f;
            p.centerHz = step % 2 ? 100 : 6000; p.feedback = step % 2 ? -.85f : .85f;
            p.toneHz = step % 2 ? 500 : 18000; p.drive = p.width = step % 2 ? 1.f : 0.f; p.enabled = step % 11 != 0;
            for (int i = 0; i < 257; ++i) block[0][i] = block[1][i] = .8f * std::sin((step * 257 + i) * .071);
            engine.process(data, 2, 257, p);
            for (const auto& ch : block) for (float sample : ch) require(std::isfinite(sample) && std::abs(sample) < 4, "Extreme automation unstable");
        }
        p.enabled = true; p.centerHz = std::numeric_limits<float>::quiet_NaN(); p.feedback = std::numeric_limits<float>::infinity();
        block[0][0] = std::numeric_limits<float>::quiet_NaN(); block[1][0] = std::numeric_limits<float>::infinity();
        engine.process(data, 2, 257, p);
        for (const auto& ch : block) for (float sample : ch) require(std::isfinite(sample), "Non-finite input escaped");
        std::cout << "PASS: dry/off, phase cancellation, static all-pass gain, mono/stereo, width, silence, sample rates, block invariance, tail, automation and invalid input\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
