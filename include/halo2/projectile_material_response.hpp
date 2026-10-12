#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <functional>
#include <optional>

namespace halo2 {
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
  // Required only when a genuinely eligible fractional chance uses RNG.
  // The caller owns the source stream; the leaf performs its actual LCG write.
  std::function<std::uint32_t&()> rng;
};
// Complete unchanged FEB40 FF090..FF1D9 response-selection segment after
// angle/speed production. Does not produce collision, perturb angle, attach,
// bounce, effect, timer or damage services. Return is the signed source short.
std::int16_t choose_source_projectile_response(std::span<const std::byte> row,
    const SourceProjectileResponseInputs&,const SourceProjectileResponseServices&);
} // namespace halo2
