#pragma once
#include "halo2/render_program.hpp"
#include <span>
#include <string_view>

namespace halo2 {
bool mcc_particle_pc_projection_names(std::string_view shader_template,
    std::string_view shader_pass,unsigned implementation);
std::span<const std::byte> mcc_particle_pc_vertex_signature(std::string_view shader_template,
    std::string_view shader_pass,unsigned implementation,unsigned classification,
    std::string_view source_vertex,unsigned source_classifications);
// Exact imported cooked pass/remap/sampler/state qualification, followed by
// the existing custom Vulkan kernel and its matching constant interface.
MaterialPass project_mcc_particle_pc_pass(std::string_view shader_template,
    const MaterialPass&,std::array<std::size_t,3> constant_counts,
    std::span<const std::byte> packed_samplers,std::span<const std::byte> packed_defaults);
}
