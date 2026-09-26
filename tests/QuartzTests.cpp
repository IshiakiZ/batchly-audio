// SPDX-License-Identifier: AGPL-3.0-or-later
#include "QuartzEngine.h"
#include <vector>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace batchly;
using Audio=std::array<std::vector<float>,2>;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
Audio tone(int sr,double hz=440,float level=.1f){Audio x{std::vector<float>(sr),std::vector<float>(sr)};for(int i=0;i<sr;++i)x[0][i]=x[1][i]=level*static_cast<float>(std::sin(i*6.28318530718*hz/sr));return x;}
Audio render(Audio x,int sr,const QuartzParameters& p,int block=257,int channels=2){QuartzEngine e;e.prepare(sr,p);for(int i=0;i<static_cast<int>(x[0].size());i+=block){float* data[]{x[0].data()+i,x[1].data()+i};e.process(data,channels,std::min(block,static_cast<int>(x[0].size())-i),p);}return x;}
double energy(const Audio& x){double sum=0;for(size_t i=x[0].size()/2;i<x[0].size();++i)sum+=x[0][i]*x[0][i];return sum;}
int main(){try{
    QuartzParameters p;p.enabled=true;
    for(int sr:{8000,44100,48000,96000,192000,384000}){
        const auto x=tone(sr);require(render(x,sr,p)==x,"Neutral path differs");
        auto q=p;q.enabled=false;require(render(x,sr,q)==x,"Off path differs");
        q=p;q.mix=0;q.inputDb=12;q.lowDb=9;require(render(x,sr,q)==x,"Dry path differs");
        q=p;q.inputDb=12;q.character=.2f;q.ceilingDb=-6;
        const auto hot=tone(sr,220,1),limited=render(hot,sr,q);
        require(limited==render(hot,sr,q,509),"Block size changes audio");
        for(float sample:limited[0])require(std::isfinite(sample)&&std::abs(sample)<=std::pow(10.,-.3)+1e-7,"Sample ceiling exceeded");
        require(limited[0]==limited[1],"Stereo detector changed image");
        require(render(hot,sr,q,257,1)[0]==limited[0],"Mono differs from matched stereo");
        auto silence=x;for(auto& c:silence)std::fill(c.begin(),c.end(),0.f);require(render(silence,sr,q)==silence,"Silence changed");
    }
    const auto bass=tone(48000,30),middle=tone(48000,800),treble=tone(48000,12000);
    p.lowDb=6;require(energy(render(bass,48000,p))>energy(bass)*3.5,"Low EQ failed");
    require(energy(render(treble,48000,p))<energy(treble)*1.05,"Low EQ leaked too far");
    p.lowDb=0;p.midDb=6;require(energy(render(middle,48000,p))>energy(middle)*3,"Mid EQ failed");
    p.midDb=0;p.highDb=6;require(energy(render(treble,48000,p))>energy(treble)*3.5,"High EQ failed");
    p=QuartzParameters{};p.enabled=true;p.character=1;
    const auto original=tone(48000,220,.5f),colored=render(original,48000,p);double difference=0;
    for(size_t i=0;i<original[0].size();++i)difference+=std::abs(original[0][i]-colored[0][i]);require(difference>100,"Character failed");
    p.character=0;p.inputDb=12;p.ceilingDb=-6;auto asymmetric=tone(48000,440,1);
    for(float& sample:asymmetric[1])sample*=.25f;
    const auto linked=render(asymmetric,48000,p);
    for(size_t i=0;i<linked[0].size();++i)require(std::abs(linked[1][i]-linked[0][i]*.25f)<1e-7,"Limiter changed an unequal stereo image");
    p.lowDb=9;p.highDb=-9;Audio impulse{std::vector<float>(48000),std::vector<float>(48000)};
    impulse[0][0]=impulse[1][0]=.5f;const auto tail=render(impulse,48000,p);
    for(size_t i=7200;i<48000;++i)require(std::abs(tail[0][i])<1e-7,"Tone filter tail exceeded 150 ms");
    p=QuartzParameters{};p.enabled=true;p.ceilingDb=-6;
    Audio recovery{std::vector<float>(48000,.1f),std::vector<float>(48000,.1f)};
    recovery[0][0]=recovery[1][0]=4;
    p.releaseMs=20;const auto fast=render(recovery,48000,p);
    p.releaseMs=500;const auto slow=render(recovery,48000,p);
    require(fast[0][4800]>slow[0][4800]*2,"Release time does not change gain recovery");
    require(std::abs(fast[0][24000]-.1f)<1e-6,"Limiter failed to recover after a transient");
    QuartzEngine engine;engine.prepare(48000,p);Audio block{std::vector<float>(257),std::vector<float>(257)};float* data[]{block[0].data(),block[1].data()};
    for(int step=0;step<800;++step){p.inputDb=step%2?12.f:-12.f;p.lowDb=p.highDb=step%2?9.f:-9.f;p.midDb=-p.lowDb;p.ceilingDb=step%2?0.f:-12.f;p.releaseMs=step%2?20.f:500.f;p.enabled=step%11!=0;for(int i=0;i<257;++i)block[0][i]=block[1][i]=2*std::sin((step*257+i)*.37);engine.process(data,2,257,p);for(const auto& c:block)for(float x:c)require(std::isfinite(x)&&std::abs(x)<8,"Automation unstable");}
    p.enabled=true;p.lowDb=std::numeric_limits<float>::quiet_NaN();block[0][0]=std::numeric_limits<float>::infinity();engine.process(data,2,257,p);for(const auto& c:block)for(float x:c)require(std::isfinite(x),"Invalid input escaped");
    std::cout<<"PASS: neutral/dry/off, three-band EQ, stereo-linked sample ceiling, character, mono, rates, silence, block invariance and automation\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
