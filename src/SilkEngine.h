// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace batchly {
struct SilkParameters {
    float depthDb = 9, selectivity = .45f, lowHz = 700, highHz = 12000;
    float attackMs = 8, releaseMs = 150, trimDb = 0, mix = 1;
    bool enabled = false, listen = false;
};

class SilkEngine {
public:
    static constexpr int bandCount = 32;

    void prepare(double rate, const SilkParameters& settings = {}) noexcept {
        sampleRate = std::isfinite(rate) ? std::clamp(rate, 8000.0, 384000.0) : 48000;
        smoothCoefficient = 1 - std::exp(-1 / (.025 * sampleRate));
        envelopeCoefficient = 1 - std::exp(-1 / (.012 * sampleRate));
        values = sanitize(settings);
        enabled = settings.enabled ? 1 : 0;
        listen = settings.listen ? 1 : 0;
        trimGain = std::pow(10.0, values[6] / 20);
        bands = {};
        reductions = {};
        controlTick = 0;
        const double top = std::min(16000.0, sampleRate * .42);
        const double spacing = std::pow(top / 80, 1.0 / (bandCount - 1));
        const double bandwidth = std::log2(spacing) * 1.3;
        for (int index = 0; index < bandCount; ++index) {
            auto& band = bands[index];
            band.frequency = 80 * std::pow(spacing, index);
            const double angle = 6.283185307179586 * band.frequency / sampleRate;
            // Public RBJ constant-peak bandpass equations, with octave bandwidth
            // correction so the detectors remain evenly spaced near Nyquist.
            const double alpha = std::sin(angle) * std::sinh(std::log(2.0) * .5 * bandwidth * angle / std::sin(angle));
            band.b0 = alpha / (1 + alpha);
            band.a1 = -2 * std::cos(angle) / (1 + alpha);
            band.a2 = (1 - alpha) / (1 + alpha);
        }
    }

    const std::array<float, bandCount>& reductionDb() const noexcept { return reductions; }

