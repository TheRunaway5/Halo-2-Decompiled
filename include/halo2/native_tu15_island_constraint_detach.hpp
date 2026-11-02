#pragma once
#include "halo2/native_tu15_island_partition_metadata.hpp"
namespace halo2 {
struct NativeTu15IslandConstraintDetachServices {
  std::function<void(std::uint32_t constraint,NativeTu15PhysicsVirtualOwner,
    NativeTu15IslandConstraintInfo,NativeTu15PhysicsFrameFp&)>get_info;
  std::function<void(NativeTu15PhysicsIsland,NativeTu15PhysicsVirtualOwner,std::uint32_t constraint,
    NativeTu15IslandConstraintInfo,NativeTu15PhysicsFrameFp&)>remove_info;
  std::function<void(std::uint32_t constraint,NativeTu15PhysicsVirtualOwner,
    std::uint32_t disposing,NativeTu15PhysicsFrameFp&)>destroy_constraint;
};
struct NativeTu15IslandConstraintDetachStats {
  std::uint64_t calls{},shifted_entries{},info_queries{},native_info_queries{},metadata_removals{},
    native_metadata_removals{},reference_releases{},protected_queries{},protected_references{},
    destructions{},unbound_services{};
};
// Entire3201B0. The caller owns16 bytes of ordinary source DATA for info;
// no GP-shaped scratch packet or parent replay supplies a getter result.
void native_tu15_detach_island_constraint(SourcePhysicsContactMemory&,NativeTu15PhysicsIsland,
  std::uint32_t constraint,NativeTu15IslandConstraintInfo scratch,NativeTu15PhysicsFrameFp&,
  const NativeTu15IslandConstraintDetachServices&,const SourcePhysicsContactRecordObserver& = {},
  NativeTu15IslandConstraintDetachStats* = nullptr);
} // namespace halo2
