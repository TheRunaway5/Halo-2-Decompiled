#pragma once
#include "halo2/native_tu15_island_merge.hpp"
namespace halo2 {
struct NativeTu15IslandLifetimeServices {
    NativeTu15ArrayGrowthServices allocation;
    std::function<std::uint32_t(std::uint32_t arena,NativeTu15PhysicsVirtualOwner,
        std::uint32_t bytes,NativeTu15PhysicsFrameFp&)> allocate_arena;
    std::function<void(std::uint32_t arena,NativeTu15PhysicsVirtualOwner,
        std::uint32_t block,std::uint32_t bytes,NativeTu15PhysicsFrameFp&)> release_arena;
};
struct NativeTu15IslandLifetimeStats {
    std::uint64_t temporary_constructors{},temporary_destructors{},fast_allocations{},
        slow_allocations{},fast_releases{},slow_releases{},owned_array_releases{},
        island_destructors{},island_disposals{},unbound_services{};
};
// Actual317A40/317AD0. Header is20 bytes of caller-owned ordinary DATA.
void native_tu15_construct_island_temporary_records(SourcePhysicsContactMemory&,
    NativeTu15ArrayHeader,std::uint32_t count,NativeTu15PhysicsFrameFp&,
    const NativeTu15IslandLifetimeServices&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15IslandLifetimeStats* = nullptr);
void native_tu15_destroy_island_temporary_records(SourcePhysicsContactMemory&,
    NativeTu15ArrayHeader,NativeTu15PhysicsFrameFp&,const NativeTu15IslandLifetimeServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15IslandLifetimeStats* = nullptr);
// Actual322100 ->3213A0, with real owned-array and island allocation lifetime.
// Return ordinary object DATA. Unknown allocator owners are typed requirements.
std::uint32_t native_tu15_destroy_physics_island(SourcePhysicsContactMemory&,
    NativeTu15PhysicsIsland,std::uint32_t disposing,NativeTu15PhysicsFrameFp&,
    const NativeTu15IslandLifetimeServices&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15IslandLifetimeStats* = nullptr);
} // namespace halo2
