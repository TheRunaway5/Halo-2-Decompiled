#pragma once
#include "halo2/source_transparent_parts.hpp"

namespace halo2 {
// Literal mode node/region fields and 137800 compound matrix blend records.
struct SourceModelCompoundNode {
    std::array<std::uint8_t,4> nodes{255,255,255,255};
    std::array<float,3> weights{};
};
void append_source_model_compounds(std::vector<std::array<float,12>> &,
    std::span<const SourceModelCompoundNode>);
struct SourceModelPaletteDefinition {
    std::uint16_t model_flags{};
    unsigned region_count{};
    std::array<unsigned,6> compound_node_counts{};
    std::array<std::vector<SourceModelCompoundNode>,6> compounds;
    std::vector<Transform> inverse_binds;
};
// One record for every source region, including a NONE selected section.
// Resident node maps retain their literal bytes; an empty map is distinct
// from the no-map section+24 count used by137800.
struct SourceModelPaletteRegion {
    std::uint8_t selected_section=255;
    std::uint8_t no_map_node_count{};
    std::vector<std::uint8_t> node_map;
};
struct SourceModelPaletteRange {std::uint16_t first{},count{};};
struct SourceModelPalette {
    std::uint16_t base_count{};
    std::vector<SourceModelPaletteRange> regions;
    // Original three float4 rows, including scale in the 3x3 basis.
    std::vector<std::array<float,12>> matrices;
};
//1376C0: actual object WORLD nodes in model order, after any original BD610
//composition, multiplied by each literal inverse bind using142A60.
std::vector<std::array<float,12>> source_model_skin_palette(
    std::span<const Transform> world_nodes,std::span<const Transform> inverse_binds);
//137800 resident palette and compound blend path. Caller supplies actual selected
//region bytes and current WORLD nodes, rather than geometry-preview order.
SourceModelPalette source_model_palette(const SourceModelPaletteDefinition &,
    unsigned geometry_lod,std::span<const Transform> world_nodes,
    std::span<const SourceModelPaletteRegion> regions);
//45CE0 mask4 uses the actual region range start, independently of rigid-node
//ordinal. Rigid NONE uses the palette's first matrix in either mode.
Vec3 source_model_palette_part_center(const SourceModelPalette &,
    unsigned region,std::int16_t rigid_node,std::uint16_t model_flags);
} // namespace halo2
