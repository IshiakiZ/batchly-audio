// SPDX-License-Identifier: AGPL-3.0-or-later
#include "RelayEngine.h"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace batchly;
using Audio = std::array<std::vector<float>, 2>;
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
Audio impulse(int sr, double seconds = 1.0) {
    Audio a {std::vector<float>(static_cast<size_t>(sr * seconds)), std::vector<float>(static_cast<size_t>(sr * seconds))};
    a[0][0] = a[1][0] = .1f; return a;
}
Audio render(Audio audio, int sr, const RelayParameters& p, int block = 257, int channels = 2) {
    RelayEngine engine; engine.prepare(sr,p);
    for (int offset = 0; offset < static_cast<int>(audio[0].size()); offset += block) {
        float* data[] {audio[0].data()+offset,audio[1].data()+offset};
        engine.process(data,channels,std::min(block,static_cast<int>(audio[0].size())-offset),p);
    }
    return audio;
}
double energy(const std::vector<float>& channel, int start, int count) {
    double e=0; for(int i=start;i<std::min(start+count,static_cast<int>(channel.size()));++i)e+=channel[i]*channel[i];return e;
}
int main() {
    try {
        RelayParameters p; p.enabled=true; p.timeMs=100; p.feedback=0; p.motion=0; p.bounce=0; p.mix=1;
        for(int sr : {8000,44100,48000,96000,192000,384000}) {
            const auto input=impulse(sr); auto off=p; off.enabled=false;
            require(render(input,sr,off)==input,"Disabled delay changed audio");
            off=p; off.mix=0; require(render(input,sr,off)==input,"Dry delay changed audio");
            const auto wet=render(input,sr,p);
            require(wet==render(input,sr,p,509),"Delay depends on block size");
            require(wet[0]==wet[1],"Zero bounce changed matched channels");
            const int arrival=sr/10;
            for(int i=0;i<arrival;++i) require(wet[0][i]==0,"Delay arrived early");
            require(std::abs(wet[0][arrival])>.001,"Delay time is inaccurate");
            require(energy(wet[0],sr/2,sr/2)<1e-12,"Zero feedback did not clear");
            const auto mono=render(input,sr,p,257,1);
            require(mono[0]==wet[0]&&mono[1]==input[1],"Mono processing mismatch");
            auto silent=input;silent[0][0]=silent[1][0]=0;
            require(render(silent,sr,p)==silent,"Silence generated sound");
            for(const auto& channel:wet)for(float x:channel)require(std::isfinite(x)&&std::abs(x)<1,"Impulse unstable");
        }
        p.feedback=.65f; p.bounce=1;
        auto wet=render(impulse(48000),48000,p);
        require(energy(wet[0],4800,2400)>.0001&&energy(wet[1],4800,2400)==0,"First bounce is not left");
        require(energy(wet[1],9600,2400)>.00001&&energy(wet[0],9600,2400)<1e-9,"Second bounce is not right");
        p.bounce=0; p.motion=1; p.rateHz=3;
        const auto moving=render(impulse(48000),48000,p); p.motion=0;
        require(moving!=render(impulse(48000),48000,p),"Motion did not alter the delay");
        p.timeMs=20; p.feedback=.92f;
        const double tail=RelayEngine::tailSeconds(p);
        const auto longTail=render(impulse(48000,tail+.1),48000,p);
        require(energy(longTail[0],static_cast<int>(tail*48000),4800)<1e-10,"Reported tail cuts off repeats");
        p.timeMs=100; p.feedback=0; p.toneHz=16000;
        const auto bright=render(impulse(48000),48000,p);p.toneHz=500;
        require(energy(render(impulse(48000),48000,p)[0],4800,4800)<energy(bright[0],4800,4800)*.3,"Tone did not soften echo");
        RelayEngine engine;engine.prepare(48000,p);Audio block{std::vector<float>(257),std::vector<float>(257)};
        float* data[]{block[0].data(),block[1].data()};
        for(int step=0;step<1500;++step) {
            p.timeMs=step%2?10:2000; p.feedback=.92f; p.motion=step%2?0.f:1.f;p.bounce=step%2?1.f:0.f;
            p.rateHz=step%2?.05f:5.f;p.glide=step%2?0.f:1.f;p.enabled=step%17!=0;
            for(int i=0;i<257;++i)block[0][i]=block[1][i]=.8f*std::sin((step*257+i)*.27);
            engine.process(data,2,257,p);
            for(const auto& channel:block)for(float x:channel)require(std::isfinite(x)&&std::abs(x)<4,"Automation destabilized delay");
        }
        p.enabled=true;p.timeMs=std::numeric_limits<float>::quiet_NaN();p.feedback=std::numeric_limits<float>::infinity();
        block[0][0]=std::numeric_limits<float>::quiet_NaN();block[1][0]=std::numeric_limits<float>::infinity();
        engine.process(data,2,257,p);for(const auto& channel:block)for(float x:channel)require(std::isfinite(x),"Invalid audio escaped");
        // Disabling clears logical history without a large real-time buffer erase.
        p.enabled=false; for(auto& channel:block)std::fill(channel.begin(),channel.end(),0.f);
        for(int n=0;n<200;++n){engine.process(data,2,257,p);for(auto& channel:block)std::fill(channel.begin(),channel.end(),0.f);}
        p.enabled=true;p.feedback=.9f;p.timeMs=500;
        for(int n=0;n<300;++n){engine.process(data,2,257,p);for(const auto& channel:block)for(float x:channel)require(x==0,"Old echo returned after re-enable");}
        std::cout<<"PASS: delay timing, off/dry, stereo bounce, mono, tone, motion, tails, silence, rates, block invariance, automation and history reset\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
