// SPDX-License-Identifier: AGPL-3.0-or-later
#include "RackEngine.h"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace batchly;
using Audio=std::array<std::vector<float>,2>;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
Audio tone(int sr,double hz=443,float level=.1f){
    Audio x{std::vector<float>(sr),std::vector<float>(sr)};
    for(int i=0;i<sr;++i)x[0][i]=x[1][i]=level*static_cast<float>(std::sin(i*6.28318530718*hz/sr));return x;
}
Audio render(Audio x,int sr,const CinderParameters& p,int block=257,int channels=2){
    CinderEngine engine;engine.prepare(sr,p);
    for(int offset=0;offset<static_cast<int>(x[0].size());offset+=block){
        float* data[]{x[0].data()+offset,x[1].data()+offset};
        engine.process(data,channels,std::min(block,static_cast<int>(x[0].size())-offset),p);
    }return x;
}
double differenceEnergy(const Audio& wet,const Audio& dry,int channel=0){
    double e=0;for(size_t i=wet[0].size()/2;i<wet[0].size();++i){const double d=wet[channel][i]-dry[channel][i];e+=d*d;}return e;
}
double roughness(const Audio& wet,const Audio& dry){
    double delta=0,energy=0,previous=0;
    for(size_t i=wet[0].size()/2;i<wet[0].size();++i){const double d=wet[0][i]-dry[0][i];energy+=d*d;delta+=(d-previous)*(d-previous);previous=d;}return delta/energy;
}
int main(){try{
    CinderParameters p;p.enabled=true;
    for(int sr:{8000,44100,48000,96000,192000,384000}){
        const auto input=tone(sr);auto q=p;q.enabled=false;require(render(input,sr,q)==input,"Disabled texture changed input");
        q=p;q.mix=0;require(render(input,sr,q)==input,"Dry texture changed input");
        q=p;q.grit=q.noise=0;require(render(input,sr,q)==input,"Zero additions changed input");
        q=p;q.drive=q.noise=0;require(render(input,sr,q)==input,"Zero drive generated grit");
        const auto wet=render(input,sr,p);require(wet==render(input,sr,p,509),"Texture depends on block size or reset is not deterministic");
        const auto mono=render(input,sr,p,257,1);require(mono[0]==wet[0]&&mono[1]==input[1],"Mono mismatch");
        q=p;q.width=0;const auto centered=render(input,sr,q);require(centered[0]==centered[1],"Width zero failed to center additions");
        auto silence=input;for(auto& ch:silence)std::fill(ch.begin(),ch.end(),0.f);
        require(render(silence,sr,p)==silence,"Idle noise generated from silence");
        for(const auto& ch:wet)for(float x:ch)require(std::isfinite(x)&&std::abs(x)<1,"Normal texture unstable");
    }
    const auto input=tone(48000,220,.15f);p.noise=0;p.grit=1;p.drive=.8f;p.toneHz=220;
    const auto focused=render(input,48000,p);p.toneHz=9000;
    require(differenceEnergy(focused,input)>differenceEnergy(render(input,48000,p),input)*20,"Tone Focus did not select its region");
    p.grit=0;p.noise=1;p.noiseHz=400;const auto low=render(input,48000,p);p.noiseHz=12000;const auto high=render(input,48000,p);
    require(roughness(high,input)>roughness(low,input)*3,"Noise Focus did not move spectral texture");
    p.width=.5f;const auto spread=render(input,48000,p);
    const double balance=differenceEnergy(spread,input,1)/differenceEnergy(spread,input,0);
    require(balance>.9&&balance<1.1,"Intermediate width unbalances noise power");
    p.width=0;auto stereo=input;stereo[1]=tone(48000,880,.08f)[0];const auto centered=render(stereo,48000,p);
    for(size_t i=0;i<stereo[0].size();++i)require(std::abs((centered[0][i]-stereo[0][i])-(centered[1][i]-stereo[1][i]))<3e-8,"Centered additions changed dry stereo image");
    Audio burst{std::vector<float>(48000*10),std::vector<float>(48000*10)};
    for(int i=0;i<2400;++i)burst[0][i]=burst[1][i]=input[0][i];
    p.decaySeconds=.02f;const auto shortTail=render(burst,48000,p);p.decaySeconds=.8f;const auto longTail=render(burst,48000,p);
    double shortEnergy=0,longEnergy=0;
    for(int i=24000;i<48000;++i){shortEnergy+=shortTail[0][i]*shortTail[0][i];longEnergy+=longTail[0][i]*longTail[0][i];}
    require(longEnergy>.001&&longEnergy>shortEnergy*100,"Decay did not extend texture release");
    for(size_t i=static_cast<size_t>((CinderEngine::tailSeconds(p)+.05)*48000);i<longTail[0].size();++i)require(std::abs(longTail[0][i])<1e-5,"Noise outlasted nominal tail allowance");
    CinderEngine engine;engine.prepare(48000,p);Audio block{std::vector<float>(257),std::vector<float>(257)};float* data[]{block[0].data(),block[1].data()};
    for(int step=0;step<1200;++step){
        p.grit=p.noise=p.drive=step%2?1.f:0.f;p.toneHz=step%2?80.f:12000.f;p.noiseHz=step%2?16000.f:200.f;
        p.decaySeconds=step%2?.02f:.8f;p.width=step%2?1.f:0.f;p.enabled=step%11!=0;
        for(int i=0;i<257;++i)block[0][i]=block[1][i]=.8f*std::sin((step*257+i)*.47);
        engine.process(data,2,257,p);for(const auto& ch:block)for(float x:ch)require(std::isfinite(x)&&std::abs(x)<8,"Extreme automation unstable");
    }
    p.enabled=true;p.noiseHz=std::numeric_limits<float>::infinity();p.decaySeconds=std::numeric_limits<float>::quiet_NaN();
    block[0][0]=std::numeric_limits<float>::quiet_NaN();block[1][0]=std::numeric_limits<float>::infinity();engine.process(data,2,257,p);
    for(const auto& ch:block)for(float x:ch)require(std::isfinite(x),"Invalid input escaped");
    std::cout<<"PASS: off/dry/neutral, silence, deterministic noise, tone/noise focus, release, tail bound, stereo balance/width, mono, sample rates, block invariance, automation, invalid input\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
