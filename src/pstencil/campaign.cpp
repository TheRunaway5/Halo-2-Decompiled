#include "halo2/pstencil/campaign.hpp"
namespace halo2::pstencil {
bool CampaignShadows::add(PhysicsLiveWorldTu15& world,std::uint32_t object,
    float metric,std::span<const Vertex> vertices) {
    if(vertices.empty() || !attempted_.insert(object).second)return false;
    const auto sample=native_incident_tu15(world,object,metric);
    if(!sample)return false;
    casters_.push_back({{vertices.begin(),vertices.end()},sample->sample.incident});
    owners_.push_back({object,sample->owner,sample->sample.incident,vertices.size()/3});
    return true;
}
}
