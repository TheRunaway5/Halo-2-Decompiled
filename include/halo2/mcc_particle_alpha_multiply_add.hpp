#pragma once
#include "halo2/render_program.hpp"
#include <span>
#include <string_view>

namespace halo2 {
inline bool mcc_particle_alpha_multiply_add_names(std::string_view shader_template,
    std::string_view shader_pass) {
    return shader_template=="shaders\\shader_templates\\transparent\\particle_alpha_multiply_add" &&
        shader_pass=="shaders\\shader_passes\\transparent\\effect_shader_alpha_multiply_add";
}
// Qualify the complete PC pass descriptor before selecting the existing custom
// Vulkan fragment/raster/sampler contract. Bitmap and vertex-constant values
// remain authored by the imported particle; this does not decode PC shaders.
MaterialPass project_mcc_particle_alpha_multiply_add_pass(const MaterialPass&,
    std::span<const std::byte> packed_samplers,
    std::span<const std::byte> packed_default_states);
}
