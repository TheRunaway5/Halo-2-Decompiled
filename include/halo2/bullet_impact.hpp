#pragma once
#include "halo2/cache.hpp"
#include "halo2/projectile_material_response.hpp"

namespace halo2 {
// Contact delivery from the actual collision backend. A shot stamp is not
// a contact. NONE material remains valid when the receiver has no material.
struct BulletImpact {
  std::uint64_t serial{}, tick{};
  std::uint32_t projectile = 0xffffffffu, weapon = 0xffffffffu;
  Vec3 position, normal, incoming;
  std::int16_t material = -1;
  bool terrain{};
  // Chosen once by the collision owner, before visual RNG or LAN delivery.
  // Absent on older/manual contacts whose response owner was not retained.
  std::optional<SourceProjectileImpactPresentation> presentation;
};
} // namespace halo2
