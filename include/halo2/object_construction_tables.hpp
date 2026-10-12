#pragma once
#include "halo2/object_type_construction.hpp"
#include <array>
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace halo2 {
// Only authenticated immutable fields consumed by the complete type-dispatch
// owners. Source addresses remain opaque; no callback is executable on host.
struct SourceObjectConstructionTables {
  std::uint32_t bank;
  std::array<std::uint32_t,13> type_addresses;
  std::map<std::uint32_t,std::vector<std::byte>> ranges;
  // Separately authenticated selected-image record+8 words. General read()
  // retains its prior narrow field ranges; no adjacent type bytes are exposed.
  std::optional<std::array<std::uint16_t,13>> base_extents;
  // Independent record+0E placement words used by D6660. These do not
  // widen the general-purpose immutable read ranges.
  std::optional<std::array<std::uint16_t,13>> placement_extents;
  std::optional<std::array<std::array<std::int16_t,2>,13>> scenario_block_offsets;
  std::span<const std::byte> read(std::uint32_t,std::size_t) const;
  std::uint16_t base_extent(std::int16_t type) const;
  std::uint16_t placement_extent(std::int32_t type) const;
  std::array<std::int16_t,2> scenario_blocks(std::int32_t type) const;
};
// Retains table ownership. Actual current tag-table group lookup stays a
// mandatory separate provider, including the original signed16 index ABI.
SourceObjectConstructionMetadata source_object_construction_metadata(
    std::shared_ptr<const SourceObjectConstructionTables>,
    std::function<std::uint32_t(std::int16_t)> tag_group);
} // namespace halo2
