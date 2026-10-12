#include "halo2/terrain_outer.hpp"
#include <bit>
#include <cmath>

namespace halo2 {
namespace {
template<class T> const T &checked(const std::vector<T> &rows,std::size_t index) {
  if(index>=rows.size())throw FormatError("Outer terrain index outside original block");
  return rows[index];
}
const RawCollisionBsp &ordinary_bsp(const TerrainCollision &terrain) {
  if(terrain.bsps.size()!=1)throw FormatError("Multiple terrain collision BSPs are outside recovered outer scope");
  return terrain.bsps[0];
}
TerrainLocation location_from_leaf(const TerrainCollision &terrain,std::int32_t leaf,std::int16_t bsp) {
  return {leaf,leaf==-1?std::int16_t(-1):checked(terrain.leaf_clusters,std::size_t(leaf)),bsp};
}
Vec3 segment_point(Vec3 start,Vec3 delta,float fraction) {
  const float x=delta.x*fraction,y=delta.y*fraction,z=delta.z*fraction;
  return {x+start.x,y+start.y,z+start.z};
}
CollisionPlane reverse_plane(CollisionPlane plane) {
  return {{0.f-plane.normal.x,0.f-plane.normal.y,0.f-plane.normal.z},0.f-plane.distance};
}
}
std::int32_t terrain_leaf(const RawCollisionBsp &bsp,Vec3 point,std::uint32_t root) {
  auto child=std::int32_t(root);
  do {
    const auto &node=checked(bsp.nodes,std::uint32_t(child)&0x7fffffu);
    const auto &plane=checked(bsp.planes,std::size_t(node.plane));
    const float z=plane.normal.z*point.z,y=plane.normal.y*point.y,x=plane.normal.x*point.x;
    const float zy=z+y,zyx=zy+x,distance=zyx-plane.distance;
    // COMISS unordered also takes the negative child; this fixture uses finite points.
    child=node.children[distance>=0?1:0];
  }while(!(std::uint32_t(child)&0x800000u));
  return child==-1?-1:std::int32_t(std::uint32_t(child)&0x7fffffu);
}
std::int32_t terrain_location(const TerrainCollision &terrain,Vec3 point,std::int16_t active_bsp,
                              TerrainLocation &location) {
  if(active_bsp==-1)throw FormatError("Current-BSP NONE locator return was not executed in source fixture");
  location=location_from_leaf(terrain,terrain_leaf(ordinary_bsp(terrain),point),active_bsp);
  return location.cluster;
}
CollisionPlane terrain_oriented_plane(const RawCollisionBsp &bsp,std::uint16_t signed_plane) {
  const auto plane=checked(bsp.planes,signed_plane&0x7fffu);
  return signed_plane&0x8000?reverse_plane(plane):plane;
}
TerrainOuterResult ordinary_terrain_query(const TerrainCollision &terrain,const OriginalData &original,
    Vec3 start,Vec3 delta,std::uint32_t mask,std::int16_t active_bsp,
    std::span<const std::uint32_t> breakable_bits,unsigned breakable_count,
    std::int16_t default_global_material) {
  if((mask&0xe) || !(mask&1) || active_bsp==-1)
    throw FormatError("Nonordinary outer collision mask/BSP is outside recovered scope");
  const auto &bsp=ordinary_bsp(terrain);
  if(!(mask&0x01800000u))mask|=0x01800000u;
  TerrainOuterResult result;result.initial.bsp=result.final.bsp=active_bsp;
  result.lower=raw_collision(bsp,original,start,delta,(mask>>23)&0x3f,
      breakable_bits,breakable_count);
  if(!result.lower.visited_leaves.empty()) {
    result.initial=location_from_leaf(terrain,result.lower.visited_leaves.front(),active_bsp);
    result.final=location_from_leaf(terrain,result.lower.visited_leaves.back(),active_bsp);
  }
  if(result.lower.hit) {
    const auto &hit=*result.lower.hit;result.type=1;result.fraction=hit.fraction;
    result.global_material=hit.material==-1?default_global_material:
        checked(terrain.global_materials,std::size_t(hit.material));
    const auto plane=checked(bsp.planes,std::size_t(hit.crossing_plane));
    result.plane=(std::uint32_t(hit.signed_plane)&0x8000u)?reverse_plane(plane):plane;
  }
  result.point=segment_point(start,delta,result.fraction);
  if(!result.type || !(mask&0x20000000u) || result.final.leaf==-1 ||
      terrain_leaf(bsp,result.point)==result.final.leaf)return result;
  const auto normal=result.plane->normal;
  const auto epsilon=std::bit_cast<float>(0x39800000u);
  const float x=epsilon*normal.x,y=epsilon*normal.y,z=epsilon*normal.z;
  result.point={x+result.point.x,y+result.point.y,z+result.point.z};
  result.normal_offset_point=result.point;
  terrain_location(terrain,result.point,active_bsp,result.final);
  if(result.final.leaf!=-1)return result;
  const float nz=normal.z*delta.z,ny=normal.y*delta.y,nx=normal.x*delta.x;
  const float zy=nz+ny,dot=zy+nx;
  // Original x87 FDIVR then float32 store. The zero-dot fallback is static
  // source evidence only; no executed parity claim is made for it.
  const float step=dot==0?std::bit_cast<float>(0x3d000000u):float(double(epsilon)/std::abs(double(dot)));
  if(!(step>0) || !std::isfinite(step))throw FormatError("Unordered/underflow retreat step is outside measured scope");
  do {
    TerrainRetreatStep row;row.step=step;row.fraction_before=result.fraction;
    row.point_before=result.point;row.location_before=result.final;
    const float next=result.fraction-step;
    result.fraction=next>0?next:0;
    result.point=segment_point(start,delta,result.fraction);
    terrain_location(terrain,result.point,active_bsp,result.final);
    row.fraction_after=result.fraction;row.point_after=result.point;row.location_after=result.final;
    result.retreat.push_back(row);
  }while(result.fraction>0 && result.final.leaf==-1);
  return result;
}
}
