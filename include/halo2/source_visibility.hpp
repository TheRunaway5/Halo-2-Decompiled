#pragma once
#include "halo2/cache.hpp"

namespace halo2 {
// The relevant fields of the original 0x108-byte portal frustum. Bounds are
// source+0x60 in min/max axis order; six (normal.xyz,d) planes start at+0x78.
struct SourceSphereFrustum {
    std::array<float,6> bounds{};
    std::array<ShaderVector,6> planes{};
};
enum class SourceSphereVisibility : std::uint32_t {
    outside=0, intersecting=1, contained=2
};
// Retail181800 preserves its SSE operation order and inclusive rejection
// boundary. Plane4 participates in rejection, but not in the contained test.
// Neither the planes nor the supplied radius are normalized here.
SourceSphereVisibility source_sphere_frustum_visibility(
    const SourceSphereFrustum&,Vec3 center,float radius);

// One original 0x1A-byte cluster entry at set+0xA6E. The six source groups
// have signed counts at+2 and signed first indices at+0xE.
struct SourceClusterFrustumRanges {
    std::int16_t source_cluster=-1;
    std::array<std::int16_t,6> counts{},first_indices{};
};
struct SourceClusterFrustumSet {
    std::int16_t group_count{};
    std::vector<SourceClusterFrustumRanges> clusters;
    std::vector<SourceSphereFrustum> frustums;
};
struct SourceClusterFrustumResult {bool visible{},contained{};};
// Retail165010 aggregates each stored source group/range in order. The index
// addresses the constructed cluster-entry list, not an inferred containing
// cluster. Invalid positive ranges are unresolved inputs and are rejected.
SourceClusterFrustumResult source_cluster_frustum_visibility(
    const SourceClusterFrustumSet&,std::size_t section_index,Vec3 center,float radius);
}
