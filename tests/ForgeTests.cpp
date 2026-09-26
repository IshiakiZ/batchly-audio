// SPDX-License-Identifier: AGPL-3.0-or-later
#include "RackEngine.h"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace batchly;
using Audio = std::array<std::vector<float>, 2>;
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
Audio tone(int sr, double hz=220, float level=.1f) {
    Audio audio {std::vector<float>(sr),std::vector<float>(sr)};
    for(int i=0;i<sr;++i) audio[0][i]=audio[1][i]=level*static_cast<float>(std::sin(i*6.28318530718*hz/sr));
    return audio;
}
Audio render(Audio audio,int sr,const ForgeParameters& p,int block=257,int channels=2) {
    ForgeEngine engine;engine.prepare(sr,p);
    for(int offset=0;offset<static_cast<int>(audio[0].size());offset+=block) {
        float* data[]{audio[0].data()+offset,audio[1].data()+offset};
        engine.process(data,channels,std::min(block,static_cast<int>(audio[0].size())-offset),p);
    }
    return audio;
}
double energy(const Audio& audio,size_t start,size_t end) {
    double result=0;for(size_t i=start;i<end;++i)result+=audio[0][i]*audio[0][i];return result;
}
ForgeParameters neutral() {
    ForgeParameters p;p.enabled=true;p.punch=p.body=p.weightDb=p.edgeDb=p.drive=0;p.ceilingDb=0;return p;
}
int main() {
    try {
        for(int sr:{8000,44100,48000,96000,192000,384000}) {
            const auto input=tone(sr); ForgeParameters p;p.enabled=true;
            auto off=p;off.enabled=false;require(render(input,sr,off)==input,"Disabled Forge changed audio");
            off=p;off.mix=0;require(render(input,sr,off)==input,"Dry path changed audio");
            require(render(input,sr,neutral())==input,"Neutral quiet input changed");
            const auto wet=render(input,sr,p);
            require(wet==render(input,sr,p,509),"Forge depends on block size");
            require(wet[0]==wet[1],"Matched stereo no longer matches");
            const auto mono=render(input,sr,p,257,1);
            require(mono[0]==wet[0]&&mono[1]==input[1],"Mono mismatch or second channel touched");
            auto silence=input;for(auto& ch:silence)std::fill(ch.begin(),ch.end(),0.f);
            require(render(silence,sr,p)==silence,"Silence generated sound");
            for(const auto& ch:wet)for(float sample:ch)require(std::isfinite(sample)&&std::abs(sample)<1,"Ordinary output unstable");
        }
        auto hit=tone(48000,900,.1f);
        for(int i=0;i<48000;++i)for(auto& ch:hit)ch[i]*=static_cast<float>(std::exp(-i/4800.0));
        auto p=neutral();p.punch=1;const auto sharp=render(hit,48000,p);p.punch=-1;const auto soft=render(hit,48000,p);
        require(energy(sharp,0,240)>energy(hit,0,240)*2,"Punch failed to emphasize onset");
        require(energy(soft,0,240)<energy(hit,0,240)*.5,"Negative Punch failed to soften onset");
        p=neutral();p.body=1;const auto body=render(hit,48000,p);
        require(energy(body,4800,9600)>energy(hit,4800,9600)*4,"Body failed to raise sustain");
        p=neutral();p.weightDb=9;const auto bass=tone(48000,50,.03f),top=tone(48000,9000,.03f);
        require(energy(render(bass,48000,p),24000,48000)>energy(bass,24000,48000)*5,"Weight failed to lift bass");
        require(energy(render(top,48000,p),24000,48000)<energy(top,24000,48000)*1.05,"Weight lifted high end excessively");
        p=neutral();p.edgeDb=9;
        require(energy(render(top,48000,p),24000,48000)>energy(top,24000,48000)*5,"Edge failed to lift top end");
        require(energy(render(bass,48000,p),24000,48000)<energy(bass,24000,48000)*1.02,"Edge changed bass excessively");
        p=neutral();p.drive=1;const auto driven=render(tone(48000,1000,.3f),48000,p);
        double real=0,imaginary=0;
        for(int i=24000;i<48000;++i){real+=driven[0][i]*std::cos(i*6.28318530718*3000/48000);imaginary+=driven[0][i]*std::sin(i*6.28318530718*3000/48000);}
        require(std::hypot(real,imaginary)/24000>.01,"Drive did not produce harmonics");
        p=neutral();p.width=0;auto stereo=tone(48000);stereo[1]=tone(48000,443)[0];
        const auto centered=render(stereo,48000,p);require(centered[0]==centered[1],"Width zero failed to center");
        p=neutral();p.punch=1;p.body=1;p.weightDb=p.edgeDb=9;p.drive=1;p.width=1.5f;p.ceilingDb=-6;
        auto loud=stereo;for(auto& ch:loud)for(auto& x:ch)x*=30;
        const auto limited=render(loud,48000,p);
        for(const auto& ch:limited)for(float sample:ch)require(std::isfinite(sample)&&std::abs(sample)<=std::pow(10.f,-6.f/20)+1e-7,"Wet sample ceiling exceeded");
        // Detector linking must also preserve unequal stereo levels when nonlinear
        // stages and width are neutral, not just when both channels are identical.
        p=neutral();p.punch=.7f;p.body=.6f;stereo=hit;for(auto& x:stereo[1])x*=.5f;
        const auto linked=render(stereo,48000,p);
        for(size_t i=0;i<linked[0].size();++i)require(std::abs(linked[1][i]-.5f*linked[0][i])<1e-7,"Stereo detector is not linked");
        ForgeEngine engine;engine.prepare(48000,p);Audio block{std::vector<float>(257),std::vector<float>(257)};float* data[]{block[0].data(),block[1].data()};
        for(int step=0;step<1200;++step) {
            p.punch=step%2?1.f:-1.f;p.body=p.drive=step%2?1.f:0.f;
            p.weightDb=p.edgeDb=step%2?9.f:0.f;p.ceilingDb=step%2?-12.f:0.f;p.width=step%2?1.5f:0.f;p.enabled=step%11!=0;
            for(int i=0;i<257;++i)block[0][i]=block[1][i]=.8f*std::sin((step*257+i)*.47);
            engine.process(data,2,257,p);
            for(const auto& ch:block)for(float sample:ch)require(std::isfinite(sample)&&std::abs(sample)<=1,"Extreme automation unstable");
        }
        p.enabled=true;p.punch=std::numeric_limits<float>::quiet_NaN();p.ceilingDb=std::numeric_limits<float>::infinity();
        block[0][0]=std::numeric_limits<float>::quiet_NaN();block[1][0]=std::numeric_limits<float>::infinity();engine.process(data,2,257,p);
        for(const auto& ch:block)for(float sample:ch)require(std::isfinite(sample),"Invalid input escaped");
        p=neutral();p.weightDb=p.edgeDb=9;Audio impulse{std::vector<float>(48000),std::vector<float>(48000)};impulse[0][0]=impulse[1][0]=.5f;
        const auto tail=render(impulse,48000,p);
        for(size_t i=3840;i<48000;++i)require(std::abs(tail[0][i])<1e-7,"Tone tail exceeded allowance");
        std::cout<<"PASS: sample rates, off/dry/neutral, onset shaping, body, weight, edge, harmonics, linked stereo, width, mono, wet ceiling, tails, block invariance, automation, invalid input\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
