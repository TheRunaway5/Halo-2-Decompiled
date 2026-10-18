#pragma once
#include "halo2/effect_light_draw.hpp"
#include "halo2/pstencil/point_caster.hpp"

namespace halo2 {
class Cache;
class PhysicsLiveWorldTu15;
struct Mesh;
struct SourceCampaignViewFrameTu15;
struct SourceViewProjectionBank;
struct SourceModelShaderView;
struct BspFogSample;
namespace pstencil {
struct CampaignLightingInput {
    const SourceViewProjectionBank& camera;
    const SourceModelShaderView& shaders;
    const BspFogSample& fog;
    Vec3 eye,forward;
    float distance_scale;
    std::array<std::int16_t,4> viewport;
    std::array<std::uint32_t,2> surface;
    std::int32_t frame;
    bool cinematic;
};
struct CampaignLightingFrame {
    std::vector<EffectLightRawDraw> draws;
    std::vector<PointCaster> casters;
    unsigned lights{},selected{};
};
// Prepare immutable shader/texture residency before Renderer construction.
// Each draw borrows an actual completed campaign main view, retaining its
// environment, particles and owned base draws before secondary light queries.
class CampaignRenderer {
public:
    CampaignRenderer(const Cache&,Mesh&,unsigned bsp);
    ~CampaignRenderer();
    CampaignLightingFrame draw(PhysicsLiveWorldTu15&,
        const SourceCampaignViewFrameTu15&,const CampaignLightingInput&);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
}
