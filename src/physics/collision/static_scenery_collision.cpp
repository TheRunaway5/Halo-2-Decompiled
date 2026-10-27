#include "halo2/static_scenery_collision.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/scenery.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/math.hpp"
#include "halo2/render_interpolation.hpp"
#include <bit>
#include <cstring>
#include <set>

namespace halo2 {
std::shared_ptr<const StaticSceneryCollisionDefinition> load_static_scenery_collision_definition(
    const Cache&cache,const ScenarioObject&object){
  if(object.type!=6||object.definition==null_datum)return {};
  const auto definition=cache.load_scenery_definition(object.definition);
  if(!definition||definition->model==null_datum)return {};
  const auto hlmt=cache.load_source_tag_metadata(definition->model);
  const Bytes h(hlmt.bytes_at(hlmt.source_address(),hlmt.size()));
  if(h.u32(4)==null_datum||h.u32(0xc)==null_datum||h.u32(0x14)!=null_datum||
      h.u32(0x24)!=null_datum||h.u32(0x50)||h.u32(0x60))return {};
  const auto selected=cache.load_initial_model_selection(object.definition,object.variant);
  if(!selected.resolved())return {};
  const auto mode=cache.load_source_tag_metadata(h.u32(4));
  const Bytes m(mode.bytes_at(mode.source_address(),mode.size()));
  if(m.u32(0x48)!=1)return {};
  const Bytes node(mode.bytes_at(m.u32(0x4c),0x60));
  ModelBone bone;bone.name=node.u32(0);bone.parent=std::bit_cast<std::int16_t>(node.u16(4));
  if(bone.parent!=-1)throw FormatError("Fixed scenery root has an invalid source parent");
  bone.bind.translation={node.f32(12),node.f32(16),node.f32(20)};
  for(unsigned i=0;i<4;++i)bone.bind.rotation[i]=node.f32(24+i*4);
  bone.inverse_scale=node.f32(40);
  for(unsigned i=0;i<12;++i)bone.inverse[i]=node.f32(44+i*4);
  auto result=std::make_shared<StaticSceneryCollisionDefinition>();
  result->hierarchy.bones.push_back(bone);result->permutations=selected.permutations;
  result->collision=cache.load_model_collision(definition->model);
  for(const auto&region:result->collision.regions)for(const auto&permutation:region)for(const auto&bsp:permutation)
    if(bsp.node!=0)throw FormatError("Fixed scenery collision refers to an unavailable WORLD node");
  return result;
}
namespace {
template<class Emit>void visit_model_collision_triangles(const ModelCollision&d,
    std::span<const Transform>nodes,std::span<const std::uint8_t>permutations,std::size_t placement,Emit emit){
  std::array<int,16> active;active.fill(-1);
  if(permutations.size()<d.model_regions.size()||d.regions.size()>16)
    throw FormatError("Authored collision selection bank unavailable");
  for(unsigned n=0;n<d.model_regions.size();++n){
    const auto&r=d.model_regions[n];const auto p=permutations[n];
    if(r.collision==255||p>=128||p>=r.permutations.size())continue;
    const auto selected=r.permutations[p];if(selected!=255)active.at(r.collision)=selected;
  }
  for(unsigned r=0;r<d.regions.size();++r){
    if(active.at(r)<0)continue;
    const auto&bank=d.regions[r].at(unsigned(active[r]));
    for(unsigned bi=0;bi<bank.size();++bi){const auto&bsp=bank[bi];
      if(bsp.node<0||std::size_t(bsp.node)>=nodes.size())throw FormatError("Authored collision WORLD node unavailable");
      const auto&matrix=nodes[unsigned(bsp.node)];
      for(unsigned si=0;si<bsp.surfaces.size();++si){const auto&s=bsp.surfaces[si];if(s.flags&16)continue;
        const auto plane=std::uint16_t(s.plane);const auto p=bsp.planes.at(plane&0x7fff).normal;
        const auto&b=matrix.basis;
        Vec3 normal{(b[2].x*p.z+b[1].x*p.y)+b[0].x*p.x,
          (b[2].y*p.z+b[1].y*p.y)+b[0].y*p.x,(b[2].z*p.z+b[1].z*p.y)+b[0].z*p.x};
        normal=normal*(plane&0x8000?-1.f:1.f);
        Vec3 first{},previous{};std::size_t vertices{};auto edge=s.first_edge;
        const auto material=d.materials.at(std::uint16_t(s.material)).global_material;
        do{const auto&e=bsp.edges.at(edge);const unsigned side=e.surfaces[0]==si?0:1;
          if(e.surfaces[side]!=si||vertices>bsp.edges.size())throw FormatError("Invalid authored collision edge cycle");
          Transform local;local.position=bsp.vertices.at(e.vertices[side]);
          const auto current=compose_transform(matrix,local).position;
          if(vertices>=2 && length(cross(previous-first,current-first))>1e-8f)
            emit(CollisionTriangle{first,previous,current,std::uint16_t(material),s.flags,
              {-1,int((r<<16)|(unsigned(active[r])<<8)|bi),int(si),int(placement),plane},normal});
          if(!vertices)first=current;
          previous=current;++vertices;edge=e.next[side];
        }while(edge!=s.first_edge);
      }
    }
  }
}
}
std::vector<CollisionTriangle> model_collision_triangles(const ModelCollision&d,
    std::span<const Transform>nodes,std::span<const std::uint8_t>permutations,std::size_t placement){
  std::vector<CollisionTriangle> result;
  visit_model_collision_triangles(d,nodes,permutations,placement,[&](const auto &triangle){result.push_back(triangle);});
  return result;
}
std::size_t model_collision_triangle_count(const ModelCollision&d,
    std::span<const Transform>nodes,std::span<const std::uint8_t>permutations,std::size_t maximum){
  std::size_t count{};
  visit_model_collision_triangles(d,nodes,permutations,0,[&](const auto &){
    if(count==maximum)throw FormatError("Generated collision exceeds its admitted triangle budget");
    ++count;
  });
  return count;
}
StaticSceneryCollision::StaticSceneryCollision(const LevelContent&content):content_(&content){}
bool StaticSceneryCollision::synchronize(const SceneObjects&scene){
  NativeAuthorityBoundary::Call call(authority_);++authority_.mutations;
  bool changed{};std::set<std::uint32_t> present;
  for(const auto&o:scene.objects()){
    if(o.handle==null_datum||o.placement>=content_->static_scenery_collision.size()||
        !content_->static_scenery_collision[o.placement])continue;
    present.insert(o.handle);
    const auto root=scene_object_transform(o);
    auto [at,fresh]=owners_.try_emplace(o.handle);auto&owner=at->second;
    const bool same_nodes=bool(o.solved_nodes)==bool(owner.solved_nodes) &&
        (!o.solved_nodes || (o.solved_nodes->size()==owner.solved_nodes->size() &&
        (o.solved_nodes->empty() || !std::memcmp(o.solved_nodes->data(),owner.solved_nodes->data(),o.solved_nodes->size()*sizeof(Transform)))));
    if(fresh||std::memcmp(&root,&owner.root,sizeof root)||owner.active!=!o.bsp_hidden||!same_nodes){
      owner={o.placement,root,!o.bsp_hidden,o.solved_nodes};changed=true;
    }
  }
  for(auto at=owners_.begin();at!=owners_.end();)if(!present.contains(at->first)){
    at=owners_.erase(at);changed=true;
  }else ++at;
  if(changed){triangles_.clear();for(const auto&[_,owner]:owners_)if(owner.active){
    const auto&d=*content_->static_scenery_collision[owner.placement];
    const auto nodes=owner.solved_nodes?*owner.solved_nodes:pose_model_nodes_world(d.hierarchy,owner.root);
    if(nodes.size()!=d.hierarchy.bones.size())throw FormatError("Scenery collision lost its current source WORLD hierarchy");
    auto part=model_collision_triangles(d.collision,nodes,d.permutations,owner.placement);
    triangles_.insert(triangles_.end(),part.begin(),part.end());
  }}
  return changed;
}
}
