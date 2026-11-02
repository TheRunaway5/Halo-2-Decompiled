#pragma once
#include "halo2/source_shadow_raw_draw.hpp"
#include <map>
#include <tuple>

namespace halo2 {
struct SourceShadowModelPart {
    std::uint32_t shader=~0u;std::uint16_t type{};
    std::vector<std::uint32_t> template_lod_layers;
};
using SourceShadowModelPartBank=std::map<std::tuple<std::uint32_t,unsigned,unsigned>,SourceShadowModelPart>;
struct SourceShadowModelPartCommand {
    std::uint32_t object=~0u,render_model=~0u;
    unsigned section{},part{};std::uint32_t shader=~0u;
    unsigned shader_lod{};std::uint32_t layers{};bool empty_ranges{};
};
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
    std::span<const SourceShadowModelPartCommand> model_commands;
};
struct SourceShadowRawPlan {
    // Actual submitting main c0..11 retained by BOTH phases. The fixed
    // generation target changes its surface/viewport, not this camera bank.
    std::array<ShaderVector,12> camera{};
    std::span<const SourceShadowRawPlanRoot> roots;
    const SourceShadowModelPartBank* model_parts{};
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
