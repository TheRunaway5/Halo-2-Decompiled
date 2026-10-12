#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
struct UnitWeaponSelectionServices {
  std::function<std::span<const std::byte>(std::uint32_t)> unit_body;
  std::function<std::span<const std::byte>(std::uint32_t)> weapon_definition;
  // Original CEAF0: unit and mode on stack, candidate's full item datum in EDX.
  // The original checks this even for the current/other-hand slot. Its equipped
  // manager/capability producer must be available; no default success exists.
  std::function<bool(std::uint32_t unit, std::uint32_t item,
                     std::uint32_t mode)> accepts_weapon;
};

// CF300 / TU1.5 D0390. Scans all four retained slots, including current primary.
// Returns the original signed AX slot; -1 means no admitted candidate. No slot,
// current/requested byte, timestamp, action, weapon or ammo store is introduced.
// Bounded native ABI: previous -1..3; direction -1,0,+1. Other ABI values fail
// explicitly rather than performing the original out-of-storage index access.
std::int16_t select_source_inventory_weapon(std::uint32_t unit,
    std::uint32_t mode, std::int16_t previous, std::int16_t direction,
    const UnitWeaponSelectionServices &);
} // namespace halo2
