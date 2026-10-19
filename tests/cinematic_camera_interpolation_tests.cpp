#include "halo2/cinematic_camera_interpolation.hpp"
#include "halo2/campaign_frame_clock.hpp"
#include <cmath>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace halo2;
static_assert(sizeof(SourceCampaignCameraViewTu15)==48);
static_assert(offsetof(CinematicCameraFrame,completed)==40 && sizeof(CinematicCameraFrame)==88);
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
bool same(const SourceCampaignCameraViewTu15&a,const SourceCampaignCameraViewTu15&b){return !std::memcmp(&a,&b,sizeof(a));}
SourceCampaignCameraViewTu15 pose(unsigned tick){const float a=float(tick)*.01f;
 return {{float(tick)*3,0,0},{std::cos(a),std::sin(a),0},{0,0,1},1+float(tick)*.001f,4.f/3,0};}
int main(int argc,char**)try {
 const bool held=argc>1;unsigned subticks{},moving{};
 for(unsigned hz:{30,60,90,120,144,165,240}) {
  CampaignFrameClock clock;CinematicCameraInterpolation camera(held);std::uint64_t tick{};
  auto previous=pose(0);std::uint64_t last_tick{};float last_fraction{};
  for(unsigned n=0;n<hz*3;++n) {
   const auto time=clock.advance(1.f/float(hz),true);tick+=time.ticks;
   const auto source=pose(unsigned(tick)),unchanged=source;
   const auto output=camera.sample({tick,1,0,0,true,time.fraction,source});
   check(same(source,unchanged),"Camera interpolation mutated completed source values");
   if(n>hz && tick==last_tick && time.fraction>last_fraction) {
    ++subticks;if(!same(previous,output))++moving;
    check(!same(previous,output),"Completed camera remains tick-stepped on zero-tick frames");
   }
   previous=output;last_tick=tick;last_fraction=time.fraction;
  }
 }
 check(subticks>100&&moving==subticks,"Camera frequency cases lost moving subframes");
 CinematicCameraInterpolation camera;auto a=pose(10),b=pose(11);
 (void)camera.sample({10,1,0,0,true,0,a});
 (void)camera.sample({10,1,0,0,true,.5f,a});
 check(same(camera.sample({11,1,0,0,true,0,b}),a),"Camera fraction zero changed its original endpoint");
 check(same(camera.sample({11,1,0,0,true,1,b}),b),"Camera fraction one changed its original endpoint");
 auto cut=pose(900);
 check(same(camera.sample({12,2,0,0,true,.001f,cut}),cut),"Authored camera cut blended departed shot");
 check(same(camera.sample({12,2,1,0,true,.5f,a}),a),"Checkpoint camera borrowed discarded timeline");
 check(same(camera.sample({12,2,1,1,true,.5f,b}),b),"BSP camera borrowed prior space");
 check(same(camera.sample({18,2,1,1,true,.1f,cut}),cut),"Skipped ticks interpolated over missing camera history");
 check(same(camera.sample({19,2,1,1,false,.1f,b}),b),"Unpaired camera interpolated invalid history");
 camera.clear();check(same(camera.sample({19,2,1,1,true,.2f,a}),a),"Camera clear retained departed history");
 for(float f:{-.1f,1.1f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}) {
  bool rejected=false;try{(void)camera.sample({20,2,1,1,true,f,b});}catch(const std::runtime_error&){rejected=true;}
  check(rejected,"Invalid camera render fraction was admitted");
 }
 camera.clear();a=pose(0);b=a;b.up={0,0,-1};
 (void)camera.sample({0,1,0,0,true,0,a});(void)camera.sample({0,1,0,0,true,.5f,a});
 const auto roll=camera.sample({1,1,0,0,true,.5f,b});
 check(std::abs(length(roll.forward)-1)<1e-5f&&std::abs(length(roll.up)-1)<1e-5f&&
       std::abs(dot(roll.forward,roll.up))<1e-5f&&std::abs(roll.up.y)>.99f,"Camera roll collapsed or lost orthogonal orientation");
 camera.clear();a=pose(10);b=pose(11);
 (void)camera.sample({10,1,0,0,true,0,a});(void)camera.sample({10,1,0,0,true,.5f,a});
 const auto paused=camera.sample({11,1,0,0,true,.75f,b});
 check(same(camera.sample({11,1,0,0,true,0,b}),paused),"Paused fractional clock rewound the camera");
 camera.clear();
 (void)camera.sample({0,1,0,0,true,0,pose(0)});
 check(same(camera.sample({0,1,0,0,true,.5f,pose(1)}),pose(1)),"Native per-frame camera was changed during initialization");
 check(same(camera.sample({1,1,0,0,true,0,pose(2)}),pose(2)),"Native per-frame camera acquired interpolation delay");
 check(same(camera.sample({1,1,0,0,true,.5f,pose(3)}),pose(3)),"Native per-frame camera was interpolated twice");
 camera.clear();
 (void)camera.sample({0,1,0,0,true,0,pose(0)});
 (void)camera.sample({0,1,0,0,true,.5f,pose(0)});
 (void)camera.sample({1,1,0,0,true,.2f,pose(1)});
 check(same(camera.sample({1,1,0,0,true,.6f,pose(2)}),pose(2)),"New per-frame camera update was delayed until the next tick");
 std::cout<<"PASS "<<moving<<" moving zero-tick frames at30/60/90/120/144/165/240Hz; exact endpoints, command cuts, restores, BSP, skipped ticks, invalid fractions and roll\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
