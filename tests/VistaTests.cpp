// SPDX-License-Identifier: AGPL-3.0-or-later
#include "VistaEngine.h"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace batchly;
using Audio=std::array<std::vector<float>,2>;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
Audio tone(int sr,double hz=440,bool centered=false){
    Audio x{std::vector<float>(sr),std::vector<float>(sr)};
    for(int i=0;i<sr;++i){x[0][i]=.2f*static_cast<float>(std::sin(i*6.28318530718*hz/sr));x[1][i]=centered?x[0][i]:-x[0][i];}return x;
}
Audio render(Audio x,int sr,const VistaParameters& p,int block=257,int channels=2){
    VistaEngine e;e.prepare(sr,p);
    for(int i=0;i<static_cast<int>(x[0].size());i+=block){float* data[]{x[0].data()+i,x[1].data()+i};e.process(data,channels,std::min(block,static_cast<int>(x[0].size())-i),p);}return x;
}
double energy(const Audio& x){double sum=0;for(size_t i=x[0].size()/2;i<x[0].size();++i)sum+=x[0][i]*x[0][i];return sum;}
int main(){try{
    VistaParameters p;p.enabled=true;
    for(int sr:{8000,44100,48000,96000,192000,384000}){
        const auto x=tone(sr);auto q=p;q.enabled=false;require(render(x,sr,q)==x,"Off path differs");
        q=p;q.mix=0;require(render(x,sr,q)==x,"Dry path differs");
        q=p;q.lowWidth=q.midWidth=q.highWidth=1;require(render(x,sr,q)==x,"Neutral widths add coloration");
        q=p;q.spread=1;q.delayMs=30;
        const auto y=render(x,sr,q);require(y==render(x,sr,q,509),"Block size changes audio");
        for(size_t i=0;i<x[0].size();++i)require(std::abs((y[0][i]+y[1][i])-(x[0][i]+x[1][i]))<1e-7,"Mono fold changed");
        require(render(x,sr,q,257,1)==x,"Mono host was changed");
        auto silence=x;for(auto& c:silence)std::fill(c.begin(),c.end(),0.f);require(render(silence,sr,q)==silence,"Silence became audible");
    }
    p.lowWidth=p.midWidth=p.highWidth=0;const auto x=tone(48000);const auto centered=render(x,48000,p);
    require(energy(centered)<1e-10,"Zero widths fail to center");
    p.lowWidth=p.midWidth=p.highWidth=2;require(std::abs(energy(render(x,48000,p))/energy(x)-4)<1e-6,"All-band width gain is wrong");
    p.lowWidth=0;p.midWidth=p.highWidth=1;
    const auto low=tone(48000,30),high=tone(48000,9000);
    require(energy(render(low,48000,p))<energy(low)*.05,"Low band did not narrow bass");
    require(energy(render(high,48000,p))>energy(high)*.9,"Low width damaged treble");
    p.lowWidth=p.midWidth=1;p.highWidth=0;
    require(energy(render(high,48000,p))<energy(high)*.2,"High width did not narrow treble");
    require(energy(render(low,48000,p))>energy(low)*.99,"High width damaged bass");
    p.lowWidth=p.midWidth=p.highWidth=1;p.spread=1;p.delayMs=17;
    const auto mono=tone(48000,713,true),wide=render(mono,48000,p);
    double side=0;for(size_t i=0;i<mono[0].size();++i){side+=std::abs(wide[0][i]-wide[1][i]);require(std::abs(wide[0][i]+wide[1][i]-2*mono[0][i])<1e-7,"Generated spread damages mono");}
    require(side>100,"Spread failed to create stereo from dual mono");
    VistaEngine e;e.prepare(48000,p);Audio block{std::vector<float>(257),std::vector<float>(257)};float* data[]{block[0].data(),block[1].data()};
    for(int n=0;n<800;++n){
        p.lowWidth=p.highWidth=n%2?0.f:2.f;p.lowHz=n%2?60.f:600.f;p.highHz=n%2?1200.f:12000.f;
        p.delayMs=n%2?1.f:30.f;p.spread=n%2?0.f:1.f;p.enabled=n%13!=0;
        for(int i=0;i<257;++i){block[0][i]=.8f*std::sin((n*257+i)*.43);block[1][i]=.4f*std::sin((n*257+i)*.71);}
        e.process(data,2,257,p);for(const auto& c:block)for(float value:c)require(std::isfinite(value)&&std::abs(value)<8,"Automation unstable");
    }
    p.enabled=true;p.lowHz=std::numeric_limits<float>::quiet_NaN();block[0][0]=std::numeric_limits<float>::infinity();e.process(data,2,257,p);
    for(const auto& c:block)for(float value:c)require(std::isfinite(value),"Invalid input escaped");
    p=VistaParameters{};p.enabled=true;p.spread=1;p.lowHz=60;p.delayMs=30;
    Audio impulse{std::vector<float>(48000),std::vector<float>(48000)};impulse[0][0]=impulse[1][0]=.5f;
    const auto tail=render(impulse,48000,p);for(size_t i=24000;i<48000;++i)require(std::abs(tail[0][i])<1e-7,"Tail exceeded allowance");
    std::cout<<"PASS: dry/off/neutral, three-band width, mono fold preservation, generated spread, mono host, rates, silence, block invariance, automation and tails\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
