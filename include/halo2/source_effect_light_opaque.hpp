#pragma once
#include "halo2/source_bsp_part_commands.hpp"
#include "halo2/source_opaque_sort.hpp"
#include "halo2/source_tag_metadata_store.hpp"
namespace halo2 {
struct SourceOpaqueGeometryMetadata {
    std::array<std::int8_t,2> selectors{}; // Actual44940 metadata+10/+11.
    std::uint16_t geometry_class{}; // Actual metadata+14 dispatcher class.
};
SourceOpaqueGeometryMetadata load_source_opaque_bsp_geometry_metadata(const Cache&,
    std::size_t bsp,MenuWorldSourceSectionKind,unsigned section);
SourceOpaqueGeometryMetadata load_source_opaque_model_geometry_metadata(const Cache&,
    std::uint32_t render_model,unsigned section);
struct SourceOpaqueGeometrySelection {
    SourceOpaqueGeometryMetadata metadata;
    std::span<const std::array<std::byte,72>> parts;
};
using SourceOpaqueGeometryService=std::function<SourceOpaqueGeometrySelection(unsigned unit)>;
// Actual1CB20 dispatcher. Geometry class3 adds2 to the signed selector,
// then checks the real VRTX class count; other classes do not read that bank.
std::uint32_t source_opaque_vertex_class(std::uint16_t geometry_class,std::int32_t selector,
    const std::function<std::uint32_t()>& vertex_class_count);
// Whole3BAB0 layer12 +ordinary-sphere4CBF0/15D70/30DA0/1CB20/4CBB0,
// followed by13DA70. The caller establishes original4BA014 bit0 and current
// command/unit/geometry ownership. Metadata retains actual source addresses:
// primary key is the compiled306-byte implementation's address, not a tag ID.
// Rejected selection mutates its current retained output slot as in source.
// Actual model and BSP metadata are supplied through the same geometry service.
// Other light implementations/layers and GPU draws remain separate owners.
// Returns the populated prefix of retained output storage.
unsigned source_effect_light_opaque_select(const SourcePartCommandBank&,
    std::span<const SourceCullDrawUnit>,const SourceTagMetadataProviders&,
    const SourceOpaqueGeometryService&,std::span<SourceOpaqueSortRecord>);
struct SourceOpaquePartRanges {
    MenuWorldSourceTopology topology{};
    bool filtered{};
    std::vector<MenuWorldSourceIndexRange> ranges;
};
// Exact4D5E2..4D626 and4DAA0 after successful actual1CAA0 binding. The
// caller owns that binding decision. Mask addresses refer to the same shared
// arena used by collection/culling; an unfiltered call never reads it.
SourceOpaquePartRanges source_effect_light_part_ranges(const SourceCullDrawUnit&,
    const SourceBspRangeGeometry&,unsigned part,const SourceBspRangeArena&,
    std::uint32_t mask_arena_address);
struct SourceOpaqueRangeGeometry {
    const SourceBspRangeGeometry* geometry{};
    bool vertex_bound{}; // Actual current1CAA0 result, not metadata presence.
};
using SourceOpaqueRangeGeometryService=std::function<SourceOpaqueRangeGeometry(
    unsigned command,unsigned unit,unsigned vertex_class)>;
struct SourceOpaqueRangeDraw {
    unsigned command{},unit{},part{},vertex_class{};
    SourceOpaquePartRanges selected;
};
// Consumes the already sorted3BAB0 prefix without regrouping materials or
// owners. Failed1CAA0 emits nothing; a successful empty filtered draw remains
// an explicit record. No resource residency, shader or GPU state is inferred.
std::vector<SourceOpaqueRangeDraw> source_effect_light_ordered_ranges(
    const SourcePartCommandBank&,std::span<const SourceCullDrawUnit>,
    std::span<const SourceOpaqueSortRecord>,const SourceBspRangeArena&,
    std::uint32_t mask_arena_address,const SourceOpaqueRangeGeometryService&);
}
