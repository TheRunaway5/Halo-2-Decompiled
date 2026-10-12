#pragma once
#include "halo2/cache.hpp"

namespace halo2 {
struct EffectLightDefinition;
struct EffectLightUniforms;
struct SourceShaderFogContext;
// Original 19BE0/19CA0 scalar37 and 1A170 color40, supplied by the resolved first-biped
// context. Call before animate_forward_material:17420 masks run afterward.
// Clones shared child passes so another object's bank remains independent.
void bind_object_camouflage(Material &,float,std::uint16_t x87_control_word=0x023f);
// Original layer19 scalar39/color41 on this selected pass only. Updates
// live/retained vertex banks before authored animation; caller owns view/fog
// and the actual pointer-backed default RGB. Child passes bind separately.
void bind_camouflage_fog_pass(Material &,const SourceShaderFogContext &);
// Original layer12/implementation1 for spherical light diffuse accumulation.
// The caller supplies source-admitted geometry and already-faded C31..39.
std::vector<Material> prepare_effect_light_diffuse_material(const Cache &,
    const Material &authored,const EffectLightDefinition &,const EffectLightUniforms &);
// Retain Material::selected_shader_lod's static BSP passes. 2D000 chooses the
// lighting implementation from the geometry policy: per-pixel (0) or
// per-vertex (1). Geometry callers supply that policy, not a texture heuristic.
// Returns false for a material without an admitted static lighting layer.
bool prepare_world_material(const Cache &, Material &, bool per_vertex = false);
// Source4D0B0/24040: retain the selected shader-LOD layer5 and separate PRT bank.
bool prepare_prt_material(const Cache &,Material &,std::shared_ptr<const VertexProgramBank>);
// 2D000 classification 3, selected by 4CBF0: the authored dynamic variant
// without live object-lighting externs. The caller must establish that mode.
bool prepare_authored_lighting_material(const Cache &, Material &);
// Source classification3 with its independent transparent/emissive passes.
// Optional extern37 is the object's resolved parent camouflage context.
bool prepare_default_object_material(const Cache &,Material &,
                                     std::optional<float> camouflage = {});
// Original plasma_shield layer15/implementation0. Binding its authored pass
// does not establish CPU-posed geometry equivalence: the local/skinned program
// uses local position for noise coordinates and posed position for projection.
// Callers must separately preserve that vertex-input contract before drawing.
bool prepare_shield_overlay_material(const Cache &,Material &);
// Bind authored LOD0/layer15/implementation0 transparent_simple_world data,
// with explicit immediate RGBA11 vertex input. Submission/geometry and the
// source draw owner's later declaration/raster overrides remain caller-owned.
bool prepare_simple_world_material(const Cache &,Material &);
}
