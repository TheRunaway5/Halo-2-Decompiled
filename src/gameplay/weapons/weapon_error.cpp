#include "halo2/weapon_error.hpp"
#include <cmath>

namespace halo2 {
void weapon_error_step(WeaponErrorState &s, const WeaponErrorDefinition &d,
                       float dt, float blend, bool firing) {
  if (firing) {
    const float recoil = d.recoil_acceleration * dt + s.recoil;
    s.recoil = recoil > 1.f ? 1.f : recoil;
    const float normal = (1.f - blend) * d.normal_acceleration;
    const float dual = d.dual_acceleration * blend;
    const float error = (normal + dual) * dt + s.error;
    s.error = error > 1.f ? 1.f : error;
  } else if (s.error > 0.f) {
    // Original recovery of recoil is gated by positive *error*, not recoil.
    const float recoil = s.recoil - d.recoil_deceleration * dt;
    s.recoil = recoil > 0.f ? recoil : 0.f;
    const float normal = (1.f - blend) * d.normal_deceleration;
    const float dual = d.dual_deceleration * blend;
    const float error = s.error - (normal + dual) * dt;
    s.error = error > 0.f ? error : 0.f;
  }
}
WeaponErrorAngles weapon_error_angles(const WeaponErrorDefinition &d,
                                     float error, float blend) {
  const float dual_low = d.dual_low == 0.f ? d.normal_low : d.dual_low;
  const float dual_high = d.dual_high == 0.f ? d.normal_high : d.dual_high;
  const float remaining = 1.f - blend;
  const float low = d.normal_low * remaining + dual_low * blend;
  const float high = d.normal_high * remaining + dual_high * blend;
  return {remaining * d.normal_minimum + d.dual_minimum * blend,
          (high - low) * error + low};
}
Vec3 weapon_random_deviation(Vec3 direction, WeaponErrorAngles angles,
                             std::uint32_t &seed,
                             std::span<const Vec3, 1026> directions) {
  seed = seed * 1664525u + 1013904223u;
  const auto axis = directions[((seed >> 16) * 1026u) >> 16];
  // The x87 cross products keep 53-bit intermediates until float stores.
  const float x = float(double(axis.z) * direction.y -
                        double(axis.y) * direction.z);
  const float y = float(double(axis.x) * direction.z -
                        double(axis.z) * direction.x);
  const double wide_z = double(axis.y) * direction.x -
                        double(axis.x) * direction.y;
  const float z = float(wide_z);
  // The final cross component is stored with FST (not FSTP), so its square
  // multiplies the retained double by the rounded float. Preserve that order.
  const double length = std::sqrt((wide_z * z + double(y) * y) + double(x) * x);
  if (std::abs(length) < double(.0001f))
    return direction;
  const float rounded_length = float(length);
  const float inverse = 1.f / rounded_length;
  const Vec3 normal{x * inverse, y * inverse, z * inverse};
  if (!(rounded_length > 0.f))
    return direction;
  seed = seed * 1664525u + 1013904223u;
  // 4650C0 is the original float reciprocal of 65535, not 1/65536.
  const double fraction = double(seed >> 16) * double(0x1.0001p-16f);
  const double angle = fraction * (double(angles.maximum) - angles.minimum) +
                       angles.minimum;
  const float sine = float(std::sin(angle)), cosine = float(std::cos(angle));
  const float projection =
      ((direction.z * normal.z + direction.y * normal.y) +
       direction.x * normal.x) * (1.f - cosine);
  return {(direction.x * cosine + normal.x * projection) -
              (direction.y * normal.z - direction.z * normal.y) * sine,
          (direction.y * cosine + normal.y * projection) -
              (direction.z * normal.x - normal.z * direction.x) * sine,
          (direction.z * cosine + normal.z * projection) -
              (direction.x * normal.y - direction.y * normal.x) * sine};
}
} // namespace halo2

extern "C" void halo2_weapon_error_step(halo2::WeaponErrorState *state,
    const halo2::WeaponErrorDefinition *definition, float dt, float blend,
    int firing) {
  halo2::weapon_error_step(*state, *definition, dt, blend, firing != 0);
}
extern "C" void halo2_weapon_error_angles(
    const halo2::WeaponErrorDefinition *definition, float error, float blend,
    halo2::WeaponErrorAngles *result) {
  *result = halo2::weapon_error_angles(*definition, error, blend);
}
extern "C" void halo2_weapon_random_deviation(halo2::Vec3 *direction,
    const halo2::WeaponErrorAngles *angles, std::uint32_t *seed,
    const halo2::Vec3 *directions) {
  *direction = halo2::weapon_random_deviation(*direction, *angles, *seed,
      std::span<const halo2::Vec3,1026>(directions,1026));
}
