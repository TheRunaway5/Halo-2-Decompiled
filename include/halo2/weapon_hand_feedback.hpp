#pragma once
#include <array>
#include <cstdint>
namespace halo2 {
// Unit+1FA/+1FB. Literal EFCC0 and D2940 arithmetic authenticated in both
// TU1.1 and TU1.5; no first-person registration or unit world is fabricated.
unsigned weapon_hand_feedback_action_id(std::uint32_t activity);
void weapon_hand_feedback_tick(std::array<std::uint8_t,2> &);
void weapon_hand_feedback_action(std::array<std::uint8_t,2> &,unsigned action,
    bool secondary,bool registered,std::int16_t tick_rate);
float weapon_hand_feedback_blend(const std::array<std::uint8_t,2> &,
    bool has_unit,bool has_weapon,bool primary,float dt);
}
