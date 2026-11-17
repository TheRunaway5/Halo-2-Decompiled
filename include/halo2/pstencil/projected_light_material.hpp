#pragma once
#include "halo2/effect_light.hpp"

namespace halo2::pstencil {
// Literal30D80, called by15D50 with shad+3E !=0 and its second argument1.
unsigned projected_light_implementation(std::uint32_t definition_flags,
    std::uint32_t feature_mask,bool material_specular);
// Literal34050 switch6: bit13 belongs to the current shader LOD's available
// layers (16D17 ->1AFCC), not a geometry/part flag or the SPAS state range.
bool projected_light_specular_mask(std::uint32_t definition_flags,
    std::uint32_t feature_mask,std::uint32_t shader_lod_layers);
//311A8..31273, projected branch after the original layer10 shadow gate.
std::uint32_t projected_light_feature_mask(Vec3 completed_specular,float epsilon,
    bool shadow_enabled,float volume_fade,std::uint16_t volume_kind);
std::vector<Material> prepare_projected_light_material(const Cache&,const Material&,
    const EffectLightDefinition&,const EffectLightUniforms&,unsigned implementation,
    std::uint32_t feature_mask);
// Source layer 11 `shadow_apply_combined` for a projected light: the alpha-only
// pass 31150 submits before the light response. Implementation 2 (30D00 for a
// specular light without a shadow buffer) writes the light's gel into target
// alpha; shadow-buffer implementations are not admitted here.
std::vector<Material> prepare_projected_light_alpha_material(const Cache&,const Material&,
    const EffectLightDefinition&,const EffectLightUniforms&,unsigned implementation);
std::vector<Material> complete_projected_light_alpha_constants(const Cache&,const Material&,
    const EffectLightDefinition&,const EffectLightUniforms&,unsigned implementation);
// A shadowed light's own shadow-buffer passes. Layer 10 (`shadow_generate`,
// implementations 1 and 2 from 30CB0) draws a caster into the light's depth
// target; layer 11 implementations 0 (one tap), 1 (three taps) and 6 (three
// taps, the cinematic per-object buffer) write the comparison times the gel
// into target alpha in place of implementation 2.
std::vector<Material> prepare_projected_light_shadow_material(const Cache&,const Material&,
    const EffectLightDefinition&,const EffectLightUniforms&,unsigned layer,unsigned implementation);
std::vector<Material> complete_projected_light_shadow_constants(const Cache&,const Material&,
    const EffectLightDefinition&,const EffectLightUniforms&,unsigned layer,unsigned implementation);
// Complete only the per-light shader/constant/raster metadata. These results
// contain no loaded images and are exclusively packet inputs for an already
// resident material slot prepared by the full function above.
std::vector<Material> complete_projected_light_constants(const Cache&,const Material&,
    const EffectLightDefinition&,const EffectLightUniforms&,unsigned implementation,
    std::uint32_t feature_mask);
}
