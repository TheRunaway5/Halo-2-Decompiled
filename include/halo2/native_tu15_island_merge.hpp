#pragma once
#include "halo2/native_tu15_array_reserve.hpp"
#include "halo2/native_tu15_island_activation.hpp"
namespace halo2 {
struct NativeTu15IslandMergeServices {
    // Actual316D40 receives source world/island DATA. The shared native
    // activation binding owns its actual agents/listeners/constraint services.
    std::function<void(NativeTu15PhysicsWorld,NativeTu15PhysicsIsland,
        NativeTu15PhysicsFrameFp&)> activate_island;
    // Actual318DE0(ESI=first_header,stack=second_header), ordinary DATA only.
    std::function<void(NativeTu15ArrayHeader,NativeTu15ArrayHeader,
        NativeTu15PhysicsFrameFp&)> merge_contact_records;
    // Actual2E31E0. Bind to the qualified native reserve/allocator composition;
    // capacity is source-calculated DATA, never an allocator result.
    std::function<void(NativeTu15ArrayHeader,std::uint32_t capacity,std::uint32_t stride,
        NativeTu15PhysicsFrameFp&)> reserve_array;
    // Current island virtual0 owner, actual disposing flag1. No postimages.
    std::function<void(NativeTu15PhysicsIsland,NativeTu15PhysicsVirtualOwner,
        std::uint32_t disposing,NativeTu15PhysicsFrameFp&)> destroy_island;
};
struct NativeTu15IslandMergeStats {
    std::uint64_t calls{},same_island_skips{},island_activations{},contact_merges{},
        array_reserves{},moved_bodies{},moved_actions{},moved_constraints{},
        moved_rows{},island_destructions{},unbound_services{};
};
// Complete source3190D0 integer algorithm. Return ordinary surviving island
// identity. Services retain the original read/callback/publication phases.
// Once selected, no execution can replay or fall back to the original parent.
NativeTu15PhysicsIsland native_tu15_merge_physics_islands(SourcePhysicsContactMemory&,
    NativeTu15PhysicsWorld,NativeTu15PhysicsIsland first,NativeTu15PhysicsIsland second,
    NativeTu15PhysicsFrameFp&,const NativeTu15IslandMergeServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15IslandMergeStats* = nullptr);
} // namespace halo2
