#include "halo2/biped_landing.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace halo2 {
namespace {
float sub(float a,float b){volatile float r=a-b;return r;}
float mul(float a,float b){volatile float r=a*b;return r;}
float div(float a,float b){volatile float r=a/b;return r;}
}
void begin_source_landing(SourceLandingState &s,const SourceLandingDefinition &d,
                          float speed,int rate,bool &force_hard) {
  if(!std::isfinite(speed)||rate<=0||rate>32767 ||
      !std::isfinite(d.soft_seconds)||!std::isfinite(d.hard_seconds)||
      !std::isfinite(d.soft_speed)||!std::isfinite(d.hard_speed)||!std::isfinite(d.maximum_speed))
    throw std::runtime_error("Invalid source landing operands");
  float interval{},seconds{};std::int16_t kind{};
  if(!force_hard) {
    if(speed<d.soft_speed)return;
    if(speed<d.hard_speed) {
      interval=sub(d.hard_speed,d.soft_speed);seconds=d.soft_seconds;
      speed=sub(speed,d.soft_speed);kind=0;
    }else {
      interval=sub(d.maximum_speed,d.hard_speed);seconds=d.hard_seconds;kind=1;
    }
  }else {
    seconds=d.hard_seconds;force_hard=false;speed=d.maximum_speed;
    interval=sub(d.maximum_speed,d.hard_speed);kind=1;
  }
  if(interval>0) {
    const float fraction=std::clamp(div(speed,interval),0.f,1.f);
    const float ticks=mul(mul(seconds,fraction),float(rate));
    if(!std::isfinite(ticks)||ticks>=2147483648.f||ticks< -2147483648.f)
      throw std::runtime_error("Source landing duration exceeds FISTP input range");
    const auto count=std::lrint(ticks);
    if(count>0) {s.kind=kind;s.elapsed=0;s.duration=std::int8_t(std::min(count,127l));}
  }
}
void advance_source_landing(SourceLandingState &s,bool moving) {
  if(s.kind==-1)return;
  s.elapsed=std::bit_cast<std::int8_t>(std::uint8_t(std::uint8_t(s.elapsed)+1));
  if(s.duration<=s.elapsed || (s.kind==0&&moving&&(s.duration>>1)<=s.elapsed))s={};
}
std::uint32_t source_landing_activity(const SourceLandingState &s,std::uint32_t ordinary) {
  return s.kind==0?0x0900001fu:s.kind==1?0x09000020u:ordinary;
}
} // namespace halo2
