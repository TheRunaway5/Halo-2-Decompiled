#pragma once
#include "halo2/mcc_bsp_vertex_layout.hpp"
#include <cstdint>

namespace halo2 {
// Only class1 rigid and class2 one-influence rigid-boned MCC streams.
// FLOAT positions/UV and their actual compression bounds stay in their source
// domain. These declarations are genuine Original FLOAT inputs; projecting
// them does not assert equality to separately compressed Xbox authoring.
struct MccModelVertexLayout {
    std::array<std::byte,96> descriptors;
    std::vector<std::byte> basis;
    std::vector<std::uint8_t> node_map;
};
MccModelVertexLayout project_mcc_rigid_model_vertex_layout(unsigned geometry_class,
    unsigned nodes_per_vertex,unsigned model_nodes,unsigned vertex_count,
    std::span<const std::byte> raw_model_header,std::span<const std::byte> descriptors,std::span<const std::byte> positions,
    std::span<const std::byte> uv,std::span<const std::byte> basis,
    std::span<const std::byte> node_map);
}
