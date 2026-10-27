#pragma once
#include "halo2/cache.hpp"
#include <optional>

namespace halo2 {
// The compiled Xbox stream53 is FLOAT3, S1 SHORT2 and UB_D3D4. This
// owning record preserves the exact20 source bytes, including XYZ bits.
struct SourceDecoratorDecalVertexTu15 {
    std::array<std::byte,20> packed{};
    bool operator==(const SourceDecoratorDecalVertexTu15&) const = default;
};
enum class SourceDecoratorDecalStorageTu15 { Xbox20, Mcc32 };
// Decode the actual NV2A fetch: S1 normalizes signed shorts, UB_D3D4
// normalizes bytes and swaps B/R. Alpha remains an authored value here.
Vertex source_decorator_decal_vertex_tu15(const SourceDecoratorDecalVertexTu15&);
// Explicit MCC FLOAT3/FLOAT2/FLOAT2/COLOR conversion to stream53. Position
// bits and COLOR bytes survive; UV0 uses the source SHORT2 midpoint inverse.
// MCC's extra UV1 is not a fetched field in the original stream53.
SourceDecoratorDecalVertexTu15 source_decorator_decal_vertex_mcc_tu15(
    std::span<const std::byte,32>);

struct SourceDecoratorDecalResourceTu15 {
    std::uint8_t type{};
    std::uint16_t primary_locator{},secondary_locator{};
    std::uint32_t bytes{},offset{};
};
struct SourceDecoratorDecalCacheTu15 {
    unsigned ordinal{},header_bytes{};
    std::array<std::byte,52> authored_header{};
    std::uint32_t resource_pointer{},resource_bytes{},section_bytes{};
    std::vector<SourceDecoratorDecalResourceTu15> resources;
    SourceDecoratorDecalStorageTu15 storage{};
    std::uint8_t stream_type{},stream_stride{};
    std::vector<std::array<std::byte,24>> placements;
    std::vector<SourceDecoratorDecalVertexTu15> vertices;
    std::vector<std::uint16_t> indices;
};
struct SourceDecoratorDecalSelectionTu15 {
    std::uint32_t decorator_set{},shader{};
    std::uint8_t class_index{},permutation_index{},type{},flags{},fade_distance{},sprite_index{};
    std::array<std::byte,40> authored_permutation{};
};
struct SourceDecoratorDecalGroupTu15 {
    unsigned ordinal{};
    std::array<std::byte,24> authored{};
    std::int16_t cluster{},cache_block{},placement_first{},placement_count{},
        vertex_first{},vertex_count{},index_first{},index_count{};
    //41630 resolves group.byte1 as class, byte2 as permutation, then the
    // actual type at class+4 and shader index at permutation+4. Invalid
    // authored routes remain absent, matching the source skip corridor.
    std::optional<SourceDecoratorDecalSelectionTu15> selection;
};
struct SourceDecoratorDecalCellTu15 {
    std::array<std::byte,24> authored{};
    std::array<std::int16_t,8> children{};
    std::int16_t cache_block{},group_count{};
    std::int32_t group_first{};
};
struct SourceDecoratorDecalDefinitionTu15 {
    unsigned ordinal{};
    // Raw immutable Cache-domain provenance. Converted live source memory
    // owns a different address; use full BSP identity, ordinal and raw group
    // bytes when joining the completed command across those domains.
    std::uint32_t source_address{};
    Vec3 grid_origin{};
    std::int32_t cells_per_dimension{};
    std::vector<SourceDecoratorDecalCacheTu15> caches;
    // Other class families retain their authored group/cell data. Only
    // actual type1/2 cache resources become decal vertex/index banks.
    std::vector<SourceDecoratorDecalGroupTu15> groups;
    std::vector<SourceDecoratorDecalCellTu15> cells;
    std::vector<std::array<std::byte,64>> projected_placements;
};
struct SourceDecoratorDecalBspTu15 {
    unsigned bsp_index{};
    std::uint32_t bsp{};
    std::vector<std::uint32_t> palette;
    std::vector<SourceDecoratorDecalDefinitionTu15> definitions;
};
// This is an immutable asset adapter. It does not create an admitted-cell
// list, select a camera or claim visibility.3F960/41630 retain those owners.
}
