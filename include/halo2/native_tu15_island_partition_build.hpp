#pragma once
#include "halo2/native_tu15_island_partition_graph.hpp"
#include "halo2/native_tu15_array_growth.hpp"
#include "halo2/native_tu15_island_integration.hpp"
namespace halo2 {
struct NativeTu15IslandPartitionBuildServices {
  std::function<void(NativeTu15ArrayHeader,std::uint32_t capacity,std::uint32_t stride,
    NativeTu15PhysicsFrameFp&)> reserve_array;
  std::function<std::uint32_t(std::uint32_t allocator,NativeTu15PhysicsVirtualOwner,
    std::uint32_t bytes,std::uint32_t alignment,NativeTu15PhysicsFrameFp&)> allocate_group;
  std::function<void(std::uint32_t allocator,NativeTu15PhysicsVirtualOwner,
    std::uint32_t data,NativeTu15PhysicsFrameFp&)> release_group;
  std::function<void(const NativeTu15ArrayReleaseRequest&,NativeTu15PhysicsFrameFp&)> release_array;
};
struct NativeTu15IslandPartitionBuildStats {
  std::uint64_t builds{},extractions{},destructions{},array_reserves{},group_allocations{},
    group_releases{},array_releases{},unbound_services{};
  NativeTu15IslandPartitionGraphStats graph;
};
// Actual37C2D0: source arrays of8-byte index edges ->native union-find and
// per-edge roots. No graph topology/result is supplied by a service callback.
void native_tu15_build_island_partition_graph(SourcePhysicsContactMemory&,NativeTu15IslandPartitionGraph,
  NativeTu15ArrayHeader edges,std::uint32_t vertices,NativeTu15PhysicsFrameFp&,
  const NativeTu15IslandPartitionBuildServices&,const SourcePhysicsContactRecordObserver& = {},
  NativeTu15IslandPartitionBuildStats* = nullptr);
// Actual37C460/37C3D0:12-byte group records and owned index-array lifetime.
// Real allocator virtual owners remain typed requirements until composed.
void native_tu15_extract_island_partition_groups(SourcePhysicsContactMemory&,NativeTu15IslandPartitionGraph,
  NativeTu15ArrayHeader groups,NativeTu15PhysicsFrameFp&,const NativeTu15IslandPartitionBuildServices&,
  const SourcePhysicsContactRecordObserver& = {},NativeTu15IslandPartitionBuildStats* = nullptr);
void native_tu15_destroy_island_partition_groups(SourcePhysicsContactMemory&,NativeTu15ArrayHeader,
  NativeTu15PhysicsFrameFp&,const NativeTu15IslandPartitionBuildServices&,
  const SourcePhysicsContactRecordObserver& = {},NativeTu15IslandPartitionBuildStats* = nullptr);
} // namespace halo2
