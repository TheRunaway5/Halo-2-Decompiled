#pragma once
#include "halo2/native_tu15_world_collision.hpp"
namespace halo2 {
struct NativeTu15WorldCollisionListener {std::uint32_t identity{};};
struct NativeTu15WorldCollisionListenerServices {
  std::function<void(NativeTu15WorldCollisionListener,NativeTu15PhysicsVirtualOwner,
    NativeTu15PhysicsWorld,NativeTu15PhysicsStepPacket,NativeTu15PhysicsFrameFp&)> notify;
  std::function<void(NativeTu15WorldCollisionListener,NativeTu15PhysicsVirtualOwner,
    NativeTu15PhysicsIsland,NativeTu15PhysicsStepPacket,NativeTu15PhysicsFrameFp&)> notify_island;
};
// Actual3228C0 reverse post-collision notification and stable null compaction.
// Callback receivers/methods and mutable array/count fields are read at their
// original phases. Custom callback owners remain explicit real services.
void native_tu15_notify_world_collision_listeners(SourcePhysicsContactMemory&,
  NativeTu15PhysicsWorld,NativeTu15PhysicsStepPacket,NativeTu15PhysicsFrameFp&,
  const NativeTu15WorldCollisionListenerServices&,
  const SourcePhysicsContactRecordObserver& = {});
// Actual3229E0 counterpart for the world's island post-collision listeners.
void native_tu15_notify_island_collision_listeners(SourcePhysicsContactMemory&,
  NativeTu15PhysicsWorld,NativeTu15PhysicsIsland,NativeTu15PhysicsStepPacket,
  NativeTu15PhysicsFrameFp&,const NativeTu15WorldCollisionListenerServices&,
  const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
