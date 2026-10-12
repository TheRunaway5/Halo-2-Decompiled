#pragma once
#include "halo2/posed_collision.hpp"

namespace halo2 {
struct TerrainLocation {
  std::int32_t leaf=-1;
  std::int16_t cluster=-1,bsp=-1;
};
static_assert(sizeof(TerrainLocation)==8);
// Original14B9D0 packed-node classifier, ordinary root0.
std::int32_t terrain_leaf(const RawCollisionBsp &,Vec3 point,std::uint32_t root=0);
// Original11D4A0 output8 bytes and EAX (cluster or NONE, rather than leaf).
std::int32_t terrain_location(const TerrainCollision &,Vec3 point,std::int16_t active_bsp,
                              TerrainLocation &);
CollisionPlane terrain_oriented_plane(const RawCollisionBsp &,std::uint16_t signed_plane);
struct TerrainRetreatStep {
  float step{},fraction_before{},fraction_after{};
  Vec3 point_before,point_after;
  TerrainLocation location_before,location_after;
};
struct TerrainOuterResult {
  std::uint32_t type{};
  float fraction=1;
  Vec3 point;
  TerrainLocation initial,final;
  RawCollisionResult lower;
  std::optional<CollisionPlane> plane;
  std::optional<std::int16_t> global_material;
  std::optional<Vec3> normal_offset_point;
  std::vector<TerrainRetreatStep> retreat;
};
// Bounded16B240 ordinary-terrain branch: no active posed/instanced/Havok
// contestants. Segment/active BSP/breakable/default material inputs are explicit.
// Lower contact and visited leaves remain untouched by outer correction.
TerrainOuterResult ordinary_terrain_query(const TerrainCollision &,const OriginalData &,
    Vec3 start,Vec3 delta,std::uint32_t mask,std::int16_t active_bsp,
    std::span<const std::uint32_t> breakable_bits,unsigned breakable_count,
    std::int16_t default_global_material);
}
