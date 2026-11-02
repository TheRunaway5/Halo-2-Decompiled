#pragma once
#include "halo2/native_tu15_island_lifetime.hpp"
namespace halo2 {
struct NativeTu15IslandContactMergeScratch {NativeTu15ArrayHeader temporary;};
struct NativeTu15IslandContactMergeServices {
    NativeTu15IslandLifetimeServices lifetime;
    std::function<void(NativeTu15ArrayHeader,std::uint32_t capacity,std::uint32_t stride,
        NativeTu15PhysicsFrameFp&)> reserve_array;
    std::function<void(std::uint32_t agent,NativeTu15PhysicsVirtualOwner,
        NativeTu15PhysicsFrameFp&)> cleanup_duplicate_agent;
    std::function<void(std::uint32_t manager,NativeTu15PhysicsVirtualOwner,
        NativeTu15PhysicsFrameFp&)> release_duplicate_manager;
};
struct NativeTu15IslandContactMergeStats {
    std::uint64_t calls{},temporary_copies{},comparisons{},first_moves{},second_moves{},
        duplicates{},agent_cleanups{},manager_releases{},array_reserves{},unbound_services{};
    NativeTu15IslandLifetimeStats lifetime;
};
// Entire318DE0 sorted16-byte contact record merge, with native temporary
// storage lifetime. Headers and temporary storage are ordinary source DATA.
// Actual duplicate child virtual owners remain explicit typed dependencies.
void native_tu15_merge_island_contact_records(SourcePhysicsContactMemory&,
    NativeTu15ArrayHeader first,NativeTu15ArrayHeader second,
    NativeTu15IslandContactMergeScratch,NativeTu15PhysicsFrameFp&,
    const NativeTu15IslandContactMergeServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15IslandContactMergeStats* = nullptr);
} // namespace halo2
