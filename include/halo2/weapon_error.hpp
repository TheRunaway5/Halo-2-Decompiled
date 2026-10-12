#pragma once
#include "halo2/cache.hpp"
#include <cstdint>

namespace halo2 {
struct WeaponErrorDefinition {
  float normal_acceleration{}, normal_deceleration{};
  float dual_acceleration{}, dual_deceleration{};
  float recoil_acceleration{}, recoil_deceleration{};
  float normal_minimum{}, normal_low{}, normal_high{};
  float dual_minimum{}, dual_low{}, dual_high{};
};
struct WeaponErrorState {
  float error{}, recoil{};
};
struct WeaponErrorAngles {
  float minimum{}, maximum{};
};
// Update 1.1 102090 and 10365B..1036F3. The original barrel firing marker
// and opposite-hand blend are caller inputs; this is not a trigger machine.
// Keep separate float operations (no contraction), round-to-nearest.
void weapon_error_step(WeaponErrorState &, const WeaponErrorDefinition &,
                       float dt, float opposite_hand_blend, bool firing);
// 108F81..10905C's ordinary error branch. Authored zero dual bounds use the
// normal bounds. Forced angle / special projectile suppression are external.
WeaponErrorAngles weapon_error_angles(const WeaponErrorDefinition &,
                                     float error, float opposite_hand_blend);
// 147A90: original direction table, seed progression and angle rotation.
// The caller supplies original shot direction and selected angle bounds.
Vec3 weapon_random_deviation(Vec3, WeaponErrorAngles, std::uint32_t &seed,
                             std::span<const Vec3, 1026> directions);
} // namespace halo2
