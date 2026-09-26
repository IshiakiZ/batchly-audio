// SPDX-License-Identifier: AGPL-3.0-or-later
#include "RackEngine.h"
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace batchly;
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
std::vector<float> tone(double sr, double hz, int count) {
    std::vector<float> data(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) data[i] = .15f * static_cast<float>(std::sin(i * 6.28318530718 * hz / sr));
    return data;
}
double energy(const std::vector<float>& data) {
    double sum = 0;
    for (size_t i = data.size() / 2; i < data.size(); ++i) sum += data[i] * data[i];
    return sum / (data.size() - data.size() / 2);
}
int main() {
    try {
        for (double sr : { 8000., 44100., 48000., 96000., 192000. }) {
            const int count = static_cast<int>(sr / 2);
            const auto original = tone(sr, 440, count);
            auto left = original, right = original;
            float* audio[] { left.data(), right.data() };
            PatinaEngine engine; PatinaParameters p;
            engine.prepare(sr, p); engine.process(audio, 2, count, p);
            require(left == original && right == original, "Disabled Patina changed the signal");
            p.enabled = true; p.mix = 0;
            engine.prepare(sr, p); engine.process(audio, 2, count, p);
            require(left == original && right == original, "Patina dry mix changed the signal");
            p.mix = 1; p.chorus = 1; p.wear = 1; p.flutter = 1; p.drive = 1;
            engine.prepare(sr, p); engine.process(audio, 2, count, p);
            require(left != original && left != right, "Wet tape and chorus did not change the audio");
            for (float value : left) require(std::isfinite(value) && std::abs(value) < 2, "Tape processing is unstable");
            left.assign(count, 0); right.assign(count, 0); audio[0] = left.data(); audio[1] = right.data();
            engine.prepare(sr, p); engine.process(audio, 2, count, p);
            require(energy(left) == 0, "Noise-off silence is not silent");
            p.hiss = 1; engine.prepare(sr, p); engine.process(audio, 2, count, p);
            require(energy(left) > 1e-8 && energy(left) < 1e-4, "Generated hiss is absent or too loud");
        }
        PatinaParameters p; p.enabled = true; p.sampleHz = 4000; p.wear = p.flutter = p.chorus = p.drive = 0;
        auto high = tone(48000, 9000, 48000), low = tone(48000, 400, 48000);
        PatinaEngine a, b; a.prepare(48000, p); b.prepare(48000, p);
        float* h[] { high.data() }; float* l[] { low.data() };
        a.process(h, 1, 48000, p); b.process(l, 1, 48000, p);
        require(energy(high) < energy(low) * .001, "Rate reduction failed to suppress high frequencies");
        p = {}; p.enabled = true; p.hiss = .2f;
        auto whole = tone(48000, 443, 48000), chunked = whole;
        a.prepare(48000, p); b.prepare(48000, p);
        float* w[] { whole.data() }; a.process(w, 1, 48000, p);
        for (int offset = 0; offset < 48000; offset += 137) {
            float* block[] { chunked.data() + offset };
            b.process(block, 1, std::min(137, 48000 - offset), p);
        }
        require(whole == chunked, "Tape sound depends on the host block size");
        p.sampleHz = std::numeric_limits<float>::quiet_NaN(); p.drive = std::numeric_limits<float>::infinity();
        whole[100] = p.sampleHz; a.process(w, 1, 48000, p);
        require(std::all_of(whole.begin(), whole.end(), [](float x) { return std::isfinite(x); }), "Invalid values poisoned the tape engine");

        RackParameters rack; rack.drift.outputDb = -3;
        auto legacy = tone(48000, 337, 24000), upgraded = legacy;
        DriftEngine old; RackEngine next;
        old.prepare(48000, rack.drift); next.prepare(48000, rack);
        float* oldAudio[] { legacy.data() }; float* newAudio[] { upgraded.data() };
        old.process(oldAudio, 1, 24000, rack.drift); next.process(newAudio, 1, 24000, rack);
        for (size_t i = 0; i < legacy.size(); ++i) require(std::abs(legacy[i] - upgraded[i]) < 1e-7, "The upgrade changed legacy Drift audio");
        rack.patina.enabled = true; rack.drift.bypass = true;
        auto original = tone(48000, 337, 24000); upgraded = original;
        next.prepare(48000, rack); next.process(newAudio, 1, 24000, rack);
        for (size_t i = 0; i < original.size(); ++i) require(std::abs(original[i] - upgraded[i]) < 1e-7, "Global bypass did not bypass both modules");
        std::cout << "PASS: tape dry/bypass, stereo, silence/hiss, rate filtering, block invariance, invalid input, legacy Drift parity, rack bypass\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
