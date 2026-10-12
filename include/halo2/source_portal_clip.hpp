#pragma once
#include "halo2/cache.hpp"

namespace halo2 {
// Original 11FC80 plane clipping, bounded to the actual 173520 caller's
// 64-vertex scratch banks. Returns the emitted prefix when output capacity
// is exhausted; the original does not return a failure sentinel in that case.
// Plane convention is normal.xyz dot position - plane.w. Neither normals nor
// polygons are normalized or reordered here.
std::vector<Vec3> source_clip_portal_plane(
    ShaderVector plane,std::span<const Vec3> polygon,
    bool keep_positive=true,float epsilon=0.0078125f,
    std::size_t output_capacity=64);

// Original 173910/120850/23A220 orthogonal eye projection and convex-polygon
// test. Axis ties prefer Z, then Y. Empty polygons retain the source result.
bool source_portal_projected_eye_inside(
    ShaderVector plane,Vec3 eye,std::span<const Vec3> polygon);

struct SourcePortalFacing {
    // Literal 173890 result: 0 rejected; 1 negative plane side; 2 positive
    // plane side; 3 near-plane or fewer than three clipped vertices.
    std::uint32_t classification{};
    bool projected_eye_inside{};
};
// Original 173890 uses an x87 distance for the inclusive 0.015625 near-plane
// test, then its stored float for side classification. The polygon is the
// authored world-space portal, not the clipped view-space polygon; the latter
// supplies only its count. Full portal/frustum construction remains separate.
SourcePortalFacing source_portal_facing(
    ShaderVector plane,Vec3 eye,std::span<const Vec3> authored_polygon,
    int clipped_vertex_count);
}
