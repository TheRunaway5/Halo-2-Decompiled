#pragma once
#include "halo2/particle_render_inputs.hpp"

namespace halo2 {
struct ParticleEmitterState {
  std::uint16_t salt{},particle_count{};
  std::uint32_t first_particle{0xffffffffu},next_emitter{0xffffffffu};
  float emission_remainder{};
  AuthoredBasis rotation{};
  Vec3 position{},previous_position{};
};
struct ParticleEmitterFrame { float scale{1};AuthoredBasis rotation{};Vec3 position{}; };
struct ParticleEmissionRequest { float fraction{},dt{},scale{1};std::uint32_t color{}; };
using ParticleEmissionCallback=std::function<void(const ParticleEmissionRequest &,ParticleEmitterState &)>;
// Original 247470: placement, emission-rate accumulation and ordered birth
// requests. The callback owns original 247950's allocation/creation work and
// may update the emitter's particle head and count. Values are caller-completed.
void update_particle_emitter(const ParticleSystemRenderDefinition &,unsigned emitter,
    ParticleEmitterState &,std::uint16_t system_flags,float dt,
    const ParticleEmitterFrame *,const std::array<float,17> &,std::uint32_t color,
    const ShaderFunctionTables &,const ParticleEmissionCallback &);
} // namespace halo2
