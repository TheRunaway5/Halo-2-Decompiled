#pragma once
#include "halo2/smg.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <span>
namespace halo2 {
// Original weapon+20C, one12-byte record per authored trigger.
struct WeaponTriggerState {
  std::uint8_t mode{},unknown1{};
  std::int16_t ticks{};
  std::uint32_t flags{},effect=0xffffffffu;
};
static_assert(sizeof(WeaponTriggerState)==12);
// Literal operands from the64-byte trigger definition. Other fields stay
// with their separate caller; no name or weapon family supplies a default.
struct WeaponTriggerDefinition {
  std::uint32_t flags{};
  std::int16_t input{},behavior{},primary_barrel{},secondary_barrel{};
  float autofire_seconds{},autofire_threshold{};
  std::int16_t charging_type{},discharge_type{};
  float charging_seconds{},charged_seconds{};
  std::int16_t charged_action{};
  float discharging_seconds{};
  std::uint32_t charging_effect=0xffffffffu,discharging_effect=0xffffffffu;
};
// Original weapon+170..174, shared by the trigger caller.
struct WeaponTriggerInputState {
  std::uint8_t current{},previous{},anchor{},direction{},ticks{};
};
static_assert(sizeof(WeaponTriggerInputState)==5);
struct WeaponTriggerInputEvents {bool primary{},charged{},released{};};
// Actual106960/107880, including signed8-bit timer interpretation.
WeaponTriggerInputEvents weapon_trigger_input(WeaponTriggerInputState &,
    const WeaponTriggerState &,const WeaponTriggerDefinition &,std::int16_t tick_rate);
struct WeaponTriggerServices {
  std::function<std::uint32_t(std::uint32_t effect)> allocate_effect;
  std::function<void()> reset_zoom;
  std::function<void(unsigned state,bool force)> weapon_state;
  std::function<void(unsigned action)> activity;
};
// These requests belong to the actual external effect/unit owners. Missing
// required services are rejected; the kernel does not fabricate world state.
bool weapon_trigger_charge_admitted(const WeaponTriggerDefinition &,
    std::span<const SmgBarrelState>,float spent);
void weapon_trigger_start_charge(WeaponTriggerState &,const WeaponTriggerDefinition &,
    std::int16_t tick_rate,bool has_unit,const WeaponTriggerServices &);
void weapon_trigger_finish_charge(WeaponTriggerState &,const WeaponTriggerDefinition &,
    std::int16_t tick_rate,unsigned index,const WeaponTriggerServices &);
void weapon_trigger_release_charge(WeaponTriggerState &,const WeaponTriggerDefinition &,
    std::span<SmgBarrelState>,std::int16_t tick_rate);
enum class WeaponTriggerLockPhase {Seeking,Lost,Acquiring,Locked};
void weapon_trigger_lock(WeaponTriggerState &,WeaponTriggerLockPhase,
    std::int16_t tick_rate,bool has_unit,const WeaponTriggerServices &);
void weapon_trigger_clear_barrel(std::span<SmgBarrelState>,std::int16_t index);
}
namespace halo2 {
struct WeaponTriggerTarget {
  std::uint32_t datum=0xffffffffu;
  std::uint8_t marker=255;
};
struct WeaponTriggerPhaseContext {
  std::int16_t tick_rate=30;
  bool blocked{},held{},pressed{},alternate{},has_unit{},has_player{},editor{};
  float spent{};
  unsigned barrel_count{};
  std::uint32_t detonation_effect=0xffffffffu; // Actual weapon definition+180.
};
struct WeaponTriggerPhaseServices : WeaponTriggerServices {
  std::function<bool(std::int16_t barrel)> barrel_admitted;
  std::function<void(std::uint32_t effect)> stop_effect;
  std::function<std::optional<WeaponTriggerTarget>()> acquire_target,track_target;
  std::function<void()> destroy_weapon;
};
// Complete original1021D0/1030F0 per-trigger phase. Caller supplies its real
// pre-phase inputs, magazine/barrel admission and target/action/effect owners.
// The shared analog-byte and target storage remain owned by the actual weapon.
bool weapon_trigger_step(WeaponTriggerState &,const WeaponTriggerDefinition &,
    std::span<SmgBarrelState>,WeaponTriggerInputState &,WeaponTriggerTarget &,
    unsigned index,const WeaponTriggerPhaseContext &,const WeaponTriggerPhaseServices &);
}
