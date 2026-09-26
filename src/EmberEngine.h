// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace batchly {
struct EmberParameters {
    float driveDb=9, shape=0, color=-.15f, filterHz=6500;
    float anchor=.65f, bias=.12f, trimDb=-5, mix=1;
    bool enabled=false;
};
class EmberEngine {
public:
    void prepare(double sampleRate,const EmberParameters& p={}) noexcept {
        sr=std::isfinite(sampleRate)?std::clamp(sampleRate,8000.0,384000.0):48000;
        smoothing=1-std::exp(-1/(.025*sr));
        colorCoefficient=filterCoefficient(700);bassCoefficient=filterCoefficient(150);dcCoefficient=filterCoefficient(5);
        values=sanitized(p);enabled=p.enabled?1:0;tick=0;clear();coefficients();
    }
    std::array<float,2> levels() const noexcept{return {inputPeak,wetPeak};}
    // First-order antiderivative antialiasing averages each nonlinear curve
    // between adjacent driven samples. This reduces, but cannot eliminate, aliasing.
    static double averagedCurve(double current,double previous,double shape) noexcept {
        shape=std::clamp(shape,0.0,1.0)*3;
        const int index=std::min(2,static_cast<int>(shape));const double blend=shape-index;
        auto average=[&](int mode){
            const double delta=current-previous;
            return std::abs(delta)<1e-5?curve(.5*(current+previous),mode)
                :(primitive(current,mode)-primitive(previous,mode))/delta;
        };
        return average(index)*(1-blend)+average(index+1)*blend;
    }
    void process(float* const* audio,int channels,int samples,const EmberParameters& p) noexcept {
        if(!audio||channels<1||samples<1)return;
        inputPeak=wetPeak=0;if(!p.enabled&&enabled==0)return;
        channels=std::min(channels,2);const auto targets=sanitized(p);
        for(int i=0;i<samples;++i){
            for(size_t n=0;n<values.size();++n)values[n]+=smoothing*(targets[n]-values[n]);
            enabled+=smoothing*((p.enabled?1.0:0.0)-enabled);
            if(++tick==16){tick=0;coefficients();}
            for(int ch=0;ch<channels;++ch){
                const double dry=std::isfinite(audio[ch][i])?audio[ch][i]:0;
                const double input=std::clamp(dry,-8.0,8.0);
                const double high=input-lowpass(input,colorCoefficient,colorState[ch]);
                const double colored=input+values[2]*high*.8;
                const double driven=std::clamp(colored*driveGain,-64.0,64.0);
                const double bias=values[5]*.6;
                // Apply today's bias to both endpoints, so changing bias on silence
                // cannot generate a click merely from differing old/new offsets.
                double wet=averagedCurve(driven+bias,previousDriven[ch]+bias,values[1])
                    -averagedCurve(bias,bias,values[1]);
                previousDriven[ch]=driven;
                wet-=lowpass(wet,dcCoefficient,dcState[ch]);
                wet=lowpass(wet,outputCoefficient,outputState[ch]);
                const double originalBass=lowpass(input,bassCoefficient,bassDry[ch]);
                const double processedBass=lowpass(wet,bassCoefficient,bassWet[ch]);
                wet+=values[4]*(originalBass-processedBass);
                wet*=trimGain;
                audio[ch][i]=static_cast<float>(dry+enabled*values[7]*(wet-dry));
                inputPeak=std::max(inputPeak,static_cast<float>(std::abs(input)));
                wetPeak=std::max(wetPeak,static_cast<float>(std::abs(wet)));
            }
        }
        if(!p.enabled&&enabled<1e-9){enabled=0;clear();}
    }
private:
    static double curve(double x,int mode) noexcept {
        switch(mode){case 0:return std::tanh(x);case 1:return std::atan(2*x)*.6366197723675813;
            case 2:return x/(1+std::abs(x));default:return std::sin(x);}
    }
    static double primitive(double x,int mode) noexcept {
        const double a=std::abs(x);
        switch(mode){
            // Stable log(cosh(x)) avoids overflow for unusually hot input.
            case 0:return a+std::log1p(std::exp(-2*a))-.6931471805599453;
            case 1:return .6366197723675813*(x*std::atan(2*x)-.25*std::log1p(4*x*x));
            case 2:return a-std::log1p(a);
            default:return -std::cos(x);
        }
    }
    static float safe(float x,float lo,float hi,float fallback) noexcept{return std::isfinite(x)?std::clamp(x,lo,hi):fallback;}
    static double flush(double x) noexcept{return std::abs(x)<1e-24?0:x;}
    static std::array<double,8> sanitized(const EmberParameters& p) noexcept {
        return {safe(p.driveDb,0.f,24.f,9.f),safe(p.shape,0.f,1.f,0.f),safe(p.color,-1.f,1.f,-.15f),safe(p.filterHz,800.f,18000.f,6500.f),
            safe(p.anchor,0.f,1.f,.65f),safe(p.bias,0.f,1.f,.12f),safe(p.trimDb,-18.f,0.f,-5.f),safe(p.mix,0.f,1.f,1.f)};
    }
    static double lowpass(double input,double coefficient,double& state) noexcept {
        const double change=(input-state)*coefficient,result=change+state;state=flush(result+change);return result;
    }
    double filterCoefficient(double frequency) const noexcept {
        const double tangent=std::tan(3.14159265359*std::min(frequency,sr*.45)/sr);return tangent/(1+tangent);
    }
    void coefficients() noexcept {
        driveGain=std::pow(10.0,values[0]/20);trimGain=std::pow(10.0,values[6]/20);outputCoefficient=filterCoefficient(values[3]);
    }
    void clear() noexcept{colorState={};dcState={};outputState={};bassDry={};bassWet={};previousDriven={};inputPeak=wetPeak=0;}
    std::array<double,8> values{};
    std::array<double,2> colorState{},dcState{},outputState{},bassDry{},bassWet{},previousDriven{};
    double sr=48000,smoothing=0,enabled=0,colorCoefficient=0,bassCoefficient=0,dcCoefficient=0,outputCoefficient=0,driveGain=1,trimGain=1;
    float inputPeak=0,wetPeak=0;int tick=0;
};
}
