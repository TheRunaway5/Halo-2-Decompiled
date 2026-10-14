#include "halo2/weapon_hand_feedback.hpp"
#include "halo2/simulation.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include <bit>
#include <cmath>
namespace halo2 {
unsigned weapon_hand_feedback_action_id(std::uint32_t activity) {
  switch(activity) {
    case 0x06000006u:return 1;case 0x06000007u:return 2;
    case 0x0900006au:return 3;case 0x0900006bu:return 4;default:return 0;
  }
}
void weapon_hand_feedback_tick(std::array<std::uint8_t,2> &timers) {
  for(auto &timer:timers)if(timer)--timer;
}
void weapon_hand_feedback_action(std::array<std::uint8_t,2> &timers,unsigned action,
    bool secondary,bool registered,std::int16_t tick_rate) {
  if(!registered || action<1 || action>4)return;
  //EFCC0: TU1.1 452000 / TU1.5 456918, FMULSS then nearest FISTP32.
  const float duration=std::bit_cast<float>(0x3e99999au);
  const float rounded=std::nearbyint(float(tick_rate)*duration);
  const auto value=rounded>=-2147483648.f && rounded<2147483648.f?
      std::int32_t(rounded):std::int32_t(-2147483647-1);
  timers[secondary]=std::uint8_t(std::uint32_t(value));
}
float weapon_hand_feedback_blend(const std::array<std::uint8_t,2> &timers,
    bool has_unit,bool has_weapon,bool primary,float dt) {
  if(!has_unit || !has_weapon)return 1.f;
  const auto timer=timers[primary?1:0];if(!timer)return 0.f;
  //D2940: TU1.1 4653C4 / TU1.5 46A464, with two float multiplies.
  const float elapsed=float(timer)*dt;
  const float scaled=elapsed*std::bit_cast<float>(0x40555555u);
  if(scaled<0.f)return 0.f;
  if(scaled>1.f)return 1.f;
  return scaled;
}

float Simulation::weapon_hand_blend(const Unit &unit,const WeaponState &weapon) const {
  const auto *primary=native_hand_weapon(unit,0);
  return weapon_hand_feedback_blend(unit.source_weapon_hand_feedback,
      unit.source_datum!=null_datum,weapon.source_datum!=null_datum,
      primary && primary->source_datum==weapon.source_datum,tick_seconds);
}
void Simulation::weapon_activity(Unit &unit,const WeaponState &weapon,std::uint32_t activity) {
  const auto action=weapon_hand_feedback_action_id(activity);
  if(multiplayer_weapon_steps_ && !source_physics_movement_ && action) {
    const auto *primary=native_hand_weapon(unit,0);
    weapon_hand_feedback_action(unit.source_weapon_hand_feedback,action,
        !primary || primary->source_datum!=weapon.source_datum,
        weapon.smg.hand_timer_registered && unit.source_player!=null_datum,30);
    //Retain the old observable SMG packet as a view of the actual unit owner.
    for(unsigned hand=0;hand<2;++hand)if(auto *state=native_hand_weapon(unit,hand))
      for(unsigned timer=0;timer<2;++timer)state->smg.hand_timers[timer]=unit.source_weapon_hand_feedback[timer];
  }
  if(first_person_activity_hook_)first_person_activity_hook_(unit,weapon,activity);
}
}
