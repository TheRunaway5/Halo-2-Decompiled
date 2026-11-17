#pragma once
#include "halo2/render_program.hpp"
#include <string_view>
namespace halo2::pstencil {
struct ReferenceLightMaterial {
    std::string_view name;
    ShaderVector v10,v11;
    unsigned utility_builtin;
    bool neutral_texture_detail{};
};
const ReferenceLightMaterial* reference_light_material(std::string_view);
bool reference_default_texture_samplers(std::string_view);
unsigned actor_light_utility_builtin(std::string_view name, int authored_specular_type);
}
