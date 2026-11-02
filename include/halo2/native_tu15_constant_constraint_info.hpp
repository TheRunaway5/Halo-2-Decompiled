#pragma once
#include "halo2/native_tu15_island_partition_metadata.hpp"
namespace halo2 {
// Entire public31D630. The receiver is unused. Preserve source store order
//0,+8,+4,+C; original EAX is the supplied ordinary info identity,RET4.
std::uint32_t native_tu15_get_constant_constraint_info(SourcePhysicsContactMemory&,
  NativeTu15IslandConstraintInfo,const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
