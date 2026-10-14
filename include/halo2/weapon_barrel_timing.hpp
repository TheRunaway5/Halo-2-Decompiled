#pragma once
#include "halo2/smg.hpp"
#include <optional>

namespace halo2 {
// Actual weap/barrel operands of TU1.5 1071D0/108F30/109030/108EA0.
// Trigger admission, heat, magazine transfer and shot preparation are separate.
struct WeaponBarrelTimingDefinition {
  std::uint32_t flags{};
  float rate_lower{}, rate_upper{}, recovery_seconds{}, recovery_fraction{};
  float spent_rate_scale{}; // weap+258
  std::int16_t burst_minimum{}, burst_maximum{};
};
struct WeaponBarrelTimingConstants { float one{}, minimum_rate{}; };
struct WeaponBarrelTimingContext {
  std::int16_t tick_rate{};
  float spent{}; // actual weapon+184, not heat
  // Present only when the actual second barrel exists.
  std::optional<std::uint8_t> alternate_mode;
  // Present only for the other carried hand with the SAME weapon definition.
  struct OtherHand { std::uint8_t mode{}; std::int8_t counter{}; };
  std::optional<OtherHand> other_hand;
};
struct WeaponBarrelGate { bool admitted{}, minimum_met{}, maximum_met{}; };
WeaponBarrelGate weapon_barrel_gate(SmgBarrelState &,
    const WeaponBarrelTimingDefinition &,const WeaponBarrelTimingContext &,
    WeaponBarrelTimingConstants);
void weapon_barrel_begin_recovery(SmgBarrelState &,
    const WeaponBarrelTimingDefinition &,std::int16_t tick_rate,
    WeaponBarrelTimingConstants);
void weapon_barrel_finish_recovery(SmgBarrelState &,
    const WeaponBarrelTimingDefinition &,std::int16_t tick_rate,
    WeaponBarrelTimingConstants);
void weapon_barrel_reset(SmgBarrelState &,const WeaponBarrelTimingDefinition &);
struct WeaponBarrelTimingStep { bool prepare{}, marker{}; };
// Original1021D0 barrel phase, after its trigger has produced flags bit0.
// The caller owns actual prepared-shot stores/effects and auto-reload admission.
WeaponBarrelTimingStep weapon_barrel_timing_step(SmgBarrelState &,
    const WeaponBarrelTimingDefinition &,const WeaponBarrelTimingContext &,
    WeaponBarrelTimingConstants,bool blocked);
} // namespace halo2
