#pragma once
#include "halo2/source_campaign_view_tu15.hpp"
#include "halo2/source_campaign_model.hpp"
#include "halo2/source_decal_draw.hpp"
#include "halo2/source_campaign_environment_frame_tu15.hpp"
#include "halo2/source_campaign_water_weather_tu15.hpp"
#include "halo2/source_texture_camera_frame_tu15.hpp"
#include "halo2/source_texture_camera_scene_prefix_tu15.hpp"
#include "halo2/renderer.hpp"
#include "halo2/sky.hpp"
#include "halo2/pstencil/campaign_renderer.hpp"
#include "halo2/pstencil/campaign.hpp"
#include <functional>
#include <memory>
namespace halo2 {
class MultiplayerObjectRuntime;
// These presentation values belong to the real object/actor/weapon owner.
// A generic model collector cannot invent an unwritten color channel or a
// shader-function variable. The callback must copy view-dependent source
// inputs while the exact source packet remains published.
struct SourceCampaignSecondaryOwnerInputsTu15 {
    std::optional<ObjectColors> colors;
    ShaderInput shader_input;
};
struct SourceCampaignSecondaryMaterialOwnerTu15 {
    // The returned six-LOD model must already have its materials in the SAME
    // destination Mesh passed to Renderer. No residency mutation during view
    // capture is allowed. Unknown selected definition is an error.
    std::function<const SourceCampaignModel &(std::uint32_t definition)> model;
    std::function<SourceCampaignSecondaryOwnerInputsTu15(
        const SourceLiveModelBaseDrawTu15 &)> presentation;
    const BspLighting *baked_lighting{};
    // Existing source-specific owners may append their own completed units
    // (e.g. SourceCampaignWeapons). They retain their original material/input
    // producers. Return the exact source ordinals consumed, never visibility.
    std::function<std::vector<unsigned>(const SourceCampaignViewFrameTu15 &,
        const SourceModelShaderView &,std::vector<std::vector<Vertex>> &,
        std::vector<ModelObjectDraw> &)> append_specialized;
};
// Owned completed geometry/packets. Exported ModelView spans borrow only this
// object or the immutable BSP/material owners supplied at construction.
// Keep it alive through Renderer::draw_texture_camera. It advances no clock.
struct SourceCampaignSecondaryPacketsTu15 {
    std::unique_ptr<SourceCampaignViewFrameTu15> source;
    SourceCampaignEnvironmentFrameTu15 environment;
    BspFogSample fog;
    SourceViewProjectionBank collection_rendering,submitting_rendering;
    SourceModelShaderView shaders;
    Camera camera;
    std::vector<std::vector<Vertex>> poses;
    std::vector<ModelObjectDraw> objects;
    std::vector<Vertex> background,vertices,reflection_background;
    pstencil::CampaignShadows shadows;
    SourceCampaignWaterWeatherFrameTu15 families;
    std::optional<pstencil::CampaignLightingFrame> lighting;
    // Decal draws retain raw Texture pointers into this immutable owner.
    // The completed packet outlives the synchronous collection class.
    std::shared_ptr<const SourceDecalScene> decal_resources;
    std::vector<SourceDecalDraw> decals;
    // Source13660 BL=textureflag1 clears both lens phase flags before2BCB0.
    // Secondary25800 also precedes ordinary-only20700 request preparation.
    bool lens_phases_source_gated{};
    ModelView view;
};
class SourceCampaignSecondaryViewTu15 {
public:
    SourceCampaignSecondaryViewTu15(const Cache &,std::size_t bsp,
        std::span<const Vertex> immutable_bsp,SourceCampaignViewTu15 &,
        const BspAtmosphere &,const BspSkyModels &,SourceCampaignWaterWeatherTu15 &,
        SourceCampaignWaterMaterialBindingsTu15,
        SourceCampaignSecondaryMaterialOwnerTu15,pstencil::CampaignRenderer * = nullptr);
    // Enumerates the actual occupied original object pool before the target
    // capability is issued. Preparing immutable model/effect resources is a
    // separate phase from source visibility/admission and consumes no RNG.
    std::vector<std::uint32_t> resident_definitions(PhysicsLiveWorldTu15 &)const;
    // Call only inside World::with_texture_camera_scene_prefix_tu15 while its
    // exact packet/camera/environment is published. It collects a fresh view,
    // never copies ordinary visibility, clusters or draw lists. begin_frame
    // is true only for the first view of the shared native render frame.
    std::unique_ptr<SourceCampaignSecondaryPacketsTu15> collect(
        PhysicsLiveWorldTu15 &,SourceCampaignWaterWeatherStateTu15,SourceTextureCameraFrameTu15 &,
        const SourceTextureCameraScenePrefixTu15 &,bool begin_frame,
        std::uint64_t frame,float effect_seconds,bool cinematic);
private:
    const Cache &cache_;
    std::size_t bsp_;
    std::span<const Vertex> bsp_vertices_;
    SourceCampaignViewTu15 &collector_;
    const BspAtmosphere &atmosphere_;
    const BspSkyModels &skies_;
    SourceCampaignWaterWeatherTu15 &families_;
    SourceCampaignWaterMaterialBindingsTu15 water_materials_;
    SourceCampaignSecondaryMaterialOwnerTu15 materials_;
    pstencil::CampaignRenderer *lighting_;
    std::shared_ptr<SourceDecalScene> decals_;
};
}
