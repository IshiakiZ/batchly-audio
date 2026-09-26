// SPDX-License-Identifier: AGPL-3.0-or-later
#include "DriftEngine.h"
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace batchly;
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
std::vector<float> signal(int count) {
    std::vector<float> values(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) values[i] = .2f * std::sin(static_cast<float>(i) * .031f);
    return values;
}
int main() {
    try {
        for (double sr : { 8000., 44100., 48000., 96000., 192000. }) {
            auto original = signal(24000), left = original, right = original;
            float* data[] = { left.data(), right.data() };
            DriftEngine engine;
            DriftParameters p; p.mix = 0;
            engine.prepare(sr, p); engine.process(data, 2, 24000, p);
            require(left == original && right == original, "Dry mix must preserve input exactly");
            p.mix = 1; p.bypass = true;
            engine.prepare(sr, p); engine.process(data, 2, 24000, p);
            for (size_t i = 0; i < left.size(); ++i)
                require(std::abs(left[i] - original[i]) < 1e-7f, "Bypass must preserve input");
            p.bypass = false; p.depth = 1; p.rateHz = 8; p.width = 1; p.noise = 1; p.follow = 1;
            engine.prepare(sr, p); engine.process(data, 2, 24000, p);
            require(left != original && left != right, "Wet stereo modulation must be audible and wide");
            for (float sample : left) require(std::isfinite(sample) && std::abs(sample) < 1, "Extreme setting is unstable");
            left.assign(24000, 0); right.assign(24000, 0);
            data[0] = left.data(); data[1] = right.data(); p.noise = 0;
            engine.prepare(sr, p); engine.process(data, 2, 24000, p);
            require(std::all_of(left.begin(), left.end(), [](float x) { return x == 0; }), "Noise-off silence must stay silent");
        }
        auto whole = signal(30000), chunked = whole;
        DriftEngine a, b; DriftParameters p;
        a.prepare(48000, p); b.prepare(48000, p);
        float* one[] = { whole.data() }; a.process(one, 1, 30000, p);
        for (int offset = 0; offset < 30000; offset += 137) {
            float* part[] = { chunked.data() + offset };
            b.process(part, 1, std::min(137, 30000 - offset), p);
        }
        require(whole == chunked, "Rendering must not depend on host buffer size");
        p.rateHz = std::numeric_limits<float>::quiet_NaN(); p.toneHz = -999;
        p.mix = std::numeric_limits<float>::infinity(); whole[2] = p.rateHz;
        a.process(one, 1, 30000, p);
        require(std::all_of(whole.begin(), whole.end(), [](float x) { return std::isfinite(x); }), "Invalid input poisoned the engine");
        a.process(nullptr, 0, 0, p);
        std::cout << "PASS: dry, bypass, stereo, extremes, silence, 5 sample rates, mono, block invariance, invalid inputs\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
