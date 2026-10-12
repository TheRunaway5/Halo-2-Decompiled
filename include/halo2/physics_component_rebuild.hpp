#pragma once
#include "halo2/object_physics_construction.hpp"
#include <array>

namespace halo2 {
struct SourcePhysicsComponentRebuildState {
  std::uint32_t component_pool_slot,object_pool_slot;
  // Actual indexed instruction operands from the selected executable.
  std::uint32_t type_remap,branch_table;
};
struct PhysicsComponentRebuildBranches {
  // Corresponding complete branch labels, verified in each selected image.
  std::uint32_t unit,item,direct_grouping,ordinary,creature;
};
struct PhysicsComponentRebuildImmutableOperands {
  std::uint32_t type_remap_address,branch_table_address;
  std::array<std::uint8_t,13> type_remap;
  std::array<std::uint32_t,6> branch_targets;
  PhysicsComponentRebuildBranches branches;
};
struct PhysicsComponentRebuildCall {
  std::uint32_t instruction,entry;
  // Only defined ABI inputs are exposed; saved/stale register words are not
  // manufactured. 1D9330 consumes four stack words, 1C8430 two.
  std::uint32_t component,object,linear,angular;
  std::array<std::uint32_t,4> arguments;
  unsigned argument_count;
  friend bool operator==(const PhysicsComponentRebuildCall &,
                         const PhysicsComponentRebuildCall &)=default;
};
struct SourcePhysicsComponentRebuildServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Original grouping/type-specific/current-body velocity owners are required
  // when reached. Defined AL results are consumed only at their source sites.
  // No recorded-callback adapter is installed in production.
  std::function<std::uint32_t(const PhysicsComponentRebuildCall &)> invoke;
  // Raw retained-memory adapters may supply these instead of a contiguous
  // span. Both access styles preserve the same original reads and stores.
  std::function<void(std::uint32_t,std::span<std::byte>)> read;
  std::function<void(std::uint32_t,std::span<const std::byte>)> write;
};
// Complete1D9090: grouping call, signed AL branch and retained object flag.
void source_physics_component_rebuild_ordinary(
    const SourcePhysicsComponentRebuildState &,std::uint32_t component,
    const SourcePhysicsComponentRebuildServices &,
    const ObjectPhysicsConstructionObserver & = {});
// Complete1D2B10: actual signed-byte table dispatch, mandatory original
// type-specific rebuild, current velocities, then ordered component flags.
// An unrecognized original indirect destination remains a required boundary.
void source_physics_component_rebuild(
    const SourcePhysicsComponentRebuildState &,std::uint32_t component_datum,
    const PhysicsComponentRebuildBranches &,
    const SourcePhysicsComponentRebuildServices &,
    const ObjectPhysicsConstructionObserver & = {});
} // namespace halo2
