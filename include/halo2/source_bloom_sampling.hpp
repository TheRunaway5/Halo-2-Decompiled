#pragma once
#include "halo2/cache.hpp"
#include <cmath>
#include <optional>

namespace halo2 {
// TU1.5 14F40/34A80/340F0 derive normalized tap offsets from the original
// logical target dimensions, independently of the native VkImage allocation.
// The caller supplies an authenticated logical extent only for an admitted
// view corridor; absence preserves its existing physical-size denominator.
inline ShaderVector source_bloom_filter_controls(float radius,Vec2 physical_extent,
    std::optional<Vec2> source_extent={}) {
    const auto extent=source_extent.value_or(physical_extent);
    if(!std::isfinite(extent.x) || !std::isfinite(extent.y) || extent.x<=0 || extent.y<=0)
        throw FormatError("Bloom filter requires positive finite tap dimensions");
    return {radius,extent.x,extent.y,0};
}
}
