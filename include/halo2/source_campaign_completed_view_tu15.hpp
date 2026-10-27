#pragma once
#include "halo2/source_campaign_view_tu15.hpp"
#include "halo2/source_campaign_environment_frame_tu15.hpp"
#include "halo2/source_texture_camera_scene_prefix_tu15.hpp"
#include "halo2/source_model_shader.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/scene.hpp"
namespace halo2 {
// Decoder of one genuine native2B9F0→132B0 CPU-prefix output. The two
// cameras stay distinct: collection/culling/coverage uses camera1, while
// native GPU12D30 constants and physical viewport use submitted camera2.
// No source producer or visibility query is replaced by this value object.
struct SourceCampaignCompletedViewTu15 {
    SourceCampaignEnvironmentFrameTu15 environment;
    BspFogSample fog;
    SourceViewProjectionBank collection_rendering,submitting_rendering;
    SourceModelShaderView shaders;
    SourceGeometryLodPolicy policy;
    Camera submitting_camera;
    std::array<std::int16_t,4> collection_rectangle{},submitting_rectangle{};
    Vec2 configured_clip,submitting_clip;
    std::array<ShaderVector,7> submitting_constants;
    // Caller must bind input.environment to THIS value's environment before
    // synchronous collect(). It is intentionally null across moves/copies.
    SourceCampaignViewInputTu15 input;
};
// Read-only exact occupied original object-pool census. Call before sealing
// the common material range, then union with authored definition resources;
// this is residency preparation, never visibility or shader admission.
std::vector<std::uint32_t> source_campaign_resident_definitions_tu15(
    const Cache &,std::size_t actual_bsp,PhysicsLiveWorldTu15 &);
// actual_horizontal_angle comes only from the existing admitted presentation
// owner for a real local-user view. NONE-user texture cameras do not mutate
// any player observer and need no invented horizontal lens value.
SourceCampaignCompletedViewTu15 source_campaign_completed_view_tu15(
    PhysicsLiveWorldTu15 &,const SourceTextureCameraScenePrefixTu15 &,
    const BspAtmosphere &,bool begin_frame,
    std::optional<float> actual_horizontal_angle = {});
}
