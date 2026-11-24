#pragma once
#include "halo2/math.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
// TU1.5 12A9D0: the Doppler shift, in cents, of a source heard by a
// listener, from both velocities along the line between them. Sound travels
// 111.548553 world units per second; the frequency ratio is held to 1/8..4.
// 1290A0 (impulses) and 21C400 (loops) add the sound class's Doppler factor
// (+0x48) times this to a 3D voice's pitch before its driver conversion.
inline float source_sound_doppler_cents_tu15(Vec3 listener, Vec3 listener_velocity, Vec3 source,
                                             Vec3 source_velocity) {
  constexpr float speed_of_sound = 111.548553f;
  auto direction = source - listener;
  const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
  direction = length > 0.f ? direction * (1.f / length) : Vec3{};
  const float listener_speed = 0.f - (listener_velocity.z * direction.z + listener_velocity.y * direction.y +
                                      listener_velocity.x * direction.x);
  const float source_speed =
      source_velocity.z * direction.z + source_velocity.y * direction.y + source_velocity.x * direction.x + speed_of_sound;
  const float ratio = (speed_of_sound - listener_speed) / (source_speed > 0.001f ? source_speed : 0.001f);
  return float(std::log(std::clamp(double(ratio), 0.125, 4.0)) * double(1731.234f));
}
} // namespace halo2
