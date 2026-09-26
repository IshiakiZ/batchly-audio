// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace batchly {
struct QuartzParameters {
    float inputDb=0,lowDb=0,midDb=0,highDb=0,character=0,ceilingDb=-1,releaseMs=120,mix=1;
    bool enabled=false;
};
class QuartzEngine {
public:
    void prepare(double rate,const QuartzParameters& p={}) noexcept {
        sr=std::isfinite(rate)?std::clamp(rate,8000.0,384000.0):48000;
        smoothing=1-std::exp(-1/(.025*sr));values=sanitized(p);enabled=p.enabled?1:0;
        lowCoefficient=coefficient(180);highCoefficient=coefficient(3500);
        lowState={};highState={};gain=1;tick=0;reduction=0;coefficients();
    }
    float gainReductionDb() const noexcept{return reduction;}
    void process(float* const* audio,int channels,int samples,const QuartzParameters& p) noexcept {
        if(!audio||channels<1||samples<1)return;
        reduction=0;if(!p.enabled&&enabled==0)return;
        channels=std::min(channels,2);const auto targets=sanitized(p);
        for(int i=0;i<samples;++i){
            for(size_t n=0;n<values.size();++n)values[n]+=smoothing*(targets[n]-values[n]);
            enabled+=smoothing*((p.enabled?1.0:0.0)-enabled);
            if(++tick==16){tick=0;coefficients();}
            std::array<double,2> dry{},wet{};double peak=0;
            for(int ch=0;ch<channels;++ch){
                dry[ch]=std::isfinite(audio[ch][i])?audio[ch][i]:0;
                const double driven=std::clamp(dry[ch],-8.0,8.0)*inputGain;
                const double low=lowpass(driven,lowCoefficient,lowState[ch]);
                const double belowHigh=lowpass(driven,highCoefficient,highState[ch]);
                const double equalized=driven+low*(lowGain-1)+(belowHigh-low)*(midGain-1)+(driven-belowHigh)*(highGain-1);
                // Blend an original soft curve into the broad EQ output. Zero
                // character retains exact linear processing and its phase.
                wet[ch]=equalized+values[4]*(std::tanh(equalized*2)/2-equalized);
                peak=std::max(peak,std::abs(wet[ch]));
            }
            const double required=peak>ceiling?ceiling/peak:1;
            // Linked instantaneous attack bounds sample peaks without lookahead.
            // Exponential release restores gain without exceeding that bound.
            gain=std::min(required,1-(1-gain)*release);
            reduction=std::max(reduction,static_cast<float>(-20*std::log10(std::max(gain,1e-12))));
            for(int ch=0;ch<channels;++ch)
                audio[ch][i]=static_cast<float>(dry[ch]+enabled*values[7]*(wet[ch]*gain-dry[ch]));
        }
        if(!p.enabled&&enabled<1e-9){enabled=0;lowState={};highState={};gain=1;}
    }
private:
    static float safe(float x,float lo,float hi,float fallback) noexcept{return std::isfinite(x)?std::clamp(x,lo,hi):fallback;}
    static std::array<double,8> sanitized(const QuartzParameters& p) noexcept {
        return {safe(p.inputDb,-12.f,12.f,0.f),safe(p.lowDb,-9.f,9.f,0.f),safe(p.midDb,-9.f,9.f,0.f),safe(p.highDb,-9.f,9.f,0.f),
            safe(p.character,0.f,1.f,0.f),safe(p.ceilingDb,-12.f,0.f,-1.f),safe(p.releaseMs,20.f,500.f,120.f),safe(p.mix,0.f,1.f,1.f)};
    }
    static double lowpass(double input,double coefficient,double& state) noexcept {
        const double change=(input-state)*coefficient,result=change+state;state=result+change;
        if(std::abs(state)<1e-24)state=0;return result;
    }
    double coefficient(double hz) const noexcept {
        const double tangent=std::tan(3.141592653589793*std::min(hz,sr*.45)/sr);return tangent/(1+tangent);
    }
    void coefficients() noexcept {
        inputGain=std::pow(10.,values[0]/20);lowGain=std::pow(10.,values[1]/20);
        midGain=std::pow(10.,values[2]/20);highGain=std::pow(10.,values[3]/20);
        ceiling=std::pow(10.,values[5]/20);release=std::exp(-1/(values[6]*.001*sr));
    }
    std::array<double,8> values{};
    std::array<double,2> lowState{},highState{};
    double sr=48000,smoothing=0,enabled=0,lowCoefficient=0,highCoefficient=0;
    double inputGain=1,lowGain=1,midGain=1,highGain=1,ceiling=1,release=0,gain=1;
    int tick=0;float reduction=0;
};
}
