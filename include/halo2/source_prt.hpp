#pragma once
#include "halo2/cache.hpp"
#include "halo2/transform.hpp"

namespace halo2 {
struct SourcePrtSection {std::uint32_t section{},offset{};};
struct SourcePrtLod {std::uint32_t cluster_basis_offset{};std::vector<SourcePrtSection> sections;};
struct SourcePrtDefinition {
    std::uint32_t render_model{};
    std::uint16_t order{},clusters{},pca_vectors{};
    std::vector<float> cluster_basis;
    std::vector<SourcePrtLod> lods;
    std::array<std::byte,32> stream_descriptor{};
    std::vector<std::byte> stream;
};
struct SourcePrtVertex {float cluster{};ShaderVector weights{};};
//24490 offsets source57/10 by(offset/5)*10. MCC56/12 expands the cluster
// short to float, retaining the five-short LOD offset units and weight bytes.
// Absent sections returnNONE.
std::optional<std::vector<SourcePrtVertex>> source_prt_vertex_stream(
    const SourcePrtDefinition &,unsigned source_lod,unsigned section,unsigned vertex_count);
struct SourcePrtSh {
    std::array<std::array<float,9>,3> coefficients{};
    Vec3 incident{};
    bool incident_valid{}; // Empty22CF0 bank leaves caller incident unchanged.
};
//24040 bank: c68 world incident, c69 scale constants, c70+ node0/1.
//Nodes contain the source sampled SH after the exact inverse-node rotation.
std::shared_ptr<const VertexProgramBank> source_prt_bank(const SourcePrtDefinition &,
    unsigned source_lod,std::span<const SourcePrtSh> rotated_nodes);
//23690 retains the final sampled incident for the entire object.24040 binds
//that same incident for every section, independently of its coefficient bank.
std::shared_ptr<const VertexProgramBank> source_prt_bank(const SourcePrtDefinition &,
    unsigned source_lod,std::span<const SourcePrtSh> rotated_nodes,Vec3 object_incident);
struct SourcePrtSectionBank {unsigned section{};std::shared_ptr<const VertexProgramBank> bank;};
// A completed layer5 bank for one owner and one resident source material.
// The image/pass owner stays resident; this immutable bank follows the draw.
struct SourcePrtMaterialBank {
    unsigned material{};std::shared_ptr<const VertexProgramBank> bank;
    unsigned section{};
};
struct SourcePrtShRecord {Vec3 position{};SourcePrtSh lighting;};
struct SourcePrtLighting {bool structure_matches{};std::vector<SourcePrtShRecord> records;};
//1376C0+23600: query the actual world node position, and rotate SH
//with the scaled world-node × inverse-bind render palette.
SourcePrtSh source_prt_node_lighting(const RenderModel &,unsigned node,
    const Transform &world_node,const SourcePrtLighting &);
//Section indices must be in the source draw collector's selected order.
//The current world-node bank is supplied by the actual posed object hierarchy.
std::vector<SourcePrtSectionBank> source_prt_section_banks(const Cache &,
    const RenderModel &,const SourcePrtDefinition &,unsigned source_lod,
    std::span<const unsigned> selected_sections,std::span<const Transform> world_nodes,
    const SourcePrtLighting &,std::optional<Vec3> retained_incident = {});
// Original22CF0/22C30: nearest sample, then nearest opposite halfspace,
// followed by the original projected interpolation. No triangle heuristic.
SourcePrtSh source_prt_sample(std::span<const SourcePrtShRecord>,Vec3 position);
// Original239BA determinant and142BF0 inverse, for the source 3x3 bank.
std::array<float,9> source_prt_inverse_basis(const std::array<float,9>&);
// Original143600, order3. Input is the exact inverse node rotation from
// 142BF0; source coefficient rotation uses signed axis ordering.
std::array<float,9> source_prt_rotate(std::array<float,9>,const std::array<float,9>& inverse_rotation);
// Original23CA0. Output is c70 onward: ambient RGB + padding, then
// per-channel PCA vectors for each cluster. The source LOD index is0..5.
std::vector<ShaderVector> source_prt_project(const SourcePrtDefinition &,
    unsigned source_lod,const SourcePrtSh &);
// 24040 c69, retained as raw source float bits.
ShaderVector source_prt_scale_constants();
}
