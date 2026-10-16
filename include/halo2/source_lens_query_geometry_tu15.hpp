#pragma once
#include "halo2/source_lens_query_gpu.hpp"

namespace halo2 {
// Consume the exact complete FIFO packet emitted by whole2E520/405560.
// sample_area is that original callback's EAX, before20B90's16-bit clamp.
SourceLensQueryDraw source_lens_query_geometry_tu15(std::span<const std::byte>,
    std::uint32_t sample_area,std::uint32_t id,std::uint64_t generation);
}
