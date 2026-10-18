#pragma once
#include "halo2/mcc_bsp_vertex_layout.hpp"
#include <cstdint>

namespace halo2 {
// Owned draw inputs for actual MCC class3 sections. The maximum stored width
// and each part's selected opaque/transparent width are distinct. No weight
// completion, normalization, CPU pose or palette creation occurs here.
struct MccSkinnedModelVertexLayout {
    std::array<std::byte,96> descriptors;
    std::vector<std::byte> positions,basis;
    std::vector<std::uint8_t> node_map;
};
MccSkinnedModelVertexLayout project_mcc_skinned_model_vertex_layout(
    unsigned opaque_influences,unsigned transparent_influences,
    unsigned model_nodes,unsigned vertex_count,
    std::span<const std::byte> raw_model_header,
    std::span<const std::byte> descriptors,std::span<const std::byte> positions,
    std::span<const std::byte> uv,std::span<const std::byte> basis,
    std::span<const std::byte> node_map,std::span<const std::byte> parts,
    std::span<const std::byte> indices);
}
