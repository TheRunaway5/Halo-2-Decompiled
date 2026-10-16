#pragma once
#include "halo2/menu_world_raster.hpp"
#include "halo2/menu_world_source_binding.hpp"
#include "halo2/menu_world_source_pass.hpp"
#include "halo2/source_shadow.hpp"

namespace halo2 {
// Unchanged completed TU1.5 3BAA0 record, its actual command and collected
// unit. Keep sorted_ordinal when one descriptor expands into several ranges.
// word3 of a descriptor is intentionally not an identity for BSP clusters:
// the original selector leaves it unwritten for classification0.
struct SourceShadowRawCommandTu15 {
    SourceShadowPhase phase{};
    std::uint32_t root_owner=0xffffffffu;
    std::uint32_t bank{},category{},layer{},sorted_ordinal{},unit_ordinal{};
    std::array<std::uint32_t,6> descriptor{},command{};
    std::array<std::uint32_t,8> unit{};
    // Current geometry owner, including the full salted identity for dynamic
    // models. An attached weapon's identity need not equal root_owner.
    MenuWorldSourceOwner geometry_owner;
};
struct SourceShadowRawRasterInput {
    std::uint32_t cull{};
    bool depth_clamp{}; // Actual495D0/496E0 NV097_SET_ZMIN_MAX_CONTROL=16.
    // Actual SDK402840 NV097_SURFACE_CLIP: physical x,y,width,height.
    // This is independent of the eight WINDOW_CLIP rectangles, SDK AA
    // scale and bound-surface dimensions. No viewport/extent inference.
    std::array<std::uint32_t,4> surface_clip;
};
// Separate from SourceShadowPlan's packed world stream. The source binding
// owns unchanged resources, the actual class0..6 program and current c50
// palette. The caller retains authored through submission; no pass/LOD,
// geometry owner, palette or ordered range is selected by this seam.
struct SourceShadowRawDraw {
    unsigned material_slot{};
    const SourceShadowMaterial *authored{};
    MenuWorldSourcePass pass;
    MenuWorldSourceBinding source;
    SourceShadowRawCommandTu15 command;
    PixelProgram program;
    ShaderConstants constants{};
    SourceShadowRawRasterInput controls;
    MenuWorldRasterPacket raster;
    std::array<float,4> clip_range;
    std::array<float,2> fog_parameters;
    // Actual bound surface dimensions/source AA supplied by the caller,
    // independent of c0..11, Vulkan viewport and raster.surface_scale.
    std::array<float,2> surface_size;
};
// Original material c18..33,3A540 c31..36 and48ED0 c40..48. Retain the
// actual main camera c0..11 and every other caller word. The binding factory
// separately installs its owned c50 palette and c170..172 compression rows.
MenuWorldSourceConstants source_shadow_raw_constants(const MenuWorldSourceConstants &,
    const SourceShadowMaterial &,const SourceShadowUniforms &);
// Immutable authored phase/raster/target admission only. The actual class,
// current owner/palette, command identity and constants require draw validation.
void validate_source_shadow_raw_material(const SourceShadowMaterial &,SourceShadowPhase);
SourceShadowRawDraw source_shadow_raw_draw(unsigned slot,const SourceShadowMaterial &,
    MenuWorldSourcePass,MenuWorldSourceBinding,SourceShadowRawCommandTu15,
    const SourceShadowUniforms &,SourceShadowRawRasterInput,MenuWorldRasterPacket,
    std::array<float,4> clip_range,std::array<float,2> fog_parameters,
    std::array<float,2> surface_size);
// Rechecks the full join against this root's actual constants immediately
// before recording. This never relaxes the existing packed-stream guards.
void validate_source_shadow_raw_draw(const SourceShadowRawDraw &,SourceShadowPhase,
    const SourceShadowUniforms &);
} // namespace halo2
