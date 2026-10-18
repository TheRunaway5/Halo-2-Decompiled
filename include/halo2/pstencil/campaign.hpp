#pragma once
#include "halo2/pstencil/lightmap.hpp"
#include "halo2/pstencil/native_incident.hpp"
#include <set>

namespace halo2::pstencil {
struct CampaignShadowOwner {
    std::uint32_t object{},sample_owner{};
    Vec3 incident;
    std::size_t triangles{};
};
// One completed camera frame. Poses belong to the campaign's actual model
// collector; the distant-light sample belongs to the native lighting cache.
// No shadow is supplied when either producer declines its input.
class CampaignShadows {
public:
    bool add(PhysicsLiveWorldTu15&,std::uint32_t object,float lighting_metric,
        std::span<const Vertex> posed_vertices);
    std::span<const LightmapCaster> casters()const{return casters_;}
    std::span<const CampaignShadowOwner> owners()const{return owners_;}
    std::size_t attempted()const{return attempted_.size();}
private:
    std::set<std::uint32_t> attempted_;
    std::vector<LightmapCaster> casters_;
    std::vector<CampaignShadowOwner> owners_;
};
}
