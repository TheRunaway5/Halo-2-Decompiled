#pragma once
#include "halo2/particle_material.hpp"
#include "halo2/particle_resource_error.hpp"
#include <string_view>

namespace halo2 {
struct ParticleMaterialValidation {
    std::string_view fragment_kernel;
    std::uint64_t program_hash{};
    unsigned instructions{},classification{};
    bool external_scene_texture{},distortion{};
    ForwardRasterState raster;
    bool blend{};
    unsigned source_blend=1,destination_blend{};
};
// Complete CPU content contract used before effect admission and again before
// GPU publication. Unsupported content throws ParticleResourceError. Vulkan
// device and allocation errors belong to the real renderer's strict boundary.
ParticleMaterialValidation validate_particle_material(const ParticleMaterial&);
}
