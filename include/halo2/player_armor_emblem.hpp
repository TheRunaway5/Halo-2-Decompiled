#pragma once
#include "halo2/appearance.hpp"
#include "halo2/shader_animation.hpp"
#include "halo2/source_player_profile.hpp"
#include "halo2/source_object_emblem.hpp"

namespace halo2 {
// Animate the actual per-player resident body bank. The profile is the live
// sanitized configuration, not the unaccepted local profile request.
void animate_player_armor_emblem(Material &, double seconds,
    const std::optional<SourcePlayerConfiguration> &, std::int16_t scenario_type,
    std::span<const std::array<std::uint32_t,3>> palette,
    const ShaderInput &fallback);
}
