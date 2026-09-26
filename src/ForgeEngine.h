// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace batchly {
struct ForgeParameters {
    float punch = .3f, body = .2f, weightDb = 2, edgeDb = 1;
    float drive = .1f, ceilingDb = -1, width = 1, mix = 1;
    bool enabled = false;
};
class ForgeEngine {
public:
    void prepare(double sampleRate, const ForgeParameters& p = {}) noexcept {
        sr = std::isfinite(sampleRate) ? std::clamp(sampleRate, 8000.0, 384000.0) : 48000;
        smoothing = coefficient(.025);
        fastAttack = coefficient(.0003); fastRelease = coefficient(.015);
        slowAttack = coefficient(.015); slowRelease = coefficient(.120);
        bodyAttack = coefficient(.002); bodyRelease = coefficient(.080);
        lowCoefficient = filterCoefficient(120);
        highCoefficient = filterCoefficient(std::min(3200.0, sr * .35));
        values = sanitized(p); enabled = p.enabled ? 1 : 0; tick = 0;
        clear(); coefficients();
    }
    std::array<float, 2> activity() const noexcept { return { transientPeak, clipPeak }; }
    void process(float* const* audio, int channels, int samples, const ForgeParameters& p) noexcept {
        if (!audio || channels < 1 || samples < 1) return;
        transientPeak = clipPeak = 0;
        if (!p.enabled && enabled == 0) return;
        channels = std::min(channels, 2); const auto targets = sanitized(p);
        for (int i = 0; i < samples; ++i) {
            for (size_t n = 0; n < values.size(); ++n) values[n] += smoothing * (targets[n] - values[n]);
            enabled += smoothing * ((p.enabled ? 1.0 : 0.0) - enabled);
            if (++tick == 16) { tick = 0; coefficients(); }
            std::array<double, 2> dry {}, wet {};
            double detector = 0;
            for (int ch = 0; ch < channels; ++ch) {
                dry[ch] = std::isfinite(audio[ch][i]) ? audio[ch][i] : 0;
                wet[ch] = std::clamp(dry[ch], -8.0, 8.0);
                detector = std::max(detector, std::abs(wet[ch]));
            }
            follow(detector, fastEnvelope, fastAttack, fastRelease);
            follow(detector, slowEnvelope, slowAttack, slowRelease);
            follow(detector, bodyEnvelope, bodyAttack, bodyRelease);
            // Two linked envelopes distinguish a new hit from its sustain without
            // lookahead. Both channels receive the same gain to preserve their image.
            const double transient = std::clamp((fastEnvelope - slowEnvelope) / std::max(.005, slowEnvelope), 0.0, 1.0);
            const double attackGain = std::pow(10.0, values[0] * transient * .6);
            const double levelDb = 20 * std::log10(std::max(bodyEnvelope, 1e-9));
            const double compressedGain = std::pow(10.0, (12 - .75 * std::max(0.0, levelDb + 24)) / 20);
            const double bodyGain = 1 + values[1] * (compressedGain - 1);
            for (int ch = 0; ch < channels; ++ch) {
                const double low = lowpass(wet[ch], lowCoefficient, lowState[ch]);
                const double high = wet[ch] - lowpass(wet[ch], highCoefficient, highState[ch]);
                wet[ch] = (wet[ch] + weightGain * low + edgeGain * high) * attackGain * bodyGain;
                const double saturated = std::tanh(wet[ch] * driveGain) / driveScale;
                wet[ch] += values[4] * (saturated - wet[ch]);
            }
            if (channels == 2) {
                const double mid = .5 * (wet[0] + wet[1]);
                const double side = .5 * (wet[0] - wet[1]) * values[6];
                wet[0] = mid + side; wet[1] = mid - side;
            }
            const double blend = enabled * values[7];
            for (int ch = 0; ch < channels; ++ch) {
                const double magnitude = std::abs(wet[ch]);
                // A linear region below 80% of Ceiling keeps quiet hits unchanged.
                // The smooth knee approaches Ceiling and never exceeds it.
                const double knee = ceiling * .8;
                const double clipped = magnitude <= knee ? wet[ch]
                    : std::copysign(knee + ceiling * .2 * std::tanh((magnitude-knee)/(ceiling*.2)), wet[ch]);
                clipPeak = std::max(clipPeak, static_cast<float>(magnitude > 1e-9 ? 1-std::abs(clipped)/magnitude : 0));
                audio[ch][i] = static_cast<float>(dry[ch] + blend * (clipped - dry[ch]));
            }
            transientPeak = std::max(transientPeak, static_cast<float>(transient));
        }
        if (!p.enabled && enabled < 1e-9) { enabled = 0; clear(); }
    }
private:
    static double flush(double x) noexcept { return std::abs(x) < 1e-24 ? 0 : x; }
    static float safe(float x, float lo, float hi, float fallback) noexcept { return std::isfinite(x) ? std::clamp(x, lo, hi) : fallback; }
    static std::array<double, 8> sanitized(const ForgeParameters& p) noexcept {
        return { safe(p.punch,-1.f,1.f,.3f), safe(p.body,0.f,1.f,.2f), safe(p.weightDb,0.f,9.f,2.f),
            safe(p.edgeDb,0.f,9.f,1.f), safe(p.drive,0.f,1.f,.1f), safe(p.ceilingDb,-12.f,0.f,-1.f),
            safe(p.width,0.f,1.5f,1.f), safe(p.mix,0.f,1.f,1.f) };
    }
    double coefficient(double seconds) const noexcept { return 1-std::exp(-1/(seconds*sr)); }
    double filterCoefficient(double frequency) const noexcept {
        const double tangent = std::tan(3.14159265359*frequency/sr); return tangent/(1+tangent);
    }
    static void follow(double input, double& state, double attack, double release) noexcept {
        state = flush(state + (input > state ? attack : release)*(input-state));
    }
    static double lowpass(double input, double coefficient, double& state) noexcept {
        const double change = (input-state)*coefficient, result = change+state;
        state = flush(result+change); return result;
    }
    void coefficients() noexcept {
        weightGain = std::pow(10.0,values[2]/20)-1; edgeGain = std::pow(10.0,values[3]/20)-1;
        driveGain = std::pow(10.0,values[4]*.9); driveScale = std::tanh(driveGain);
        ceiling = std::pow(10.0,values[5]/20);
    }
    void clear() noexcept { lowState.fill(0); highState.fill(0); fastEnvelope=slowEnvelope=bodyEnvelope=0; transientPeak=clipPeak=0; }
    std::array<double, 8> values {};
    std::array<double, 2> lowState {}, highState {};
    double sr=48000, smoothing=0, enabled=0, fastAttack=0, fastRelease=0, slowAttack=0, slowRelease=0, bodyAttack=0, bodyRelease=0;
    double fastEnvelope=0, slowEnvelope=0, bodyEnvelope=0, lowCoefficient=0, highCoefficient=0;
    double weightGain=0, edgeGain=0, driveGain=1, driveScale=1, ceiling=1;
    float transientPeak=0, clipPeak=0;
    int tick=0;
};
}
