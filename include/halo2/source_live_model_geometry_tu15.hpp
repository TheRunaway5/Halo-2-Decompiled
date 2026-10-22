#pragma once
#include "halo2/source_bsp_part_commands.hpp"
#include "halo2/source_draw_unit.hpp"
#include "halo2/source_object_draw_collector.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/appearance.hpp"
#include "halo2/source_shadow.hpp"
#include <map>

namespace halo2 {
class PhysicsLiveWorldTu15;
struct SourcePrtDefinition;
struct RenderModel;
class SourceDynamicModelShaders;
struct SourceModelShaderView;
// An owned main-view handoff. Secondary queries reuse the source cache and
// frame arena, so every input below is copied before those queries run.
struct SourceLiveModelBaseDrawTu15 {
    SourceCullDrawUnit unit;
    unsigned ordinal{},lod{},lighting_class{};
    std::uint8_t coverage{},override_slot{};
    std::uint32_t shader_override=0xffffffffu,object{},definition{},object_flags{},lighting_metadata{};
    ObjectLightingState lighting;
    ShadowCasterSphere sphere;
    std::array<std::uint32_t,4> packed_colors{};
    std::uint8_t color_count{};
    SourceModelCommandGeometry geometry;
    std::vector<std::array<float,12>> palette_rows;
};
// Original131BB0 unpack of the completed cache's actual color channels.
// Unwritten channels retain the caller's prior bank; no color is invented.
ObjectColors source_live_model_base_colors_tu15(const SourceLiveModelBaseDrawTu15&,
    ObjectColors retained);
// Validate a secondary unit against its retained main selection and actual
// current c50 upload, including bitwise matrix equality. Tokens alone cannot
// establish that the geometry arena still contains the same pose.
bool source_live_model_base_matches_tu15(const SourceLiveModelBaseDrawTu15&,
    PhysicsLiveWorldTu15&,const SourceCullDrawUnit&);
// Owned ordinary CPU base geometry from the completed main selection. All six
// authored LOD banks and the effective source-rendered WORLD hierarchy belong
// to the caller. Every section uses its retained main coverage byte. Admission
// is atomic for a full owner; unsupported lighting/overrides return no bank.
// This does not choose a pose, colors, lighting state or shader-function owner.
// An optional provider lends one complete immutable skin for this synchronous
// call. Ordered section selection and per-view shader evaluation stay here.
using SourceLiveModelPoseProviderTu15=std::function<std::span<const Vertex>(
    std::uint32_t,unsigned,const RenderModel&,std::span<const Transform>)>;
std::optional<std::vector<Vertex>> source_live_model_base_vertices_tu15(
    std::uint32_t owner,std::uint32_t definition,std::uint32_t render_model,
    std::span<const SourceLiveModelBaseDrawTu15>,std::span<const RenderModel,6>,
    const SourceDynamicModelShaders&,std::span<const Transform>,const SourceModelShaderView&,
    const SourceLiveModelPoseProviderTu15& = {});
class SourceLiveModelBaseSelectionTu15 {
public:
    // Observe each completed4B5D0 output, including recursively built children.
    // Neither method changes original admission, ordering or source storage.
    void remember(const SourceObjectDrawCollector&);
    std::vector<SourceLiveModelBaseDrawTu15> copy(PhysicsLiveWorldTu15&,
        const SourceDrawUnitState&,
        const std::function<bool(const SourceCullDrawUnit&)>& available={})const;
private:
    std::map<std::pair<std::uint32_t,std::uint32_t>,unsigned> lods_;
};
// Read the selected unit's actual resident resource and current1D690 palette.
// The palette belongs to this live cache token, never to a shared model tag.
SourceModelCommandGeometry source_live_model_command_geometry_tu15(
    PhysicsLiveWorldTu15&,const SourceCullDrawUnit&);
// Current45550 banks, including BAC30 WORLD nodes and authored inverse binds.
// The borrowed ranges must remain alive through the cull operation.
SourceCullDrawBanks source_live_model_cull_geometry_tu15(
    PhysicsLiveWorldTu15&,const SourceCullDrawUnit&,const SourceBspRangeGeometry&);
// TU1.5 4D820/15280 c50 upload selection, including the rigid single-matrix
// branch and the selected region's actual remapped matrix range.
std::vector<std::array<float,12>> source_live_model_palette_rows_tu15(
    PhysicsLiveWorldTu15&,const SourceCullDrawUnit&,const SourceModelCommandGeometry&);
// Read completed24020 c68.. banks by full owner and the draw's region ordinal.
// Absent owner/zero node count preserves the original no-upload result.
std::shared_ptr<const VertexProgramBank> source_live_model_prt_bank_tu15(
    PhysicsLiveWorldTu15&,const SourceCullDrawUnit&,const SourcePrtDefinition&);
}
