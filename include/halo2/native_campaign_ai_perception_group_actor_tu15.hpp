#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
//274750: insert a real property into the actor list, resolve its actual target
// point, and publish the source x87 distance. FP controls/status are explicit;
// caller-owned live x87 values remain outside this balanced arithmetic kernel.
void native_campaign_ai_perception_property_link_actor_tu15(
    SourcePhysicsContactMemory &,std::uint32_t actor,std::uint32_t property,
    NativeTu15PhysicsFrameFp &,unsigned occupied_x87=0);
//274670: prepend the group and allocate/link a real property for each actor.
// A refused property allocation skips that actor, as the original does.
void native_campaign_ai_perception_group_link_actors_tu15(
    SourcePhysicsContactMemory &,std::uint32_t clump_record,std::uint32_t group,
    NativeTu15PhysicsFrameFp &,unsigned occupied_x87=0);
}
