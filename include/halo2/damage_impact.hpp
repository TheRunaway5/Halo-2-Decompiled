#pragma once
#include "halo2/transform.hpp"
namespace halo2 {
// Context of an admitted native damage request, before its source response.
// Radius direction and melee origin/aim are resolved from the retained world.
struct DamageImpactContext {
  enum class Kind { Direct, Melee, Radius } kind=Kind::Direct;
  Vec3 direction,position;
  float scale=1.f;
  std::uint32_t source_definition=0xffffffffu; // Actual originating projectile.
};
}
