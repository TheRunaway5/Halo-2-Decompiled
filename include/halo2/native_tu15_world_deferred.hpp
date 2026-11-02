#pragma once
#include "halo2/native_tu15_island_activation.hpp"
namespace halo2 {
struct NativeTu15DeferredResult {std::uint32_t identity{};};
struct NativeTu15WorldDeferredServices {
    std::function<void(NativeTu15PhysicsWorld,NativeTu15PhysicsIsland,
        NativeTu15PhysicsIsland,NativeTu15PhysicsFrameFp&)>merge_islands;
    std::function<void(NativeTu15PhysicsWorld,NativeTu15PhysicsIsland,
        NativeTu15PhysicsFrameFp&)>deactivate_island;
    std::function<void(NativeTu15PhysicsWorld,NativeTu15DeferredResult,
        NativeTu15PhysicsBody,NativeTu15PhysicsFrameFp&)>remove_body;
    std::function<void(NativeTu15PhysicsWorld,NativeTu15DeferredResult,
        NativeTu15PhysicsBody,NativeTu15PhysicsFrameFp&)>remove_update;
    std::function<void(NativeTu15PhysicsBody,NativeTu15PhysicsVirtualOwner,
        std::uint32_t,NativeTu15PhysicsFrameFp&)>destroy_reference;
};
struct NativeTu15WorldDeferredStats {
    std::uint64_t operation_calls{},action_calls{},delete_calls{},update_calls{},
        merges{},deactivations{},body_removals{},update_removals{},
        reference_releases{},destructions{};
};
// Actual282BC0/282BE0 merge-allocation monitors, shared with319750.
void native_tu15_begin_world_merge_monitor(SourcePhysicsContactMemory&,
    NativeTu15PhysicsIsland,const SourcePhysicsContactRecordObserver& = {});
void native_tu15_end_world_merge_monitor(SourcePhysicsContactMemory&,
    NativeTu15PhysicsIsland,const SourcePhysicsContactRecordObserver& = {});
// Actual319750/316E90/319CD0/316E10. Services perform the genuine named work;
// results are never supplied by this queue owner. Data pointers and counts
// remain lazy and are reread after callbacks at the source boundaries.
void native_tu15_drain_world_operations(SourcePhysicsContactMemory&,
    NativeTu15PhysicsWorld,NativeTu15PhysicsFrameFp&,const NativeTu15WorldDeferredServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15WorldDeferredStats* = nullptr);
void native_tu15_drain_world_added_actions(SourcePhysicsContactMemory&,
    NativeTu15PhysicsWorld,NativeTu15PhysicsFrameFp&,const NativeTu15WorldDeferredServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15WorldDeferredStats* = nullptr);
void native_tu15_drain_world_deletes(SourcePhysicsContactMemory&,
    NativeTu15PhysicsWorld,NativeTu15DeferredResult,NativeTu15PhysicsFrameFp&,
    const NativeTu15WorldDeferredServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15WorldDeferredStats* = nullptr);
void native_tu15_drain_world_updates(SourcePhysicsContactMemory&,
    NativeTu15PhysicsWorld,NativeTu15DeferredResult,NativeTu15PhysicsFrameFp&,
    const NativeTu15WorldDeferredServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15WorldDeferredStats* = nullptr);
} // namespace halo2
