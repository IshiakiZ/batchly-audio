// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace batchly {
struct DriftParameters {
    float depth = 0.35f;
    float rateHz = 0.45f;
    float wander = 0.65f;
    float toneHz = 7000.0f;
    float follow = 0.0f;
    float noise = 0.0f;
    float width = 0.75f;
    float mix = 0.5f;
    float outputDb = 0.0f;
    bool bypass = false;
};

class DriftEngine {
public:
    void prepare(double sampleRate, const DriftParameters& parameters = {}) {
        sr = std::isfinite(sampleRate) ? std::clamp(sampleRate, 8000.0, 384000.0) : 48000.0;
        for (auto& channel : delay) channel.assign(static_cast<size_t>(sr * 0.06) + 8, 0.0f);
        smoothing = static_cast<float>(1.0 - std::exp(-1.0 / (0.025 * sr)));
        attack = static_cast<float>(1.0 - std::exp(-1.0 / (0.008 * sr)));
        release = static_cast<float>(1.0 - std::exp(-1.0 / (0.16 * sr)));
        dcPole = static_cast<float>(std::exp(-2.0 * pi * 25.0 / sr));
        current = sanitise(parameters);
        writeIndex = 0;
        phase = 0.0;
        randomState = 0x54832ab1u;
        noiseState = 0x1d84f503u;
        lowpass = {}; dcInput = {}; dcOutput = {}; envelope = 0.0f;
        randomFrom = { random(randomState), random(randomState) };
        randomTo = { random(randomState), random(randomState) };
        bypassBlend = current.bypass ? 1.0f : 0.0f;
        modulation = {};
    }

    void process(float* const* audio, int channels, int samples, const DriftParameters& requested) noexcept {
        if (channels < 1 || samples < 1 || delay[0].empty() || audio == nullptr) return;
        channels = std::min(channels, 2);
        const auto target = sanitise(requested);
        for (int i = 0; i < samples; ++i) {
            smooth(current.depth, target.depth); smooth(current.rateHz, target.rateHz);
            smooth(current.wander, target.wander); smooth(current.toneHz, target.toneHz);
            smooth(current.follow, target.follow); smooth(current.noise, target.noise);
            smooth(current.width, target.width); smooth(current.mix, target.mix);
            smooth(current.outputDb, target.outputDb);
            smooth(bypassBlend, target.bypass ? 1.0f : 0.0f);
            phase += current.rateHz / sr;
            if (phase >= 1.0) {
                phase -= 1.0;
                randomFrom = randomTo;
                randomTo = { random(randomState), random(randomState) };
            }
            // A smooth random curve gives drifting pitch without discontinuous delay jumps.
            const float t = static_cast<float>(phase);
            const float curve = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
            std::array<float, 2> dry {};
            for (int ch = 0; ch < channels; ++ch)
                dry[ch] = std::isfinite(audio[ch][i]) ? audio[ch][i] : 0.0f;
            float level = std::abs(dry[0]);
            if (channels == 2) level = std::max(level, std::abs(dry[1]));
            envelope += (level > envelope ? attack : release) * (level - envelope);
            const float cutoff = std::clamp(current.toneHz * (1.0f + current.follow * std::min(2.0f, envelope * 5.0f)),
                                             250.0f, static_cast<float>(sr * 0.43));
            const float filterAmount = static_cast<float>(1.0 - std::exp(-2.0 * pi * cutoff / sr));
            const float gain = std::pow(10.0f, current.outputDb / 20.0f);
            const float leftRandom = randomFrom[0] + curve * (randomTo[0] - randomFrom[0]);
            const float rightRandom = randomFrom[1] + curve * (randomTo[1] - randomFrom[1]);
            const float noiseLevel = 0.004f * current.noise * current.noise;
            for (int ch = 0; ch < channels; ++ch) {
                const float spread = ch == 1 ? current.width : 0.0f;
                const float periodic = static_cast<float>(std::sin(2.0 * pi * (phase + 0.25 * spread)));
                const float wandering = leftRandom + spread * (rightRandom - leftRandom);
                const float movement = periodic + current.wander * (wandering - periodic);
                modulation[ch] = current.depth * movement;
                delay[ch][writeIndex] = dry[ch];
                const float delaySamples = static_cast<float>(sr) * (0.014f + 0.006f * modulation[ch]);
                float readPosition = static_cast<float>(writeIndex) - delaySamples;
                if (readPosition < 0.0f) readPosition += static_cast<float>(delay[ch].size());
                const auto index = static_cast<size_t>(readPosition);
                const float fraction = readPosition - static_cast<float>(index);
                const float wetSample = delay[ch][index] + fraction *
                    (delay[ch][(index + 1) % delay[ch].size()] - delay[ch][index]);
                lowpass[ch] += filterAmount * (wetSample + noiseLevel * random(noiseState) - lowpass[ch]);
                // Remove DC from the wet branch while leaving the dry signal untouched.
                const float wet = lowpass[ch] - dcInput[ch] + dcPole * dcOutput[ch];
                dcInput[ch] = lowpass[ch]; dcOutput[ch] = wet;
                const float effected = (dry[ch] + current.mix * (wet - dry[ch])) * gain;
                audio[ch][i] = effected + bypassBlend * (dry[ch] - effected);
            }
            writeIndex = (writeIndex + 1) % delay[0].size();
        }
    }

    std::array<float, 2> getModulation() const noexcept { return modulation; }

private:
    static constexpr double pi = 3.14159265358979323846;
    static float bound(float value, float low, float high, float fallback) noexcept {
        return std::isfinite(value) ? std::clamp(value, low, high) : fallback;
    }
    static DriftParameters sanitise(DriftParameters p) noexcept {
        p.depth = bound(p.depth, 0, 1, .35f); p.rateHz = bound(p.rateHz, .05f, 8, .45f);
        p.wander = bound(p.wander, 0, 1, .65f); p.toneHz = bound(p.toneHz, 250, 18000, 7000);
        p.follow = bound(p.follow, 0, 1, 0); p.noise = bound(p.noise, 0, 1, 0);
        p.width = bound(p.width, 0, 1, .75f); p.mix = bound(p.mix, 0, 1, .5f);
        p.outputDb = bound(p.outputDb, -24, 12, 0);
        return p;
    }
    static float random(uint32_t& state) noexcept {
        // A private deterministic generator keeps render results repeatable and audio-thread safe.
        state ^= state << 13; state ^= state >> 17; state ^= state << 5;
        return static_cast<float>(state >> 8) / 8388607.5f - 1.0f;
    }
    void smooth(float& value, float target) noexcept { value += smoothing * (target - value); }
    double sr = 48000, phase = 0;
    size_t writeIndex = 0;
    float smoothing = 0, attack = 0, release = 0, dcPole = 0, envelope = 0, bypassBlend = 0;
    uint32_t randomState = 1, noiseState = 2;
    DriftParameters current;
    std::array<std::vector<float>, 2> delay;
    std::array<float, 2> randomFrom {}, randomTo {}, lowpass {}, dcInput {}, dcOutput {}, modulation {};
};
}
