#pragma once
#include "halo2/native_tu15_island_integration.hpp"
namespace halo2 {
struct NativeTu15CollisionDispatcher {std::uint32_t identity{};};
struct NativeTu15WorldCollisionServices {
  std::function<std::uint64_t()> timestamp_counter;
  std::function<void(NativeTu15PhysicsWorld,NativeTu15PhysicsFrameFp&)> drain_operations;
  std::function<void(NativeTu15PhysicsIsland,NativeTu15CollisionDispatcher,
    NativeTu15PhysicsStepConfiguration,NativeTu15PhysicsFrameFp&)> collide_island;
  std::function<void(NativeTu15PhysicsWorld,NativeTu15PhysicsIsland,
    NativeTu15PhysicsStepPacket,NativeTu15PhysicsFrameFp&)> notify_island_listeners;
  std::function<void(NativeTu15PhysicsWorld,NativeTu15PhysicsFrameFp&)> drain_deletes;
  std::function<void(NativeTu15PhysicsWorld,NativeTu15PhysicsFrameFp&)> drain_updates;
  std::function<void(NativeTu15PhysicsWorld,NativeTu15PhysicsStepPacket,
    NativeTu15PhysicsFrameFp&)> notify_world_listeners;
};
struct NativeTu15WorldCollisionStats {
  std::uint64_t worlds{},islands{},operation_drains{},delete_drains{},update_drains{},island_notifications{},world_notifications{};
};
// Actual319D50 world collision scheduling, retained configuration/step scaling,
// mutable forward island traversal, phase flags and callback/profile order.
// Island collision321E50 and listener/deferred-operation owners remain explicit
// actual services. This typed owner has no virtual CPU or reference outcomes.
void native_tu15_collide_physics_world(SourcePhysicsContactMemory&,
  NativeTu15PhysicsWorld,NativeTu15PhysicsStepPacket,NativeTu15PhysicsFrameFp&,
  unsigned actual_preserved_x87_depth,const NativeTu15WorldCollisionServices&,
  const SourcePhysicsContactRecordObserver& = {},NativeTu15WorldCollisionStats* = nullptr);
} // namespace halo2
