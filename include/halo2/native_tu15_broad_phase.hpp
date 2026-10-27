#pragma once
#include "halo2/native_tu15_island_collision.hpp"
namespace halo2 {
struct NativeTu15BroadPhasePairArray {std::uint32_t identity{};};
struct NativeTu15BroadPhaseScratch {NativeTu15BroadPhasePairArray first,second;};
struct NativeTu15BroadPhaseAllocator {std::uint32_t identity{};};
struct NativeTu15BroadPhaseShape {std::uint32_t identity{};};
struct NativeTu15BroadPhasePairs {std::uint32_t data{},count{};};
struct NativeTu15BroadPhaseServices {
 std::function<std::uint64_t()> timestamp_counter;
 std::function<std::uint32_t(NativeTu15BroadPhaseAllocator,NativeTu15PhysicsVirtualOwner,std::uint32_t,NativeTu15PhysicsFrameFp&)> allocate_block;
 std::function<void(NativeTu15BroadPhaseAllocator,NativeTu15PhysicsVirtualOwner,std::uint32_t,std::uint32_t,NativeTu15PhysicsFrameFp&)> release_block;
 std::function<void(NativeTu15BroadPhaseAllocator,NativeTu15PhysicsVirtualOwner,std::uint32_t,std::uint32_t,std::uint32_t,NativeTu15PhysicsFrameFp&)> release_owned_pairs;
 std::function<void(NativeTu15BroadPhaseShape,NativeTu15PhysicsVirtualOwner,std::uint32_t transform,std::uint32_t margin_bits,std::uint32_t output,NativeTu15PhysicsFrameFp&)> shape_bounds;
 std::function<void(NativeTu15CollisionDispatcher,NativeTu15PhysicsVirtualOwner,std::uint32_t entries,std::uint32_t bounds,std::uint32_t count,NativeTu15BroadPhasePairArray,NativeTu15BroadPhasePairArray,NativeTu15PhysicsFrameFp&)> sweep;
 std::function<void(NativeTu15BroadPhasePairArray,std::uint32_t required_capacity,std::uint32_t stride,NativeTu15PhysicsFrameFp&)> grow_pairs;
 std::function<void(NativeTu15PhysicsIsland,NativeTu15BroadPhasePairs,NativeTu15BroadPhasePairs,NativeTu15PhysicsFrameFp&)> consume_pairs;
};
// Actual3177E0/317870 temporary pair-array lifetime. Each header names20
// ordinary writable bytes supplied by its native caller. Shared fields remain
// memory-backed so aliases and genuine remaining callbacks retain source order.
void native_tu15_construct_broad_phase_pairs(SourcePhysicsContactMemory&,
 NativeTu15BroadPhasePairArray,std::uint32_t reserve,NativeTu15PhysicsFrameFp&,
 const NativeTu15BroadPhaseServices&,const SourcePhysicsContactRecordObserver& = {});
void native_tu15_destroy_broad_phase_pairs(SourcePhysicsContactMemory&,
 NativeTu15BroadPhasePairArray,NativeTu15PhysicsFrameFp&,
 const NativeTu15BroadPhaseServices&,const SourcePhysicsContactRecordObserver& = {});
// Actual321700 broad-phase orchestration. Both scratch headers require owned
// backing; their numeric identities are semantic packets, never CPU frames.
// Shape/sweep/growth/consume/custom allocator owners remain explicit services.
void native_tu15_update_broad_phase(SourcePhysicsContactMemory&,NativeTu15PhysicsIsland,
 NativeTu15CollisionDispatcher,NativeTu15PhysicsStepConfiguration,
 NativeTu15BroadPhaseScratch,NativeTu15PhysicsFrameFp&,unsigned preserved_x87_depth,
 const NativeTu15BroadPhaseServices&,const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
