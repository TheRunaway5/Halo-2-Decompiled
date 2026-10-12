#include "halo2/first_person_dual.hpp"
#include "halo2/source_model_pose.hpp"
#include <algorithm>
#include <functional>
namespace halo2 {
FirstPersonDualRoot first_person_dual_root(const AnimationClip &clip,std::uint32_t name) {
  if(clip.node_flags.size()!=clip.nodes.size() || clip.parents.size()!=clip.nodes.size())
    throw FormatError("Missing original first-person graph node flags");
  const auto found=std::find(clip.nodes.begin(),clip.nodes.end(),name);
  if(found==clip.nodes.end())return {};
  const auto node=int(found-clip.nodes.begin()),parent=clip.parents[node];
  if(parent<0)return {};
  if(std::size_t(parent)>=clip.nodes.size())throw FormatError("Invalid first-person graph parent");
  const auto flag=clip.node_flags[parent]&8u?0x10u:8u;
  const auto opposite=std::find_if(clip.node_flags.begin(),clip.node_flags.end(),
      [&](auto flags){return (flags&flag)==flag;});
  if(opposite==clip.node_flags.end())return {};
  return {node,int(opposite-clip.node_flags.begin())};
}
Transform source_first_person_dual_local(Transform t) {
  //1E0D46..1E0E1A. Subtractions, rather than sign XOR, preserve source zeros.
  t.scale=1;
  t.position.y=0.f-t.position.y;
  t.basis[0].y=0.f-t.basis[0].y;
  t.basis[2].y=0.f-t.basis[2].y;
  const auto f=t.basis[0],u=t.basis[2];
  t.basis[1]={f.z*u.y-u.z*f.y,f.x*u.z-f.z*u.x,u.x*f.y-f.x*u.y};
  return t;
}
std::vector<Transform> source_first_person_dual_nodes(std::span<const BonePose> locals,
    std::span<const int> parents,FirstPersonDualRoot mirror,const Transform &root) {
  if(locals.size()!=parents.size() || locals.size()>255 || mirror.node<0 || mirror.parent<0 ||
      std::size_t(mirror.node)>=locals.size() || std::size_t(mirror.parent)>=locals.size())
    throw FormatError("Invalid original dual first-person hierarchy");
  std::vector<Transform> result(locals.size());std::vector<unsigned> state(locals.size());
  std::function<void(unsigned)> resolve=[&](unsigned i) {
    if(i>=locals.size() || state[i]==1)throw FormatError("Cyclic original dual hierarchy");
    if(state[i]==2)return;
    state[i]=1;auto local=source_model_local_transform(locals[i]);
    const int parent=int(i)==mirror.node?mirror.parent:parents[i];
    if(int(i)==mirror.node)local=source_first_person_dual_local(local);
    if(parent< -1)throw FormatError("Invalid original dual parent");
    if(parent>=0){resolve(unsigned(parent));result[i]=compose_transform(result[parent],local);}
    else result[i]=compose_transform(root,local);
    state[i]=2;
  };
  for(unsigned i=0;i<locals.size();++i)resolve(i);
  return result;
}
} // namespace halo2
