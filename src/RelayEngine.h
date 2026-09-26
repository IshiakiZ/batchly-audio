// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace batchly {
struct RelayParameters {
    float timeMs = 350, feedback = .35f, toneHz = 6500, motion = .15f;
    float rateHz = .3f, bounce = .65f, glide = .35f, mix = .3f;
    bool enabled = false;
};
class RelayEngine {
public:
    void prepare(double sampleRate, const RelayParameters& p = {}) {
        sr = std::isfinite(sampleRate) ? std::clamp(sampleRate, 8000.0, 384000.0) : 48000;
        capacity = static_cast<int>(std::ceil(sr * 2.1)) + 4;
        for (auto& channel : delay) channel.assign(static_cast<size_t>(capacity), 0);
        smoothing = 1 - std::exp(-1 / (.025 * sr));
        bassCoefficient = 1 - std::exp(-6.28318530718 * 25 / sr);
        values = sanitized(p); enabled = p.enabled ? 1 : 0; tick = 0;
        clearHistory(); coefficients();
    }
    std::array<float, 2> levels() const noexcept { return wetPeak; }
    static double tailSeconds(const RelayParameters& p) noexcept {
        const auto v = sanitized(p);
        const double longestDelay = v[0] * .001 + std::min(.018, v[0] * .00035) * v[3];
        const double repeats = v[1] > 0 ? std::ceil(std::log(.0001) / std::log(v[1])) : 0;
        return (repeats + 1) * longestDelay + .25;
    }
    void process(float* const* audio, int channels, int samples, const RelayParameters& p) noexcept {
        if (!audio || channels < 1 || samples < 1 || capacity == 0) return;
        wetPeak.fill(0);
        if (!p.enabled && enabled == 0) return;
        channels = std::min(channels, 2); const auto targets = sanitized(p);
        for (int i = 0; i < samples; ++i) {
            for (size_t n = 1; n < values.size(); ++n) values[n] += smoothing * (targets[n] - values[n]);
            values[0] += glideCoefficient * (targets[0] - values[0]);
            enabled += smoothing * ((p.enabled ? 1.0 : 0.0) - enabled);
            if (++tick == 16) { tick = 0; coefficients(); }
            std::array<double, 2> dry {}, wet {};
            const double bounce = channels == 2 ? values[5] : 0;
            for (int ch = 0; ch < channels; ++ch) {
                dry[ch] = std::isfinite(audio[ch][i]) ? audio[ch][i] : 0;
                const double movement = std::sin(6.28318530718 * (phase + ch * bounce * .25));
                const double depthMs = std::min(18.0, values[0] * .35) * values[3];
                const double delayed = read(ch, (values[0] + depthMs * movement) * .001 * sr);
                lowState[ch] = flush(lowState[ch] + toneCoefficient * (delayed - lowState[ch]));
                bassState[ch] = flush(bassState[ch] + bassCoefficient * (lowState[ch] - bassState[ch]));
                wet[ch] = lowState[ch] - bassState[ch];
            }
            // Bounce gradually feeds a mono input into the left delay and swaps
            // feedback channels. At full bounce, each repeat alternates sides.
            const double mono = .5 * (dry[0] + dry[1]);
            for (int ch = 0; ch < channels; ++ch) {
                const double input = channels == 1 ? dry[0]
                    : ch == 0 ? (1 - bounce) * dry[0] + bounce * mono : (1 - bounce) * dry[1];
                const double feedback = (1 - bounce) * wet[ch] + bounce * wet[1 - ch];
                // A bounded, slope-at-most-one write curve keeps the feedback
                // loop stable even while delay time bends the pitch.
                delay[ch][writeIndex] = static_cast<float>(2 * std::tanh(.5 * (std::clamp(input, -8.0, 8.0) + values[1] * feedback)));
                audio[ch][i] = static_cast<float>(dry[ch] + values[7] * enabled * (wet[ch] - dry[ch]));
                wetPeak[ch] = std::max(wetPeak[ch], static_cast<float>(std::abs(wet[ch])));
            }
            if (++writeIndex == capacity) writeIndex = 0;
            filled = std::min(filled + 1, capacity - 2);
            phase += values[4] / sr; if (phase >= 1) phase -= 1;
        }
        if (!p.enabled && enabled < 1e-9) { enabled = 0; clearHistory(); }
    }
private:
    static double flush(double x) noexcept { return std::abs(x) < 1e-24 ? 0 : x; }
    static float safe(float x, float lo, float hi, float fallback) noexcept { return std::isfinite(x) ? std::clamp(x, lo, hi) : fallback; }
    static std::array<double, 8> sanitized(const RelayParameters& p) noexcept {
        return { safe(p.timeMs,10.f,2000.f,350.f), safe(p.feedback,0.f,.92f,.35f), safe(p.toneHz,500.f,16000.f,6500.f),
            safe(p.motion,0.f,1.f,.15f), safe(p.rateHz,.05f,5.f,.3f), safe(p.bounce,0.f,1.f,.65f),
            safe(p.glide,0.f,1.f,.35f), safe(p.mix,0.f,1.f,.3f) };
    }
    double read(int channel, double distance) const noexcept {
        distance = std::clamp(distance, 1.0, static_cast<double>(capacity - 2));
        const int recentAge = static_cast<int>(distance); const double fraction = distance - recentAge;
        const auto atAge = [&](int age) -> double {
            return age > filled ? 0 : delay[channel][(writeIndex + capacity - age) % capacity];
        };
        return atAge(recentAge) * (1 - fraction) + atAge(recentAge + 1) * fraction;
    }
    void clearHistory() noexcept {
        // Invalidating history avoids clearing a multi-second buffer on the audio thread.
        writeIndex = filled = 0; phase = 0; lowState.fill(0); bassState.fill(0); wetPeak.fill(0);
    }
    void coefficients() noexcept {
        toneCoefficient = 1 - std::exp(-6.28318530718 * std::min(values[2], sr * .45) / sr);
        glideCoefficient = 1 - std::exp(-1 / ((.005 + values[6] * values[6] * .495) * sr));
    }
    std::array<std::vector<float>, 2> delay;
    std::array<double, 8> values {};
    std::array<double, 2> lowState {}, bassState {};
    std::array<float, 2> wetPeak {};
    double sr = 48000, smoothing = 0, enabled = 0, phase = 0;
    double toneCoefficient = 0, bassCoefficient = 0, glideCoefficient = 0;
    int writeIndex = 0, filled = 0, capacity = 0, tick = 0;
};
}
