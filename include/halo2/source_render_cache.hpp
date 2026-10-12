#pragma once
#include "halo2/math.hpp"
#include <array>
#include <cstdint>
#include <span>

namespace halo2 {
// The original 0x100-byte cached object render-state record. Raw storage retains
// bytes that 2C3B0/3DDD0 deliberately leave untouched during pressure rebinds.
struct SourceRenderCacheRecord {
    std::array<std::uint32_t,64> words{};
    std::uint32_t owner() const { return words[1]; }
    std::uint32_t last_render_frame() const { return words[3]; }
    std::uint32_t geometry_token() const { return words[0x4c/4]; }
    std::uint32_t geometry_stamp() const { return words[0x50/4]; }
};
static_assert(sizeof(SourceRenderCacheRecord)==0x100);
struct SourceRenderContext { std::array<std::uint32_t,30> words{}; };
static_assert(sizeof(SourceRenderContext)==0x78);
// Complete fixed 2C3B0 initializer, including its partial-word writes.
void source_render_context_initialize(SourceRenderContext&);

struct SourceRenderCachePool {
    static constexpr std::uint32_t capacity=256;
    std::array<SourceRenderCacheRecord,capacity> records{};
    std::array<std::uint32_t,8> occupied{};
    std::uint32_t cursor{}, highwater{}, active_count{};
    std::uint16_t next_generation=0xe163;
    bool active{};
    // 2B540 -> 16B7A0. Reset clears salts and occupancy; other record bytes
    // survive unless the caller supplies original pool debug-fill flag3.
    void activate(bool debug_fill=false);
    void deactivate() { active=false; } // 2B5A0.
    // Complete 3DDD0 allocation/reuse/oldest-record rebind. These are actual
    // object+118/+11A permutations and BAD50 current RGBs, not tag defaults.
    std::uint32_t acquire(std::uint32_t owner, std::uint32_t& object_cache_token,
        std::span<const std::uint8_t> regions, std::span<const Vec3> current_rgb,
        std::uint32_t render_frame);
    // Literal full-owner lookup. Source does not check token generation or
    // occupancy here. The native input is bounded to the allocated 256 slots.
    SourceRenderCacheRecord* owner_record(std::uint32_t owner,std::uint32_t token);
    void mark_rendered(std::uint32_t owner,std::uint32_t object_cache_token,
                       std::uint32_t render_frame);
    std::int8_t view_lod(std::uint32_t owner,std::uint32_t token,std::int32_t view);
    // Complete 4C640: for views0..3 it calls 3DDD0 before the byte write.
    void set_view_lod(std::uint32_t owner,std::uint32_t& object_cache_token,
        std::span<const std::uint8_t> regions,std::span<const Vec3> current_rgb,
        std::uint32_t frame,std::int32_t view,std::int8_t lod);
};
std::uint32_t source_render_geometry_stamp(std::uint32_t frame,std::int32_t view);
// Literal ordinary 2B5D0 frame entry. Auxiliary 2C8A0 increments its frame
// without this arena reset; its other services are not included here.
void source_render_frame_begin(std::uint32_t& frame,std::uint32_t& arena_cursor);
struct SourceRenderResourceDecision {
    bool fallback_requested{}, succeeded{};
    std::int16_t actual_lod{};
    std::array<std::int8_t,2> cache_lod_writes{};
    std::uint8_t cache_lod_write_count{};
};
// 4C190 control AFTER literal initial/fallback 4C2B0 results. This does not
// replace residency, selected section outputs, or region-change production.
SourceRenderResourceDecision source_render_resource_decision(std::int16_t requested_lod,
    std::int8_t prior_view_lod,bool initial_succeeded,bool fallback_succeeded);
struct SourceRenderGeometryBegin {
    bool reuse_candidate{};
    std::int16_t selected_lod{};
};
SourceRenderGeometryBegin source_render_geometry_begin(bool context_present,
    std::uint32_t context_token,std::uint32_t context_stamp,std::int8_t prior_view_lod,
    std::int8_t requested_lod,std::uint32_t frame,std::int32_t view);

struct SourceRenderGeometryInput {
    SourceRenderGeometryBegin begin;
    std::int8_t requested_lod{};
    // Literal 4C190 outputs and 137650 return. Their residency/node services
    // are outside this helper and must be supplied by an actual producer.
    bool resource_succeeded{}, region_changed{};
    std::int16_t actual_lod{};
    std::int32_t node_count{}, source_model_ordinal{};
    bool first_person{}, context_present{};
    std::uint32_t context_token=0xffffffff,context_stamp{};
    std::uint32_t frame{};
    std::int32_t view{};
};
enum class SourceRenderGeometryContinuation { omitted, build_nodes, reuse_tail };
struct SourceRenderGeometryResult {
    SourceRenderGeometryContinuation continuation=SourceRenderGeometryContinuation::omitted;
    bool reused{}, context_written{};
    std::uint8_t node_count{};
    std::uint32_t token=0xffffffff,context_token=0xffffffff,context_stamp{};
};
// 4B801..4B912 / 4BA4D. Arena is shared by every view, reset by ordinary
// 2B5D0 once per frame. Signed overflow and the 28-bit token mask are literal.
SourceRenderGeometryResult source_render_geometry_allocate(
    const SourceRenderGeometryInput&,std::uint32_t& arena_cursor);
struct SourceRenderGeometryTail {
    bool call_prt{}, call_colors{}, call_owner_query{}, accepted{};
};
// The caller has completed 137800 and, when requested, 23690. A failed PRT
// return still calls colors/query. No query result is synthesized here.
SourceRenderGeometryTail source_render_geometry_tail(bool reused,bool geometry_succeeded,
    bool context_present,bool prt_enabled,bool prt_succeeded);
}
