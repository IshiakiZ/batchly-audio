// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace batchly {
struct CinderParameters {
    float grit=.35f, noise=.12f, toneHz=1800, noiseHz=6500;
    float drive=.3f, decaySeconds=.15f, width=.7f, mix=1;
    bool enabled=false;
};
class CinderEngine {
public:
    void prepare(double sampleRate,const CinderParameters& p={}) noexcept {
        sr=std::isfinite(sampleRate)?std::clamp(sampleRate,8000.0,384000.0):48000;
        smoothing=1-std::exp(-1/(.025*sr));attack=1-std::exp(-1/(.001*sr));
        values=sanitized(p);enabled=p.enabled?1:0;tick=0;
        randomState=0x92c41f07u;clear();coefficients();
    }
    std::array<float,2> levels() const noexcept {return {gritPeak,noisePeak};}
    void process(float* const* audio,int channels,int samples,const CinderParameters& p) noexcept {
        if(!audio||channels<1||samples<1)return;
        gritPeak=noisePeak=0;
        if(!p.enabled&&enabled==0)return;
        channels=std::min(channels,2);const auto targets=sanitized(p);
        for(int i=0;i<samples;++i){
            for(size_t n=0;n<values.size();++n)values[n]+=smoothing*(targets[n]-values[n]);
            enabled+=smoothing*((p.enabled?1.0:0.0)-enabled);
            if(++tick==16){tick=0;coefficients();}
            std::array<double,2> dry{},tone{},noise{};double detector=0;
            for(int ch=0;ch<channels;++ch){
                dry[ch]=std::isfinite(audio[ch][i])?audio[ch][i]:0;
                const double input=std::clamp(dry[ch],-8.0,8.0);
                detector=std::max(detector,std::abs(input));
                const double band=bandpass(input,toneLow,toneHigh,toneInput[ch]);
                // The difference removes the undriven curve. A second band filter
                // keeps the generated harmonics near the selected tonal region.
                const double grit=(std::tanh(band*driveGain)-std::tanh(band))/std::sqrt(driveGain);
                tone[ch]=bandpass(grit,toneLow,toneHigh,toneOutput[ch])*values[0]*2;
            }
            envelope=flush(envelope+(detector>envelope?attack:release)*(detector-envelope));
            // Fixed seeded noise is generated here, never sampled from a sound library.
            // Always advance both streams, keeping left/mono output independent of block size.
            const double leftNoise=nextNoise(),rightNoise=nextNoise();
            noise[0]=bandpass(leftNoise,noiseLow,noiseHigh,noiseFilter[0]);
            noise[1]=bandpass(rightNoise,noiseLow,noiseHigh,noiseFilter[1]);
            if(channels==2){
                const double mid=.5*(tone[0]+tone[1]),side=.5*(tone[0]-tone[1])*values[6];
                tone[0]=mid+side;tone[1]=mid-side;
                // Equal-power normalization avoids making the right noise layer
                // quieter at intermediate widths as its correlation changes.
                const double width=values[6],common=1-width;
                noise[1]=(noise[0]*common+noise[1]*width)/std::sqrt(common*common+width*width);
            }
            for(int ch=0;ch<channels;++ch){
                const double texture=noise[ch]*values[1]*envelope*.6;
                const double addition=tone[ch]+texture;
                audio[ch][i]=static_cast<float>(dry[ch]+enabled*values[7]*addition);
                gritPeak=std::max(gritPeak,static_cast<float>(std::abs(tone[ch])));
                noisePeak=std::max(noisePeak,static_cast<float>(std::abs(texture)));
            }
        }
        if(!p.enabled&&enabled<1e-9){enabled=0;clear();}
    }
    static double tailSeconds(const CinderParameters& p) noexcept {
        // Ten envelope time constants cover a nominal decay below -80 dB,
        // with extra room for the lowest band filters to settle.
        return 10*safe(p.decaySeconds,.02f,.8f,.15f)+.3;
    }
private:
    static float safe(float x,float lo,float hi,float fallback) noexcept{return std::isfinite(x)?std::clamp(x,lo,hi):fallback;}
    static double flush(double x) noexcept{return std::abs(x)<1e-24?0:x;}
    static std::array<double,8> sanitized(const CinderParameters& p) noexcept {
        return {safe(p.grit,0.f,1.f,.35f),safe(p.noise,0.f,1.f,.12f),safe(p.toneHz,80.f,12000.f,1800.f),
            safe(p.noiseHz,200.f,16000.f,6500.f),safe(p.drive,0.f,1.f,.3f),safe(p.decaySeconds,.02f,.8f,.15f),
            safe(p.width,0.f,1.f,.7f),safe(p.mix,0.f,1.f,1.f)};
    }
    static double lowpass(double input,double coefficient,double& state) noexcept {
        const double change=(input-state)*coefficient,result=change+state;state=flush(result+change);return result;
    }
    static double bandpass(double input,double low,double high,std::array<double,2>& state) noexcept {
        return lowpass(input-lowpass(input,low,state[0]),high,state[1]);
    }
    double filterCoefficient(double frequency) const noexcept {
        const double tangent=std::tan(3.14159265359*std::min(frequency,sr*.45)/sr);return tangent/(1+tangent);
    }
    void coefficients() noexcept {
        const double toneCenter=std::min(values[2],sr*.3),noiseCenter=std::min(values[3],sr*.3);
        toneLow=filterCoefficient(toneCenter*.5);toneHigh=filterCoefficient(toneCenter*2);
        noiseLow=filterCoefficient(noiseCenter*.5);noiseHigh=filterCoefficient(noiseCenter*2);
        driveGain=1+15*values[4]*values[4];release=1-std::exp(-1/(values[5]*sr));
    }
    double nextNoise() noexcept {
        randomState^=randomState<<13;randomState^=randomState>>17;randomState^=randomState<<5;
        return static_cast<double>(randomState)/2147483648.0-1;
    }
    void clear() noexcept{toneInput={};toneOutput={};noiseFilter={};envelope=0;gritPeak=noisePeak=0;}
    std::array<double,8> values{};
    std::array<std::array<double,2>,2> toneInput{},toneOutput{},noiseFilter{};
    double sr=48000,smoothing=0,enabled=0,attack=0,release=0,envelope=0;
    double toneLow=0,toneHigh=0,noiseLow=0,noiseHigh=0,driveGain=1;
    float gritPeak=0,noisePeak=0;
    uint32_t randomState=0x92c41f07u;int tick=0;
};
}
