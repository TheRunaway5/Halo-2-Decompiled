#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <functional>
#include <optional>

namespace halo2 {
struct Vec3;
// Complete original11CE20 (including its binary32 stores and real pi
// initializer operand), used before FEB40's half-pi subtraction.
float source_projectile_vector_angle(Vec3,Vec3);
std::optional<unsigned> source_projectile_material_effect_slot(std::int16_t);
struct SourceProjectileImpactPresentation {
  std::int16_t response{};
  float scale_a=1,scale_b{};
  bool admitted=true;
};
// Initialized default-row scalar consumer: response0, no noise/chance, and
// scaleA from damage. Verified constructors for disc, TU1.1 and TU1.5; this
// does not manufacture the default row's unrelated tag-reference payloads.
SourceProjectileImpactPresentation source_projectile_default_impact_presentation(
    Vec3 normal,Vec3 velocity,float damage_scale,float object_scale);
// Complete1015A0 authored-row search over retained banks. An empty result
// requires the executable's default response payload owner; it is not an
// assumed bounce row. Original malformed parent cycles are a host hard stop.
std::optional<std::size_t> source_projectile_material_response(
    std::span<const std::byte> response_rows,
    std::span<const std::int16_t> material_parents,std::int16_t material);
struct SourceProjectileResponseInputs {
  std::int32_t collision_type{};
  // These values are only requested if their original gate executes. The
  // ordinary plasma row has no angle/speed limits and needs neither value.
  std::optional<float> angle,speed;
};
struct SourceProjectileResponseServices {
  float one{},rng_scale{};
  // Required only for an admitted object collision (type4) with a type gate.
  std::function<std::uint8_t()> object_type;
  // Required when positive impact noise or an eligible fractional chance uses RNG.
  // The caller owns the source stream; the leaf performs its actual LCG write.
  std::function<std::uint32_t&()> rng;
};
struct SourceProjectileImpactAngles {float angle{},approach{};};
// FEB40's pre-selection noise consumes the actual primary stream before the
// chance draw. Direction is the caller's normalized incoming velocity.
SourceProjectileImpactAngles source_projectile_impact_angles(
    std::span<const std::byte> row,Vec3 normal,Vec3 direction,Vec3 velocity,
    const SourceProjectileResponseServices&);
float source_projectile_contact_damage_scale(std::uint32_t flags,
    Vec3 projectile_position,Vec3 contact,float traveled,float damage_range,
    float stored_scale);
float source_projectile_effect_scale(std::span<const std::byte> row,
    float angle,float damage_scale);
struct SourceProjectileDamageInputs {
  std::int16_t body_material=-1,shield_material=-1;
  float amount{},before_shield{},body{},shield{},body_request{},shield_request{},region_request{};
  bool region_absorbs{};
};
struct SourceProjectileDamagePresentation {std::int16_t material=-1;float scale{};};
// Parentless D7B80 material/scaleB consumer, after real damage accumulation.
// Region/material selection, amount admission and vitality are caller owners.
SourceProjectileDamagePresentation source_projectile_damage_presentation(
    const SourceProjectileDamageInputs&);
// Complete unchanged FEB40 FF090..FF1D9 response-selection segment after
// angle/speed production. Does not produce collision, perturb angle, attach,
// bounce, effect, timer or damage services. Return is the signed source short.
std::int16_t choose_source_projectile_response(std::span<const std::byte> row,
    const SourceProjectileResponseInputs&,const SourceProjectileResponseServices&);
} // namespace halo2
