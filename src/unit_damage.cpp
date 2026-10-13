#include "halo2/unit_damage.hpp"
#include <cmath>
namespace halo2 {
void validate_unit_damage_voice_events(std::span<const UnitDamageVoiceEvent> events,std::uint64_t snapshot_tick) {
  if(events.size()>64)throw FormatError("Unit damage voice journal exceeds retention");
  std::uint64_t serial{},tick{};
  for(const auto &e:events) {
    if(!e.serial || e.serial<=serial || e.tick<tick || e.tick>snapshot_tick || e.target==null_datum ||
        !std::isfinite(e.position.x) || !std::isfinite(e.position.y) || !std::isfinite(e.position.z) ||
        !std::isfinite(e.recent_body) || !std::isfinite(e.recent_shield) || !std::isfinite(e.visible_body) ||
        e.recent_body<0 || e.recent_shield<0 || e.visible_body<0)
      throw FormatError("Invalid unit damage voice journal");
    serial=e.serial;tick=e.tick;
  }
}
void validate_unit_damage_effect_events(std::span<const UnitDamageEffectEvent> events,
    std::uint64_t snapshot_tick) {
  if(events.size()>64)throw FormatError("Unit damage effect journal exceeds retention");
  std::uint64_t serial=0,tick=0;
  for(const auto &event:events) {
    if(!event.serial || event.serial<=serial || event.tick<tick || event.tick>snapshot_tick ||
        unsigned(event.effect.kind)>unsigned(UnitDamageEffectKind::ShieldRecharging))
      throw FormatError("Invalid unit damage effect journal");
    serial=event.serial;tick=event.tick;
  }
}
UnitVitalityUpdate update_unit_vitality(RechargeState &state,
    const VitalityDefinition &definition,float seconds,float team_scale) {
  UnitVitalityUpdate result;
  // TU1.1 D78FC..D7939 / retail D5DE0: the old charging bit gates
  // 176780, independently of the recharge amount or its eventual clamp.
  if(!state.dead && definition.shield>0 && state.shield<1.f &&
      state.shield_stun==0 && !state.shield_recharging)
    result.effect=UnitDamageEffect{UnitDamageEffectKind::ShieldRecharging,
        definition.shield_recharging_effect};
  result.changed=recharge(state,seconds,definition.body_rate,definition.shield_rate,
      definition.body_max,team_scale,definition.health>0,definition.shield>0);
  return result;
}
UnitDamageResult apply_unit_damage(RechargeState &vitality,
    const VitalityDefinition &definition, const WeaponDefinition &w,
    std::span<const DamageGroup> groups,
    std::span<const LevelContent::DamageMaterial> materials, float amount,bool radius,std::uint16_t unit_flags) {
  UnitDamageResult normalized;
  const auto effect = [&](UnitDamageEffectKind kind, std::uint32_t datum) {
    normalized.effect_requests[normalized.effect_count++] = {kind, datum};
  };
  float remaining = amount;
  if (!(w.damage_flags & (1 << 9)) && definition.shield > 0) {
    const auto &v = definition;
    ShieldDamageState state{vitality.shield, vitality.shield_stun,
                            std::uint16_t((vitality.shield_damaged ? 2 : 0) |
                                          (vitality.shield_depleted ? 8 : 0) |
                                          (vitality.shield_recharging ? 512 : 0))};
    ShieldDamageInput input;
    input.damage = remaining;
    input.maximum_shield = v.shield;
    input.minimum_stun_damage = v.minimum_shield_stun_damage;
    input.stun_seconds = v.shield_delay;
    input.damaged_threshold = v.shield_damaged_threshold;
    input.side_effect = w.damage_side_effect;
    input.radius=radius;
    if (v.shield_material >= 0 &&
        std::size_t(v.shield_material) < materials.size()) {
      const auto &material = materials[v.shield_material];
      input.multiplier = damage_multiplier(groups, w.damage_group,
                                            w.damage_group_b, material.armor_a,
                                            material.armor_b);
    }
    const auto result = shield_damage(state, input);
    remaining = result.body_damage;
    normalized.shield = result.shield_damage;
    if(result.flags&8)normalized.report_flags|=2;
    vitality.shield = state.shield;
    vitality.shield_stun = state.stun;
    vitality.shield_damaged = (state.flags & 2) != 0;
    vitality.shield_depleted = (state.flags & 8) != 0;
    // D9110 invokes D6A70 before its threshold-damaged effect. Do not derive
    // these transitions from vitality loss: zero damage and side effects can
    // admit them, while a previously depleted shield suppresses repetition.
    if (result.depleted_effect)
      effect(UnitDamageEffectKind::ShieldDepleted, v.shield_depleted_effect);
    if (result.damaged_effect)
      effect(UnitDamageEffectKind::ShieldDamaged, v.shield_damaged_effect);
  }
  if (!(w.damage_flags & (1 << 6)) && definition.health > 0) {
    const auto &v = definition;
    BodyDamageState state{vitality.body, vitality.body_stun,
                           std::uint16_t((vitality.dead ? 4 : 0)|(unit_flags&0x4000))};
    BodyDamageInput input;
    input.damage = remaining;
    input.maximum_body = input.death_maximum_body = v.health;
    input.minimum_stun_damage = v.minimum_body_stun_damage;
    input.stun_seconds = v.body_delay;
    input.info_flags = v.damage_flags;
    input.definition_flags = w.damage_flags;
    if (v.body_material >= 0 &&
        std::size_t(v.body_material) < materials.size()) {
      const auto &material = materials[v.body_material];
      input.multiplier = damage_multiplier(groups, w.damage_group,
                                            w.damage_group_b, material.armor_a,
                                            material.armor_b);
    }
    // This path has no recovered model-region entry. The original null
    // entry branch uses the authored default section and indirect material;
    // headshots wait for authoritative collision-region selection.
    if (v.default_section >= 0 &&
        std::size_t(v.default_section) < v.sections.size()) {
      const auto &section = v.sections[v.default_section];
      input.region_valid = true;
      input.region_flags = section.flags;
      input.region_vitality = section.vitality;
    }
    const auto result = body_damage(state, input);
    normalized.body = result.normalized_damage;
    if(result.depleted)normalized.report_flags|=1;
    vitality.body = state.body;
    vitality.body_stun = state.stun;
    vitality.dead = (state.flags & 4) != 0;
    if (result.depleted && !vitality.shield_depleted) {
      // D6800 also invokes D6A70 on a body-only death, after any earlier
      // threshold effect. Its emission requires a positive maximum shield;
      // the depletion flag is written even for an unshielded object.
      if (v.shield > 0)
        effect(UnitDamageEffectKind::ShieldDepleted, v.shield_depleted_effect);
      vitality.shield_depleted = true;
    }
  }
  return normalized;
}
}
