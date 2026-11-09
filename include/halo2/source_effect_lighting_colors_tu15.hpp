#pragma once
#include "halo2/source_effect_location.hpp"
#include <functional>
#include <optional>

namespace halo2 {
struct SourceEffectLightingParametersTu15 {
    std::uint32_t flags=5;
    SourceEffectColors colors;
    std::uint32_t probe_reference{};
};
struct SourceEffectLightingSampleTu15 {
    std::uint32_t diffuse_rgb{},lightmap_rgb{};
};
struct SourceEffectLightingProbeTu15 {
    Vec3 incident{0,0,-1};
    SourceEffectLightingSampleTu15 sample{0xff404040u,0xff404040u};
    std::uint32_t reference{};
};
struct SourceEffectLightingServicesTu15 {
    // BCDF0/BCFB0/3DDC0 and the actual ready byte: no value means that the
    // completed lookup found no usable sample. An unbound owner must throw.
    std::function<std::optional<SourceEffectLightingSampleTu15>()> render_cache;
    // Owns the actual D50F0/location/D4F90 branch. No colors means that the
    // completed original probe made no stores, not that its owner is absent.
    std::function<std::optional<SourceEffectColors>(const SourceEffectLightingProbeTu15&)> probe;
};
// Semantic color selection/write-back from17E460. This does not implement its
// original register ABI, cache allocation, location decoding or lightmap ray.
// Their explicit native services retain those separate ownership obligations.
// The caller supplies the creator's initialized colors and distinct request
// parameters; unknown producer branches refuse rather than supplying a color.
bool source_effect_lighting_colors_tu15(SourceEffectColors&,
    SourceEffectLightingParametersTu15*,bool probe,
    const SourceEffectLightingServicesTu15&);
} // namespace halo2
