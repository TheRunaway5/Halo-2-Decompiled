#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace halo2 {
struct SourceSpatialPoolStorage {
  std::span<std::byte, 0x4c> header;
  std::span<std::byte> elements, bitmap;
};
enum class SourceSpatialTarget {
  heads, forward_header, forward_elements, forward_bitmap,
  reverse_header, reverse_elements, reverse_bitmap, reverse_head,
  visit_prefix, overflow
};
struct SourceSpatialStore {
  SourceSpatialTarget target;
  std::uint32_t instruction;
  std::size_t offset;
  std::vector<std::byte> before, after;
  friend bool operator==(const SourceSpatialStore &, const SourceSpatialStore &) = default;
};
using SourceSpatialObserver = std::function<void(const SourceSpatialStore &)>;
struct SourceSpatialRegistryStorage {
  std::span<std::byte> heads;
  SourceSpatialPoolStorage forward, reverse;
  std::span<std::byte, 4> reverse_head;
};
struct SourceSpatialRegistration {
  std::uint32_t object;
  std::span<const std::byte, 12> center;
  float radius, zero;
  std::span<const std::byte, 8> location;
  std::span<const std::byte> payload;
  std::span<std::byte, 8> visit_prefix;
  std::span<std::byte, 512> cluster_scratch;
  std::span<std::byte, 1> overflow;
};
using SourceRegistryClusterService = std::function<std::int32_t(std::int16_t,
    std::span<const std::byte, 12>, float, std::int16_t, std::span<std::byte, 512>)>;

// Original unsorted1CE650/1CE830, retaining complete raw pools/bitmaps, full
// datums, salts and allocation/free/link order. Measured positive-radius,
// location-valid/no-explicit-mask registration; cluster producer is explicit.
void source_register_spatial_object(SourceSpatialRegistryStorage,
    const SourceSpatialRegistration &, const SourceRegistryClusterService &,
    const SourceSpatialObserver & = {});
void source_unregister_spatial_object(SourceSpatialRegistryStorage,
    std::uint32_t object, const SourceSpatialObserver & = {});
} // namespace halo2
