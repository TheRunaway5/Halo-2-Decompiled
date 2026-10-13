#pragma once
#include "halo2/particle_sprite.hpp"
#include "halo2/source_vertex_declaration.hpp"

namespace halo2 {
struct ParticleModelDefinition {
  std::uint32_t flags{},shader{},required_values{};
  unsigned orientation{};
  std::array<ParticleProperty,4> properties; // scale XYZ, rotation
  std::vector<std::array<std::uint16_t,2>> spans; // first, count
  std::vector<std::array<ShaderVector,5>> vertices;
  std::vector<std::uint16_t> indices;
};
ParticleModelDefinition load_particle_model(const Cache &,std::uint32_t);
// Original224FD0's per-particle transform registers and selected index span.
// Model geometry and shader declaration remain authored, not a sprite proxy.
struct ParticleModelSubmission {
  std::array<ShaderVector,3> transform;
  unsigned first{},count{};
};
ParticleModelSubmission particle_model_submission(const ParticleModelDefinition &,
    const ParticleSpriteInput &,std::uint16_t model_random,
    ParticleValueCache &,const ParticleValueSources &,const ShaderFunctionTables &);
}
