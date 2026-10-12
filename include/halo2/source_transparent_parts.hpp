#pragma once
#include "halo2/cache.hpp"
#include "halo2/transform.hpp"
#include <array>
#include <optional>
#include <span>
#include <vector>

namespace halo2 {
struct SourceInitialModelDefinition;
struct SourceInitialModelSelection;
struct SourceTransparentCollectorSection {unsigned region{},section{};};
// Original collector identities, preserving the region ordinal used by the
// parallel4C0D0 sort-order bank even when preceding regions have NONE sections.
std::vector<SourceTransparentCollectorSection> source_transparent_model_collector(
    const SourceInitialModelDefinition &,const SourceInitialModelSelection &,unsigned geometry_lod);
std::uint8_t source_transparent_model_bias_nibble(
    std::span<const std::int16_t> actual_region_sort_orders,unsigned actual_region,unsigned actual_model_count);
// Authored 72-byte part identity and fields consumed by45CE0/41040. Template
// masks are metadata; selecting an actual shader LOD remains a view service.
struct SourceTransparentPart {
    std::array<std::uint32_t,18> authored_words{};
    std::uint16_t ordinal{},type{},flags{};
    std::int16_t material=-1;
    std::uint16_t index_first{},index_count{},subpart_first{},subpart_count{};
    Vec3 center{};
    float lod_scale{};
    std::uint32_t shader=0xffffffffu;
    std::uint16_t shader_priority_flags{},shader_command_flags{}; //rawshader+16/+3C
    std::vector<std::uint32_t> template_lod_layers;
    std::optional<std::array<float,4>> plane; //1E280's authored key lookup
};
struct SourceTransparentPartSection {
    unsigned section{};
    std::int16_t rigid_node=-1; //model section+2C
    std::array<float,6> cluster_bounds{}; //BSP cluster+54..68, source coverage input
    std::vector<SourceTransparentPart> parts;
    //Literal model section+24 and resident data+64/+68. Empty map remains
    //empty;137800's no-map fallback is a separate operation.
    std::uint8_t no_map_node_count{};
    std::vector<std::uint8_t> resident_node_map;
};
struct SourceTransparentInstance {
    unsigned ordinal{},definition{};
    Transform transform{};
    Vec3 center{}; //instance+3C; deliberately independent of part center
    float radius{}; //instance+48, source coverage input
};
struct SourceTransparentPlane {
    std::uint16_t geometry{},part_and_instance{};
    std::array<float,4> plane{};
};
struct SourceTransparentBspParts {
    std::vector<SourceTransparentPartSection> clusters,instance_definitions;
    std::vector<SourceTransparentInstance> instances;
    std::vector<SourceTransparentPlane> planes; //source insertion order
};
struct SourceTransparentModelParts {
    std::uint32_t render_model=0xffffffffu;
    std::uint16_t model_flags{}; //rawmode+4; mask4 uses actual context node remap
    // Actual collector order, including duplicate sections and empty collectors.
    std::vector<SourceTransparentPartSection> sections;
};
struct SourceTransparentPartGeometry {
    std::uint32_t shader=0xffffffffu;
    Vec3 center{};
    float depth_bias{};
    std::optional<std::array<float,4>> plane;
    float lod_scale=1;
};
SourceTransparentPartGeometry source_transparent_cluster_geometry(const SourceTransparentPart &);
SourceTransparentPartGeometry source_transparent_instance_geometry(
    const SourceTransparentPart &,const SourceTransparentInstance &);
// world_node_positions is the actual posed context+44 matrix translation bank.
// For mode mask4, supply the actual context+4 remap result for this draw token;
// no authored node-map/default-node approximation is admitted.
SourceTransparentPartGeometry source_transparent_model_geometry(
    const SourceTransparentPart &,std::int16_t rigid_node,std::uint16_t model_flags,
    std::span<const Vec3> world_node_positions,
    std::optional<std::int16_t> actual_context_node,std::uint8_t actual_depth_bias_nibble);
}
