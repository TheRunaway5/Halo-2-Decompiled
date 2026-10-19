#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace halo2 {
class Cache;
// A serialized MCC mode section's actual type104/channel1 resource. The
// canonical metadata copy changes only its typed destination to Xbox100.
// Source cache metadata and raw resource allocations remain immutable.
struct MccModelNodeMap {
    std::uint32_t descriptor_field{}, encoded_resource{}, resource_bytes{};
    std::uint32_t node_offset{}, node_count{}, model_nodes{};
    std::string model;
};
std::vector<MccModelNodeMap> plan_mcc_model_node_maps(const Cache&);
// Project only the node-map header fields overlapping this I/O range. The
// complete immutable decoded container authenticates partial-sector reads.
// Native130980/22A680 still allocates, fixes up and publishes the actual data.
void project_mcc_model_node_map_range(std::span<const MccModelNodeMap>,
    std::uint32_t encoded_offset, std::span<const std::byte> decoded_container,
    std::span<std::byte> delivered);
}
