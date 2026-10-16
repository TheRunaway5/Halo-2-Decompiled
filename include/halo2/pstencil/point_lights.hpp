#pragma once
#include "halo2/world_effects.hpp"
#include "halo2/model_markers.hpp"

namespace halo2::pstencil {
struct PointLightOwner {
    std::uint32_t object,definition;
    Transform root;
    std::span<const Transform> nodes;
    std::span<const std::uint8_t> permutations;
};
// Direct ligh attachments supplement the existing effe light producer only
// in pStencil. Current authored colors/radii and actual object WORLD poses
// feed the same source sphere, spatial query and receiver collectors.
class PointLights {
public:
    PointLights(const Cache&,unsigned bsp);
    ~PointLights();
    bool has(std::uint32_t definition)const;
    void synchronize(std::span<const PointLightOwner>,std::uint64_t tick);
    std::vector<std::uint32_t> definitions()const;
    std::vector<std::string> unavailable()const;
    SourceEffectLightView view(Vec3,const SourceEffectLightCollectionContext&,
        const SourceEffectLightCandidateContext&,std::int32_t frame);
    std::vector<WorldEffectLightReceiver> receivers(const SourceEffectLightView&,
        const SourceBspAdmissionBsp&,std::span<const std::uint8_t>,const SourceBspAdmission*);
    std::vector<WorldEffectLight> lights()const;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
