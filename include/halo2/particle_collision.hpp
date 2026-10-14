#pragma once
#include "halo2/particle_movement.hpp"
#include <optional>

namespace halo2 {
struct ParticleCollisionHit {
  float fraction{};
  Vec3 position, normal;
  bool structure{};
  std::int16_t material=-1;
  std::int32_t type=-1;
};
inline bool source_particle_collision_dies(std::uint32_t flags,std::uint32_t type) {
  //2C32FF..2C3348, including complete22B750/22B760 getters.
  return ((type==1u || type==3u) && (flags&0x400u)) ||
      (type==2u && (flags&0x800u));
}
using ParticleCollisionQuery =
    std::function<std::optional<ParticleCollisionHit>(
        std::uint32_t source_flags, Vec3 origin, Vec3 displacement)>;
using ParticleCollisionChild = std::function<void(
    const ParticleSimulationState &, const ParticleCollisionHit &)>;
// Retail246EB0 / TU1.1 248F80. Both collision and death sound/effect children
// consume this velocity gain; collision dispatch follows bounce stores.
float source_particle_collision_child_gain(Vec3 velocity);
// 2BF380 / 2C2D90's response after a real collision query. The world backend
// and optional collision child-effect dispatch are separate owners.
void source_particle_collision_response(ParticleSimulationState &,
                                        const ParticleCollisionHit &,
                                        float friction, float restitution);
// Source collider1 countdown, property evaluation, query and bounce. Optional
// child dispatch occurs on first contact after bounce and before setting bit2.
void apply_particle_collider_controller(
    const ParticleMovementController &, std::uint32_t movement_flags,
    std::uint32_t particle_flags, std::uint32_t head, float elapsed,
    const ParticleValueSources &, const ShaderFunctionTables &,
    const ParticleMovementPool &, const ParticleCollisionQuery &,
    const ParticleCollisionChild & = {});
} // namespace halo2
