#pragma once
#include "halo2/cache.hpp"
#include "halo2/physics_contact_runtime.hpp"
#include <optional>
namespace halo2 {
struct WeaponHudTarget {
  bool acquired{};
  std::optional<Vec3> position;
};
struct WeaponHudOwnership {
  bool seated{},handheld_visible=true;
  std::uint32_t vehicle_weapon=0xffffffffu;
  std::uint32_t parent_hud=0xffffffffu;
};
// Original13B275..13B3F9 (TU1.5 13C5E5..13C769). Reads the retained
// seat, inventory and child hierarchy; it does not create a weapon owner.
WeaponHudOwnership weapon_hud_ownership(const SourcePhysicsContactMemory &,
    std::uint32_t unit,SourcePhysicsContactImage);
// Original per-weapon HUD status at13C719/13DA89. Padding is caller-owned.
struct SourceWeaponHudState {
  std::uint32_t datum=0xffffffffu;
  std::uint8_t present{},unknown05{};
  std::int16_t loaded{},reserve{},capacity{},maximum_reserve{},unknown0e{};
  float age{},heat{},charge{};
  std::uint32_t unknown1c{};
  std::uint8_t overheated{},magazine_working{},input_held{},charging{},target{},locking{},locked{},unknown27{};
  float lock_progress{};
  Vec3 target_position;
};
static_assert(sizeof(SourceWeaponHudState)==0x38);
struct VehicleWeaponHudStatus {
  WeaponHudOwnership ownership;
  std::uint32_t definition=0xffffffffu;
  SourceWeaponHudState weapon;
};
struct WeaponHudProjectionInput {
  std::uint32_t datum=0xffffffffu;
  float heat{},age{};
  std::uint8_t charge{};
  std::uint16_t weapon_flags{},input_flags{};
  bool magazine_present{};
  std::int16_t magazine_mode{},loaded{},capacity{},maximum_reserve{};
  std::int32_t shared_reserve{};
  bool trigger_present{};
  std::int16_t trigger_behavior{},charging_type{},trigger_ticks{},tick_rate=30;
  std::uint8_t trigger_state{};
  bool acquire_target{};
  std::optional<Vec3> target_position;
};
// Numeric projection only. Caller supplies real104AA0 reserve and target/
// marker query results. This does not acquire a target or own a weapon world.
void weapon_hud_project(SourceWeaponHudState &,const WeaponHudProjectionInput &);
float weapon_hud_input(const SourceWeaponHudState *,unsigned selector,float elapsed);
std::uint16_t weapon_hud_mask(const SourceWeaponHudState *,unsigned type);
std::uint16_t weapon_hud_cutoffs(const SourceWeaponHudState *,
    std::array<unsigned,3> thresholds,std::uint16_t mask);
}
