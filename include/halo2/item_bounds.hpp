#pragma once
#include "halo2/item_transform_math.hpp"
#include <span>

namespace halo2 {
struct SourceItemBoundsInputs { float one, zero, squared_movement_limit; };
// Original BEDE0 measured physical-NONE primary sphere and copied alternate
// sphere. Changed registered spheres require the still-separate registry
// producers and fail here. Complete bodies include current node matrices.
bool source_item_bounds(std::span<std::byte> body,
    std::span<const std::byte> definition, const SourceItemBoundsInputs &,
    const SourceMatrixMathObserver & = {});
} // namespace halo2
