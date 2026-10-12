#pragma once
#include "halo2/source_shadow.hpp"

namespace halo2 {
// These are actual resident Mesh material slots, allocated before Renderer.
// A slot keeps its authored images, texture states, selected pass and VP.
struct SourceShadowPlanMaterial {
    unsigned material_slot{};
    const SourceShadowMaterial *authored{};
};
struct SourceShadowRasterInput {
    // Actual caller D3D winding cull: NONE0, CW0x900 or CCW0x901.
    // This may override the authored material state; it is not inferred from
    // object identity, mirroring or the fixed target's dimensions.
    unsigned cull{};
    bool depth_clamp{};
    bool operator==(const SourceShadowRasterInput &)const=default;
};
struct SourceShadowPlanDraw {
    unsigned material_slot{};
    // Original ordered part/subpart triangles after the caller's proved world
    // transform/UV decode. No regrouping by material is permitted here.
    std::span<const Vertex> vertices;
    // Actual completed draw bindings, including animation, change colors and
    // opacity. The GPU hook must not repeat extern binding after animation.
    PixelProgram program;
    ShaderConstants constants{};
    std::optional<SourceShadowRasterInput> raster;
};
struct SourceShadowPlanRoot {
    SourceShadowUniforms uniforms;
    std::span<const SourceShadowPlanDraw> casters,receivers;
};
struct SourceShadowPlan {
    // Actual original c0..11, including c10/c11's viewport/feature state.
    // Generation uses the fixed target projection globals below; receiver
    // uses this scene bank. This is not an inferred GLM camera projection.
    std::array<ShaderVector,12> camera{};
    std::span<const SourceShadowPlanRoot> roots;
};
struct SourceShadowPlanStats {
    std::size_t roots{},caster_draws{},receiver_draws{},vertices{};
};
struct SourceShadowPlanBindings {
    PixelProgram program;
    ShaderConstants constants{};
    std::array<ShaderVector,12> camera{};
    VertexProgramBank globals;
    std::array<unsigned,4> external_targets{};
};
// Shared fixed-target material admission, also used by SourceShadowRender.
void validate_source_shadow_plan_material(const SourceShadowMaterial &,
                                          SourceShadowPhase);
// Snapshot a fully bound pass without copying any resident texture data.
SourceShadowPlanDraw source_shadow_plan_draw(unsigned material_slot,
    const SourceShadowMaterial &,std::span<const Vertex>,SourceShadowRasterInput);
// Checks every admitted root before command-buffer render-pass mutation.
// The bounded hook accepts the authored world-coordinate compiled variants;
// skin/palette variants need a separate actual input stream/register service.
SourceShadowPlanStats validate_source_shadow_plan(const SourceShadowPlan &,
    std::span<const SourceShadowPlanMaterial> resident);
// Only 3A550/48EE0 globals are overlaid: c31..36 and c40..48. The unused gap
// c37..39 is not an admitted shader input. Pixel and material bindings survive.
// Each returned bank must have distinct GPU storage until its draw completes.
SourceShadowPlanBindings source_shadow_plan_bindings(const SourceShadowPlan &,
    const SourceShadowPlanRoot &,const SourceShadowPlanDraw &,
    const SourceShadowPlanMaterial &);
// Explicit production binding7 bank. Original c0..11 is independent of the
// fixed target viewport; c18..33 remains the completed ordinary material bank.
// c12..17 and c37..39 are zero storage, not recovered shadow shader inputs.
// This preserves the normal first34 globals seam in SourceShadowPlanBindings.
VertexProgramBank source_shadow_plan_vertex_bank(const SourceShadowPlanBindings &);
}
