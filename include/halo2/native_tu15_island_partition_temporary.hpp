#pragma once
#include "halo2/native_tu15_island_lifetime.hpp"
namespace halo2 {
enum class NativeTu15IslandPartitionTemporaryKind {indices,edges};
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
