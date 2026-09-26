// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace batchly {
struct GleamParameters {
    float presenceDb = 2.5f, airDb = 4, focusHz = 7500, excite = .15f;
    float tame = .5f, width = 1, trimDb = -3, mix = 1;
    bool enabled = false;
};
class GleamEngine {
public:
    void prepare(double sampleRate, const GleamParameters& p = {}) noexcept {
        sr = std::isfinite(sampleRate) ? std::clamp(sampleRate, 8000.0, 384000.0) : 48000;
        smoothing = 1 - std::exp(-1 / (.025 * sr));
        attack = 1 - std::exp(-1 / (.0015 * sr)); release = 1 - std::exp(-1 / (.07 * sr));
        presenceCoefficient = filterCoefficient(std::min(2400.0, sr * .3));
        values = sanitized(p); enabled = p.enabled ? 1 : 0; tick = 0;
        clear(); coefficients();
    }
    float highLevel() const noexcept { return highPeak; }
    float reduction() const noexcept { return reductionPeak; }
    void process(float* const* audio, int channels, int samples, const GleamParameters& p) noexcept {
        if (!audio || channels < 1 || samples < 1) return;
        highPeak = reductionPeak = 0;
        if (!p.enabled && enabled == 0) return;
        channels = std::min(channels, 2); const auto targets = sanitized(p);
        for (int i = 0; i < samples; ++i) {
            for (size_t n = 0; n < values.size(); ++n) values[n] += smoothing * (targets[n] - values[n]);
            enabled += smoothing * ((p.enabled ? 1.0 : 0.0) - enabled);
            if (++tick == 16) { tick = 0; coefficients(); }
            std::array<double, 2> dry {}, presence {}, high {}, addition {};
            double detector = 0;
            for (int ch = 0; ch < channels; ++ch) {
                dry[ch] = std::isfinite(audio[ch][i]) ? audio[ch][i] : 0;
                const double input = std::clamp(dry[ch], -8.0, 8.0);
                presence[ch] = highpass(input, presenceCoefficient, presenceState[ch]);
                high[ch] = highpass(input, airCoefficient, airState[ch]);
                detector = std::max(detector, std::abs(high[ch]));
                // The difference of two soft curves adds original harmonic color.
                // A second high-pass keeps this added layer out of the bass.
                const double harmonic = std::tanh(high[ch] * 4) - std::tanh(high[ch]);
                addition[ch] = presenceGain * presence[ch] + airGain * high[ch]
                    + values[3] * highpass(harmonic, airCoefficient, harmonicState[ch]);
            }
            envelope = flush(envelope + (detector > envelope ? attack : release) * (detector - envelope));
            // Linked detection preserves stereo balance and tames only the added brightness.
            const double control = 1 / (1 + values[4] * std::max(0.0, envelope - .035) * 24);
            const double mid = .5 * (addition[0] + addition[1]);
            const double side = .5 * (addition[0] - addition[1]) * values[5];
            if (channels == 2) { addition[0] = mid + side; addition[1] = mid - side; }
            const double blend = values[7] * enabled;
            for (int ch = 0; ch < channels; ++ch) {
                const double wet = (dry[ch] + addition[ch] * control) * trimGain;
                audio[ch][i] = static_cast<float>(dry[ch] + blend * (wet - dry[ch]));
            }
            highPeak = std::max(highPeak, static_cast<float>(detector));
            reductionPeak = std::max(reductionPeak, static_cast<float>(1 - control));
        }
        if (!p.enabled && enabled < 1e-9) { enabled = 0; clear(); }
    }
private:
    static double flush(double x) noexcept { return std::abs(x) < 1e-24 ? 0 : x; }
    static float safe(float x, float lo, float hi, float fallback) noexcept { return std::isfinite(x) ? std::clamp(x, lo, hi) : fallback; }
    static std::array<double, 8> sanitized(const GleamParameters& p) noexcept {
        return { safe(p.presenceDb,0.f,9.f,2.5f), safe(p.airDb,0.f,12.f,4.f), safe(p.focusHz,4000.f,14000.f,7500.f),
            safe(p.excite,0.f,1.f,.15f), safe(p.tame,0.f,1.f,.5f), safe(p.width,0.f,1.5f,1.f),
            safe(p.trimDb,-12.f,0.f,-3.f), safe(p.mix,0.f,1.f,1.f) };
    }
    // A trapezoidal integrator gives a complementary high-pass with unity gain
    // at Nyquist, so the lift amounts retain their meaning at high frequencies.
    static double highpass(double input, double coefficient, double& state) noexcept {
        const double change = (input - state) * coefficient;
        const double low = change + state;
        state = flush(low + change);
        return input - low;
    }
    double filterCoefficient(double frequency) const noexcept {
        const double tangent = std::tan(3.14159265359 * frequency / sr);
        return tangent / (1 + tangent);
    }
    void clear() noexcept { presenceState.fill(0); airState.fill(0); harmonicState.fill(0); envelope = 0; highPeak = reductionPeak = 0; }
    void coefficients() noexcept {
        airCoefficient = filterCoefficient(std::min(values[2], sr * .4));
        presenceGain = std::pow(10.0, values[0] / 20) - 1;
        airGain = std::pow(10.0, values[1] / 20) - 1;
        trimGain = std::pow(10.0, values[6] / 20);
    }
    std::array<double, 8> values {};
    std::array<double, 2> presenceState {}, airState {}, harmonicState {};
    double sr = 48000, smoothing = 0, enabled = 0, attack = 0, release = 0, envelope = 0;
    double presenceCoefficient = 0, airCoefficient = 0, presenceGain = 0, airGain = 0, trimGain = 1;
    float highPeak = 0, reductionPeak = 0;
    int tick = 0;
};
}
