#pragma once
#include "halo2/native_tu15_island_lifetime.hpp"
namespace halo2 {
enum class NativeTu15IslandPartitionTemporaryKind {indices,edges};
// Allocation/ownership boundaries for the retained caller ABI bridge. These
// phases describe DATA lifetime, not instruction delivery or a register file.
enum class NativeTu15PartitionTemporaryPhase {
  initialized,allocation_inputs,slow_allocation_table,fast_allocation_begin,fast_allocation_inputs,
  fast_allocation_complete,allocation_complete,data_published,capacity_published,
  release_inputs,release_boundary,slow_release_table,fast_release_inputs,release_capacity,owned_release_allocator,
  owned_release_data
};
struct NativeTu15PartitionTemporaryPhaseData {
  std::uint32_t count{},arena{},available{},bytes{},block{},boundary{},cursor{},flags{},allocator{},table{},data{};
};
struct NativeTu15PartitionTemporaryPhases {
  std::function<void(NativeTu15PartitionTemporaryPhase,
    const NativeTu15PartitionTemporaryPhaseData&)> observe;
};
// Count acquisition occurs after header initialization, preserving a genuine
// caller input cell that can alias source storage. Ordinary typed callers use
// the value-taking API below; the shared DATA algorithm remains identical.
NativeTu15ArrayHeader native_tu15_construct_island_partition_temporary_with_phases(
  SourcePhysicsContactMemory&,NativeTu15ArrayHeader,const std::function<std::uint32_t()>& count,
  NativeTu15IslandPartitionTemporaryKind,NativeTu15PhysicsFrameFp&,
  const NativeTu15IslandLifetimeServices&,const NativeTu15PartitionTemporaryPhases&,
  const SourcePhysicsContactRecordObserver& = {},NativeTu15IslandLifetimeStats* = nullptr);
void native_tu15_destroy_island_partition_temporary_with_phases(SourcePhysicsContactMemory&,
  NativeTu15ArrayHeader,NativeTu15IslandPartitionTemporaryKind,NativeTu15PhysicsFrameFp&,
  const NativeTu15IslandLifetimeServices&,const NativeTu15PartitionTemporaryPhases&,
  const SourcePhysicsContactRecordObserver& = {},NativeTu15IslandLifetimeStats* = nullptr);
// Actual2EBA20/2EB850 (4-byte indices) and3177E0/317870 (8-byte edges).
// Each header is20 bytes of caller-owned DATA; original arena ownership and
// any subsequently grown owned array remain distinct lifetimes.
NativeTu15ArrayHeader native_tu15_construct_island_partition_temporary(SourcePhysicsContactMemory&,
  NativeTu15ArrayHeader,std::uint32_t count,NativeTu15IslandPartitionTemporaryKind,
  NativeTu15PhysicsFrameFp&,const NativeTu15IslandLifetimeServices&,
  const SourcePhysicsContactRecordObserver& = {},NativeTu15IslandLifetimeStats* = nullptr);
void native_tu15_destroy_island_partition_temporary(SourcePhysicsContactMemory&,
  NativeTu15ArrayHeader,NativeTu15IslandPartitionTemporaryKind,NativeTu15PhysicsFrameFp&,
  const NativeTu15IslandLifetimeServices&,const SourcePhysicsContactRecordObserver& = {},
  NativeTu15IslandLifetimeStats* = nullptr);
} // namespace halo2
