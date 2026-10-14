#pragma once
#include "halo2/menu_world_source_geometry.hpp"
#include "halo2/source_draw_unit_cull.hpp"
#include "halo2/source_portal_visibility.hpp"

namespace halo2 {
// Literal resident BSP geometry counts and unchanged type0/8/40/48 resources.
// This is not the rendered-material/LOD or raw vertex binding producer.
struct SourceBspRangeGeometry {
    std::int32_t part_count{},subpart_count{},accelerator_count{};
    std::uint32_t index_count{};
    std::vector<std::array<std::byte,72>> parts;
    std::vector<std::array<std::byte,8>> subparts;
    std::vector<std::byte> accelerator;
    std::vector<std::uint16_t> mapping;
    std::vector<SourceCullPartBound> bounds; // Literal type16/channel20.
};
// Borrowed positive accelerator or owned linear bound/subpart banks for45560.
// The returned accelerator pointer requires this geometry to outlive the call.
SourceCullLinearGeometry source_bsp_sphere_cull_geometry(const SourceBspRangeGeometry&);
SourceBspRangeGeometry source_bsp_range_geometry(const MenuWorldSourceSection &);
// The same resident part/subpart/index ABI for an actual selected model
// section. Model pose/palette and vertex binding remain separate owners.
SourceBspRangeGeometry source_model_range_geometry(const MenuWorldSourceSection &);
// Range-only model loader, independent of raw vertex/declaration support.
SourceBspRangeGeometry load_source_model_range_geometry(const Cache&,std::uint32_t render_model,unsigned section);
// Range-only loader does not require a supported vertex format/material pass.
// All scheduled BSP units consume mask arena space, including units whose raw
// vertex path remains unsupported by the renderer.
SourceBspRangeGeometry load_source_bsp_range_geometry(const Cache &,std::size_t bsp,
    MenuWorldSourceSectionKind,unsigned section);

// Original1332B0 does not clear storage. A positive request whose end is>=512
// aliases offset0 and leaves used unchanged. Null is only the nonpositive path.
struct SourceBspRangeArena {
    std::array<std::uint32_t,512> words{};
    std::uint16_t used{};
    std::optional<unsigned> allocate(std::int32_t subpart_count);
};
//1348E0's arena portion: clear only the previously allocated prefix, then
// reset its cursor. Main and secondary query contexts own separate arenas.
void source_bsp_range_arena_begin(SourceBspRangeArena&);
struct SourceBspQueryResetState {
    std::array<std::uint16_t,4> queue_counts{}; // Current context+C/10/14/18 headers+4.
    std::array<std::uint32_t,16> query_mask{}; // Context+21C.
    SourceBspRangeArena arena;
    std::uint16_t association_count{}; // Context+A5E.
    std::array<std::uint8_t,512> cluster_entries{}; // Context+1C.
};
// Whole1348E0 over the current query's retained fields. Queue capacities,
// pointer columns, inactive mask words and other context bytes are not reset.
void source_bsp_query_begin(SourceBspQueryResetState&);

// Original44E90's six source planes become accelerator(nx,ny,nz,-d).
// The optional transform is the actual draw-unit+1C bank, including its scale.
// Every SSE addition/subtraction and reciprocal is kept in source order.
std::array<ShaderVector,6> source_bsp_accelerator_planes(
    const std::array<ShaderVector,6> &,const std::optional<Transform> &);

// Original458D0(first tree) /45A80(second tree), range-mode1. Direct list keys
// and inclusive range endpoint pairs are actual2DD930 outputs. Existing mask
// bits survive. Rejected input leaves flags and mask untouched.
void source_bsp_expand_accelerator_lists(const SourceBspRangeGeometry &,
    bool parts_tree,std::span<const std::uint32_t> direct,
    std::span<const std::uint32_t> ranges,std::span<std::uint32_t> mask,
    std::uint32_t &unit_flags);

// Original4DAA0 emits separate ordered indexed calls. Triangle lists coalesce
// only adjacent index intervals; strips retain one call per marked subpart.
// A complete filtered empty result has no ranges, unlike unsupported input.
std::vector<MenuWorldSourceIndexRange> source_bsp_part_ranges(
    const SourceBspRangeGeometry &,unsigned part,MenuWorldSourceTopology,
    bool filtered,std::span<const std::uint32_t> mask);

struct SourceBspFrustumDrawBank {
    const SourceBspRangeGeometry *geometry{};
    bool geometry_resolved{};
    std::optional<Transform> transform;
};
struct SourceBspFrustumCullContext {
    std::uint32_t main_prefix{},draw_count{},mode{};
    std::int32_t query_mode{};
    std::uint32_t mask_arena_address{}; // Actual context+25C backing identity.
};
// Actual44720 query0 +44E90 accelerated corridor. Cluster-entry ordinals and
// association bits are supplied by the joined source frame, not authored IDs.
// Units and shared-address mask banks commit only when every consumed service
// completes. Unsupported geometry/traversal cannot expose a partial result.
void source_bsp_frustum_cull(std::span<SourceCullDrawUnit>,
    std::span<const SourceBspFrustumDrawBank>,SourceBspRangeArena &,
    const SourceBspFrustumCullContext &,const SourcePortalVisibilitySet &,
    std::span<const std::array<std::uint32_t,4>> associations);
} // namespace halo2
