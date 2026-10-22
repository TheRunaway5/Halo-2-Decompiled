#include "halo2/posed_collision.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace halo2 {
namespace {
float coordinate(Vec3 p,unsigned index) {
  if(index>2)throw FormatError("Invalid original collision projection");
  return index==0?p.x:index==1?p.y:p.z;
}
template<class T> const T &checked(const std::vector<T> &values,std::size_t index) {
  if(index>=values.size())throw FormatError("Collision index outside authored block");
  return values[index];
}
unsigned projection_axis(Vec3 normal) {
  const float x=std::abs(normal.x),y=std::abs(normal.y),z=std::abs(normal.z);
  if(z>=y && z>=x)return 2;
  return y>=x?1:0;
}
Vec3 local_vector(const Transform &inverse,Vec3 value) {
  if(inverse.scale!=1) {
    value.x*=inverse.scale;value.y*=inverse.scale;value.z*=inverse.scale;
  }
  const auto &f=inverse.basis[0],&l=inverse.basis[1],&u=inverse.basis[2];
  return {((l.x*value.y+f.x*value.x)+u.x*value.z),
          ((u.y*value.z+l.y*value.y)+f.y*value.x),
          ((u.z*value.z+l.z*value.y)+f.z*value.x)};
}
CollisionPlane world_plane(const Transform &matrix,CollisionPlane plane,bool reverse) {
  const auto &f=matrix.basis[0],&l=matrix.basis[1],&u=matrix.basis[2];
  const auto n=plane.normal;
  Vec3 normal{(u.x*n.z+l.x*n.y)+f.x*n.x,
              (u.y*n.z+l.y*n.y)+f.y*n.x,
              (u.z*n.z+l.z*n.y)+f.z*n.x};
  float distance=((matrix.position.z*normal.z+matrix.scale*plane.distance)+
      matrix.position.y*normal.y)+matrix.position.x*normal.x;
  if(reverse) {
    normal={0.f-normal.x,0.f-normal.y,0.f-normal.z};distance=0.f-distance;
  }
  return {normal,distance};
}
int surface_2d(const RawCollisionBsp &bsp,std::int16_t index,Vec2 point) {
  std::size_t visits=0;
  while(index>=0) {
    if(++visits>bsp.nodes2.size())throw FormatError("Cyclic authored 2D collision tree");
    const auto &node=checked(bsp.nodes2,unsigned(index));
    const float value=(node.normal.y*point.y+node.normal.x*point.x)-node.distance;
    index=node.children[value>=0?1:0];
  }
  return index==-1?-1:std::uint16_t(index)&0x7fff;
}
struct LowerHit {
  float fraction=std::numeric_limits<float>::max();
  int crossing_plane=-1,leaf=-1,surface=-1;
};
struct Traversal {
  const RawCollisionBsp &bsp;
  const OriginalData &original;
  Vec3 origin,delta;
  unsigned flags;
  LowerHit &hit;
  int previous_leaf=-1,previous_class=0,crossing_plane=-1;
  std::size_t visits=0;
  std::vector<std::int32_t> *visited_leaves{};
  std::span<const std::uint32_t> breakable_bits{};
  unsigned breakable_count{};
  int candidate_surface(int leaf,float fraction,bool containment) {
    if(crossing_plane<0)return -1;
    const auto &candidate=checked(bsp.leaves,unsigned(leaf));
    for(unsigned i=candidate.first;i<unsigned(candidate.first)+candidate.count;++i) {
      const auto &ref=checked(bsp.references,i);
      if((std::uint16_t(ref.plane)&0x7fff)!=unsigned(crossing_plane))continue;
      const auto &plane=checked(bsp.planes,unsigned(crossing_plane));
      const unsigned axis=projection_axis(plane.normal);
      const bool orientation=(coordinate(plane.normal,axis)>0)!=bool(std::uint16_t(ref.plane)&0x8000);
      const auto &projection=original.collision_projection[axis*2+unsigned(orientation)];
      const Vec3 point{delta.x*fraction+origin.x,delta.y*fraction+origin.y,delta.z*fraction+origin.z};
      const Vec2 projected{coordinate(point,unsigned(projection[0])),coordinate(point,unsigned(projection[1]))};
      const int surface=surface_2d(bsp,ref.node,projected);
      if(!containment || (surface>=0 && collision_surface_contains(bsp,unsigned(surface),projected,projection,
          breakable_bits,breakable_count)))
        return surface;
    }
    return -1;
  }
  bool walk(std::int32_t index,float low,float high,unsigned depth=0) {
    // Bounds detect malformed input without changing finite original decisions.
    if(depth>bsp.nodes.size()+1)throw FormatError("Cyclic authored 3D collision tree");
    while((std::uint32_t(index)&0x800000u)==0) {
      if(++visits>1000000)throw FormatError("Invalid authored collision traversal");
      const auto &node=checked(bsp.nodes,std::uint32_t(index)&0x7fffff);
      if(node.plane<0)throw FormatError("Negative collision node plane");
      const auto &plane=checked(bsp.planes,unsigned(node.plane));const auto n=plane.normal;
      const float p=((origin.z*n.z+origin.y*n.y)+origin.x*n.x)-plane.distance;
      const float q=(delta.z*n.z+delta.y*n.y)+delta.x*n.x;
      const bool first=(q*low+p)>=0,last=(q*high+p)>=0;
      if(first==last) {index=node.children[first?1:0];continue;}
      const float crossing=float(-(double(p)/double(q)));
      if(walk(node.children[q<=0?1:0],low,crossing,depth+1))return true;
      if(crossing>=hit.fraction)return false;
      crossing_plane=node.plane;
      index=node.children[q>0?1:0];low=crossing;
    }
    const int leaf=index==-1?-1:int(std::uint32_t(index)&0x7fffff);
    const int classification=leaf<0?3:((checked(bsp.leaves,unsigned(leaf)).flags&1)?2:1);
    int candidate=-1;bool containment=false;
    if((flags&1) && (previous_class==1 || previous_class==2) && classification==3)
      candidate=previous_leaf;
    else if((flags&2) && previous_class==3 && (classification==1 || classification==2))
      candidate=leaf;
    else if(!(flags&4) && previous_class==2 && classification==2) {
      candidate=(flags&1)?previous_leaf:leaf;containment=true;
    }
    if(candidate>=0) {
      const int selected=candidate_surface(candidate,low,containment);
      if(selected>=0) {
        const auto &surface=checked(bsp.surfaces,unsigned(selected));
        if(!((flags&8) && (surface.flags&2)) && !((flags&16) && (surface.flags&8))) {
          hit={low,crossing_plane,candidate,selected};return true;
        }
      }
    }
    if(leaf>=0 && visited_leaves) {
      if(visited_leaves->size()<256)visited_leaves->push_back(leaf);
      else visited_leaves->back()=leaf; // Original overflow store is result+418.
    }
    previous_leaf=leaf;previous_class=classification;return false;
  }
};
}
bool collision_surface_contains(const RawCollisionBsp &bsp,unsigned index,Vec2 point,
    std::array<std::int16_t,3> projection,std::span<const std::uint32_t> breakable_bits,
    unsigned breakable_count) {
  const auto &surface=checked(bsp.surfaces,index);
  if(!breakable_bits.empty() && (surface.flags&8) && surface.breakable<breakable_count) {
    if(surface.breakable/32>=breakable_bits.size())throw FormatError("Truncated breakable collision mask");
    if(!(breakable_bits[surface.breakable/32]&(1u<<(surface.breakable&31))))return false;
  }
  unsigned current=surface.first_edge;std::size_t visits=0;
  do {
    if(++visits>bsp.edges.size())throw FormatError("Cyclic authored surface edge graph");
    const auto &edge=checked(bsp.edges,current);
    const unsigned right=edge.surfaces[1]==index?1:0;
    const auto &a=checked(bsp.vertices,edge.vertices[right]);
    const auto &b=checked(bsp.vertices,edge.vertices[1-right]);
    const float ax=coordinate(a,unsigned(projection[0])),ay=coordinate(a,unsigned(projection[1]));
    const float bx=coordinate(b,unsigned(projection[0])),by=coordinate(b,unsigned(projection[1]));
    const float first=(by-ay)*(point.x-ax),second=(point.y-ay)*(bx-ax);
    if(first-second>0)return false;
    current=edge.next[right];
  } while(current!=surface.first_edge);
  return true;
}
RawCollisionResult raw_collision(const RawCollisionBsp &bsp,const OriginalData &original,
    Vec3 origin,Vec3 delta,unsigned flags,std::span<const std::uint32_t> breakable_bits,
    unsigned breakable_count,float previous_fraction) {
  RawCollisionResult result;LowerHit hit;hit.fraction=previous_fraction;
  Traversal traversal{bsp,original,origin,delta,flags,hit};
  traversal.visited_leaves=&result.visited_leaves;
  traversal.breakable_bits=breakable_bits;traversal.breakable_count=breakable_count;
  if(!bsp.nodes.empty() && traversal.walk(0,0.f,1.f)) {
    const auto &surface=checked(bsp.surfaces,unsigned(hit.surface));
    result.hit=RawCollisionHit{hit.fraction,hit.crossing_plane,hit.leaf,hit.surface,
        surface.plane,surface.material,surface.flags,surface.breakable};
  }
  result.prior_leaf=traversal.previous_leaf;
  result.prior_class=traversal.previous_class;
  result.prior_plane=traversal.crossing_plane;
  return result;
}
std::optional<PosedCollisionHit> posed_collision(const ModelCollision &model,
    std::span<const Transform> nodes,std::span<const std::uint8_t> active,
    const OriginalData &original,Vec3 origin,Vec3 delta,unsigned flags,float previous_fraction) {
  std::array<std::int8_t,16> selections;selections.fill(-1);
  if(active.size()<model.model_regions.size())throw FormatError("Incomplete model collision permutation state");
  for(unsigned i=0;i<model.model_regions.size();++i) {
    const auto &region=model.model_regions[i];const auto permutation=active[i];
    if(region.collision==255 || permutation>=128 || permutation>=region.permutations.size())continue;
    const auto selected=region.permutations[permutation];if(selected==255)continue;
    if(region.collision>=selections.size())throw FormatError("Collision region outside source selection bank");
    selections[region.collision]=std::int8_t(selected);
  }
  LowerHit best;std::optional<PosedCollisionHit> result;
  for(unsigned region=0;region<model.regions.size();++region) {
    const int permutation=selections[region];
    if(permutation<0 || unsigned(permutation)>=model.regions[region].size())continue;
    const auto &bank=model.regions[region][permutation];
    for(unsigned index=0;index<bank.size();++index) {
      const auto &bsp=bank[index];
      if(bsp.node<0 || unsigned(bsp.node)>=nodes.size())throw FormatError("Missing current collision node matrix");
      const auto &matrix=nodes[bsp.node];const auto inverse=inverse_transform(matrix);
      auto local_origin=local_vector(inverse,origin);
      local_origin.x+=inverse.position.x;local_origin.y+=inverse.position.y;local_origin.z+=inverse.position.z;
      const auto local_delta=local_vector(inverse,delta);
      const float high=std::clamp(best.fraction,0.f,1.f);
      best.fraction=std::max(best.fraction,0.f);
      Traversal traversal{bsp,original,local_origin,local_delta,flags,best};
      if(bsp.nodes.empty() || !traversal.walk(0,0,high))continue;
      const auto &surface=checked(bsp.surfaces,unsigned(best.surface));
      const auto &material=checked(model.materials,std::uint16_t(surface.material));
      result=PosedCollisionHit{best.fraction,
          world_plane(matrix,checked(bsp.planes,unsigned(best.crossing_plane)),bool(std::uint16_t(surface.plane)&0x8000)),
          (region<<24)|(unsigned(permutation)<<16)|(index<<8)|std::uint8_t(bsp.node),
          best.leaf,best.surface,surface.plane,std::int16_t(region),bsp.node,
          surface.material,material.global_material,surface.flags,surface.breakable};
    }
  }
  if(result && result->fraction<previous_fraction)return result;
  return {};
}
}
