// SPDX-License-Identifier: AGPL-3.0-or-later
#include "RackEngine.h"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace batchly;
using Audio=std::array<std::vector<float>,2>;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
Audio tone(int sr,double hz=110,float level=.15f){
    Audio x{std::vector<float>(sr),std::vector<float>(sr)};
    for(int i=0;i<sr;++i)x[0][i]=x[1][i]=level*static_cast<float>(std::sin(i*6.28318530718*hz/sr));return x;
}
Audio render(Audio x,int sr,const EmberParameters& p,int block=257,int channels=2){
    EmberEngine engine;engine.prepare(sr,p);
    for(int offset=0;offset<static_cast<int>(x[0].size());offset+=block){
        float* data[]{x[0].data()+offset,x[1].data()+offset};engine.process(data,channels,std::min(block,static_cast<int>(x[0].size())-offset),p);
    }return x;
}
double harmonic(const Audio& x,double hz,int sr=48000){
    double real=0,imaginary=0;for(size_t i=x[0].size()/2;i<x[0].size();++i){const double phase=i*6.28318530718*hz/sr;real+=x[0][i]*std::cos(phase);imaginary+=x[0][i]*std::sin(phase);}return std::hypot(real,imaginary)/(x[0].size()/2);
}
double difference(const Audio& x,const Audio& y){double e=0;for(size_t i=0;i<x[0].size();++i){const double d=x[0][i]-y[0][i];e+=d*d;}return e;}
int main(){try{
    // Independently integrate each curve numerically to check its analytic average.
    for(int mode=0;mode<4;++mode)for(double end:{-3.,-.21,.001,.3,2.4}){
        const double start=-.7;double sum=0;const int count=4096;
        for(int n=0;n<count;++n){const double x=start+(end-start)*(n+.5)/count;
            sum+=mode==0?std::tanh(x):mode==1?std::atan(2*x)*2/3.141592653589793:mode==2?x/(1+std::abs(x)):std::sin(x);}
        require(std::abs(EmberEngine::averagedCurve(end,start,mode/3.0)-sum/count)<1e-6,"Curve integral is incorrect");
    }
    Audio naive{std::vector<float>(48000),std::vector<float>(48000)},averaged=naive;double previous=0;
    for(int n=0;n<48000;++n){const double x=8*std::sin(n*6.28318530718*9000/48000);naive[0][n]=static_cast<float>(std::tanh(x));averaged[0][n]=static_cast<float>(EmberEngine::averagedCurve(x,previous,0));previous=x;}
    require(harmonic(averaged,21000)/harmonic(averaged,9000)<harmonic(naive,21000)/harmonic(naive,9000)*.75,"First folded harmonic was not reduced relative to the fundamental");
    EmberParameters p;p.enabled=true;
    for(int sr:{8000,44100,48000,96000,192000,384000}){
        const auto input=tone(sr);auto q=p;q.enabled=false;require(render(input,sr,q)==input,"Disabled Ember changed audio");
        q=p;q.mix=0;require(render(input,sr,q)==input,"Dry Ember changed audio");
        const auto wet=render(input,sr,p);require(wet==render(input,sr,p,509),"Ember depends on block size");
        require(wet[0]==wet[1],"Matched stereo differs");
        const auto mono=render(input,sr,p,257,1);require(mono[0]==wet[0]&&mono[1]==input[1],"Mono mismatch");
        auto silence=input;for(auto& ch:silence)std::fill(ch.begin(),ch.end(),0.f);
        require(render(silence,sr,p)==silence,"Bias created sound from silence");
        for(const auto& ch:wet)for(float x:ch)require(std::isfinite(x)&&std::abs(x)<1,"Normal saturation unstable");
    }
    const auto input=tone(48000,110,.3f);p.trimDb=0;p.anchor=p.color=p.bias=0;p.filterHz=18000;p.driveDb=18;
    std::array<Audio,4> modes;
    for(int mode=0;mode<4;++mode){p.shape=mode/3.f;modes[mode]=render(input,48000,p);require(harmonic(modes[mode],330)>.001,"Saturation mode generated no third harmonic");}
    for(int a=0;a<4;++a)for(int b=a+1;b<4;++b)require(difference(modes[a],modes[b])>1,"Saturation shapes are not distinct");
    p.shape=0;p.bias=1;const auto asymmetric=render(input,48000,p);
    require(harmonic(asymmetric,220)>.01&&harmonic(asymmetric,220)>harmonic(modes[0],220)*100,"Bias failed to generate even harmonics");
    double dc=0;for(size_t i=24000;i<48000;++i)dc+=asymmetric[0][i];require(std::abs(dc/24000)<1e-4,"DC offset remained after settling");
    p.bias=0;const auto bass=tone(48000,40,.1f);p.anchor=0;const auto unanchored=render(bass,48000,p);p.anchor=1;const auto anchored=render(bass,48000,p);
    require(std::abs(harmonic(anchored,40)-harmonic(bass,40))<std::abs(harmonic(unanchored,40)-harmonic(bass,40))*.5,"Anchor did not restore original bass level");
    p.anchor=0;p.driveDb=6;const auto high=tone(48000,6000,.1f);p.filterHz=18000;const double bright=harmonic(render(high,48000,p),6000);p.filterHz=800;
    require(harmonic(render(high,48000,p),6000)<bright*.3,"Filter failed to soften top end");
    p.filterHz=18000;p.color=-1;const auto dark=render(high,48000,p);p.color=1;require(harmonic(render(high,48000,p),6000)>harmonic(dark,6000)*2,"Color control failed");
    EmberEngine engine;engine.prepare(48000,p);Audio block{std::vector<float>(257),std::vector<float>(257)};float* data[]{block[0].data(),block[1].data()};
    for(int step=0;step<1200;++step){
        p.driveDb=step%2?24.f:0.f;p.shape=p.bias=p.anchor=step%2?1.f:0.f;p.color=step%2?1.f:-1.f;
        p.filterHz=step%2?800.f:18000.f;p.trimDb=step%2?-18.f:0.f;p.enabled=step%11!=0;
        for(int i=0;i<257;++i)block[0][i]=block[1][i]=.8f*std::sin((step*257+i)*.47);
        engine.process(data,2,257,p);for(const auto& ch:block)for(float x:ch)require(std::isfinite(x)&&std::abs(x)<8,"Extreme automation unstable");
    }
    p.enabled=true;p.shape=std::numeric_limits<float>::quiet_NaN();p.filterHz=std::numeric_limits<float>::infinity();
    block[0][0]=std::numeric_limits<float>::quiet_NaN();block[1][0]=std::numeric_limits<float>::infinity();engine.process(data,2,257,p);
    for(const auto& ch:block)for(float x:ch)require(std::isfinite(x),"Invalid input escaped");
    p=EmberParameters{};p.enabled=true;Audio impulse{std::vector<float>(48000),std::vector<float>(48000)};impulse[0][0]=impulse[1][0]=.5f;
    const auto tail=render(impulse,48000,p);for(size_t i=28800;i<48000;++i)require(std::abs(tail[0][i])<1e-7,"Filter tail exceeded allowance");
    std::cout<<"PASS: curve integrals, folded-harmonic reduction, off/dry, four shapes, bias/even harmonics, DC removal, anchor, color/filter, mono, silence, rates, block invariance, automation and invalid input\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
