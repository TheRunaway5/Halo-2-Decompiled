#pragma once
#include "halo2/render_program.hpp"
#include <string_view>
namespace halo2::pstencil {
struct ReferenceLightMaterial {
    std::string_view name;
    ShaderVector v10,v11;
    unsigned utility_builtin;
};
const ReferenceLightMaterial* reference_light_material(std::string_view);
}
