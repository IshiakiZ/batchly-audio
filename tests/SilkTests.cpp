// SPDX-License-Identifier: AGPL-3.0-or-later
#include "SilkEngine.h"
#include <vector>
#include <random>
#include <iostream>
#include <stdexcept>
#include <limits>
using namespace batchly;
using Audio = std::array<std::vector<float>, 2>;
void require(bool success, const char* message) { if (!success) throw std::runtime_error(message); }
Audio tone(int rate, double frequency, float level = .2f, int seconds = 1) {
    Audio audio { std::vector<float>(rate * seconds), std::vector<float>(rate * seconds) };
    for (size_t i = 0; i < audio[0].size(); ++i)
        audio[0][i] = audio[1][i] = level * static_cast<float>(std::sin(6.283185307179586 * frequency * i / rate));
    return audio;
}
Audio render(Audio audio, int rate, const SilkParameters& settings, int block = 257, int channels = 2) {
    SilkEngine engine; engine.prepare(rate, settings);
    for (int offset = 0; offset < static_cast<int>(audio[0].size()); offset += block) {
        float* data[] { audio[0].data() + offset, audio[1].data() + offset };
        engine.process(data, channels, std::min(block, static_cast<int>(audio[0].size()) - offset), settings);
    }
    return audio;
}
double energy(const Audio& audio) {
    double sum = 0;
    for (size_t i = audio[0].size()/2; i < audio[0].size(); ++i) sum += audio[0][i] * audio[0][i];
    return sum;
}
int main() { try {
    SilkParameters settings; settings.enabled = true; settings.depthDb = 18; settings.selectivity = .15f;
    for (int rate : {8000, 44100, 48000, 96000, 192000, 384000}) {
        const auto input = tone(rate, 1800);
        auto dry = settings; dry.mix = 0;
        require(render(input, rate, dry) == input, "Dry path differs");
        dry = settings; dry.enabled = false;
        require(render(input, rate, dry) == input, "Off path differs");
        dry = settings; dry.depthDb = 0;
        require(render(input, rate, dry) == input, "Zero depth colors the signal");
        const auto output = render(input, rate, settings);
        std::cout << rate << " Hz tone energy ratio " << energy(output)/energy(input) << '\n';
        require(energy(output) < energy(input) * .5, "Isolated resonance not reduced");
        require(output == render(input, rate, settings, 509), "Host block size changes sound");
        require(output[0] == output[1], "Matched stereo image changed");
        require(output[0] == render(input, rate, settings, 257, 1)[0], "Mono differs from matched stereo");
        auto asymmetrical = input; for (auto& sample : asymmetrical[1]) sample *= .25f;
        const auto linked = render(asymmetrical, rate, settings);
        for (size_t i = 0; i < output[0].size(); ++i)
            require(std::abs(linked[1][i] - linked[0][i] * .25f) < 1e-7, "Stereo link changes balance");
        auto silence = input; for (auto& channel : silence) std::fill(channel.begin(), channel.end(), 0.f);
        require(render(silence, rate, settings) == silence, "Silence generates audio");
        for (auto& channel : output) for (float sample : channel) require(std::isfinite(sample), "Invalid sample escaped");
    }
    const auto bass = tone(48000, 100);
    require(energy(render(bass, 48000, settings)) > energy(bass) * .99, "Protected bass changed");
    for (double frequency : {800.0, 1000.0, 1300.0, 2400.0, 3700.0, 6000.0, 9500.0}) {
        const auto probe = tone(48000, frequency);
        const double ratio = energy(render(probe, 48000, settings)) / energy(probe);
        std::cout << frequency << " Hz frequency sweep ratio " << ratio << '\n';
        require(ratio < .7, "Frequency coverage has a hole");
    }
    auto noise = tone(48000, 0, 0, 3);
    std::mt19937 random(1301); std::normal_distribution<float> normal(0, .08f);
    for (size_t i = 0; i < noise[0].size(); ++i) noise[0][i] = noise[1][i] = normal(random);
    require(energy(render(noise, 48000, settings)) > energy(noise) * .95, "Broadband audio is overcorrected");
    auto shortTone = tone(48000, 1800);
    shortTone[0].resize(2400); shortTone[1].resize(2400);
    auto fast = settings; fast.attackMs = .5f;
    auto slow = settings; slow.attackMs = 100;
    require(energy(render(shortTone,48000,fast)) < energy(render(shortTone,48000,slow))*.85, "Attack does not change cut response");
    auto tailInput = tone(48000,1800,.2f,2);
    for (auto& channel : tailInput) std::fill(channel.begin()+48000,channel.end(),0.f);
    const auto tail = render(tailInput,48000,settings);
    for (size_t i=91200;i<tail[0].size();++i) require(std::abs(tail[0][i])<1e-7,"Filter tail persists beyond one second");
    const auto input = tone(48000, 1800);
    const auto normalOutput = render(input, 48000, settings);
    auto audition = settings; audition.listen = true;
    const auto removed = render(input, 48000, audition);
    for (size_t i = 0; i < input[0].size(); ++i)
        require(std::abs(normalOutput[0][i] + removed[0][i] - input[0][i]) < 1e-7, "Removed-signal audition does not reconstruct input");
    SilkEngine engine; engine.prepare(48000, settings);
    Audio block { std::vector<float>(257), std::vector<float>(257) };
    float* data[] { block[0].data(), block[1].data() };
    for (int step = 0; step < 400; ++step) {
        settings.depthDb = step%2 ? 18.f : 0.f; settings.lowHz = step%2 ? 80.f : 2000.f;
        settings.highHz = step%2 ? 2500.f : 18000.f; settings.attackMs = step%2 ? .5f : 100.f;
        settings.releaseMs = step%2 ? 20.f : 800.f; settings.listen = step%3 == 0; settings.enabled = step%7 != 0;
        for (int i=0; i<257; ++i) block[0][i] = block[1][i] = .5f * std::sin((step*257+i)*.37);
        engine.process(data, 2, 257, settings);
        for (auto& channel : block) for (float sample : channel)
            require(std::isfinite(sample) && std::abs(sample) < 4, "Automated cuts are unstable");
    }
    settings.enabled = true; settings.depthDb = std::numeric_limits<float>::quiet_NaN();
    block[0][0] = std::numeric_limits<float>::infinity(); engine.process(data, 2, 257, settings);
    for (auto& channel : block) for (float sample : channel) require(std::isfinite(sample), "Invalid input poisoned filters");
    std::cout << "PASS: selective cuts, protected bass, broadband preservation, stereo link, dry/off, mono, silence, removed signal, rates and automation\n";
    return 0;
} catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; } }
