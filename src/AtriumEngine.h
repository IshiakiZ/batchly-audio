// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace batchly {
struct AtriumParameters {
    float decaySeconds = 2.4f, size = .55f, preDelayMs = 24, dampingHz = 6500;
    float lowCutHz = 120, motion = .2f, width = 1, mix = .25f;
    bool enabled = false;
};

class AtriumEngine {
public:
    void prepare(double sampleRate, const AtriumParameters& settings = {}) {
        sr = std::isfinite(sampleRate) ? std::clamp(sampleRate, 8000.0, 384000.0) : 48000;
        smooth = static_cast<float>(1 - std::exp(-1 / (.04 * sr)));
        values = sanitized(settings); enabled = settings.enabled ? 1.f : 0.f;
        for (auto& delay : lines) delay.prepare(static_cast<int>(sr * .15) + 8);
        for (auto& delay : preDelay) delay.prepare(static_cast<int>(sr * .251) + 8);
        for (int ch = 0; ch < 2; ++ch) {
            for (int stage = 0; stage < 3; ++stage) {
                diffuserLengths[ch][stage] = static_cast<int>(sr * (.0047 + .0023 * stage + .0007 * ch));
                diffusers[ch][stage].prepare(diffuserLengths[ch][stage] + 2);
            }
        }
        damped.fill(0); inputLow.fill(0); phase.fill(0); tick = 0; wetPeak = 0;
        updateNetwork();
        delays = targetDelays;
    }

    // Two decay periods leave a conservative tail allowance, including pre-delay
    // and the upstream rack's short modulation delays, for offline file exports.
    static double tailSeconds(const AtriumParameters& settings) noexcept {
        return 2.0 * safe(settings.decaySeconds, .2f, 12.f, 2.4f)
            + safe(settings.preDelayMs, 0.f, 250.f, 24.f) * .001 + .8;
    }
    float getWetPeak() const noexcept { return wetPeak; }

