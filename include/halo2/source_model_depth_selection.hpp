#pragma once
#include "halo2/model.hpp"
#include <span>

namespace halo2 {
// Original expanded mesh ordinals, retained through part/anatomical selection.
// first addresses the final ordinary vertex bank before renderer grouping.
struct SourceModelDepthSelection {
    std::uint32_t object=~0u,definition=~0u,render_model=~0u;
    unsigned lod{};
    const RenderModel* model{};
    std::size_t first{};
    std::span<const std::uint32_t> source_vertices;
};
}
