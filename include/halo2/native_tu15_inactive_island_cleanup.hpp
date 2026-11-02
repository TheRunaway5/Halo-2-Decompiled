#pragma once
#include "halo2/native_tu15_island_integration.hpp"
namespace halo2 {
struct NativeTu15InactiveIslandCleanupServices {
    std::function<void(std::uint32_t,NativeTu15PhysicsVirtualOwner,NativeTu15PhysicsFrameFp&)>invalidate_agent;
    std::function<void(std::uint32_t,NativeTu15PhysicsVirtualOwner,NativeTu15PhysicsFrameFp&)>invalidate_manager;
    std::function<std::uint32_t(std::uint32_t,NativeTu15PhysicsVirtualOwner,NativeTu15PhysicsFrameFp&)>policy_kind;
    std::function<void(NativeTu15PhysicsBody,std::uint32_t,NativeTu15PhysicsFrameFp&)>select_policy;
};
struct NativeTu15InactiveIslandCleanupStats {
    std::uint64_t calls{},invalidated_agents{},invalidated_managers{},cleared_pairs{},policy_queries{},policy_selections{};
};
// Actual282AB0. Source pair/body arrays and counts are reread across callbacks;
// the manager identity is retained before agent invalidation as in TU1.5.
void native_tu15_cleanup_inactive_island(SourcePhysicsContactMemory&,NativeTu15PhysicsIsland,
    NativeTu15PhysicsFrameFp&,const NativeTu15InactiveIslandCleanupServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15InactiveIslandCleanupStats* = nullptr);
} // namespace halo2
