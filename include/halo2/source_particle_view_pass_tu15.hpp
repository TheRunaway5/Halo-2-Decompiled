#pragma once
#include "halo2/physics_world_runtime.hpp"
#include <array>
namespace halo2 {
// Value boundary for2BCB3..2BCE4. The original caller increments4C685C and
// builds these exact13430 input words. It does not establish pass scheduling,
// preceding queue retirement, or the remaining2BCB0 rendering services.
struct SourceParticleViewPassTu15 {std::array<std::uint32_t,3> words;};
SourceParticleViewPassTu15 source_particle_view_pass_tu15(SourcePhysicsContactMemory &,
    std::uint32_t boundary_mode,std::uint32_t boundary_distance_bits);
}
