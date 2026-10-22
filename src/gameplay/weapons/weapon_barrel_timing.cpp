#include "halo2/weapon_barrel_timing.hpp"
#include <bit>
#include <cmath>
#include <limits>

namespace halo2 {
namespace {
std::uint32_t floor_word(double value) {
  // Original __floor followed by _ftol returns the low DWORD of a signed
  // 64-bit conversion. FISTP's indefinite integer has a zero low DWORD.
  const double rounded=std::floor(value);
  if(!(rounded>=-9223372036854775808.0 && rounded<9223372036854775808.0))return 0;
  return std::uint32_t(std::uint64_t(std::int64_t(rounded)));
}
std::int16_t short_word(std::uint32_t value) {
  return std::bit_cast<std::int16_t>(std::uint16_t(value));
}
}
WeaponBarrelGate weapon_barrel_gate(SmgBarrelState &s,
    const WeaponBarrelTimingDefinition &d,const WeaponBarrelTimingContext &context,
    WeaponBarrelTimingConstants constants) {
  const float rate=(d.rate_upper-d.rate_lower)*s.rate+d.rate_lower;
  float interval=rate>constants.minimum_rate?float(context.tick_rate)/rate:0.f;
  if(d.spent_rate_scale>0.f) {
    const float scale=context.spent*d.spent_rate_scale+constants.one;
    interval=scale*interval;
  }
  bool admitted=false;
  const bool blocked=context.alternate_mode &&
      (((d.flags&(1u<<9)) && *context.alternate_mode==1) ||
       ((d.flags&(1u<<10)) && (*context.alternate_mode==2 || *context.alternate_mode==3)));
  if(!blocked)
    admitted=!s.shot_count || float(s.counter)+constants.one>=interval;
  WeaponBarrelGate result{admitted,false,false};
  if(d.burst_minimum && int(s.shot_count)<d.burst_minimum) {
    result.minimum_met=false;
  } else if(d.burst_maximum && int(s.shot_count)>=d.burst_maximum) {
    result.admitted=false;result.maximum_met=true;
  } else result.minimum_met=true;
  if(s.flags&0x40u) {result.admitted=false;result.maximum_met=true;return result;}
  if(!result.admitted)return result;
  if(context.other_hand && context.other_hand->mode==1 && context.other_hand->counter==0) {
    result.admitted=false;return result;
  }
  if(constants.one>interval && interval>constants.minimum_rate)
    s.subshot=constants.one/interval+s.subshot;
  return result;
}
void weapon_barrel_begin_recovery(SmgBarrelState &s,
    const WeaponBarrelTimingDefinition &d,std::int16_t tick_rate,
    WeaponBarrelTimingConstants constants) {
  const float total=float(tick_rate)*d.recovery_seconds;
  const float first=(constants.one-d.recovery_fraction)*total;
  std::uint32_t ticks=floor_word(first);
  const double whole=std::floor(double(total));
  const float remainder=float(double(total)-whole);
  s.recovery=s.recovery+remainder;
  if(s.recovery>=constants.one) {
    const double extra=std::floor(double(s.recovery));
    s.recovery=float(double(s.recovery)-extra);
    ticks+=floor_word(extra);
  }
  s.ticks=short_word(ticks);s.mode=2;
}
void weapon_barrel_reset(SmgBarrelState &s,const WeaponBarrelTimingDefinition &d) {
  s.counter=0;s.shot_count=0;s.mode=0;
  if(!(d.flags&(1u<<11)))s.flags&=~1u;
}
void weapon_barrel_finish_recovery(SmgBarrelState &s,
    const WeaponBarrelTimingDefinition &d,std::int16_t tick_rate,
    WeaponBarrelTimingConstants constants) {
  const double total=double(tick_rate)*double(d.recovery_seconds);
  const double first=(double(constants.one)-double(d.recovery_fraction))*double(tick_rate)*double(d.recovery_seconds);
  const auto ticks=floor_word(total)-floor_word(first);
  s.mode=3;s.ticks=short_word(ticks);
  // Original tests the DWORD subtraction, before its signed-short store.
  if(ticks==0)weapon_barrel_reset(s,d);
}
WeaponBarrelTimingStep weapon_barrel_timing_step(SmgBarrelState &s,
    const WeaponBarrelTimingDefinition &d,const WeaponBarrelTimingContext &context,
    WeaponBarrelTimingConstants constants,bool blocked) {
  if(s.ticks>0)--s.ticks;
  if(s.mode==0) {
    if(blocked)s.flags&=~1u;
    else if(s.flags&1u) {
      const bool alternate_blocked=context.alternate_mode &&
          (((d.flags&(1u<<9)) && *context.alternate_mode==1) ||
           ((d.flags&(1u<<10)) && *context.alternate_mode==3));
      if(!alternate_blocked)s.mode=1;
    }
    if(s.mode!=1)return {};
  } else if(s.mode==2) {
    if(s.ticks==0)weapon_barrel_finish_recovery(s,d,context.tick_rate,constants);
    return {};
  } else if(s.mode==3) {
    if(s.ticks==0)weapon_barrel_reset(s,d);
    return {};
  } else if(s.mode!=1)return {};
  if(blocked) {
    weapon_barrel_begin_recovery(s,d,context.tick_rate,constants);return {};
  }
  const auto gate=weapon_barrel_gate(s,d,context,constants);
  if(gate.admitted) {
    if(gate.minimum_met && !(s.flags&1u)) {
      weapon_barrel_begin_recovery(s,d,context.tick_rate,constants);return {};
    }
    s.flags&=~1u;
    return {true,true};
  }
  if(gate.maximum_met || (gate.minimum_met && !(s.flags&1u))) {
    weapon_barrel_begin_recovery(s,d,context.tick_rate,constants);return {};
  }
  s.counter=std::int8_t(std::min(127,int(s.counter)+1));
  return {false,true};
}
} // namespace halo2
