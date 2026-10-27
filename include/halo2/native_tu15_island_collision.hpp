#pragma once
#include "halo2/native_tu15_world_collision.hpp"
namespace halo2 {
struct NativeTu15IslandCollisionServices {
  std::function<void(NativeTu15PhysicsIsland,NativeTu15CollisionDispatcher,
    NativeTu15PhysicsStepConfiguration,NativeTu15PhysicsFrameFp&)> first_phase;
  std::function<void(NativeTu15PhysicsIsland,NativeTu15PhysicsStepConfiguration,
    NativeTu15PhysicsFrameFp&)> flag_set_phase;
  std::function<void(NativeTu15PhysicsIsland,NativeTu15PhysicsStepConfiguration,
    NativeTu15PhysicsFrameFp&)> flag_clear_phase;
};
// Actual149B90 sums the two live allocator record tails, with signed-positive
// counts, reverse stride8 traversal and32-bit wrapping. Empty heaps do not read
// their storage descriptors. No allocation totals are supplied by a fixture.
std::uint32_t native_tu15_island_allocated_bytes(SourcePhysicsContactMemory&);
// Actual282B70/282B90 allocation monitoring and source-order publications.
void native_tu15_begin_island_collision_monitor(SourcePhysicsContactMemory&,
  NativeTu15PhysicsIsland,const SourcePhysicsContactRecordObserver& = {});
void native_tu15_end_island_collision_monitor(SourcePhysicsContactMemory&,
  NativeTu15PhysicsIsland,const SourcePhysicsContactRecordObserver& = {});
// Actual321E50 coordinator. The first phase runs before rereading island+20
// and the resulting world's neutral flag23E. All three remaining phase owners
// are explicit real services; this owner contains no virtual CPU or fallback.
void native_tu15_collide_physics_island(SourcePhysicsContactMemory&,
  NativeTu15PhysicsIsland,NativeTu15CollisionDispatcher,
  NativeTu15PhysicsStepConfiguration,NativeTu15PhysicsFrameFp&,
  const NativeTu15IslandCollisionServices&,
  const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
