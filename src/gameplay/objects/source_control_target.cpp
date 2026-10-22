#include "halo2/source_control_target.hpp"
#include "halo2/math.hpp"
#include "halo2/object_light_spatial.hpp"
#include "halo2/source_object_spatial_query.hpp"
#include <cmath>

namespace halo2 {
namespace {
bool finite(Vec3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
}
std::vector<std::uint32_t> source_control_spatial_candidates(
    const SourceControlTargetQuery &q,const SourceObjectLocation &location,
    const ObjectLightSpatialBsp &bsp,const SourceObjectSpatialPool &collidable,
    const SourceObjectSpatialPool &noncollidable,SourceObjectSpatialQueryState &state) {
    if(q.unit_parent!=0xffffffffu)return {};
    if(!std::isfinite(q.unit_bound_radius) || q.unit_bound_radius<0)
        throw FormatError("invalid source interaction query radius");
    SourceObjectSpatialQueryInput input;input.types=0xffffebcfu;
    input.location=location;input.center=q.unit_bound_center;
    input.radius=(q.unit_bound_radius+.4f)+3.f;
    return source_object_spatial_query(bsp,collidable,noncollidable,input,state);
}
bool source_device_usable(const SourceDeviceGroup *position,
                          const SourceDeviceGroup *power,std::uint32_t flags){
    if(!position)return false;
    if(!power)throw FormatError("source control has no actual power group");
    return (position->flags&3u)!=3u && !(flags&2u) && power->value==1.f;
}
bool source_control_side_allowed(bool typed,std::uint32_t flags,
    std::uint16_t count,Vec3 marker,Vec3 forward){
    if(!typed || (flags&1u))return true;
    if(count>1)throw FormatError("control marker query exceeds original maximum1");
    if(count!=1)return true;
    if(!finite(marker) || !finite(forward))throw FormatError("nonfinite control marker direction");
    return !((forward.y*marker.y+forward.z*marker.z)+forward.x*marker.x>0.f);
}
std::uint32_t source_control_initial_flags(std::uint32_t flags){
    return (flags&1u)|((flags&0x10u)>>3);
}
bool source_player_action_target_replace(SourcePlayerActionTarget &current,
    const SourcePlayerActionTarget &candidate,Vec3 unit,Vec3 previous,Vec3 next){
    if(candidate.action<current.action)return false;
    if(candidate.action==current.action){
        if(!finite(unit)||!finite(previous)||!finite(next))
            throw FormatError("nonfinite source interaction root position");
        const auto a=next-unit,b=previous-unit;
        const float candidate_distance=(a.z*a.z+a.x*a.x)+a.y*a.y;
        const float current_distance=(b.z*b.z+b.y*b.y)+b.x*b.x;
        if(current_distance<=candidate_distance)return false;
    }
    current=candidate;return true;
}
bool source_control_target_candidate(const SourceControlTargetQuery &q,
    const SourceControlTargetCandidate &c,SourcePlayerActionTarget &current,
    Vec3 current_root){
    if(q.unit_parent!=0xffffffffu)return false;
    if(c.object==0xffffffffu)throw FormatError("control candidate has NONE identity");
    if(!finite(q.unit_bound_center)||!finite(c.bound_center)||
       !std::isfinite(q.unit_bound_radius)||!std::isfinite(c.bound_radius)||
       q.unit_bound_radius<0 || c.bound_radius<0 ||
       !finite(q.unit_center)||!finite(q.unit_forward))
        throw FormatError("invalid source control candidate geometry");
    //151110 excludes controls beyond (candidate+3C + (unit+3C + .4)).
    // Its BB050 radius adds a further3.0, which belongs to the spatial query.
    const float unit_radius=q.unit_bound_radius+.4f;
    const float radius=c.bound_radius+unit_radius;
    const auto delta=c.bound_center-q.unit_bound_center;
    const float squared=(delta.x*delta.x+delta.z*delta.z)+delta.y*delta.y;
    if(!(squared<=radius*radius))return false;
    return source_control_action_candidate(q,c,current,current_root);
}
bool source_control_action_candidate(const SourceControlTargetQuery &q,
    const SourceControlTargetCandidate &c,SourcePlayerActionTarget &current,
    Vec3 current_root){
    if(c.object==0xffffffffu || !finite(c.bound_center)||
       !std::isfinite(c.bound_radius)||c.bound_radius<0 ||
       !finite(q.unit_center)||!finite(q.unit_forward))
        throw FormatError("invalid source control action candidate geometry");
    if(!source_object_light_segment_intersects(q.unit_center,q.unit_forward,
                                               c.bound_center,c.bound_radius))return false;
    if(!source_control_side_allowed(true,c.control_flags,c.marker_count,
                                    c.marker_forward,q.unit_forward))return false;
    if(!source_device_usable(c.position,c.power,c.device_flags))return false;
    return source_player_action_target_replace(current,{3,0,c.object},
                                               q.unit_root,current_root,c.root);
}
}
