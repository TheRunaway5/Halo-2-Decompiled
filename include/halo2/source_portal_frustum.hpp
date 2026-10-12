#pragma once
#include "halo2/source_portal_clip.hpp"
#include "halo2/transform.hpp"

namespace halo2 {
// Original 23A430 positive-halfspace convex clipping. Plane is (nx,ny,d),
// with nx*x+ny*y-d; neutral vertices are retained only when there are also
// positive vertices, or when the polygon has no negative vertices. The
// all-inside source path copies the input without consulting capacity.
std::vector<Vec2> source_clip_portal_polygon_2d(
    std::array<float,3> plane,std::span<const Vec2> polygon,
    float epsilon=0.0001f,std::size_t output_capacity=64);

// Original 23A2B0 edge normalization and successive clipping, as called by
// 173380. Inputs are the projected subject and clip polygon in source winding.
// A nonempty subject needs at least one clipping edge; actual admitted portal
// and initial-camera polygons satisfy that source caller contract.
std::vector<Vec2> source_intersect_portal_polygons(
    std::span<const Vec2> subject,std::span<const Vec2> clip_polygon,
    float epsilon=0.0001f,std::size_t output_capacity=64);

// Fields consumed from the original 1BC camera bank by 173520. Rectangle uses
// source (minX,maxX,minY,maxY) order. The separate context plane is initialized
// to (0,0,-1,0) by173130; it is not the camera's authored near plane.
struct SourcePortalProjectionView {
    Transform world_to_view;
    Vec3 eye;
    ShaderVector rectangle{};
    std::vector<Vec2> initial_polygon;
    bool far_enabled{};
    float far_distance{};
    ShaderVector clipping_plane{0,0,-1,0};
};
struct SourcePortalGeometry {
    std::int16_t back=-1,front=-1;
    std::uint32_t plane_index{},flags{};
    std::vector<Vec3> vertices;
};
struct SourcePortalProjection {
    std::uint32_t flags{},classification{};
    std::int16_t back=-1,front=-1;
    float minimum_depth{};
    ShaderVector rectangle{};
    bool projected_eye_inside{};
    // Class0 retains the source clipped count, but its polygon scratch is
    // unwritten. Such a result reserves64 arena entries and is never consumed.
    std::uint16_t clipped_count{},arena_entries{};
    std::vector<Vec2> polygon;
};
// Untouched173520 projection/facing branch. This function computes one portal;
// source caching, queue/arena bounds and the live open-portal mask are separate.
SourcePortalProjection source_project_portal(
    const SourcePortalProjectionView&,const SourcePortalGeometry&,ShaderVector authored_plane);
}
