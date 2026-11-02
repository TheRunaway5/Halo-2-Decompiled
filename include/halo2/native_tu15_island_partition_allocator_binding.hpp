#pragma once
#include "halo2/native_tu15_island_partition_build.hpp"
#include "halo2/native_tu15_island_lifetime.hpp"
#include "halo2/native_tu15_raw_allocator.hpp"
namespace halo2 {
struct NativeTu15IslandPartitionAllocatorActivity {
  std::uint64_t array_allocations{},array_releases{},pool_allocations{},pool_releases{},
    arena_allocations{},arena_releases{},unbound_services{};
  NativeTu15RawAllocatorStats raw;
};
// Explicit caller-owned native service composition. Returned callbacks require
// this object to outlive them. No executor, thread-local binding or GP/SIMD
// packet is retained. Unknown actual owners are counted and throw.
class NativeTu15IslandPartitionAllocatorBinding {
  SourcePhysicsContactMemory&memory_;SourcePhysicsContactRecordObserver observer_;
  NativeTu15IslandPartitionAllocatorActivity*activity_;
  [[noreturn]]void unbound(const char*);
  std::uint32_t pool_allocate(const NativeTu15ArrayAllocationRequest&,NativeTu15PhysicsFrameFp&);
  void pool_release(const NativeTu15ArrayReleaseRequest&,NativeTu15PhysicsFrameFp&);
  NativeTu15RawAllocatorServices raw_services();
public:
  NativeTu15IslandPartitionAllocatorBinding(SourcePhysicsContactMemory&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15IslandPartitionAllocatorActivity* = nullptr);
  NativeTu15IslandPartitionAllocatorBinding(const NativeTu15IslandPartitionAllocatorBinding&)=delete;
  NativeTu15IslandPartitionAllocatorBinding&operator=(const NativeTu15IslandPartitionAllocatorBinding&)=delete;
  NativeTu15ArrayGrowthServices array_services();
  NativeTu15IslandLifetimeServices lifetime_services();
  NativeTu15IslandPartitionBuildServices graph_services();
};
} // namespace halo2
