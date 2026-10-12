#pragma once
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>

namespace halo2 {
// Literal owned executable tables. Callback words and source addresses are
// opaque identities; no guest address is executable on the host.
struct SourceObjectTypeDescriptor {
  std::uint32_t type, source_address;
  std::array<std::byte,16> bytes;
};
struct SourceUnitActionTables {
  std::uint32_t action_bank, object_type_bank;
  std::array<std::uint32_t,60> descriptor_addresses;
  std::array<std::array<std::byte,16>,60> descriptors;
  // Only types with verified complete allocation-table correspondence.
  std::array<SourceObjectTypeDescriptor,3> object_types;

  std::span<const std::byte> descriptor(std::uint32_t type) const {
    if (type >= descriptors.size())
      throw std::out_of_range("Source unit action type is outside owned table");
    return descriptors[type];
  }
  std::optional<std::int16_t> base_extent(std::uint32_t type) const {
    for (const auto &record : object_types)
      if (record.type == type) {
        const auto word = std::uint16_t(std::to_integer<unsigned>(record.bytes[8]) |
            (std::to_integer<unsigned>(record.bytes[9]) << 8));
        return std::bit_cast<std::int16_t>(word);
      }
    return std::nullopt;
  }
};
} // namespace halo2
