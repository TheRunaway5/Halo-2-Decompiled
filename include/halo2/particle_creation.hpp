#pragma once
#include "halo2/particle_emitter.hpp"

namespace halo2 {
struct ParticleSimulationState {
  std::uint16_t salt{},flags{};
  std::uint32_t next_particle{0xffffffffu};
  float age{},inverse_lifetime{},system_fraction{};
  std::array<std::uint16_t,4> random{};
  Vec3 position{},velocity{};
  float rotation_rate{},phase{};
  std::uint32_t seed_color{};
};
struct ParticleCreationInput {
  std::uint32_t allocated_datum{};std::uint64_t particle_identity{};
  float system_fraction{};
  ParticleEmissionRequest request;
  Vec3 effect_velocity{}; // Original NONE owner uses the original zero vector.
};
// Original successful 247950 creation through 247E2A, before child-system and
// birth-effect dispatch. The caller supplies the actual allocated slot (the
// original allocator clears 64 bytes then writes its salt), cache owners and
// RNG. This function links the particle, completes its cache, initializes and
// places it, and increments the emitter count. Allocation/dispatch are separate.
void create_emitter_particle(const ParticleSystemRenderDefinition &,unsigned emitter,
    ParticleEmitterState &,ParticleSimulationState &,const ParticleCreationInput &,
    ParticleRenderValueCache &,const ParticleValueSources &,std::uint32_t &seed,
    const OriginalData &,const ShaderFunctionTables &);
} // namespace halo2
