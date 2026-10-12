#pragma once
#include "halo2/content.hpp"

namespace halo2 {
// Results of the caller's original visibility, model-count and geometry-LOD
// owners. These inputs do not replace those owners or select/pose geometry.
struct ShieldOverlayGeometry {
  bool visible{}, model_present{}, first_person{};
  std::int16_t model_count{};
  std::uint8_t lod = 0xff;
  std::uint32_t mode{};
};
enum class ShieldOverlayKind { Damaged, Overshield };
struct ShieldOverlay {
  ShieldOverlayKind kind{};
  std::uint32_t shader = null_datum;
  float vitality{};
};
struct ShieldOverlaySelection {
  std::array<ShieldOverlay, 2> overlays{};
  std::size_t count{};
  std::uint32_t flags{}; // Original geometry-context damage1 / overshield20h.
  std::span<const ShieldOverlay> selected() const { return {overlays.data(), count}; }
};
// Retail 4BCC0 geometry gates, then 4BF83..4C03A. No shader-LOD selection,
// collector allocation, draw, particle dispatch or lighting-cache binding.
ShieldOverlaySelection select_shield_overlays(const VitalityDefinition &,
    float recent_shield_damage, float shield, const ShieldOverlayGeometry &);
}
