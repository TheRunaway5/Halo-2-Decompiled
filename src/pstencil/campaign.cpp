#include "halo2/pstencil/campaign.hpp"
#include <algorithm>
namespace halo2::pstencil {
bool CampaignShadows::add(PhysicsLiveWorldTu15& world,std::uint32_t object,
    float metric,std::span<const Vertex> vertices,std::optional<Vec3> sun,float sun_extrusion) {
    if(vertices.empty())return false;
    const auto [slot,first]=attempted_.try_emplace(object,declined);
    if(!first) {
        if(slot->second==declined)return false;
        auto& caster=casters_[slot->second];
        caster.vertices.insert(caster.vertices.end(),vertices.begin(),vertices.end());
        if(caster.sun && sun)caster.extrusion=std::max(caster.extrusion,sun_extrusion);
        owners_[slot->second].triangles+=vertices.size()/3;
        return true;
    }
    if(sun) {
        // The sky-selected incident has the same layer-6 object eligibility
        // as a native surface sample; choosing the sun must not bypass it.
        if(!native_lightmap_shadow_eligible_tu15(world,object))return false;
        slot->second=casters_.size();
        casters_.push_back({{vertices.begin(),vertices.end()},*sun,true,sun_extrusion});
        owners_.push_back({object,object,*sun,vertices.size()/3});
        return true;
    }
    const auto sample=native_incident_tu15(world,object,metric);
    if(!sample)return false;
    slot->second=casters_.size();
    casters_.push_back({{vertices.begin(),vertices.end()},sample->sample.incident});
    owners_.push_back({object,sample->owner,sample->sample.incident,vertices.size()/3});
    return true;
}
}
