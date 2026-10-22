#include "halo2/cinematic_crossfade.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
void CinematicCrossfadeState::screen_start(bool reset) {
  if(reset || !initialized){capture=false;alpha=0;output_strength=0;}
  initialized=true;active=true;
}
void CinematicCrossfadeState::screen_stop(){*this={};}
void CinematicCrossfadeState::set(float seconds,double clock) {
  if(!std::isfinite(seconds) || !std::isfinite(clock))
    throw FormatError("Cinematic crossfade requires finite original timer inputs");
  pending=true;strength=1.f;start=float(clock);end=float(clock+double(seconds));
}
void CinematicCrossfadeState::update(double clock) {
  if(!std::isfinite(clock))throw FormatError("Cinematic crossfade clock is nonfinite");
  if(!active)return;
  alpha=0;
  if(end!=start) {
    // Original FLD/FSub/FDIV retains the subtraction in the53-bit x87
    // temporary; rounding end-start to binary32 changes the last fade frame.
    const double duration=double(end)-double(start);
    const float fraction=float(std::clamp((clock-double(start))/duration,0.,1.));
    alpha=std::clamp(1.f-fraction,0.f,1.f);
    if(alpha>0 && pending){capture=true;pending=false;}
  }
  output_strength=strength;
}
std::optional<PixelProgram> cinematic_crossfade_program(float alpha,float strength) {
  if(!std::isfinite(alpha) || !std::isfinite(strength))
    throw FormatError("Cinematic crossfade requires finite original composite inputs");
  if(alpha<=0)return {};
  const double result=std::pow(double(std::min(alpha,1.f)),double(std::max(strength,0.f)));
  if(!(result>0))return {};
  const float effective=result>1.?1.f:float(result);
  if(effective<=0)return {};
  PixelProgram program;
  program.words[8]=0x08110000;program.words[9]=0x1100;
  program.words[43]=std::uint32_t(std::nearbyint(double(effective)*255.))<<24;
  program.words[53]=0x11001;program.words[54]=1;
  return program;
}
CinematicCrossfadeFrame CinematicCrossfadeState::frame()const {
  return {capture && alpha>0,cinematic_crossfade_program(alpha,output_strength)};
}
}
