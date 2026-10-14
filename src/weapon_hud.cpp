#include "halo2/weapon_hud.hpp"
#include <bit>
#include <cmath>
namespace halo2 {
void weapon_hud_project(SourceWeaponHudState &out,const WeaponHudProjectionInput &in) {
  out.datum=in.datum;out.present=1;out.age=in.age;out.heat=in.heat;
  out.charge=float(in.charge)*std::bit_cast<float>(0x3b808081u); //465024/46A0C4
  out.overheated=(in.weapon_flags&2u)!=0;out.input_held=(in.input_flags&2u)!=0;
  out.loaded=in.magazine_present?in.loaded:0;out.capacity=in.magazine_present?in.capacity:0;
  out.reserve=in.magazine_present?std::bit_cast<std::int16_t>(std::uint16_t(in.shared_reserve)):0;
  out.maximum_reserve=in.magazine_present?in.maximum_reserve:0;
  out.magazine_working=in.magazine_present && in.magazine_mode>0 && in.magazine_mode<=6;
  out.charging=in.trigger_present && in.trigger_state==2 && in.trigger_behavior==2 && in.charging_type==1;
  out.target=out.locking=out.locked=0;out.lock_progress=0;out.target_position={};
  if(in.trigger_present && in.trigger_behavior==5 && in.loaded>0) {
    if(in.trigger_state==0)out.target=in.acquire_target;
    else if(in.trigger_state==5 || in.trigger_state==6) {
      if(in.target_position)out.target_position=*in.target_position;
      if(in.trigger_state==5) {
        const float rounded=std::nearbyint(float(in.tick_rate)*.5f); //46502C/46A0CC
        const auto ticks=rounded>=-2147483648.f && rounded<2147483648.f?
            std::int32_t(rounded):std::int32_t(-2147483647-1);
        out.locking=1;out.lock_progress=1.f-float(in.trigger_ticks)/float(ticks);
      } else out.locked=1;
    }
  }
}
float weapon_hud_input(const SourceWeaponHudState *item,unsigned selector,float elapsed) {
  if(!item)return 0;
  switch(selector) {
    case 48:return float(item->loaded);case 49:return item->heat;
    case 50:return 100.f-item->age*100.f;case 51:return float(item->reserve);
    case 52:return item->charge;case 53:return item->overheated?1.f:0.f;
    case 54:return item->capacity?float(item->loaded)/float(item->capacity):0.f;
    case 55:return item->overheated?elapsed:0.f;case 56:return 1.f-item->age;
    case 57:return item->lock_progress;default:throw FormatError("Unknown original weapon HUD selector");
  }
}
std::uint16_t weapon_hud_mask(const SourceWeaponHudState *item,unsigned type) {
  std::uint16_t result=type==1?1:type==2?2:type==3?4:0;
  if(!item)return result;
  if(item->overheated)result|=0x40;
  if((item->capacity && !item->loaded && item->maximum_reserve && !item->reserve) || item->age>=1.f)result|=0x80;
  if(item->target)result|=0x100;
  if(item->locking)result|=0x200;
  if(item->locked)result|=0x400;
  return result;
}
std::uint16_t weapon_hud_cutoffs(const SourceWeaponHudState *item,
    std::array<unsigned,3> thresholds,std::uint16_t mask) {
  if(!item)return mask;
  mask&=~0x38u;
  if(float(std::uint8_t(thresholds[0]))>item->age)mask|=8;
  if(item->loaded<int(std::uint8_t(thresholds[1])))mask|=0x10;
  if(item->reserve<int(std::uint8_t(thresholds[2])))mask|=0x20;
  return mask;
}
}
