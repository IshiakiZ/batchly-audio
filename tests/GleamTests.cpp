// SPDX-License-Identifier: AGPL-3.0-or-later
#include "RackEngine.h"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace batchly;
using Audio = std::array<std::vector<float>, 2>;
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
Audio tone(int sr, double hz = 2500, float level = .1f) {
    Audio audio {std::vector<float>(sr), std::vector<float>(sr)};
    for (int i = 0; i < sr; ++i) audio[0][i] = audio[1][i] = level * static_cast<float>(std::sin(i * 6.28318530718 * hz / sr));
    return audio;
}
Audio render(Audio audio, int sr, const GleamParameters& p, int block = 257, int channels = 2) {
    GleamEngine engine; engine.prepare(sr, p);
    for (int offset = 0; offset < static_cast<int>(audio[0].size()); offset += block) {
        float* data[] {audio[0].data() + offset, audio[1].data() + offset};
        engine.process(data, channels, std::min(block, static_cast<int>(audio[0].size()) - offset), p);
    }
    return audio;
}
double energy(const Audio& audio) {
    double result = 0; for (size_t i = audio[0].size()/2; i < audio[0].size(); ++i) result += audio[0][i] * audio[0][i]; return result;
}
double harmonic(const Audio& audio, double hz, int sr) {
    double re = 0, im = 0;
    for (size_t i = audio[0].size()/2; i < audio[0].size(); ++i) {
        const double phase = 6.28318530718 * hz * i / sr;
        re += audio[0][i] * std::cos(phase); im += audio[0][i] * std::sin(phase);
    }
    return std::sqrt(re*re + im*im) / (audio[0].size()/2);
}
int main() {
    try {
        GleamParameters p; p.enabled = true;
        for (int sr : {8000,44100,48000,96000,192000,384000}) {
            const auto input = tone(sr); auto off = p; off.enabled = false;
            require(render(input,sr,off) == input, "Disabled enhancer changed audio");
            off = p; off.mix = 0; require(render(input,sr,off) == input, "Dry path changed audio");
            auto neutral = p; neutral.presenceDb = neutral.airDb = neutral.excite = neutral.trimDb = 0;
            require(render(input,sr,neutral) == input, "Neutral settings changed audio");
            const auto wet = render(input,sr,p);
            require(wet == render(input,sr,p,509), "Enhancer depends on block size");
            require(wet[0] == wet[1], "Matched channels no longer match");
            const auto mono = render(input,sr,p,257,1); require(mono[0] == wet[0] && mono[1] == input[1], "Mono mismatch");
            auto silence=input; for (auto& ch:silence) std::fill(ch.begin(),ch.end(),0.f);
            require(render(silence,sr,p)==silence,"Silence generated sound");
            for (const auto& ch:wet) for(float sample:ch) require(std::isfinite(sample)&&std::abs(sample)<1,"Normal output unstable");
        }
        p.trimDb=p.excite=p.tame=0; p.presenceDb=9; p.airDb=12;
        const auto low=tone(48000,80), high=tone(48000,12000);
        require(energy(render(low,48000,p))/energy(low)<1.03,"Brightness changed bass excessively");
        require(energy(render(high,48000,p))/energy(high)>4,"High end not boosted");
        p.presenceDb=0; p.focusHz=4000; const double lowerFocus=energy(render(tone(48000,7000),48000,p));
        p.focusHz=14000; require(lowerFocus>energy(render(tone(48000,7000),48000,p))*1.5,"Focus did not move the lift");
        p.focusHz=6000; const auto loud=tone(48000,10000,.7f); const double untamed=energy(render(loud,48000,p));
        p.tame=1; require(energy(render(loud,48000,p))<untamed*.7,"Tame did not reduce added brightness");
        p.airDb=p.tame=0; p.excite=1; const auto clean=tone(48000,3000,.65f);
        require(harmonic(render(clean,48000,p),9000,48000)>1e-5,"Excite did not generate harmonics");
        p.presenceDb=6; p.airDb=4; p.width=0; auto stereo=tone(48000,440); stereo[1]=tone(48000,880)[0];
        const auto centered=render(stereo,48000,p);
        for(size_t i=0;i<stereo[0].size();++i)
            require(std::abs((centered[0][i]-stereo[0][i])-(centered[1][i]-stereo[1][i]))<1e-7,"Width zero changed dry stereo or failed to center enhancement");
        GleamEngine engine; engine.prepare(48000,p); Audio block {std::vector<float>(257),std::vector<float>(257)};
        float* data[]{block[0].data(),block[1].data()};
        for(int step=0;step<1000;++step) {
            p.presenceDb=step%2?0.f:9.f; p.airDb=step%2?12.f:0.f; p.focusHz=step%2?4000:14000;
            p.excite=p.tame=step%2?1.f:0.f; p.width=step%2?0.f:1.5f; p.trimDb=step%2?-12.f:0.f; p.enabled=step%11!=0;
            for(int i=0;i<257;++i)block[0][i]=block[1][i]=.8f*std::sin((step*257+i)*.47);
            engine.process(data,2,257,p);
            for(const auto& ch:block)for(float sample:ch)require(std::isfinite(sample)&&std::abs(sample)<8,"Extreme automation unstable");
        }
        p.enabled=true; p.focusHz=std::numeric_limits<float>::infinity(); p.airDb=std::numeric_limits<float>::quiet_NaN();
        block[0][0]=std::numeric_limits<float>::quiet_NaN(); block[1][0]=std::numeric_limits<float>::infinity(); engine.process(data,2,257,p);
        for(const auto& ch:block)for(float sample:ch)require(std::isfinite(sample),"Invalid input escaped");
        std::cout<<"PASS: neutral/off/dry, silence, bass retention, high lift, focus, tame, harmonics, stereo width, mono, sample rates, block invariance, automation, invalid input\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
