#include "TestHarness.h"
#include "../Source/Core/EffectsDsp.h"
#include <functional>
#include <cstring>
using namespace mps;
namespace {
using Buffer = std::vector<float>;
Buffer renderFx(FxType type, const Buffer& input, int block, double sr,
                const std::function<void(FxSlotParams&)>& setup = {})
{
    FxSlotParams params; params.setDefaultsForType(type);
    if (setup) setup(params);
    FxSlotProcessor proc; proc.prepare(sr);
    Buffer output(input.size()), right(input.size());
    for (int i=0;i<(int)input.size();i+=block)
        proc.process(input.data()+i,input.data()+i,output.data()+i,right.data()+i,
                     std::min(block,(int)input.size()-i),params);
    return output;
}
void normalized(FxSlotParams& p) { p.parameterEncoding.set(2); for(int i=0;i<18;i++)p.parameter(i).set(.5f);p.mix.set(1); }
float maxDiff(const Buffer& a,const Buffer& b) { float d=0;for(size_t i=0;i<a.size();i++)d=std::max(d,std::abs(a[i]-b[i]));return d; }
Buffer tone(int n,float freq,float sr,float amplitude=.2f) { Buffer x(n);for(int i=0;i<n;i++)x[i]=amplitude*std::sin(2*kPi*freq*i/sr);return x; }
float rms(const Buffer& x,int start) { double e=0;for(int i=start;i<(int)x.size();i++)e+=x[i]*x[i];return (float)std::sqrt(e/(x.size()-start)); }
}
TEST(fxSoundMaxerIsNotDryBypass)
{
    auto input=tone(48000,80,48000);
    auto out=renderFx(FxType::SoundMaxer,input,256,48000,normalized);
    CHECK(maxDiff(input,out)>.02f);
}
TEST(fxDelayTimeFeedbackAndLongSilence)
{
    for(double sr:{44100.0,48000.0,96000.0}) {
        Buffer input((int)(sr*3));input[0]=1;
        auto setup=[](FxSlotParams& p){p.p1.set(1000);p.p2.set(1000);p.p3.set(50);p.p4.set(0);p.p5.set(0);p.mix.set(1);};
        auto a=renderFx(FxType::StereoDelay,input,64,sr,setup);
        auto b=renderFx(FxType::StereoDelay,input,1024,sr,setup);
        CHECK_NEAR(a[(int)sr],1,1e-6);CHECK_NEAR(a[(int)(2*sr)],.5,1e-6);
        CHECK_NEAR(maxDiff(a,b),0,1e-6);
    }
}
TEST(fxNormalizedDelayUsesTimeAndFeedbackControls)
{
    Buffer input(96000);input[0]=1;
    auto out=renderFx(FxType::StereoDelay,input,128,48000,[](FxSlotParams& p){
        normalized(p);p.p1.set(1);p.p2.set((500.f-5)/2495);p.p3.set(0);p.p4.set(1);
        p.p5.set(1);p.p6.set(1);p.p7.set(.5f);p.p9.set(50.f/95);p.p10.set(50.f/95);
    });
    CHECK_NEAR(out[24000],1,1e-5);CHECK_NEAR(out[48000],.5,1e-5);
}
TEST(fxReverbTailAndModelGeometry)
{
    Buffer input(96000);input[0]=.5f;
    auto a=renderFx(FxType::HallReverb,input,64,48000);
    auto b=renderFx(FxType::HallReverb,input,1024,48000);
    CHECK_NEAR(maxDiff(a,b),0,1e-6);
    auto hall=renderFx(FxType::HallReverb,input,256,48000,normalized);
    auto room=renderFx(FxType::RoomReverb,input,256,48000,normalized);
    auto chamber=renderFx(FxType::ChamberReverb,input,256,48000,normalized);
    CHECK(maxDiff(hall,room)>.001f);CHECK(maxDiff(hall,chamber)>.001f);
}
TEST(fxSuboctaverFrequencies)
{
    for(float sr:{44100.f,48000.f,96000.f})for(int octave=1;octave<=2;octave++) {
        PremiumUtilityAmpDsp dsp;dsp.prepare(sr);auto l=tone((int)(2*sr),200,sr,.5f);auto r=l;
        dsp.process(l.data(),r.data(),(int)l.size(),FxType::Suboctaver,0,octave==1?100:0,octave==2?100:0,2,1);
        int crosses=0;for(int i=(int)sr+1;i<(int)l.size();i++)if(l[i-1]<0&&l[i]>=0)crosses++;
        CHECK(std::abs(crosses-(octave==1?100:50))<=1);
    }
}
TEST(fxLimiterCeilingAndTimeControls)
{
    for(float sr:{44100.f,48000.f,96000.f}) {
        PremiumDynamicsDsp dsp;dsp.prepare(sr);
        Buffer l((int)sr),r((int)sr);l[100]=8;r[101]=-6;
        dsp.process(l.data(),r.data(),(int)sr,FxType::StereoPrecisionLimiter,18,-12,100,6,1,10,20);
        float peak=0;for(int i=0;i<(int)sr;i++)peak=std::max({peak,std::abs(l[i]),std::abs(r[i])});
        CHECK(peak<=dbToGain(-12)+1e-6f);CHECK(peak>.1f);
    }
    auto input=tone(48000,440,48000,.7f);
    auto a=renderFx(FxType::StereoPrecisionLimiter,input,256,48000,[](FxSlotParams& p){normalized(p);p.p4.set(0);});
    auto b=renderFx(FxType::StereoPrecisionLimiter,input,256,48000,[](FxSlotParams& p){normalized(p);p.p4.set(1);});
    CHECK(maxDiff(a,b)>.0001f);
}
TEST(fxPultecFrequencyAndBandwidthControls)
{
    auto input=tone(48000,6000,48000,.05f);
    auto run=[&](int param,float v){return renderFx(FxType::StereoXtecEQ1,input,256,48000,[&](FxSlotParams& p){normalized(p);p.p4.set(.8f);p.parameter(param).set(v);});};
    CHECK(maxDiff(run(8,0),run(8,1))>.005f);
    CHECK(maxDiff(run(7,0),run(7,1))>.005f);
}
TEST(fxPhaserBaseAndMoodModeControls)
{
    auto input=tone(24000,440,48000);
    for(auto type:{FxType::StereoPhaser,FxType::MoodFilter}) {
        auto run=[&](float v){return renderFx(type,input,256,48000,[&](FxSlotParams& p){normalized(p);p.parameter(type==FxType::StereoPhaser?3:4).set(v);});};
        CHECK(maxDiff(run(0),run(1))>.001f);
    }
}
TEST(fxExciterZeroMixIsDry)
{
    PremiumTubeEqDsp dsp;dsp.prepare(48000);auto input=tone(48000,7000,48000);auto l=input,r=input;
    dsp.process(l.data(),r.data(),48000,FxType::StereoExciter,3000,50,50,0,80,0,1);
    CHECK_NEAR(maxDiff(l,input),0,1e-7);
}
TEST(fxDeEsserPreservesLowFrequency)
{
    PremiumDynamicsDsp dsp;dsp.prepare(48000);auto input=tone(48000,100,48000,.5f);auto l=input,r=input;
    dsp.process(l.data(),r.data(),48000,FxType::StereoDeEsser,80,80,0,80,1,80,0);
    CHECK(std::abs(20*std::log10(rms(l,24000)/rms(input,24000)))<.5f);
}
TEST(fxDualPitchFullyWetHasNoImmediateDry)
{
    PitchShifterDsp dsp;dsp.prepare(48000);Buffer l(256),r(256);l[0]=r[0]=1;
    dsp.processDual(l.data(),r.data(),256,0,0,50,100,0,0,0,50,100,100,1);
    CHECK_NEAR(l[0],0,1e-7);CHECK_NEAR(r[0],0,1e-7);
}
TEST(fxDualBanksAreIndependent)
{
    for(auto type:{FxType::DualXtecEQ1,FxType::DualLeisureCompressor,FxType::DualUltimoCompressor,FxType::DualTubeStage}) {
        auto input=tone(8192,440,48000);
        auto run=[&](float gain, bool inPlace){
            FxSlotProcessor dsp;dsp.prepare(48000);FxSlotParams p;normalized(p);p.fxType.set((float)type);
            p.rightParametersEnabled.setBool(true);
            for(int i=0;i<18;i++)p.rightParameters[i].set(.5f);
            p.rightParameters[0].set(gain);
            Buffer l=input,r=input;
            if(inPlace)dsp.process(l.data(),r.data(),l.data(),r.data(),(int)l.size(),p);
            else dsp.process(input.data(),input.data(),l.data(),r.data(),(int)l.size(),p);
            return std::make_pair(l,r);
        };
        auto a=run(.2f,false), b=run(.8f,false), c=run(.8f,true);
        CHECK_NEAR(maxDiff(a.first,b.first),0,1e-6);
        CHECK(maxDiff(a.second,b.second)>.001f);
        CHECK_NEAR(maxDiff(b.first,c.first),0,1e-6);CHECK_NEAR(maxDiff(b.second,c.second),0,1e-6);
    }
}
TEST(fxTubeStageEvenControlProducesSecondHarmonic)
{
    auto magnitude=[](const Buffer& x,int harmonic){double real=0,imag=0;for(int i=24000;i<48000;i++){double phase=2.0*kPi*1000*harmonic*i/48000;real+=x[i]*std::cos(phase);imag+=x[i]*std::sin(phase);}return std::hypot(real,imag)/24000;};
    PremiumUtilityAmpDsp dsp;dsp.prepare(48000);auto l=tone(48000,1000,48000,.2f),r=l;
    dsp.process(l.data(),r.data(),48000,FxType::StereoTubeStage,0,100,0,0,1,0,0);
    CHECK(magnitude(l,2)>.001f);
}
TEST(fxPhysicalZeroSettingsAreNotNormalized)
{
    auto input=tone(4096,440,48000);
    auto output=renderFx(FxType::WaveDesigner,input,256,48000);
    CHECK_NEAR(maxDiff(input,output),0,.001f);
}
TEST(fxAllModelsAreFiniteAtControlEndpoints)
{
    for(float sr:{44100.f,48000.f,96000.f})for(int type=0;type<getNumFxTypes();type++)for(float value:{0.f,.5f,1.f}) {
        auto input=tone(4096,1370,sr,.35f);
        auto out=renderFx((FxType)type,input,256,sr,[&](FxSlotParams& p){normalized(p);for(int i=0;i<18;i++)p.parameter(i).set(value);});
        bool finite=true;for(float x:out)finite &= std::isfinite(x);
        CHECK(finite);
    }
}
TEST(fxUltimoTwentyIsNotAllButtons)
{
    auto run=[](float ratio){PremiumDynamicsDsp dsp;dsp.prepare(48000);auto l=tone(8192,440,48000,.6f),r=l;dsp.process(l.data(),r.data(),8192,FxType::UltimoCompressor,40,20,ratio,80,1,50,0);return l;};
    CHECK(maxDiff(run(20),run(24))>.001f);
}
TEST(fxChangingAlgorithmClearsPreviousTail)
{
    FxSlotProcessor dsp;dsp.prepare(48000);FxSlotParams p;p.setDefaultsForType(FxType::StereoDelay);
    p.p1.set(20);p.p2.set(20);p.p3.set(90);Buffer l(4096),r(4096),out(4096),other(4096);l[0]=r[0]=1;
    dsp.process(l.data(),r.data(),out.data(),other.data(),4096,p);
    p.setDefaultsForType(FxType::TapDelay);std::fill(l.begin(),l.end(),0);std::fill(r.begin(),r.end(),0);
    dsp.process(l.data(),r.data(),out.data(),other.data(),4096,p);
    CHECK_NEAR(rms(out,0),0,1e-8);
}

