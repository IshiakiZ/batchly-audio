// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace batchly {
struct VistaParameters {
    float lowWidth=.8f, midWidth=1.1f, highWidth=1.3f, lowHz=180, highHz=3500;
    float spread=0, delayMs=11, mix=1;
    bool enabled=false;
};
class VistaEngine {
public:
    void prepare(double sampleRate,const VistaParameters& p={}) {
        sr=std::isfinite(sampleRate)?std::clamp(sampleRate,8000.0,384000.0):48000;
        delay.assign(static_cast<size_t>(std::ceil(sr*.035))+4,0);
        smoothing=1-std::exp(-1/(.025*sr));values=sanitized(p);
        enabled=p.enabled?1:0;clear();coefficients();
    }
    std::array<float,2> levels() const noexcept{return {midPeak,sidePeak};}
    void process(float* const* audio,int channels,int samples,const VistaParameters& p) noexcept {
        if(!audio||channels<1||samples<1||delay.empty())return;
        midPeak=sidePeak=0;
        // A mono host has no side channel. Leave its original audio intact.
        if(channels==1||(!p.enabled&&enabled==0))return;
        const auto targets=sanitized(p);
        for(int i=0;i<samples;++i){
            for(size_t n=0;n<values.size();++n)values[n]+=smoothing*(targets[n]-values[n]);
            enabled+=smoothing*((p.enabled?1.0:0.0)-enabled);
            if(++tick==16){tick=0;coefficients();}
            const double left=std::isfinite(audio[0][i])?audio[0][i]:0;
            const double right=std::isfinite(audio[1][i])?audio[1][i]:0;
            const double mid=std::clamp((left+right)*.5,-8.0,8.0);
            const double side=std::clamp((left-right)*.5,-8.0,8.0);
            const double low=lowpass(side,lowCoefficient,sideLow);
            const double belowHigh=lowpass(side,highCoefficient,sideHigh);
            // Complementary differences sum to the original side signal. A
            // neutral width therefore needs no crossover phase compensation.
            const double middle=belowHigh-low, high=side-belowHigh;
            const double upperMid=mid-lowpass(mid,lowCoefficient,midLow);
            delay[position]=upperMid;
            double read=static_cast<double>(position)-values[6]*sr*.001;
            while(read<0)read+=delay.size();
            const auto first=static_cast<size_t>(read);const double fraction=read-first;
            const double delayed=delay[first]*(1-fraction)+delay[(first+1)%delay.size()]*fraction;
            position=(position+1)%delay.size();
            const double added=(delayed-upperMid)*.5*values[5];
            const double change=(low*(values[0]-1)+middle*(values[1]-1)+high*(values[2]-1)+added)*values[7]*enabled;
            // Equal and opposite additions preserve the original mono fold-down.
            audio[0][i]=static_cast<float>(left+change);
            audio[1][i]=static_cast<float>(right-change);
            midPeak=std::max(midPeak,static_cast<float>(std::abs(mid)));
            sidePeak=std::max(sidePeak,static_cast<float>(std::abs(side+change)));
        }
        if(!p.enabled&&enabled<1e-9){enabled=0;clear();}
    }
private:
    static float safe(float x,float lo,float hi,float fallback) noexcept{return std::isfinite(x)?std::clamp(x,lo,hi):fallback;}
    static std::array<double,8> sanitized(const VistaParameters& p) noexcept {
        return {safe(p.lowWidth,0.f,2.f,.8f),safe(p.midWidth,0.f,2.f,1.1f),safe(p.highWidth,0.f,2.f,1.3f),
            safe(p.lowHz,60.f,600.f,180.f),safe(p.highHz,1200.f,12000.f,3500.f),safe(p.spread,0.f,1.f,0.f),
            safe(p.delayMs,1.f,30.f,11.f),safe(p.mix,0.f,1.f,1.f)};
    }
    static double lowpass(double input,double coefficient,double& state) noexcept {
        const double change=(input-state)*coefficient,result=change+state;
        state=result+change;if(std::abs(state)<1e-24)state=0;return result;
    }
    double coefficient(double hz) const noexcept {
        const double tangent=std::tan(3.141592653589793*std::min(hz,sr*.45)/sr);return tangent/(1+tangent);
    }
    void coefficients() noexcept{lowCoefficient=coefficient(values[3]);highCoefficient=coefficient(values[4]);}
    void clear() noexcept {
        sideLow=sideHigh=midLow=0;position=0;tick=0;midPeak=sidePeak=0;
        // At most 35 ms of delay, cleared only on prepare or after disable fades.
        std::fill(delay.begin(),delay.end(),0.0);
    }
    std::vector<double> delay;
    std::array<double,8> values{};
    double sr=48000,smoothing=0,enabled=0,lowCoefficient=0,highCoefficient=0;
    double sideLow=0,sideHigh=0,midLow=0;
    size_t position=0;int tick=0;float midPeak=0,sidePeak=0;
};
}
