// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace batchly {
struct PatinaParameters {
    float sampleHz = 16000, drive = .25f, wear = .2f, flutter = .12f;
    float hiss = 0, chorus = .18f, toneHz = 11000, mix = 1;
    bool enabled = false;
};

class PatinaEngine {
public:
    void prepare(double sampleRate, const PatinaParameters& settings = {}) {
        sr = std::isfinite(sampleRate) ? std::clamp(sampleRate, 8000.0, 384000.0) : 48000;
        current = sanitise(settings);
        smoothing = static_cast<float>(1 - std::exp(-1 / (.025 * sr)));
        active = current.enabled ? 1.f : 0.f;
        for (auto& line : delay) line.assign(static_cast<size_t>(sr * .05) + 8, 0);
        pre = {}; post = {}; held = {}; previous = {}; noiseLow = {};
        saturatedPrevious = {}; antiderivativePrevious = {}; dcInput = {}; dcOutput = {};
        writeIndex = 0; sampleClock = 0; wowPhase = flutterPhase = chorusPhase = 0;
        coefficientClock = 0; noiseSeed = 0x1b7d4853u; motion = {};
        dcPole = static_cast<float>(std::exp(-2 * pi * 12 / sr));
        updateFilters();
    }

    void process(float* const* audio, int channels, int samples, const PatinaParameters& settings) noexcept {
        if (!audio || channels < 1 || samples < 1 || delay[0].empty()) return;
        channels = std::min(channels, 2);
        const auto target = sanitise(settings);
        for (int i = 0; i < samples; ++i) {
            smooth(current.sampleHz, target.sampleHz); smooth(current.drive, target.drive);
            smooth(current.wear, target.wear); smooth(current.flutter, target.flutter);
            smooth(current.hiss, target.hiss); smooth(current.chorus, target.chorus);
            smooth(current.toneHz, target.toneHz); smooth(current.mix, target.mix);
            smooth(active, target.enabled ? 1.f : 0.f);
            if (coefficientClock++ % 32 == 0) updateFilters();
            advance(wowPhase, .37); advance(flutterPhase, 7.9); advance(chorusPhase, .63);
            const float wow = static_cast<float>(.74 * std::sin(2 * pi * wowPhase)
                + .26 * std::sin(2 * pi * (wowPhase * 3 + .17)));
            const float flutter = static_cast<float>(std::sin(2 * pi * flutterPhase));
            const double increment = std::min(1.0, current.sampleHz / sr);
            const double before = sampleClock;
            sampleClock += increment;
            const bool capture = sampleClock >= 1;
            const float fraction = capture ? static_cast<float>((1 - before) / increment) : 0;
            if (capture) sampleClock -= 1;
            const float driveGain = std::pow(10.f, current.drive * .9f);
            const float compensation = 1 / std::sqrt(driveGain);
            for (int ch = 0; ch < channels; ++ch) {
                const float dry = std::isfinite(audio[ch][i]) ? audio[ch][i] : 0;
                const float boundedInput = std::clamp(dry, -16.f, 16.f);
                const double driven = boundedInput * driveGain;
                // Integrating tanh between input samples reduces sharp nonlinear transitions.
                // log(cosh(x)) is evaluated in a form that cannot overflow at high input levels.
                const double magnitude = std::abs(driven);
                const double integral = magnitude + std::log1p(std::exp(-2 * magnitude)) - std::log(2.0);
                const double difference = driven - saturatedPrevious[ch];
                const double shaped = std::abs(difference) > 1e-5
                    ? (integral - antiderivativePrevious[ch]) / difference
                    : std::tanh(.5 * (driven + saturatedPrevious[ch]));
                saturatedPrevious[ch] = driven; antiderivativePrevious[ch] = integral;
                const float saturated = boundedInput + current.drive * (static_cast<float>(shaped) * compensation - boundedInput);
                float filtered = saturated;
                for (auto& filter : pre[ch]) filtered = filter.tick(filtered);
                if (capture) held[ch] = previous[ch] + fraction * (filtered - previous[ch]);
                previous[ch] = filtered;
                float reduced = held[ch];
                for (auto& filter : post[ch]) reduced = filter.tick(reduced);
                delay[ch][writeIndex] = reduced;
                const float movement = current.wear * wow + .16f * current.flutter * flutter;
                motion[ch] = movement;
                float wet = read(ch, .012f + .003f * movement);
                const float chorusMotion = static_cast<float>(std::sin(2 * pi * (chorusPhase + ch * .25)));
                const float chorusSample = read(ch, .019f + .0025f * chorusMotion + .003f * movement);
                wet += current.chorus * .45f * (chorusSample - wet);
                const float noise = random();
                noiseLow[ch] += .07f * (noise - noiseLow[ch]);
                wet += .003f * current.hiss * current.hiss * (noise - noiseLow[ch]);
                const float dcRemoved = wet - dcInput[ch] + dcPole * dcOutput[ch];
                dcInput[ch] = wet; dcOutput[ch] = dcRemoved;
                const float blend = active * current.mix;
                audio[ch][i] = dry + blend * (dcRemoved - dry);
            }
            writeIndex = (writeIndex + 1) % delay[0].size();
        }
    }
    std::array<float, 2> getMovement() const noexcept { return motion; }

private:
    static constexpr double pi = 3.14159265358979323846;
    struct LowPass {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        void configure(double cutoff, double sampleRate, double q) noexcept {
            const double w = 2 * pi * cutoff / sampleRate, c = std::cos(w), alpha = std::sin(w) / (2 * q);
            const double divisor = 1 + alpha;
            b0 = (1 - c) / (2 * divisor); b1 = 2 * b0; b2 = b0;
            a1 = -2 * c / divisor; a2 = (1 - alpha) / divisor;
        }
        float tick(float x) noexcept {
            const double y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y;
            return static_cast<float>(y);
        }
    };
    void updateFilters() noexcept {
        const double cutoff = std::min(current.sampleHz * .40, sr * .40);
        // Fourth-order filters around the rate reducer soften aliases and staircase images.
        // These are finite-slope filters, not a brick-wall or a commercial circuit model.
        for (int ch = 0; ch < 2; ++ch) {
            pre[ch][0].configure(cutoff, sr, .5411961); pre[ch][1].configure(cutoff, sr, 1.306563);
            post[ch][0].configure(std::min(cutoff, static_cast<double>(current.toneHz)), sr, .5411961);
            post[ch][1].configure(std::min(cutoff, static_cast<double>(current.toneHz)), sr, 1.306563);
        }
    }
    float read(int ch, float seconds) const noexcept {
        double position = static_cast<double>(writeIndex) - sr * seconds;
        if (position < 0) position += delay[ch].size();
        const auto index = static_cast<size_t>(position);
        const float fraction = static_cast<float>(position - index);
        return delay[ch][index] + fraction * (delay[ch][(index + 1) % delay[ch].size()] - delay[ch][index]);
    }
    static PatinaParameters sanitise(PatinaParameters p) noexcept {
        auto bound = [](float value, float low, float high, float fallback) {
            return std::isfinite(value) ? std::clamp(value, low, high) : fallback;
        };
        p.sampleHz = bound(p.sampleHz, 2000, 48000, 16000); p.drive = bound(p.drive, 0, 1, .25f);
        p.wear = bound(p.wear, 0, 1, .2f); p.flutter = bound(p.flutter, 0, 1, .12f);
        p.hiss = bound(p.hiss, 0, 1, 0); p.chorus = bound(p.chorus, 0, 1, .18f);
        p.toneHz = bound(p.toneHz, 400, 18000, 11000); p.mix = bound(p.mix, 0, 1, 1);
        return p;
    }
    void smooth(float& value, float target) noexcept { value += smoothing * (target - value); }
    void advance(double& phase, double hz) noexcept { phase += hz / sr; if (phase >= 1) phase -= 1; }
    float random() noexcept {
        noiseSeed ^= noiseSeed << 13; noiseSeed ^= noiseSeed >> 17; noiseSeed ^= noiseSeed << 5;
        return static_cast<float>(noiseSeed >> 8) / 8388607.5f - 1;
    }
    double sr = 48000, sampleClock = 0, wowPhase = 0, flutterPhase = 0, chorusPhase = 0;
    float smoothing = 0, active = 0, dcPole = 0;
    uint32_t noiseSeed = 1, coefficientClock = 0;
    size_t writeIndex = 0;
    PatinaParameters current;
    std::array<std::vector<float>, 2> delay;
    std::array<std::array<LowPass, 2>, 2> pre, post;
    std::array<float, 2> held {}, previous {}, noiseLow {}, dcInput {}, dcOutput {}, motion {};
    std::array<double, 2> saturatedPrevious {}, antiderivativePrevious {};
};
}
