#pragma once
#include "halo2/native_tu15_island_partition_build.hpp"
namespace halo2 {
struct NativeTu15IslandPartitionEdgeScratch {NativeTu15ArrayHeader action_bodies;std::uint32_t inline_bodies{};};
struct NativeTu15IslandPartitionEdgeServices {
  std::function<void(NativeTu15ArrayHeader,std::uint32_t capacity,std::uint32_t stride,
    NativeTu15PhysicsFrameFp&)>reserve_array;
  NativeTu15ArrayGrowthServices allocation;
  std::function<void(std::uint32_t action,NativeTu15PhysicsVirtualOwner,
    NativeTu15ArrayHeader bodies,NativeTu15PhysicsFrameFp&)>enumerate_action_bodies;
};
struct NativeTu15IslandPartitionEdgeStats {
  std::uint64_t calls{},actions{},action_pairs{},constraint_pairs{},contact_pairs{},array_reserves{},
    action_array_releases{},unbound_services{};
};
// Entire321EB0. Derive index edges directly from current dynamic action,
// constraint and contact operands, retaining source order and late rereads.
// Action body enumeration remains its actual typed virtual owner requirement.
void native_tu15_collect_island_partition_edges(SourcePhysicsContactMemory&,NativeTu15PhysicsIsland,
  NativeTu15ArrayHeader output,NativeTu15IslandPartitionEdgeScratch,NativeTu15PhysicsFrameFp&,
  const NativeTu15IslandPartitionEdgeServices&,const SourcePhysicsContactRecordObserver& = {},
  NativeTu15IslandPartitionEdgeStats* = nullptr);
} // namespace halo2
