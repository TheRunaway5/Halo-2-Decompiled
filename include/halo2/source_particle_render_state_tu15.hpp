#pragma once
#include "halo2/particle_render_inputs.hpp"
#include "halo2/source_runtime_memory.hpp"

namespace halo2 {
struct SourceParticleRenderCameraTu15 {
    Vec3 position{},forward{},up{};
    std::optional<float> scale;
};
struct SourceParticleRenderStateTu15 {
    ParticleSystemRenderDefinition definition;
    ParticleRenderState state;
};
// Read the completed original pools consumed by1784B0. This consumer does not
// emit, move, age or admit particles, or update their last-drawn timestamps.
SourceParticleRenderStateTu15 source_particle_render_state_tu15(
    const Cache &,const SourceRuntimeMemory &,std::uint32_t system,
    std::uint32_t location,const SourceParticleRenderCameraTu15 &);
}
