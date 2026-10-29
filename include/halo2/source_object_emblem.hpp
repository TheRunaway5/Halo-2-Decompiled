#pragma once
#include "halo2/cache.hpp"
#include "halo2/appearance.hpp"
#include "halo2/shader_animation.hpp"
#include "halo2/source_player_profile.hpp"

namespace halo2 {
using SourceObjectEmblem = std::array<std::uint8_t,4>;
// Original14BFC0: player+89 (live configuration+45) -> object+24.
SourceObjectEmblem source_player_object_emblem(const SourcePlayerConfiguration &);
// BCC20 -> BC970. A missing scenario does not suppress emblems; type0 does
// unless object+26 bit3 is set. Optional presence matches BCC20's recognized
// return; enabled matches its separate active output. A suppressed recognized
// name returns zero/inactive; a disabled foreground toggle is zero/active.
std::optional<FunctionValue> source_object_emblem_input(const SourceObjectEmblem &,
    std::optional<std::int16_t> scenario_type, std::uint32_t name);
// Original7F790 ->1A06A0: signed appearance slots2/3, actual matg+160,
// original white fallback. Team/neutral overrides only change slots0/1.
std::array<Vec3,2> source_player_emblem_colors(const SourcePlayerConfiguration &,
    std::span<const std::array<std::uint32_t,3>> palette);
// Complete TU1.5 830C0/83050/1AA2B0. Team primary replaces channel0;
// personal primary becomes channel1. Neutral overrides only channels0/1.
ObjectColors source_player_appearance_colors(const SourcePlayerConfiguration &,
    std::int16_t team,bool neutral,std::span<const std::array<std::uint32_t,3>> palette,
    std::span<const std::array<std::uint32_t,3>> team_palette);
}
