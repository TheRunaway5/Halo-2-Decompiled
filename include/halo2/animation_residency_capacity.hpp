#pragma once
#include "halo2/unit_animation_resolution.hpp"
#include <functional>
namespace halo2 {
struct SourceAnimationCapacityBindings {
  // Actual selected-image/runtime address tokens; none is a readiness default.
  std::uint32_t cache_pointer_slot, physical_index, physical_lower_table,
      physical_upper_table, requested_bytes_slot, block_capacity_slot,
      backing_pointer_slot;
};
struct SourceAnimationCapacityServices {
  SourceAnimationResourceMemory memory;
  SourceAnimationCapacityBindings bindings;
  // Complete opaque OS boundary: apply protection to real existing backing.
  // Original2D7A1A/ordinal182 is void; no successful status is synthesized.
  std::function<void(std::uint32_t base, std::uint32_t bytes,
                     std::uint32_t protection)> protect;
  // Required complete13EFA0 when actual occupied cache entry exceeds capacity.
  std::function<void(std::uint32_t cache, std::uint32_t full_datum)> retire;
};
// Complete124980, including arena cursor ownership and13F020 coupling.
// mode must be the actual owning caller's value (e.g. original1908E0 output).
// No active flag, loaded descriptor, body field or IO completion is produced.
void configure_source_animation_capacity(
    std::int32_t mode, const SourceAnimationCapacityServices &,
    const AnimationResourceObserver & = {});
// Complete13F020 over actual retained cache/pool/bitmap/entry storage.
// Missing retirement ownership stops at its real branch after earlier stores.
void resize_source_animation_cache(
    std::uint32_t cache, std::uint32_t block_capacity,
    const SourceAnimationCapacityServices &,
    const AnimationResourceObserver & = {});
} // namespace halo2
