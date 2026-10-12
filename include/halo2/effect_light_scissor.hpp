#pragma once
#include "halo2/transform.hpp"
#include <optional>

namespace halo2 {
using EffectLightSphereProjection=std::array<float,6>; // Xlo,Xhi,Ylo,Yhi,Zlo,Zhi.
// Original1650A0. The current render camera's inverse transform is distinct
// from a receiver/frustum camera. Rejection writes no projected bounds.
// Uses the original PC53 x87 sqrt/asin/atan/tan sequence; unsupported native
// math backends reject instead of silently substituting a library result.
std::optional<EffectLightSphereProjection> source_effect_light_sphere_projection(
    const Transform &inverse_view,Vec3 center,float radius);
struct EffectLightScissorInput {
    Transform inverse_view;
    Vec3 center{};float radius{}; // Actual light record+28/+34, not tick shape.
    std::array<float,4> clipping_rectangle{}; // Camera+A4..B0.
    std::array<std::int16_t,4> viewport{}; // Actual485A8A: top,left,bottom,right.
    float near_distance{},far_distance{}; // Camera+74/+8C.
    bool light_present{}; // Original light identity NONE selects the full view.
};
struct EffectLightScissor {
    std::array<float,6> bounds{}; // Left,right,top,bottom,Zlo,Zhi at485964.
    bool light_present{}; // Actual4858D0 remains true on projection fallback.
};
// Original4A3D7..4A677, including its unnegated Y overlap test and later Y
// reversal. Projection failure, zero extent and disjoint bounds retain the
// full view/depth fallback. No rounding to Vulkan scissors or3A550 upload is
// performed here; those are subsequent source/device owners.
EffectLightScissor source_effect_light_scissor(const EffectLightScissorInput &);
}
