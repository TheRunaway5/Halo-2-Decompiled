#pragma once
#include "halo2/cache.hpp"
#include "halo2/menu_world_source_binding.hpp"
#include "halo2/menu_world_source_pass.hpp"
#include "halo2/menu_world_raster.hpp"

namespace halo2 {
// One completed original layer12 sphere diffuse pass. The caller supplies
// admitted, culled triangles in original order and retains every bank through
// Renderer::draw. This seam does not infer light visibility or receivers.
struct EffectLightDraw {
    unsigned material_slot{};
    std::span<const Vertex> vertices;
    PixelProgram program;
    ShaderConstants constants{};
    PackedVertexProgramBank vertex_bank;
};
EffectLightDraw source_effect_light_draw(unsigned slot,const Material &,
    std::span<const Vertex>);
void validate_effect_light_draw(const EffectLightDraw &,const Material &resident);
// Raw source geometry/palette variant. The caller supplies the actual selected
// layer12 implementation and ordered range; no CPU vertex conversion occurs.
// Raster/clip/fog are actual per-view caller state, retained through draw().
struct EffectLightRawDraw {
    unsigned material_slot{};
    MenuWorldSourceBinding source;
    PixelProgram program;
    ShaderConstants constants{};
    PackedVertexProgramBank vertex_bank;
    MenuWorldRasterPacket raster;
    std::array<float,4> clip_range;
    std::array<float,2> fog_parameters;
    // Actual occupied source light record and authored definition. Diagnostic
    // provenance only; these fields do not enter the GPU constant banks.
    std::uint32_t light_identity=0xffffffffu,light_definition=0xffffffffu;
    // Actual current source light origin, retained for the optional capped
    // volume backend. It must not be inferred from inverse shader constants.
    std::optional<Vec3> point_light_position;
    // Authored sphere specular companion passes. Original ignores this bank;
    // pStencil submits it inside the same per-light stencil test as diffuse.
    std::shared_ptr<const std::vector<EffectLightDraw>> pstencil_specular;
    std::uint32_t pstencil_light_domain{}; // Separate optional direct-light allocator.
    bool pstencil_casts_shadow=true;
};
// The original material upload covers c18..33 and3A550 covers c31..39.
// Other owner/view constants survive, including compression/palette inputs
// subsequently installed by menu_world_source_binding_class().
MenuWorldSourceConstants source_effect_light_raw_constants(
    const MenuWorldSourceConstants &,const Material &completed);
EffectLightRawDraw source_effect_light_raw_draw(unsigned slot,const Material &,
    const MenuWorldSourcePass &,MenuWorldSourceBinding,MenuWorldRasterPacket,
    std::array<float,4> clip_range,std::array<float,2> fog_parameters);
}
