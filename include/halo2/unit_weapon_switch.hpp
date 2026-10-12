#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <array>

namespace halo2 {
// The entries are literal TU1.1 owning leaves. A callback must execute its
// producer or fail; absence never means successful admission or completed time.
struct WeaponSwitchLeafCall {
  std::uint32_t entry;
  std::array<std::uint32_t, 6> arguments{};
  unsigned count{};
  friend bool operator==(const WeaponSwitchLeafCall &, const WeaponSwitchLeafCall &) = default;
};
struct UnitWeaponSwitchServices {
  std::function<std::span<std::byte>(std::uint32_t)> unit_body;
  // Resolved signed unit+346 component, not a fixed unit-body offset.
  std::function<std::span<std::byte>(std::uint32_t)> action_component;
  std::function<std::span<std::byte>(std::uint32_t)> weapon_body;
  std::function<std::span<const std::byte>(std::uint32_t)> weapon_definition;
  std::function<std::uint32_t(const WeaponSwitchLeafCall &)> invoke;
  std::function<std::uint32_t()> source_tick;
};
enum class WeaponSwitchBuffer { Unit, Weapon, ActionComponent };
struct WeaponSwitchStore {
  std::uint32_t instruction, datum;
  WeaponSwitchBuffer buffer;
  std::size_t offset;
  unsigned bytes;
  std::uint32_t before, after;
  friend bool operator==(const WeaponSwitchStore &, const WeaponSwitchStore &) = default;
};
using WeaponSwitchStoreObserver = std::function<void(const WeaponSwitchStore &)>;

// E9FE0: request is the original 32-byte packet; type8 primary/type18 other hand.
// E9B40/EA180 retain actual put-away/manager gates, with no native fixed delay.
bool begin_source_weapon_switch(std::uint32_t unit, std::span<const std::byte> request,
    const UnitWeaponSwitchServices &, const WeaponSwitchStoreObserver & = {});
bool update_source_weapon_switch(std::uint32_t unit, std::uint32_t type,
    const UnitWeaponSwitchServices &, const WeaponSwitchStoreObserver & = {});
void finish_source_weapon_switch(std::uint32_t unit, std::uint32_t type,
    const UnitWeaponSwitchServices &, const WeaponSwitchStoreObserver & = {});
void interrupt_source_weapon_switch(std::uint32_t unit, std::uint32_t type,
    const UnitWeaponSwitchServices &, const WeaponSwitchStoreObserver & = {});

// Owning carry helpers are exposed so a source graph callback may compose the
// same code. They preserve full datum identity and the current/requested split.
bool put_away_source_weapon(std::uint32_t unit, bool primary, bool immediate,
    bool silent, const UnitWeaponSwitchServices &, const WeaponSwitchStoreObserver & = {});
void clear_current_source_weapon(std::uint32_t unit, bool primary,
    const UnitWeaponSwitchServices &, const WeaponSwitchStoreObserver & = {});
void equip_requested_source_weapon(std::uint32_t unit, bool primary, std::uint32_t mode,
    bool immediate, bool silent, const UnitWeaponSwitchServices &,
    const WeaponSwitchStoreObserver & = {});
} // namespace halo2
