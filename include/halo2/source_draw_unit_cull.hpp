#pragma once
#include "halo2/transform.hpp"
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace halo2 {
struct SourceBspRangeGeometry;
struct SourceBspRangeArena;
// Literal retail4BA138 record. Pointer words retain their source meaning; the
// separately supplied banks provide the bytes behind those pointers.
struct SourceCullDrawUnit {std::array<std::uint32_t,8> words{};};
static_assert(sizeof(SourceCullDrawUnit)==32);
struct SourceCullPartBound {Vec3 center{};float radius{};std::uint8_t node{};};
struct SourceCullOwnerPose {
    std::uint32_t owner{}; // Actual first word behind draw-unit+10.
    std::vector<Transform> nodes; // Original B8C00 current node bank.
    std::vector<Transform> inverse_bind; // mode+4C, node stride60, transform+28.
};
struct SourceCullLinearGeometry {
    std::int32_t accelerator_count{}; // Actual geometry+30.
    std::int32_t subpart_count{}; // Signed geometry+8; inactive storage is retained.
    std::vector<std::uint16_t> subpart_bounds; // geometry+C subparts, stride8,+4; FFFF admits.
    std::vector<SourceCullPartBound> bounds; // geometry+14, stride14.
    std::optional<SourceCullOwnerPose> owner_pose;
    // Borrowed resident geometry+2C bytecode and literal mapping. Consumed
    // only by the positive accelerator path; must outlive this cull call.
    const SourceBspRangeGeometry *accelerator{};
    // The original reads the context owner and WORLD nodes only for an
    // unmasked, bounded model subpart. Resolve live banks at that point;
    // accelerator, FFFF bounds and already selected subparts do not read them.
    std::function<SourceCullOwnerPose()> resolve_owner_pose;
};
struct SourceCullPartMask {
    std::uint32_t source_address{}; // Literal draw-unit+14 backing identity.
    std::vector<std::uint32_t> words; // One admission bit per subpart, not material part.
};
struct SourceCullDrawBanks {
    std::optional<Transform> transform; // Actual draw-unit+1C, when nonzero.
    std::optional<SourceCullLinearGeometry> geometry; // Null geometry skips.
    bool geometry_resolved{}; // Distinguishes observed null from absent lookup.
};
struct SourceDrawUnitCullContext {
    std::uint32_t main_prefix{},draw_count{},mode{}; // 4C0B7C,4C0B78,+2A78.
    std::int32_t query_mode{}; // Context+2A7C; only sphere1 is implemented.
    Vec3 center{};float radius{}; // Context+2A68/+2A74.
};
struct SourceDrawUnitSphereCall {std::uint16_t unit{};Vec3 center{};float radius{};};
// Complete44720 sphere branch and45560 linear/accelerated branches. The current
// model/BSP/instance resolver must supply the actual per-unit geometry, rather
// than whole-model bounds. Query radius is deliberately unchanged by transforms.
// Masks are keyed by the actual backing address, preserving repeated-unit
// writes and shared masks in source order. Missing consumed banks,
// non-sphere eligible units and missing positive accelerator banks
// reject unresolved services. Skipped units do not demand unused metadata.
std::vector<SourceDrawUnitSphereCall> source_draw_unit_sphere_cull(
    std::span<SourceCullDrawUnit>,std::span<SourceCullDrawBanks>,
    std::span<SourceCullPartMask>,const SourceDrawUnitCullContext&);
// The live context owns one 512-word arena. Distinct mask starts can overlap
// after1332B0 overflows; every read/write must observe that shared storage.
std::vector<SourceDrawUnitSphereCall> source_draw_unit_sphere_cull(
    std::span<SourceCullDrawUnit>,std::span<SourceCullDrawBanks>,
    SourceBspRangeArena&,std::uint32_t mask_arena_address,
    const SourceDrawUnitCullContext&);
}