    void process(float* const* audio, int channels, int samples, const AtriumParameters& settings) noexcept {
        if (!audio || channels < 1 || samples < 1 || lines[0].data.empty()) return;
        channels = std::min(channels, 2);
        const auto targets = sanitized(settings);
        wetPeak = 0;
        for (int sample = 0; sample < samples; ++sample) {
            for (size_t i = 0; i < values.size(); ++i) values[i] += smooth * (targets[i] - values[i]);
            enabled += smooth * ((settings.enabled ? 1.f : 0.f) - enabled);
            if (++tick == 32) { tick = 0; updateNetwork(); }
            std::array<float, 2> dry { finite(audio[0][sample]), finite(audio[channels - 1][sample]) };
            std::array<float, 2> input {};
            for (int ch = 0; ch < 2; ++ch) {
                const float bounded = std::clamp(dry[ch], -16.f, 16.f) * enabled;
                inputLow[ch] = flush(inputLow[ch] + highPassCoefficient * (bounded - inputLow[ch]));
                const float filtered = bounded - inputLow[ch];
                preDelay[ch].write(filtered);
                input[ch] = preDelay[ch].read(static_cast<float>(sr * values[2] * .001));
                preDelay[ch].advance();
                for (int stage = 0; stage < 3; ++stage) {
                    auto& diffuser = diffusers[ch][stage];
                    const float delayed = diffuser.read(static_cast<float>(diffuserLengths[ch][stage]));
                    const float output = delayed - .6f * input[ch];
                    diffuser.write(input[ch] + .6f * output); diffuser.advance();
                    input[ch] = output;
                }
            }
            std::array<float, 8> returns {};
            float sum = 0;
            for (size_t line = 0; line < lines.size(); ++line) {
                // Ramp each read position between coefficient updates to avoid
                // stepping the delay while Size or Motion is being automated.
                delays[line] += (targetDelays[line] - delays[line]) / static_cast<float>(32 - tick);
                const float value = lines[line].read(delays[line]);
                damped[line] = flush(damped[line] + dampingCoefficient * (value - damped[line]));
                returns[line] = damped[line] * gains[line];
                sum += returns[line];
            }
            float left = 0, right = 0;
            for (size_t line = 0; line < lines.size(); ++line) {
                // The Householder reflection mixes all eight paths without adding
                // energy. Decay gains below one then make the feedback die away.
                const float feedback = .25f * sum - returns[line];
                const float injection = .25f * (input[0] * inputLeft[line] + input[1] * inputRight[line]);
                lines[line].write(flush(feedback + injection)); lines[line].advance();
                left += returns[line] * outputLeft[line] * .5f;
                right += returns[line] * outputRight[line] * .5f;
            }
            const float mid = .5f * (left + right), side = .5f * (left - right) * values[6];
            const std::array<float, 2> wet { mid + side, mid - side };
            const float blend = values[7] * enabled;
            for (int ch = 0; ch < channels; ++ch) {
                const float reverb = channels == 1 ? mid : wet[ch];
                audio[ch][sample] = dry[ch] + blend * (reverb - dry[ch]);
                wetPeak = std::max(wetPeak, std::abs(reverb * enabled));
            }
        }
    }
private:
    struct Delay {
        std::vector<float> data;
        int position = 0;
        void prepare(int capacity) { data.assign(static_cast<size_t>(capacity), 0); position = 0; }
        void write(float value) noexcept { data[static_cast<size_t>(position)] = value; }
        void advance() noexcept { if (++position == static_cast<int>(data.size())) position = 0; }
        float read(float distance) const noexcept {
            // Separate the fractional delay from the integer ring position so
            // interpolation precision stays the same across buffer wraparound.
            const int integer = static_cast<int>(distance);
            const float fraction = distance - integer;
            int recent = position - integer;
            if (recent < 0) recent += static_cast<int>(data.size());
            const int older = recent == 0 ? static_cast<int>(data.size()) - 1 : recent - 1;
            return data[static_cast<size_t>(recent)] * (1 - fraction) + data[static_cast<size_t>(older)] * fraction;
        }
    };
    static float finite(float value) noexcept { return std::isfinite(value) ? value : 0; }
    static float flush(float value) noexcept { return std::abs(value) < 1e-20f ? 0 : value; }
    static float safe(float value, float lo, float hi, float fallback) noexcept {
        return std::isfinite(value) ? std::clamp(value, lo, hi) : fallback;
    }
    static std::array<float, 8> sanitized(const AtriumParameters& p) noexcept {
        return { safe(p.decaySeconds, .2f, 12.f, 2.4f), safe(p.size, 0.f, 1.f, .55f),
            safe(p.preDelayMs, 0.f, 250.f, 24.f), safe(p.dampingHz, 500.f, 18000.f, 6500.f),
            safe(p.lowCutHz, 20.f, 2000.f, 120.f), safe(p.motion, 0.f, 1.f, .2f),
            safe(p.width, 0.f, 1.f, 1.f), safe(p.mix, 0.f, 1.f, .25f) };
    }
    void updateNetwork() noexcept {
        const double scale = .55 + 1.25 * values[1];
        dampingCoefficient = static_cast<float>(1 - std::exp(-6.28318530718 * std::min<double>(values[3], sr * .4) / sr));
        highPassCoefficient = static_cast<float>(1 - std::exp(-6.28318530718 * std::min<double>(values[4], sr * .4) / sr));
        for (size_t line = 0; line < lines.size(); ++line) {
            const double seconds = baseSeconds[line] * scale;
            gains[line] = static_cast<float>(std::pow(.001, seconds / values[0]));
            targetDelays[line] = static_cast<float>(sr * (seconds + .00045 * values[5] * std::sin(phase[line])));
            phase[line] += 6.28318530718 * (.071 + .013 * line) * 32 / sr;
            if (phase[line] >= 6.28318530718) phase[line] -= 6.28318530718;
        }
    }
    inline static constexpr std::array<double, 8> baseSeconds { .0297, .0371, .0411, .0437, .0531, .0593, .0677, .0739 };
    inline static constexpr std::array<float, 8> inputLeft { 1, 1, 1, 1, -1, -1, -1, -1 };
    inline static constexpr std::array<float, 8> inputRight { 1, -1, 1, -1, 1, -1, 1, -1 };
    inline static constexpr std::array<float, 8> outputLeft { 1, 1, -1, -1, 1, 1, -1, -1 };
    inline static constexpr std::array<float, 8> outputRight { 1, -1, -1, 1, 1, -1, -1, 1 };
    std::array<Delay, 8> lines;
    std::array<Delay, 2> preDelay;
    std::array<std::array<Delay, 3>, 2> diffusers;
    std::array<std::array<int, 3>, 2> diffuserLengths {};
    std::array<float, 8> values {}, delays {}, targetDelays {}, gains {}, damped {};
    std::array<double, 8> phase {};
    std::array<float, 2> inputLow {};
    double sr = 48000;
    float smooth = 0, enabled = 0, dampingCoefficient = 0, highPassCoefficient = 0, wetPeak = 0;
    int tick = 0;
};
}
