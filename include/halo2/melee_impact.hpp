#pragma once
#include "halo2/cache.hpp"
namespace halo2 {
// D0830 -> D0340 contact presentation, separate from projectile responses.
// Material NONE cannot be replaced by a guessed flesh/shield material.
struct MeleeImpact {
  std::uint64_t serial{},tick{};
  std::uint32_t attacker=0xffffffffu,weapon=0xffffffffu,damage=0xffffffffu;
  Vec3 position,normal,forward,attacker_position;
  std::int16_t material=-1;
};
}
