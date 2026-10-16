#pragma once
#include "halo2/cache.hpp"
#include <string_view>

namespace halo2::pstencil {
struct EnvironmentParameters {
    ShaderVector facing,glancing;
    std::uint32_t midrange_factor_argb{},midrange_color_argb{};
};
// Exact original path plus unanimous static values across validated original
// caches. Missing paths keep their current authored values.
const EnvironmentParameters* reference_environment_parameters(std::string_view shader);
void apply_reference_environment(const EnvironmentParameters&,PixelProgram&,
    std::array<ShaderVector,16>& vertex_attributes);
// Bounded current-asset interface: independent tangent-space environment pass.
// Other independent/combined response families remain with their current code.
bool supports_environment_response(const Material&);
// Recognized combined dynamic-lightmap consumer. Only its isolated reflection
// strength fields may be adapted; its tint also affects diffuse and is retained.
bool supports_combined_environment_strength(const Material&);
void adapt_combined_environment_strength(PixelProgram&,const EnvironmentParameters*,bool polished_armor);
// Actual default-layered resident six-combiner response. Preserve completed
// current tint/brightness attributes, environment/bump textures and alpha blend;
// copy the current view direction and the consumer-proved midrange parameters.
PixelProgram adapt_environment_response(const PixelProgram& completed_current);
// Literal older callback fallback construction, retained for reference tests.
// Its model-frame admission remains unproved; the live adapter does not use it.
// Caller supplies the actual window vector; textures/attributes/alpha ownership
// remain the explicitly declared current-asset adaptation.
PixelProgram fallback_environment_response(Vec3 window_direction);
}
