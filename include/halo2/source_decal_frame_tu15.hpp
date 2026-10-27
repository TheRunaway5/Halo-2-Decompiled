#pragma once
#include "halo2/source_decal_vertex_cache.hpp"

namespace halo2 {
// Owning readback of one quiescent TU1.5 cache. Full salted allocation tokens
// share pages only within this value; a later frame may replace any allocation.
struct SourceDecalFrameRecordTu15 {
    std::uint32_t datum{},definition{},cache_datum{},tick{},tint{};
    std::int16_t cluster=-1;
    std::uint8_t layer{},flags{};
    float lifetime{},decay{};
    std::uint32_t primary_datum{},next_cluster_datum{};
    std::int8_t bitmap_index{};
    std::uint16_t quad_count{};
};
struct SourceDecalFrameAllocationTu15 {
    std::uint32_t cache_datum{};
    std::vector<std::array<SourceDecalCacheVertex,4>> quads;
};
struct SourceDecalFrameHeadTu15 {
    std::uint16_t cluster{};
    std::uint8_t layer{};
    std::uint32_t datum{};
};
struct SourceDecalFrameTu15 {
    // Actual original game clock; not a borrowed pointer or render stamp.
    std::uint32_t tick{};
    std::vector<SourceDecalFrameRecordTu15> records;
    std::vector<SourceDecalFrameAllocationTu15> allocations;
    // Preserve all seven original head banks. Ordinary rendering owns0..5;
    // layer6 is a distinct original diagnostic/special pass.
    std::vector<SourceDecalFrameHeadTu15> heads;
};
inline constexpr unsigned source_decal_record_capacity_tu15=1024;
inline constexpr unsigned source_decal_page_capacity_tu15=1280;
inline constexpr unsigned source_decal_head_layers_tu15=7;
inline constexpr unsigned source_decal_cluster_capacity_tu15=512;

SourceDecalCacheVertex source_decal_frame_vertex_tu15(Vec3,std::uint32_t packed);
void validate_source_decal_frame_tu15(const SourceDecalFrameTu15&);
SourceDecalFrameTu15 source_decal_frame_tu15(const SourceRuntimeMemory&);
}
