#pragma once

namespace halo2::native_fog_resolution {

enum class Blend { primary, secondary, color_c8, sky, patchy };

// Resolve accumulated colors and parameters only for blends that have both
// positive coverage and a positive weight. Patchy fog also requires a valid
// datum, which is published in the same ordered store group.
template<class Resolution> void resolve(Resolution &fog) {
    if (fog.has_weight(Blend::primary))
        fog.resolve_six(Blend::primary);
    if (fog.has_weight(Blend::secondary))
        fog.resolve_six(Blend::secondary);
    if (fog.has_weight(Blend::color_c8))
        fog.resolve_six(Blend::color_c8);
    if (fog.has_weight(Blend::sky))
        fog.resolve_sky();
    if (fog.has_patchy_datum() && fog.has_weight(Blend::patchy))
        fog.resolve_patchy();
    if (fog.has_bloom())
        fog.resolve_bloom();
}

} // namespace halo2::native_fog_resolution
