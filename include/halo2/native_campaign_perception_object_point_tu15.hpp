#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
// Real object marker/pose snapshot consumers.1EBCB0 publishes orientation and
// component velocity;26DDD0 preserves its unwritten orientation and uses the
// root object's stored velocity. They deliberately keep distinct semantics.
void native_campaign_perception_object_point_tu15(SourcePhysicsContactMemory &,
    std::uint32_t object,std::uint32_t destination,NativeTu15PhysicsFrameFp &);
void native_campaign_perception_object_state_tu15(SourcePhysicsContactMemory &,
    std::uint32_t object,std::uint32_t destination,NativeTu15PhysicsFrameFp &);
void native_campaign_perception_attachment_point_tu15(SourcePhysicsContactMemory &,
    std::uint32_t object,std::uint32_t destination,NativeTu15PhysicsFrameFp &);
bool native_campaign_perception_cluster_point_tu15(SourcePhysicsContactMemory &,
    std::uint32_t cluster,std::uint32_t query_point,std::uint32_t destination,NativeTu15PhysicsFrameFp &);
bool native_campaign_perception_actor_point_tu15(SourcePhysicsContactMemory &,
    std::uint32_t actor,std::uint32_t query_point,std::uint32_t destination,NativeTu15PhysicsFrameFp &);
}
