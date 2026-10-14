#pragma once
#include "halo2/content.hpp"
#include "halo2/source_effect_location.hpp"
namespace halo2 {
enum class UnitDamageEffectKind { ShieldDepleted, ShieldDamaged, ShieldRecharging };
struct UnitDamageEffect {
  UnitDamageEffectKind kind{};
  std::uint32_t datum = null_datum;
  // D6A70 uses the default owner, D9110 the incoming damage owner, and
  // D5DE0 the target's D66D0 owner. Allocation/attachment is a caller stage.
};
struct DamageOwner {
  std::uint32_t player = null_datum, unit = null_datum;
  std::int16_t designator = -1;
};
// Native transport retains every admitted attempt, including a NONE effect.
// Full salted target identity prevents a respawn from inheriting an old hit.
struct UnitDamageEffectEvent {
  std::uint64_t serial{},tick{};
  std::uint32_t target=null_datum;
  UnitDamageEffect effect;
  DamageOwner owner;
  SourceEffectColors colors;
};
// CA0B0 ->1147E0 consumes the completed normalized damage report, including
// recent damage after D9640. Retain it independently of threshold effects.
struct UnitDamageVoiceEvent {
  std::uint64_t serial{},tick{};
  std::uint32_t target=null_datum,definition=null_datum,report_flags{};
  Vec3 position;
  float recent_body{},recent_shield{},visible_body{};
};
void validate_unit_damage_voice_events(std::span<const UnitDamageVoiceEvent>,std::uint64_t tick);
void validate_unit_damage_effect_events(std::span<const UnitDamageEffectEvent>,
    std::uint64_t snapshot_tick);
struct UnitDamageResult {
  float body{}, shield{}; // Full normalized requests, including lethal overflow.
  std::uint32_t report_flags{}; // D9490 bits0/1 from new body/shield depletion.
  std::array<UnitDamageEffect, 2> effect_requests{};
  std::size_t effect_count{};
  std::span<const UnitDamageEffect> effects() const {
    return {effect_requests.data(), effect_count};
  }
};
// Ordinary non-radius, no model-region entry path. Admission, hit finding,
// amount selection, reaction/attribution and ownership are caller stages.
// Effect attempts are retained in source order, including NONE datums. Shield
// shader overlay state, effect dispatch and parent notifications are callers.
UnitDamageResult apply_unit_damage(RechargeState &, const VitalityDefinition &,
    const WeaponDefinition &, std::span<const DamageGroup>,
    std::span<const LevelContent::DamageMaterial>, float amount, bool radius=false,std::uint16_t unit_flags=0);
struct UnitVitalityUpdate {
  bool changed{};
  std::optional<UnitDamageEffect> effect;
};
// Ordinary scalar D5DE0 branch, with its ordered effect attempt retained.
// The request precedes the charging flag and shield write: testing the final
// flag loses zero-rate/overshoot transitions. Overshield/actor/difficulty and
// model-section ownership remain separate original branches.
UnitVitalityUpdate update_unit_vitality(RechargeState &,const VitalityDefinition &,
    float seconds,float team_scale=1.f);
}
