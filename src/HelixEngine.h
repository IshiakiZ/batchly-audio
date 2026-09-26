// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace batchly {
struct HelixParameters {
    float rateHz = .24f, depth = .65f, feedback = .35f, centerHz = 650;
    float toneHz = 11000, drive = .1f, width = .75f, mix = .5f;
    bool enabled = false;
};
class HelixEngine {
public:
    void prepare(double sampleRate, const HelixParameters& p = {}) noexcept {
        sr = std::isfinite(sampleRate) ? std::clamp(sampleRate, 8000.0, 384000.0) : 48000;
        smooth = 1 - std::exp(-1 / (.035 * sr)); values = sanitized(p);
        enabled = p.enabled ? 1 : 0; phase = 0; tick = 0; clear(); updateCoefficients();
    }
    std::array<float, 2> sweep() const noexcept { return sweepPosition; }
    void process(float* const* audio, int channels, int samples, const HelixParameters& p) noexcept {
        if (!audio || channels < 1 || samples < 1) return;
        if (!p.enabled && enabled == 0) return;
        channels = std::min(channels, 2); const auto targets = sanitized(p);
        for (int i = 0; i < samples; ++i) {
            for (size_t c = 0; c < values.size(); ++c) values[c] += smooth * (targets[c] - values[c]);
            enabled += smooth * ((p.enabled ? 1.0 : 0.0) - enabled);
            phase += values[0] / sr; if (phase >= 1) phase -= 1;
            if (++tick == 16) { tick = 0; updateCoefficients(); }
            for (int ch = 0; ch < channels; ++ch) {
                const double dry = std::isfinite(audio[ch][i]) ? audio[ch][i] : 0;
                const double input = std::clamp(dry, -8.0, 8.0);
                const double gain = 1 + 5 * values[5];
                const double driven = input + values[5] * (std::tanh(input * gain) / std::sqrt(gain) - input);
                double sample = driven * enabled + values[2] * std::tanh(feedback[ch]);
                // All-pass stages rotate phase without imposing a static EQ curve.
                // Their moving phase cancellations become audible when dry audio is blended back in.
                for (int stage = 0; stage < 8; ++stage) {
                    const double result = coefficients[ch][stage] * sample + delays[ch][stage];
                    delays[ch][stage] = flush(sample - coefficients[ch][stage] * result);
                    sample = result;
                }
                feedback[ch] = flush(sample);
                tone[ch] = flush(tone[ch] + toneCoefficient * (sample - tone[ch]));
                const double wet = tone[ch] * (1 - .35 * std::abs(values[2]));
                const double blend = enabled * values[7];
                audio[ch][i] = static_cast<float>(dry + blend * (wet - dry));
            }
        }
        if (!p.enabled && enabled < 1e-9) { enabled = 0; clear(); }
    }
private:
    static double flush(double x) noexcept { return std::abs(x) < 1e-24 ? 0 : x; }
    static float safe(float x, float lo, float hi, float fallback) noexcept { return std::isfinite(x) ? std::clamp(x, lo, hi) : fallback; }
    static std::array<double, 8> sanitized(const HelixParameters& p) noexcept {
        return { safe(p.rateHz,.03f,8.f,.24f), safe(p.depth,0.f,1.f,.65f), safe(p.feedback,-.85f,.85f,.35f),
            safe(p.centerHz,100.f,6000.f,650.f), safe(p.toneHz,500.f,18000.f,11000.f), safe(p.drive,0.f,1.f,.1f),
            safe(p.width,0.f,1.f,.75f), safe(p.mix,0.f,1.f,.5f) };
    }
    void clear() noexcept { for (auto& channel : delays) channel.fill(0); feedback.fill(0); tone.fill(0); }
    void updateCoefficients() noexcept {
        static constexpr double spacing[8] { .58, .73, .9, 1.1, 1.33, 1.61, 1.96, 2.38 };
        toneCoefficient = 1 - std::exp(-6.28318530718 * std::min(values[4], sr * .4) / sr);
        for (int ch = 0; ch < 2; ++ch) {
            const double sweep = std::sin(6.28318530718 * (phase + ch * values[6] * .5));
            sweepPosition[ch] = static_cast<float>(sweep * values[1]);
            const double center = values[3] * std::pow(2.0, 2.7 * values[1] * sweep);
            for (int stage = 0; stage < 8; ++stage) {
                const double frequency = std::clamp(center * spacing[stage], 30.0, sr * .42);
                const double tangent = std::tan(3.14159265359 * frequency / sr);
                coefficients[ch][stage] = (tangent - 1) / (tangent + 1);
            }
        }
    }
    std::array<double, 8> values {};
    std::array<std::array<double, 8>, 2> delays {}, coefficients {};
    std::array<double, 2> feedback {}, tone {};
    std::array<float, 2> sweepPosition {};
    double sr = 48000, smooth = 0, phase = 0, enabled = 0, toneCoefficient = 0;
    int tick = 0;
};
}
