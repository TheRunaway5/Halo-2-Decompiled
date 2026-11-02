#pragma once
#include "halo2/native_tu15_island_activation.hpp"
namespace halo2 {
struct NativeTu15IslandConstraintInfo {std::uint32_t identity{};};
struct NativeTu15IslandPartitionMetadataStats {std::uint64_t additions{},removals{},weight_updates{};};
// Actual3200B0/320100. The ordinary16-byte info packet is read at its true
// source offsets; removal consumes only+4/+8/+C. No saved GP/padding values
// represent packet DATA. The unused constraint formal argument has no effect.
void native_tu15_add_island_constraint_info(SourcePhysicsContactMemory&,NativeTu15PhysicsIsland,
  NativeTu15IslandConstraintInfo,const SourcePhysicsContactRecordObserver& = {},
  NativeTu15IslandPartitionMetadataStats* = nullptr);
void native_tu15_remove_island_constraint_info(SourcePhysicsContactMemory&,NativeTu15PhysicsIsland,
  NativeTu15IslandConstraintInfo,const SourcePhysicsContactRecordObserver& = {},
  NativeTu15IslandPartitionMetadataStats* = nullptr);
// Actual320130,32-bit wrapping accumulation; actual323420 returns128.
void native_tu15_add_island_partition_weight(SourcePhysicsContactMemory&,NativeTu15PhysicsIsland,
  std::uint32_t weight,const SourcePhysicsContactRecordObserver& = {},
  NativeTu15IslandPartitionMetadataStats* = nullptr);
std::uint32_t native_tu15_default_motion_partition_weight() noexcept;
} // namespace halo2
