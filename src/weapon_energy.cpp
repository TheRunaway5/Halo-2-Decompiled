#include "halo2/weapon_energy.hpp"
#include "halo2/fp_registration.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2 {
std::int16_t weapon_animation_activity_timing(const FirstPersonBindingDefinition &definition,
    bool dual,std::uint32_t activity,std::int16_t section,
    std::int16_t tick_rate,float animation_seconds) {
  if(!definition.record_present || definition.graph==0xffffffffu)return 0;
  FirstPersonHandBinding hand;hand.dual_wield=dual;
  for(const auto &clip:first_person_clips(hand,definition))if(clip.activity==activity) {
    std::optional<std::int16_t> event4,event0;
    for(const auto &event:clip.events)if(event.type==0) {
      if(event.id==4 && !event4)event4=event.frame;
      if(event.id==0 && !event0)event0=event.frame;
    }
    return weapon_animation_timing(clip.frames,event4,event0,section,tick_rate,animation_seconds);
  }
  return 0;
}
void weapon_heat_step(WeaponEnergyState &s,const WeaponHeatDefinition &d,
    float dt,float one,const WeaponHeatServices &services) {
  if(s.heat>0.f) {
    if(s.heat>=d.maximum && !(s.weapon_flags&2u)) {
      s.weapon_flags|=2u;
      if(services.has_unit && services.reset_zoom)services.reset_zoom();
      if(d.weapon_type==3 && (s.weapon_flags&8u))s.weapon_flags&=~8u;
      else if(services.action)services.action(14);
    }
    if(s.cooling_inhibitor==0.f) {
      const bool overheated=(s.weapon_flags&2u)!=0;
      float loss=dt*((overheated && d.overheated_loss>0.f)?d.overheated_loss:d.loss);
      if(d.spent_loss_scale>0.f)loss=(one-s.spent*d.spent_loss_scale)*loss;
      s.heat=s.heat-loss;
      if(s.heat<0.f)s.heat=0.f;
      if(overheated && !(s.weapon_flags&4u)) {
        const auto ticks=std::max<std::int16_t>(1,services.vent_timing_ticks?services.vent_timing_ticks():0);
        if((s.heat-d.cooled_threshold)/loss<=float(ticks)) {
          s.weapon_flags|=4u;
          if(services.action)services.action(15);
        }
      }
    }
    if((s.weapon_flags&2u) && s.heat<d.cooled_threshold)s.weapon_flags&=~6u;
  }
  if(d.overheat_effect!=0xffffffffu) {
    const bool holstered=(s.item_flags&2u)!=0;
    const bool active=(s.weapon_flags&2u) && !holstered;
    if(active!=(s.overheat_handle!=0xffffffffu)) {
      if(active) {
        s.overheat_handle=services.create_overheat?services.create_overheat(d.overheat_effect):0xffffffffu;
      } else {
        if(services.stop_overheat)services.stop_overheat(s.overheat_handle,holstered);
        s.overheat_handle=0xffffffffu;
      }
    }
  }
  s.cooling_inhibitor=0.f;
  if(s.blocking_ticks>0)--s.blocking_ticks;
}
bool weapon_age(WeaponEnergyState &s,float amount,float one,bool infinite_ammo,
    const WeaponAgeDefinition &d,const std::function<void(std::uint32_t)> &effect) {
  if(!(s.item_flags&8u) || !(s.spent<one) || infinite_ammo)return false;
  s.spent=amount+s.spent;
  if(!(s.spent>=one))return false;
  s.spent=one;
  if(effect)effect(d.exhausted_effect);
  if(d.aged_definition!=0xffffffffu)s.definition=d.aged_definition;
  return true;
}
std::int16_t weapon_vent_timing(std::int16_t frames,std::optional<std::int16_t> event4,
    std::int16_t tick_rate,float animation_seconds) {
  return weapon_animation_timing(frames,event4,{},1,tick_rate,animation_seconds);
}
std::int16_t weapon_animation_timing(std::int16_t frames,
    std::optional<std::int16_t> event4,std::optional<std::int16_t> event0,
    std::int16_t section,std::int16_t tick_rate,float animation_seconds) {
  int selected=frames;
  if(section==1 && event4 && *event4!=-1 && frames>*event4)selected=*event4;
  else if(section==2)selected=event0.value_or(-1);
  else if(section!=0 && section!=1 && event0 && *event0>0)selected=*event0;
  if(selected==-1)return -1;
  if(selected<=0)return 0;
  const float seconds=float(selected)*animation_seconds;
  const float rounded=std::nearbyint(float(tick_rate)*seconds);
  const auto converted=rounded>=-2147483648.f && rounded<2147483648.f
      ?std::int32_t(rounded):std::int32_t(-2147483647-1);
  return std::max<std::int16_t>(1,std::bit_cast<std::int16_t>(std::uint16_t(converted)));
}
bool weapon_battery_misfire(float spent,float threshold,float chance,float one,
    float random_scale,std::uint32_t &seed) {
  if(!(threshold>0.f && threshold<one && spent>threshold))return false;
  seed=seed*1664525u+1013904223u;
  // Original x87 uses PC53 for each operation, without intermediate float stores.
  const double probability=((double(spent)-double(threshold))*double(chance))/(double(one)-double(threshold));
  const double sample=double(seed>>16)*double(random_scale);
  return probability>sample;
}
bool weapon_heat_detonation(float heat,float threshold,float chance,float random_scale,std::uint32_t &seed) {
  if(!(heat>threshold))return false;
  seed=seed*1664525u+1013904223u;
  return double(chance)>double(seed>>16)*double(random_scale);
}
} // namespace halo2
