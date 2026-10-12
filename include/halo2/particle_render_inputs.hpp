#pragma once
#include "halo2/particle_sprite.hpp"
#include "halo2/particle_birth.hpp"
#include <functional>

namespace halo2 {
struct ParticleEmitterRenderDefinition {
  ParticleBirthDefinition birth;
  ParticleProperty emission_rate; // +08, original rate is multiplied by 30.
  Vec3 offset{};Vec2 orientation{}; // +9C/+A8.
  std::array<ParticleProperty,3> properties; // +48 scale, +58 tint, +68 alpha.
  std::uint32_t flags{},required_values{}; // +B0/+B4; shared/per-particle evaluation.
};
struct ParticleSystemRenderDefinition {
  std::uint32_t particle_datum{};
  std::int16_t coordinates{};
  std::uint16_t flags{}; // Authored system +16.
  std::vector<ParticleEmitterRenderDefinition> emitters;
};
// Completed simulation records, in the original location/emitter/particle-chain
// order. Identity keys must distinguish different allocations of the same slot.
struct ParticleRenderParticle {
  std::uint64_t identity{};
  ParticleValueState values;
  Vec3 velocity{};
  std::uint32_t seed_color{};
};
struct ParticleRenderEmitter {
  AuthoredBasis rotation{};
  Vec3 position{};
  std::vector<ParticleRenderParticle> particles;
};
struct ParticleRenderState {
  std::uint64_t system_identity{},location_identity{};
  ParticleValueSources sources;
  bool first_person{},uses_camera_scale{};
  std::optional<float> camera_scale; // Original 3EB70 caller result, when used.
  std::vector<ParticleRenderEmitter> emitters;
};
struct ParticleRenderValueCache {
  ParticleValueCache values;
  std::uint64_t system_identity{},location_identity{},particle_identity{};
};
using ParticleRenderCallback = std::function<void(std::uint64_t,
    const ParticleSpriteInput &,ParticleValueCache &,const ParticleValueSources &)>;
// Original 175610, including owner invalidation, property evaluation and
// coordinate transforms. The callback may complete additional sprite values,
// just as original 224550 does before the next particle is selected.
void for_each_particle_render_input(const ParticleSystemRenderDefinition &,
    const ParticleRenderState &,ParticleRenderValueCache &,
    const ShaderFunctionTables &,const ParticleRenderCallback &);
} // namespace halo2
