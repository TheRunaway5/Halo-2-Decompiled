#pragma once
#include "halo2/particle_sprite.hpp"
#include "halo2/original_data.hpp"

namespace halo2 {
struct ParticleBirthDefinition {
  // Original emitter +18 lifetime, +28 speed, +38 rotation, +7C radius,
  // +8C angle. Shape is +78; allocation/emission are separate operations.
  std::array<ParticleProperty,5> properties;
  std::uint32_t shape{};
};
struct ParticleBirthState {
  float inverse_lifetime{},rotation_rate{};
  Vec3 position{},velocity{};
};
// Original 249170 initialization for shapes 0 through 5. Other shapes require
// independent evidence and are rejected. RNG ownership is supplied explicitly.
// Normalization follows the observed Xemu CPU result; this does not establish
// the approximate RSQRTSS result on physical Xbox hardware.
ParticleBirthState initialize_particle_birth(const ParticleBirthDefinition &,
    const std::array<float,17> &,std::uint32_t &seed,
    const OriginalData &,const ShaderFunctionTables &);
} // namespace halo2
