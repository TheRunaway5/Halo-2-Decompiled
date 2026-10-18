#include "halo2/pstencil/reference_light_materials.hpp"
namespace halo2::pstencil {
namespace {
#include "reference_light_materials.inc"
}
const ReferenceLightMaterial* reference_light_material(std::string_view name) {
    for(const auto& material:reference_light_materials)if(material.name==name)return &material;
    return nullptr;
}
}
