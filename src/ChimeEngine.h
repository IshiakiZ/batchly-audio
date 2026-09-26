// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace batchly {
struct ChimeParameters {
    float ringSeconds = 1.6f, colorHz = 8000, drive = .12f, spread = .5f;
    float detune = .25f, motion = .15f, width = .85f, mix = .4f;
    int root = 0, scale = 1, octave = 3;
    bool enabled = false;
};

class ChimeEngine {
public:
    inline static constexpr std::array<const char*, 12> noteNames { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    inline static constexpr std::array<const char*, 6> scaleNames { "Major", "Minor", "Dorian", "Major pent.", "Minor pent.", "Whole tone" };
    static int noteCount(int scale) noexcept { return scale == 3 || scale == 4 ? 5 : scale == 5 ? 6 : 7; }
    static int semitone(int scale, int degree) noexcept {
        static constexpr int intervals[6][7] { {0,2,4,5,7,9,11}, {0,2,3,5,7,8,10}, {0,2,3,5,7,9,10},
            {0,2,4,7,9,0,0}, {0,3,5,7,10,0,0}, {0,2,4,6,8,10,0} };
        return intervals[std::clamp(scale, 0, 5)][std::clamp(degree, 0, 6)];
    }
    static double tailSeconds(const ChimeParameters& p) noexcept { return safe(p.ringSeconds, .08f, 6.f, 1.6f) * 2 + .25; }
    void prepare(double sampleRate, const ChimeParameters& p = {}) noexcept {
        sr = std::isfinite(sampleRate) ? std::clamp(sampleRate, 8000.0, 384000.0) : 48000;
        smoothing = 1 - std::exp(-1 / (.035 * sr)); values = sanitized(p);
        enabled = p.enabled ? 1 : 0; phase = 0; tick = 0; tone.fill(0); meters.fill(0);
        for (auto& voice : voices) voice = {};
        tune(p, true); coefficients();
    }
    std::array<float, 7> levels() const noexcept { return meters; }
    void process(float* const* audio, int channels, int samples, const ChimeParameters& p) noexcept {
        if (!audio || channels < 1 || samples < 1) return;
        if (!p.enabled && enabled == 0) { meters.fill(0); return; }
        channels = std::min(channels, 2);
        const auto targets = sanitized(p); tune(p, false); meters.fill(0);
        for (int i = 0; i < samples; ++i) {
            for (size_t control = 0; control < values.size(); ++control) values[control] += smoothing * (targets[control] - values[control]);
            enabled += smoothing * ((p.enabled ? 1.0 : 0.0) - enabled);
            if (++tick == 16) { tick = 0; coefficients(); }
            std::array<double, 2> dry { finite(audio[0][i]), finite(audio[channels - 1][i]) };
            const double driveGain = 1 + values[2] * 9;
            std::array<double, 2> input {};
            for (int ch = 0; ch < 2; ++ch)
                input[ch] = std::tanh(std::clamp(dry[ch], -8.0, 8.0) * driveGain) / std::sqrt(driveGain) * enabled;
            std::array<double, 2> wet {};
            for (size_t n = 0; n < voices.size(); ++n) {
                auto& voice = voices[n];
                voice.weight += smoothing * (voice.targetWeight - voice.weight);
                // A damped complex rotation is a resonator whose state magnitude
                // contracts even when its frequency changes during automation.
                for (int ch = 0; ch < 2; ++ch) {
                    const double re = voice.real[ch], im = voice.imag[ch];
                    voice.real[ch] = flush(voice.cosine[ch] * re - voice.sine[ch] * im + excitation * input[ch] * voice.weight);
                    voice.imag[ch] = flush(voice.sine[ch] * re + voice.cosine[ch] * im);
                    const double registerWeight = n < 7 ? 1.0 - .45 * values[3] : values[3];
                    const double pan = channels == 1 ? 1.0 : 1.0 + (ch == 0 ? -1.0 : 1.0) * panPosition[n % 7] * values[6] * .65;
                    const double signal = voice.imag[ch] * registerWeight * pan;
                    wet[ch] += signal;
                    meters[n % 7] = std::max(meters[n % 7], static_cast<float>(std::abs(signal) * enabled));
                }
            }
            const double normalise = 1.6 / std::sqrt(static_cast<double>(activeCount) * (1 + values[3] * values[3]));
            for (int ch = 0; ch < 2; ++ch) {
                const double limited = 1.25 * std::tanh(wet[ch] * normalise / 1.25);
                tone[ch] = flush(tone[ch] + colorCoefficient * (limited - tone[ch]));
            }
            const double mid = .5 * (tone[0] + tone[1]);
            const double side = .5 * (tone[0] - tone[1]) * values[6];
            const double blend = values[7] * enabled;
            audio[0][i] = static_cast<float>(dry[0] + blend * (mid + side - dry[0]));
            if (channels == 2) audio[1][i] = static_cast<float>(dry[1] + blend * (mid - side - dry[1]));
        }
        if (!p.enabled && enabled < 1e-9) {
            enabled = 0; tone.fill(0);
            for (auto& voice : voices) { voice.real.fill(0); voice.imag.fill(0); }
        }
    }
private:
    struct Voice {
        std::array<double, 2> real {}, imag {}, cosine {}, sine {};
        double hz = 0, targetHz = 0, weight = 0, targetWeight = 0;
    };
    static double finite(float x) noexcept { return std::isfinite(x) ? x : 0; }
    static double flush(double x) noexcept { return std::abs(x) < 1e-24 ? 0 : x; }
    static float safe(float x, float lo, float hi, float fallback) noexcept { return std::isfinite(x) ? std::clamp(x, lo, hi) : fallback; }
    static std::array<double, 8> sanitized(const ChimeParameters& p) noexcept {
        return { safe(p.ringSeconds,.08f,6.f,1.6f),safe(p.colorHz,500.f,16000.f,8000.f),safe(p.drive,0.f,1.f,.12f),
            safe(p.spread,0.f,1.f,.5f),safe(p.detune,0.f,1.f,.25f),safe(p.motion,0.f,1.f,.15f),
            safe(p.width,0.f,1.f,.85f),safe(p.mix,0.f,1.f,.4f) };
    }
    void tune(const ChimeParameters& p, bool initial) noexcept {
        const int scale = std::clamp(p.scale, 0, 5); activeCount = noteCount(scale);
        for (size_t n = 0; n < voices.size(); ++n) {
            auto& v = voices[n];
            const int degree = static_cast<int>(n % 7), registerOffset = n < 7 ? 0 : 12;
            const int midi = 12 * (std::clamp(p.octave, 2, 4) + 1) + std::clamp(p.root, 0, 11) + semitone(scale, degree) + registerOffset;
            v.targetHz = 440.0 * std::pow(2.0, (midi - 69) / 12.0);
            v.targetWeight = degree < activeCount ? 1 : 0;
            if (initial) { v.hz = v.targetHz; v.weight = v.targetWeight; }
        }
    }
    void coefficients() noexcept {
        const double radius = std::exp(-6.90775527898 / (values[0] * sr));
        // Energy scaling avoids making a longer ring disproportionately loud.
        excitation = .12 * std::sqrt(1 - radius * radius);
        colorCoefficient = 1 - std::exp(-6.28318530718 * std::min(values[1], sr * .4) / sr);
        const double tuningSmoothing = 1 - std::pow(1 - smoothing, 16);
        for (size_t n = 0; n < voices.size(); ++n) {
            auto& v = voices[n]; v.hz += tuningSmoothing * (v.targetHz - v.hz);
            for (int ch = 0; ch < 2; ++ch) {
                const double cents = (ch == 0 ? -1 : 1) * values[4] * values[6] * (2 + n % 7)
                    + 4 * values[5] * std::sin(phase * (1 + n * .071) + n * .7);
                const double hz = std::min(sr * .42, v.hz * std::pow(2.0, cents / 1200));
                const double angle = 6.28318530718 * hz / sr;
                v.cosine[ch] = radius * std::cos(angle); v.sine[ch] = radius * std::sin(angle);
            }
        }
        phase += 6.28318530718 * .17 * 16 / sr;
        if (phase > 100000) phase = 0;
    }
    inline static constexpr std::array<double, 7> panPosition { -.8, .65, -.35, .9, 0, -.65, .35 };
    std::array<Voice, 14> voices {};
    std::array<double, 8> values {};
    std::array<double, 2> tone {};
    std::array<float, 7> meters {};
    double sr = 48000, smoothing = 0, enabled = 0, phase = 0, excitation = 0, colorCoefficient = 0;
    int tick = 0, activeCount = 7;
};
}
