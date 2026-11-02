#pragma once
#include "halo2/native_tu15_island_regroup.hpp"
#include "halo2/native_tu15_island_partition_allocator_binding.hpp"
namespace halo2 {
struct NativeTu15IslandRegroupBindingActivity {
  std::uint64_t attempts{},completed{},refusals{},unknown_motion_weights{},unknown_constraint_info{},
    unknown_info_updates{},unknown_island_destructors{},unknown_action_enumerators{};
  std::uint64_t constant_info_queries{},extended_info_queries{};
  NativeTu15IslandRegroupStats regroup;
  NativeTu15IslandPartitionAllocatorActivity allocator;
};
// The sole native service composition used by the World regroup phase. Every
// unknown real owner is counted and refused; no original CPU executor exists
// in this interface. Scratch is distinct caller-owned ordinary DATA.
void native_tu15_regroup_physics_islands_with_native_services(SourcePhysicsContactMemory&,
  NativeTu15PhysicsWorld,NativeTu15IslandRegroupScratch,NativeTu15PhysicsFrameFp&,
  const SourcePhysicsContactRecordObserver& = {},NativeTu15IslandRegroupBindingActivity* = nullptr);
} // namespace halo2
