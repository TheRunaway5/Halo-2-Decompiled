#pragma once
#include "halo2/cache.hpp"

namespace halo2 {
// Contact delivery from the actual collision backend. A shot stamp is not
// a contact. NONE material remains valid when the receiver has no material.
struct BulletImpact {
  std::uint64_t serial{}, tick{};
  std::uint32_t projectile = 0xffffffffu, weapon = 0xffffffffu;
  Vec3 position, normal, incoming;
  std::int16_t material = -1;
  bool terrain{};
};
} // namespace halo2
