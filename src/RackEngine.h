// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "DriftEngine.h"
#include "PatinaEngine.h"
#include "AtriumEngine.h"
#include "ChimeEngine.h"

namespace batchly {
struct RackParameters {
    DriftParameters drift;
    PatinaParameters patina;
    AtriumParameters atrium;
    ChimeParameters chime;
    bool driftEnabled = true;
};

class RackEngine {
public:
    void prepare(double sampleRate, const RackParameters& settings = {}) {
        const double sr = std::isfinite(sampleRate) ? std::clamp(sampleRate, 8000.0, 384000.0) : 48000;
        smoothing = static_cast<float>(1 - std::exp(-1 / (.025 * sr)));
        outputDb = safeGain(settings.drift.outputDb); bypass = settings.drift.bypass ? 1.f : 0.f;
        auto driftSettings = settings.drift;
        driftSettings.outputDb = 0; driftSettings.bypass = !settings.driftEnabled;
        drift.prepare(sr, driftSettings); patina.prepare(sr, settings.patina);
        atrium.prepare(sr, settings.atrium);
        chime.prepare(sr, settings.chime);
    }
    void process(float* const* audio, int channels, int samples, const RackParameters& settings) noexcept {
        if (!audio || channels < 1 || samples < 1) return;
        channels = std::min(channels, 2);
        auto driftSettings = settings.drift;
        driftSettings.outputDb = 0; driftSettings.bypass = !settings.driftEnabled;
        // Fixed scratch space accepts any host block size without allocating on the audio thread.
        for (int offset = 0; offset < samples; offset += capacity) {
            const int count = std::min(capacity, samples - offset);
            std::array<float*, 2> block {};
            for (int ch = 0; ch < channels; ++ch) {
                block[ch] = audio[ch] + offset;
                for (int i = 0; i < count; ++i) dry[ch][i] = std::isfinite(block[ch][i]) ? block[ch][i] : 0;
            }
            drift.process(block.data(), channels, count, driftSettings);
            patina.process(block.data(), channels, count, settings.patina);
            atrium.process(block.data(), channels, count, settings.atrium);
            chime.process(block.data(), channels, count, settings.chime);
            for (int i = 0; i < count; ++i) {
                outputDb += smoothing * (safeGain(settings.drift.outputDb) - outputDb);
                bypass += smoothing * ((settings.drift.bypass ? 1.f : 0.f) - bypass);
                const float gain = std::pow(10.f, outputDb / 20);
                for (int ch = 0; ch < channels; ++ch) {
                    const float effected = block[ch][i] * gain;
                    block[ch][i] = effected + bypass * (dry[ch][i] - effected);
                }
            }
        }
    }
    std::array<float, 2> driftMotion() const noexcept { return drift.getModulation(); }
    std::array<float, 2> tapeMotion() const noexcept { return patina.getMovement(); }
    float reverbPeak() const noexcept { return atrium.getWetPeak(); }
    std::array<float, 7> chimeLevels() const noexcept { return chime.levels(); }
    static double tailSeconds(const RackParameters& settings) noexcept {
        const double upstream = settings.atrium.enabled ? AtriumEngine::tailSeconds(settings.atrium)
            : settings.patina.enabled ? .16 : .08;
        return upstream + (settings.chime.enabled ? ChimeEngine::tailSeconds(settings.chime) : 0);
    }
private:
    static float safeGain(float x) noexcept { return std::isfinite(x) ? std::clamp(x, -24.f, 12.f) : 0; }
    static constexpr int capacity = 512;
    DriftEngine drift;
    PatinaEngine patina;
    AtriumEngine atrium;
    ChimeEngine chime;
    std::array<std::array<float, capacity>, 2> dry {};
    float outputDb = 0, bypass = 0, smoothing = 0;
};
}
