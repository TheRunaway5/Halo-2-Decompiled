#pragma once
#include "halo2/cache.hpp"
#include "halo2/source_king_boundary.hpp"
namespace halo2 {
// Preserve the immediate NV2A triangle strip's alternating winding in the
// native renderer's triangle-list stream. No source register value is changed.
std::array<Vertex,192> source_king_boundary_triangles(const SourceKingBoundary&,
    std::uint32_t resident_material);
}
