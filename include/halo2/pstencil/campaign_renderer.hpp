#pragma once
#include "halo2/effect_light_draw.hpp"
#include "halo2/pstencil/point_caster.hpp"
#include "halo2/path_tracing/scene.hpp"

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
    bool path_tracing{};
    // Completed source12F80 bank from submitting camera2. Collection and
    // receiver admission retain camera above. Absent preserves old callers.
    const std::array<ShaderVector,12>* submitting_constants{};
};
struct CampaignLightingFrame {
    std::vector<EffectLightRawDraw> draws;
    std::vector<PointCaster> casters;
    unsigned lights{},selected{};
    std::vector<path_tracing::Light> path_tracing_lights;
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
