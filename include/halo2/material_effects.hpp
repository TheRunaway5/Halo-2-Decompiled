#pragma once
#include "halo2/cache.hpp"

namespace halo2 {
// Original189E10's visual outputs: primary, material sweetener, secondary.
// Caller supplies the actual fallback material and sweetener variant.
std::array<std::uint32_t, 3> source_material_visual_effects(
    const Cache &, std::uint32_t foot, std::int16_t fallback_material,
    std::int16_t contact_material, unsigned action, std::int16_t variant);
// Original189E10's independent audio outputs in the same request order.
std::array<std::uint32_t, 3> source_material_sound_effects(
    const Cache &, std::uint32_t foot, std::int16_t fallback_material,
    std::int16_t contact_material, unsigned action, std::int16_t variant);
} // namespace halo2
