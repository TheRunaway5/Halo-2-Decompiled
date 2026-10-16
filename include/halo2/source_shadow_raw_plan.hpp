#pragma once
#include "halo2/source_shadow_raw_draw.hpp"

namespace halo2 {
// Independent raw residency. These slots are allocated in the scene Mesh
// before Renderer creation; packed world-stream admission stays separate.
struct SourceShadowRawPlanMaterial {
    unsigned material_slot{};
    const SourceShadowMaterial *authored{};
};
struct SourceShadowRawPlanRoot {
    std::uint32_t root_owner=0xffffffffu;
    SourceShadowUniforms uniforms;
    // Actual4CE298/4CE280 header+8, respectively. Original32917/3292F
    // tests these masks before selection/submission, not sorted draw counts.
    std::array<std::uint32_t,2> command_masks{};
    std::span<const SourceShadowRawDraw> casters,receivers;
};
struct SourceShadowRawPlan {
    // Actual submitting main c0..11 retained by BOTH phases. The fixed
    // generation target changes its surface/viewport, not this camera bank.
    std::array<ShaderVector,12> camera{};
    std::span<const SourceShadowRawPlanRoot> roots;
};
struct SourceShadowRawPlanStats {
    std::size_t roots{},caster_draws{},receiver_draws{},indices{};
};
bool source_shadow_raw_phases_present(const std::array<std::uint32_t,2>&);
// Checks the completed raw owner/pass/constant joins before any render-pass
// mutation. Descriptor ordinals and repeated subpart ranges retain source
// order. This does not select, sort, cull or manufacture a successful binding.
SourceShadowRawPlanStats validate_source_shadow_raw_plan(const SourceShadowRawPlan &,
    std::span<const SourceShadowRawPlanMaterial> resident);
}