    void process(float* const* audio, int channels, int samples, const SilkParameters& settings) noexcept {
        if (!audio || channels < 1 || samples < 1) return;
        if (!settings.enabled && enabled == 0) return;
        channels = std::min(channels, 2);
        const auto targets = sanitize(settings);
        for (int sample = 0; sample < samples; ++sample) {
            for (size_t index = 0; index < values.size(); ++index)
                values[index] += smoothCoefficient * (targets[index] - values[index]);
            enabled += smoothCoefficient * ((settings.enabled ? 1.0 : 0.0) - enabled);
            listen += smoothCoefficient * ((settings.listen ? 1.0 : 0.0) - listen);
            std::array<double, 2> dry {}, wet {};
            for (int channel = 0; channel < channels; ++channel) {
                dry[channel] = std::isfinite(audio[channel][sample]) ? audio[channel][sample] : 0;
                wet[channel] = std::clamp(dry[channel], -8.0, 8.0);
            }
            for (auto& band : bands) {
                double linkedPower = 0;
                for (int channel = 0; channel < channels; ++channel) {
                    const double detected = band.filter(std::clamp(dry[channel], -8.0, 8.0), band.detector[channel]);
                    linkedPower = std::max(linkedPower, detected * detected);
                }
                band.power += envelopeCoefficient * (linkedPower - band.power);
            }
            if (++controlTick == controlInterval) {
                controlTick = 0;
                updateCuts();
            }
            for (auto& band : bands) {
                band.cut += band.cutStep;
                for (int channel = 0; channel < channels; ++channel) {
                    // Subtracting part of a unity-peak band gives a bounded notch.
                    // Cascading notches avoids summing overlapping cuts below zero.
                    const double selected = band.filter(wet[channel], band.processor[channel]);
                    wet[channel] -= band.cut * selected;
                }
            }
            for (int channel = 0; channel < channels; ++channel) {
                const double removed = dry[channel] - wet[channel];
                const double audition = wet[channel] + listen * (removed - wet[channel]);
                audio[channel][sample] = static_cast<float>(dry[channel] + enabled * values[7] * (audition * trimGain - dry[channel]));
            }
        }
        if (!settings.enabled && enabled < 1e-9) {
            enabled = 0;
            for (auto& band : bands) {
                band.detector = {}; band.processor = {};
                band.power = band.cut = band.cutStep = band.smoothedDb = 0;
            }
            reductions = {};
        }
    }

private:
    struct FilterState { double first = 0, second = 0; };
    struct Band {
        double frequency = 0, b0 = 0, a1 = 0, a2 = 0;
        double power = 0, cut = 0, cutStep = 0, smoothedDb = 0;
        std::array<FilterState, 2> detector {}, processor {};
        double filter(double input, FilterState& state) const noexcept {
            const double output = b0 * input + state.first;
            state.first = -a1 * output + state.second;
            state.second = -b0 * input - a2 * output;
            if (std::abs(state.first) < 1e-24) state.first = 0;
            if (std::abs(state.second) < 1e-24) state.second = 0;
            return output;
        }
    };
    static double safe(float value, float low, float high, float fallback) noexcept {
        return std::isfinite(value) ? std::clamp(value, low, high) : fallback;
    }
    static std::array<double, 8> sanitize(const SilkParameters& settings) noexcept {
        return { safe(settings.depthDb, 0, 18, 9), safe(settings.selectivity, 0, 1, .45f),
            safe(settings.lowHz, 80, 2000, 700), safe(settings.highHz, 2500, 18000, 12000),
            safe(settings.attackMs, .5f, 100, 8), safe(settings.releaseMs, 20, 800, 150),
            safe(settings.trimDb, -12, 6, 0), safe(settings.mix, 0, 1, 1) };
    }
    void updateCuts() noexcept {
        const double attack = 1 - std::exp(-controlInterval / (values[4] * .001 * sampleRate));
        const double release = 1 - std::exp(-controlInterval / (values[5] * .001 * sampleRate));
        const double threshold = 1.5 + 6 * values[1];
        trimGain = std::pow(10.0, values[6] / 20);
        for (int index = 0; index < bandCount; ++index) {
            auto& band = bands[index];
            double neighbors = 0;
            int count = 0;
            for (int offset : {-2, -1, 1, 2}) {
                const int neighbor = index + offset;
                if (neighbor >= 0 && neighbor < bandCount) {
                    neighbors += bands[neighbor].power / bands[neighbor].b0;
                    ++count;
                }
            }
            // Divide by noise bandwidth before comparing adjacent detectors. Flat
            // broadband energy should not look like a high-frequency resonance.
            const double power = band.power / band.b0;
            const double prominence = std::clamp(1 - threshold * (neighbors / count + 1e-9) / (power + 1e-9), 0.0, 1.0);
            const double lowerWeight = std::clamp((band.frequency / values[2] - .75) * 4, 0.0, 1.0);
            const double upperWeight = std::clamp((1.25 - band.frequency / values[3]) * 4, 0.0, 1.0);
            const double targetDb = values[0] * prominence * lowerWeight * upperWeight;
            band.smoothedDb += (targetDb > band.smoothedDb ? attack : release) * (targetDb - band.smoothedDb);
            const double nextCut = 1 - std::pow(10.0, -band.smoothedDb / 20);
            band.cutStep = (nextCut - band.cut) / controlInterval;
            reductions[index] = static_cast<float>(band.smoothedDb);
        }
    }
    static constexpr int controlInterval = 32;
    std::array<Band, bandCount> bands {};
    std::array<float, bandCount> reductions {};
    std::array<double, 8> values {};
    double sampleRate = 48000, smoothCoefficient = 0, envelopeCoefficient = 0;
    double enabled = 0, listen = 0, trimGain = 1;
    int controlTick = 0;
};
}
