#include <cstring>
#include <cstdio>
#include "EffectsDsp.h"
using namespace mps;
std::vector<float> render(FxType t,int block=256,int changed=-1,float value=.5f,bool impulse=false){
 FxSlotParams p; p.setDefaultsForType(t); for(int j=0;j<18;j++)p.parameter(j).set(.5f); p.mix.set(1); p.parameterEncoding.set(2); if(changed>=0)p.parameter(changed).set(value);
 FxSlotProcessor f; f.prepare(48000); std::vector<float> o(96000),r(block),l(block),a(block),b(block);
 for(int k=0;k<96000;k+=block){int n=std::min(block,96000-k);for(int i=0;i<n;i++){l[i]=impulse?(k+i==0?.5f:0.f):.2f*std::sin(2*kPi*440*(k+i)/48000.f);r[i]=l[i];}f.process(l.data(),r.data(),a.data(),b.data(),n,p);std::copy_n(a.data(),n,o.data()+k);}return o;
}
float diff(const std::vector<float>&a,const std::vector<float>&b){float d=0;for(int i=0;i<a.size();i++)d=std::max(d,std::abs(a[i]-b[i]));return d;}
int main(){
 printf("Hall/Room identical settings max difference: %.9g\n",diff(render(FxType::HallReverb),render(FxType::RoomReverb)));
 printf("Hall/Chamber max difference: %.9g\n",diff(render(FxType::HallReverb),render(FxType::ChamberReverb)));
 auto sm=render(FxType::SoundMaxer);float d=0;for(int i=0;i<sm.size();i++)d=std::max(d,std::abs(sm[i]-.2f*std::sin(2*kPi*440*i/48000.f)));printf("SoundMaxer versus dry max difference: %.9g\n",d);
 printf("Hall impulse block64 versus block1024 max difference: %.9g\n",diff(render(FxType::HallReverb,64,-1,.5,true),render(FxType::HallReverb,1024,-1,.5,true)));
 printf("Phaser BASE UI p4 min/max max difference: %.9g\n",diff(render(FxType::StereoPhaser,256,3,0),render(FxType::StereoPhaser,256,3,1)));
 printf("Limiter RELEASE UI p5 min/max max difference: %.9g\n",diff(render(FxType::StereoPrecisionLimiter,256,4,0),render(FxType::StereoPrecisionLimiter,256,4,1)));
 for(int t=0;t<61;t++){auto x=render((FxType)t);bool finite=true;float pk=0;for(float v:x){finite &= std::isfinite(v);pk=std::max(pk,std::abs(v));}printf("MODEL %02d %s finite=%d peak=%.6g\n",t,getFxMetadata((FxType)t).name,finite,pk);}
 PremiumUtilityAmpDsp sub;sub.prepare(48000);std::vector<float> l(96000),r(96000);for(int i=0;i<96000;i++)l[i]=r[i]=.5f*std::sin(2*kPi*200*i/48000.f);sub.process(l.data(),r.data(),96000,FxType::Suboctaver,0,100,0,2,1);int crossings=0;for(int i=48001;i<96000;i++)if(l[i-1]<0&&l[i]>=0)crossings++;printf("Suboctaver -1 positive crossings in last 1s, 200Hz input: %d (expected 100)\n",crossings);
}
