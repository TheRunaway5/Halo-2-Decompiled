#pragma once
#include "halo2/pstencil/projected_light_selection_tu15.hpp"
#include "halo2/pstencil/shared_light_materials.hpp"
#include "halo2/pstencil/point_caster.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/effect_light_view.hpp"

namespace halo2::pstencil {
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
    ProjectedLights(const Cache&,Mesh&,unsigned bsp,std::shared_ptr<SharedLightMaterials>);
    ~ProjectedLights();
    const std::vector<std::string>& unavailable()const;
    ProjectedLightFrame draw(PhysicsLiveWorldTu15&,const SourceEffectLightViewRecord&,
        float selected_fade,const NativeLightReceiver&,const NativeLightGeometry&,
        const SourceViewProjectionBank&,Vec3 eye,std::array<std::int16_t,4> viewport,
        std::array<std::uint32_t,2> surface,std::vector<PointCaster>&,std::uint32_t domain);
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
