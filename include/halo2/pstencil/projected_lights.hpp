#pragma once
#include "halo2/pstencil/projected_light_selection_tu15.hpp"
#include "halo2/pstencil/shared_light_materials.hpp"
#include "halo2/pstencil/point_caster.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/effect_light_view.hpp"

namespace halo2::pstencil {
struct CampaignLightingPassTimings;
struct ProjectedLightFrame {
    std::uint32_t features{};
    unsigned selected_commands{},color_shift{};
    std::vector<EffectLightRawDraw> draws;
};
// Optional current-asset projected-light GPU bridge. Residency is prepared
//before Renderer construction; actual query/geometry/sort/owner/palette/ranges
//and complete native constants feed each draw independently.
class ProjectedLights {
public:
    ProjectedLights(const Cache&,Mesh&,unsigned bsp,std::shared_ptr<SharedLightMaterials>,
        bool preload_raster_materials=true,std::span<const Material> additional_materials={});
    ~ProjectedLights();
    void switch_bsp(Mesh&,unsigned bsp,std::span<const Material> additional_materials={},
        bool preload_raster_materials=true);
    void ensure_raster_materials(Mesh&,std::span<const Material> additional_materials={});
    bool raster_materials_preloaded()const noexcept;
    const std::vector<std::string>& unavailable()const;
    ProjectedLightFrame draw(PhysicsLiveWorldTu15&,const SourceEffectLightViewRecord&,
        float selected_fade,const NativeLightReceiver&,const NativeLightGeometry&,
        const SourceViewProjectionBank&,Vec3 eye,std::array<std::int16_t,4> viewport,
        std::array<std::uint32_t,2> surface,std::vector<PointCaster>&,std::uint32_t domain,
        const std::array<ShaderVector,12>* submitting_constants=nullptr,
        const PresentedModelPalette& presented_palette={},
        // Ordinary (non-stencil) rendering: submit the light's source layer 11
        // alpha pass before its layer 12 response, as 31150 does.
        bool original_alpha_layer=false,
        // The frame's count of light shadow targets in use. Supplied by an
        // ordinary-rendering caller that draws them (31150 feature bit 2);
        // each shadowed light and each cinematic object takes the next one.
        unsigned* shadow_targets=nullptr,
        // Preserve native constants/selections while omitting host raster
        // packets and stencil casters for another lighting backend.
        bool materialize_draws=true,
        // Only pStencil needs the host-side volume caster output.
        bool materialize_point_casters=true,
        CampaignLightingPassTimings* timing=nullptr);
    static constexpr unsigned shadow_target_limit=8;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
