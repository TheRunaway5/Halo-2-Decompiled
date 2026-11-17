#include "halo2/pstencil/reference_light_materials.hpp"
#include <algorithm>
#include <bit>
namespace halo2::pstencil {
namespace {
#include "reference_light_materials.inc"
#include "reference_elite_light_materials.inc"
#include "reference_actor_light_materials.inc"

const ReferenceLightMaterial* previous_reference(std::string_view name) {
    for (const auto& material : reference_light_materials)
        if (material.name == name) return &material;
    for (const auto& material : elite_light_materials)
        if (material.name == name) return &material;
    return nullptr;
}
}
const ReferenceLightMaterial* reference_light_material(std::string_view name) {
    if (const auto* material = previous_reference(name)) return material;
    const auto found = std::ranges::lower_bound(actor_light_materials, name, {}, &ReferenceLightMaterial::name);
    return found != actor_light_materials.end() && found->name == name ? &*found : nullptr;
}

bool reference_default_texture_samplers(std::string_view name) {
    return previous_reference(name) != nullptr;
}

unsigned actor_light_utility_builtin(std::string_view name, int authored_specular_type) {
    if (const auto* material = reference_light_material(name)) return material->utility_builtin;
    if (name.starts_with("objects\\characters\\") || name.starts_with("objects\\weapons\\")) {
        // Retail shad +3E: Default, Dull and Shiny select the corresponding
        // recovered 2003 lobes. The selected layer still owns admission.
        if (authored_specular_type == 2) return 10;
        if (authored_specular_type == 3) return 9;
    }
    return 6;
}
}
