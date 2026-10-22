#define SDL_MAIN_HANDLED
#include "halo2/pc_mouse_look.hpp"
#include "halo2/gameplay_fov.hpp"
#include <SDL.h>
#include <bit>
#include <iostream>
#include <limits>

using namespace halo2;
namespace {
unsigned checks{};
void check(bool value,const char *reason) {
  ++checks;if(!value)throw std::runtime_error(reason);
}
void motion(PcMouseLook &look,int x,int y,const PcKeyboardSettings &settings,float &yaw,float &pitch) {
  SDL_Event event{};event.type=SDL_MOUSEMOTION;event.motion.xrel=x;event.motion.yrel=y;
  check(SDL_PushEvent(&event)==1,"Mouse event was not queued");
  while(SDL_PollEvent(&event))if(event.type==SDL_MOUSEMOTION)
    look.motion(event.motion.xrel,event.motion.yrel,settings,yaw,pitch);
}
}
int main()try {
  SDL_SetMainReady();check(SDL_Init(SDL_INIT_EVENTS)==0,"SDL event initialization failed");
  for(float baseline:{.0025f,.003f})for(int sensitivity:{1,3,10})for(bool invert:{false,true}) {
    PcKeyboardSettings settings;settings.sensitivity=sensitivity;settings.invert=invert;
    PcMouseLook look(baseline);
    float yaw=.75f,pitch=-.25f;
    motion(look,27,-13,settings,yaw,pitch);
    const float gain=baseline*float(sensitivity)/3.f;
    check(std::bit_cast<unsigned>(yaw)==std::bit_cast<unsigned>(.75f-27*gain) &&
          std::bit_cast<unsigned>(pitch)==std::bit_cast<unsigned>(-.25f+13*gain*(invert?-1.f:1.f)),
          "Unzoomed mouse sensitivity or inversion changed");
    for(unsigned fov:{70u,78u,90u,120u})for(float aspect:{2.f/3.f,4.f/3.f,16.f/9.f,32.f/9.f}) {
      const float horizontal=gameplay_field_of_view(fov);
      const float wide=gameplay_camera_angles(horizontal,horizontal,aspect).projection_angle;
      // Cover a range of narrow and intermediate rendered lenses. Exact
      // authored optics are exercised by weapon_scope_parity_tests.
      // These are declared test lenses, not replacement authored weapon data.
      for(float magnification:{1.f,1.125f,1.5f,2.f,3.f,5.f,7.f,10.f}) {
        const float scoped=float(2*std::atan(std::tan(double(horizontal)*.5)/magnification));
        const float narrow=gameplay_camera_angles(scoped,horizontal,aspect).projection_angle;
        look.projection(narrow,wide);yaw=pitch=0;
        motion(look,1,-1,settings,yaw,pitch);
        // Independent projected-motion check: a small angular displacement
        // occupies angle/tan(vertical_FOV/2) screen units on either axis.
        const double actual=-double(yaw)/std::tan(double(narrow)*.5);
        const double expected=double(gain)/std::tan(double(wide)*.5);
        check(std::abs(actual-expected)<std::abs(expected)*3e-6,
              "Scoped mouse movement accelerates on screen instead of preserving fine control");
        check(pitch==(-yaw)*(invert?-1.f:1.f),"Scope applies different gains to yaw and pitch");
        float batched_yaw{},batched_pitch{},single_yaw{},single_pitch{};
        motion(look,80,-40,settings,batched_yaw,batched_pitch);
        for(unsigned n=0;n<40;++n)motion(look,2,-1,settings,single_yaw,single_pitch);
        check(std::abs(batched_yaw-single_yaw)<1e-6f && std::abs(batched_pitch-single_pitch)<1e-6f,
              "Mouse packet rate adds acceleration or drops fine motion");
        motion(look,0,0,settings,yaw,pitch);
        check(yaw!=0,"Fine scoped mouse counts were lost");
      }
      look.projection(wide*.5f,wide);look.reset();yaw=pitch=0;
      motion(look,1,1,settings,yaw,pitch);
      check(yaw==-gain,"Leaving the scoped camera retained its mouse gain");
    }
  }
  PcMouseLook local(.0025f),other(.0025f);
  local.projection(.1f,1.f);
  PcKeyboardSettings settings;float yaw{},pitch{};
  motion(other,1,0,settings,yaw,pitch);
  check(yaw==-.0025f,"Scoped mouse gain leaked between local views");
  for(float bad:{0.f,-1.f,3.2f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}) {
    bool rejected{};try{local.projection(bad,1.f);}catch(const std::invalid_argument &){rejected=true;}
    check(rejected,"Invalid rendered lens admitted a mouse gain");
  }
  SDL_Quit();std::cout<<checks<<" SDL mouse checks passed: hip-fire, scoped screen motion, FOV/aspect, fine counts, packet rate, inversion and view isolation\n";
}catch(const std::exception &error){SDL_Quit();std::cerr<<error.what()<<'\n';return 1;}
