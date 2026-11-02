#pragma once
#include "halo2/native_tu15_world_collision_listeners.hpp"
namespace halo2 {
// Actual322820/322950 post-integration notifications. These use different
// source cohorts from the post-collision listeners, and world notification
// also visits the current3161F0 world+C0 listener after stable compaction.
void native_tu15_notify_world_step_listeners(SourcePhysicsContactMemory&,
    NativeTu15PhysicsWorld,NativeTu15PhysicsStepPacket,NativeTu15PhysicsFrameFp&,
    const NativeTu15WorldCollisionListenerServices&,
    const SourcePhysicsContactRecordObserver& = {});
void native_tu15_notify_island_step_listeners(SourcePhysicsContactMemory&,
    NativeTu15PhysicsWorld,NativeTu15PhysicsIsland,NativeTu15PhysicsStepPacket,
    NativeTu15PhysicsFrameFp&,const NativeTu15WorldCollisionListenerServices&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
