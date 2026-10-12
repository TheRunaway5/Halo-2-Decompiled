#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
// Original owning leaves remain explicit. These functions neither manufacture
// admission nor replace owner/world/drop producers with successful returns.
struct UnitInventoryServices {
  std::function<std::span<std::byte>(std::uint32_t)> unit_body;
  std::function<std::uint8_t()> options_mode;
  std::function<void(std::uint32_t item, std::uint32_t owner)> assign_owner;
  std::function<void(std::uint32_t item)> remove_world_membership;
  std::function<void(std::uint32_t unit, std::uint32_t item,
                     std::uint32_t removal_mode)> release_item;
  std::function<void(std::uint32_t entity, std::uint32_t mask)> network_dirty;
};
struct UnitInventoryStore {
  std::uint32_t instruction, before, after;
  std::size_t offset;
  unsigned bytes;
  friend bool operator==(const UnitInventoryStore &, const UnitInventoryStore &) = default;
};
using UnitInventoryStoreObserver = std::function<void(const UnitInventoryStore &)>;

// CFE60 reads the actual options byte before resolving the unit. Its two words
// advance from the low sequence byte; dirty ownership is a separate service.
void advance_source_inventory_sequence(std::uint32_t unit,
    const UnitInventoryServices &, const UnitInventoryStoreObserver & = {});
// CFED0's owning orchestration, with source order and full item identity.
// Supported slot storage is 0..3; out-of-range native inputs fail explicitly.
// No boolean return contract is invented. This does not allocate/delete items,
// clear their originating map row or perform the leaf producers above.
void set_source_inventory_slot(std::uint32_t unit, std::uint32_t item,
    std::int16_t slot, std::uint32_t removal_mode, const UnitInventoryServices &,
    const UnitInventoryStoreObserver & = {});
} // namespace halo2
