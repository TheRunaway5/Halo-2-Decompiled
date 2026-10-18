#pragma once
#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace halo2 {
// Owned draw-input projection only. Position/UV source bytes remain unchanged;
// the first three descriptors and physical N/T/B slots acquire the exact
// original FLOAT3/FLOAT2/NORMPACKED3 declarations. Extra lighting descriptors
// and every source allocation remain outside this projection's ownership.
struct MccBspVertexLayout {
    std::array<std::byte,96> descriptors;
    std::vector<std::byte> basis;
};
MccBspVertexLayout project_mcc_bsp_vertex_layout(unsigned vertex_count,
    std::span<const std::byte> descriptors,std::span<const std::byte> positions,
    std::span<const std::byte> uv,std::span<const std::byte> basis);
}
