#pragma once
#include "halo2/native_tu15_contact_collector_info.hpp"
namespace halo2 {
// Entire public31F840. Reuses the sole325E90 semantic info owner, then adds
//0x88 to current info+8 with32-bit wrapping. Original EAX retains the supplied
// info identity; no GP/state postimage is reconstructed. FP is untouched.
std::uint32_t native_tu15_get_extended_constraint_info(SourcePhysicsContactMemory&,
  std::uint32_t constraint,NativeTu15IslandConstraintInfo,
  const SourcePhysicsContactRecordObserver& = {});
// Entire public31F740. The conveyor constraint extends the same325E90
// current info+8 by0x28, retaining its source store site and RET4 contract.
std::uint32_t native_tu15_get_conveyor_constraint_info(SourcePhysicsContactMemory&,
  std::uint32_t constraint,NativeTu15IslandConstraintInfo,
  const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
