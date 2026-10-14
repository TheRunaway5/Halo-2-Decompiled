#include "halo2/prop_sandbox.hpp"
#include "halo2/math.hpp"
#include "halo2/unit_damage.hpp"
#include "halo2/source_object_posed_target.hpp"
#include "halo2/pickup_visibility.hpp"
#include "halo2/render_interpolation.hpp"
#include <algorithm>
#include <cstring>
#include <set>
#include <utility>

namespace halo2 {
namespace {
Vec3 point(const Transform& t,Vec3 p){
  Transform local;local.position=p;return compose_transform(t,local).position;
}
Vec3 normal(const Transform& t,Vec3 p){
  return {(t.basis[2].x*p.z+t.basis[1].x*p.y)+t.basis[0].x*p.x,
          (t.basis[2].y*p.z+t.basis[1].y*p.y)+t.basis[0].y*p.x,
          (t.basis[2].z*p.z+t.basis[1].z*p.y)+t.basis[0].z*p.x};
}
std::array<int,16> selections(const PropSandboxDefinition& d,std::span<const std::uint8_t> permutations){
  std::array<int,16> result;result.fill(-1);
  for(unsigned n=0;n<d.collision.model_regions.size();++n){
    const auto& r=d.collision.model_regions[n];const auto p=permutations[n];
    if(r.collision==255 || p>=128 || p>=r.permutations.size())continue;
    const auto selected=r.permutations[p];if(selected==255)continue;
    result.at(r.collision)=selected;
  }
  return result;
}
}
PropSandbox::PropSandbox(const LevelContent& c):content_(&c){}
bool PropSandbox::synchronize(const SceneObjects& scene){
  bool changed=std::exchange(collision_dirty_,false);std::set<std::uint32_t> present;
  for(const auto& o:scene.objects()){
    if(o.handle==null_datum || o.placement>=content_->prop_sandbox.size())continue;
    const auto& d=content_->prop_sandbox[o.placement];if(!d)continue;
    present.insert(o.handle);
    const auto root=scene_object_transform(o);
    auto [it,fresh]=props_.try_emplace(o.handle);auto& p=it->second;
    if(fresh){p.state.handle=o.handle;p.state.placement=o.placement;
      p.state.definition=d->datum;p.state.sections.resize(d->sections.size());p.definition=d.get();
      p.state.regions=source_model_region_bank(d->initial_regions);
      p.state.vitality.body=d->vitality.health>0?1.f:0.f;
      p.state.vitality.shield=d->vitality.shield>0?1.f:0.f;}
    const bool nodes_changed=o.solved_nodes && (o.solved_nodes->size()!=p.nodes.size() ||
        (!p.nodes.empty() && std::memcmp(o.solved_nodes->data(),p.nodes.data(),p.nodes.size()*sizeof(Transform))));
    if(fresh || std::memcmp(&root,&p.root,sizeof(root)) || nodes_changed){
      p.root=root;p.nodes=o.solved_nodes?*o.solved_nodes:pose_model_nodes_world(d->model,root);
      if(p.nodes.size()!=d->model.bones.size())throw FormatError("Prop collision lost its actual source WORLD hierarchy");
      changed=true;
    }
    const bool active=!o.bsp_hidden;
    if(p.active!=active)changed=true;
    p.active=active;
  }
  for(auto i=props_.begin();i!=props_.end();){
    if(!present.contains(i->first)){i=props_.erase(i);changed=true;}else ++i;
  }
  return changed;
}
std::vector<CollisionTriangle> PropSandbox::triangles()const{
  std::vector<CollisionTriangle> result;
  for(const auto& [_,p]:props_){
    if(!p.active)continue;
    const auto& d=*p.definition;const auto active=selections(d,p.state.regions.selection.permutations);
    for(unsigned r=0;r<d.collision.regions.size();++r){
      if(active.at(r)<0)continue;
      const auto& bank=d.collision.regions[r].at(unsigned(active[r]));
      for(unsigned bi=0;bi<bank.size();++bi){const auto& bsp=bank[bi];
        const auto& matrix=p.nodes.at(unsigned(bsp.node));
        for(unsigned si=0;si<bsp.surfaces.size();++si){const auto& s=bsp.surfaces[si];
          if(s.flags&16)continue;
          auto n=bsp.planes.at(std::uint16_t(s.plane)&0x7fff).normal;
          n=normal(matrix,n)*(std::uint16_t(s.plane)&0x8000?-1.f:1.f);
          std::vector<Vec3> polygon;auto edge=s.first_edge;
          do{const auto& e=bsp.edges.at(edge);const unsigned side=e.surfaces[0]==si?0:1;
            if(e.surfaces[side]!=si || polygon.size()>bsp.edges.size())throw FormatError("Invalid prop collision edge cycle");
            polygon.push_back(point(matrix,bsp.vertices.at(e.vertices[side])));edge=e.next[side];
          }while(edge!=s.first_edge);
          const auto material=d.collision.materials.at(std::uint16_t(s.material)).global_material;
          for(unsigned v=1;v+1<polygon.size();++v)
            if(length(cross(polygon[v]-polygon[0],polygon[v+1]-polygon[0]))>1e-8f)
              result.push_back({polygon[0],polygon[v],polygon[v+1],std::uint16_t(material),s.flags,
                {-1,int((r<<16)|(unsigned(active[r])<<8)|bi),int(si),int(p.state.placement),std::uint16_t(s.plane)},n});
        }
      }
    }
  }
  return result;
}
std::optional<PropSandbox::Hit> PropSandbox::raycast(Vec3 origin,Vec3 delta,float maximum,
    std::uint32_t ignored,std::uint32_t ignored_other)const{
  std::optional<Hit> result;
  for(const auto& [handle,p]:props_){const auto& d=*p.definition;
    if(!p.active)continue;
    if(handle==ignored || handle==ignored_other)continue;
    // Original16A560(0x2480000F) produces lower flags9 for the ordinary
    // projectile object query. Complete1EFF40 performs the posed traversal.
    const auto hit=source_object_posed_target(d.collision,p.nodes,
        std::span<const std::uint8_t>(p.state.regions.selection.permutations),origin,delta,9);
    if(hit && hit->fraction<maximum){maximum=hit->fraction;result=Hit{handle,*hit};}
  }
  return result;
}
bool PropSandbox::damage(std::uint32_t handle,const WeaponDefinition& weapon,float amount,PropDamageOwner owner,bool radius,const SourceModelRegionOwners &owners){
  return damage_contact(handle,weapon,amount,owner,radius,owners).changed;
}
PropSandbox::DamageContactResult PropSandbox::damage_contact(std::uint32_t handle,
    const WeaponDefinition& weapon,float amount,PropDamageOwner owner,bool radius,const SourceModelRegionOwners &owners){
  auto& p=props_.at(handle);
  const auto before=p.state.vitality;
  UnitDamageResult result;
  if(!before.dead){
    p.state.owner=owner;
    result=apply_unit_damage(p.state.vitality,p.definition->vitality,weapon,
        content_->damage_groups,content_->damage_materials,amount,radius);
    if(p.state.vitality.dead)deplete(p,owners);
  }
  DamageContactResult contact{p.state.vitality.body!=before.body || p.state.vitality.shield!=before.shield,{}};
  if(!radius && p.definition->uniform_impact_material && p.definition->uniform_impact_region_absorption){
    const auto& v=p.definition->vitality;
    contact.presentation=source_projectile_damage_presentation({std::int16_t(v.body_material),std::int16_t(v.shield_material),
        amount,before.shield,p.state.vitality.body,p.state.vitality.shield,result.body,result.shield,0.f,
        p.definition->impact_region_absorbs});
  }
  return contact;
}
void PropSandbox::deplete(Prop& p,const SourceModelRegionOwners &owners){
  // D7E00 dispatches only damage sections with bit1. Complete admitted
  // DC460 branch for authored zero skip/threshold responses, no special death.
  for(unsigned s=0;s<p.definition->sections.size();++s){const auto& section=p.definition->sections[s];
    if(!(section.flags&2))continue;
    for(unsigned r=0;r<section.responses.size();++r){const auto& response=section.responses[r];auto& state=p.state.sections[s];
      if(state.destroyed&(1u<<r) || response.threshold!=0)continue;
      if(response.skip_fraction!=0 || response.flags&0x1800000)
        throw FormatError("Prop death response requires original RNG/special-death owner");
      if((response.total_threshold<1 && (response.flags&0x200000)) ||
          (response.total_threshold>1 && !(response.flags&0x200000)))continue;
      if(response.delay<=0)dispatch(p,s,r,owners);
      else{
        if((state.pending&0xfff8) && (state.pending&7)!=r)dispatch(p,s,r,owners);
        state.pending=std::uint16_t((unsigned(std::nearbyint(30.f*response.delay))<<3)|(r&7));
        if(response.delay_effect!=null_datum)throw FormatError("Prop delayed visual-effect owner unavailable");
      }
    }
    p.state.sections[s].damage=255;
  }
}
void PropSandbox::dispatch(Prop& p,unsigned section,unsigned index,const SourceModelRegionOwners &owners){
  const auto& r=p.definition->sections.at(section).responses.at(index);
  p.state.sections.at(section).destroyed|=std::uint16_t(1u<<index);
  if(r.region!=-1 && r.state!=-1){
    const auto before=p.state.regions.selection.permutations;
    source_model_regions_change(p.definition->region_definition,p.state.regions,{r.region,r.state},owners);
    collision_dirty_|=before!=p.state.regions.selection.permutations;
  }
  const auto resolve=[&](std::uint32_t id){
    std::optional<Transform> result;
    if(id==0 || id==null_datum)return std::optional<Transform>{p.root};
    const auto found=p.definition->markers.find(id);
    if(found!=p.definition->markers.end()){
      const auto posed=pose_model_markers(found->second,p.nodes,p.state.regions.selection.permutations,{},1);
      if(!posed.empty())result=posed.front().world;
    }
    return result;
  };
  const auto marker=resolve(r.marker),effect=resolve(r.effect_marker);
  events_.push_back({p.state.handle,effect?r.effect:null_datum,marker?r.damage:null_datum,
      marker?marker->position:p.root.position,marker?marker->basis[0]:p.root.basis[0],
      bool(r.flags&128),p.state.owner,effect?effect->position:p.root.position,
      effect?effect->basis[0]:p.root.basis[0]});
  if(r.flags&128)p.active=false;
}
Vec3 PropSandbox::closest_point(std::uint32_t handle,Vec3 origin)const{
  const auto& p=props_.at(handle);float distance=INFINITY;Vec3 result{};bool found=false;
  for(const auto& t:triangles())if(t.source.instance==int(p.state.placement)){
    const auto q=collision_triangle_closest_point(origin,t);const auto delta=q-origin;
    const float d=(delta.z*delta.z+delta.y*delta.y)+delta.x*delta.x;
    if(d<distance){distance=d;result=q;found=true;}
  }
  if(!found)throw FormatError("Current prop triangle target-point service unavailable");
  return result;
}
std::vector<PropSandboxEvent> PropSandbox::tick(const std::function<void(const PropSandboxEvent&)>& owner,const SourceModelRegionOwners &owners){
  std::vector<std::uint32_t> order;
  for(const auto& [h,_]:props_)order.push_back(h);
  std::sort(order.begin(),order.end(),[](auto a,auto b){return (a&0xffff)<(b&0xffff);});
  std::size_t delivered=0;
  const auto deliver=[&]{while(delivered<events_.size()){
    const auto event=events_[delivered++];if(owner)owner(event);
  }};
  deliver();
  for(auto h:order){auto& p=props_.at(h);if(!p.active)continue;
    for(unsigned s=0;s<p.state.sections.size();++s){auto& state=p.state.sections[s];
      const auto count=state.pending>>3;if(!count)continue;
      state.pending=std::uint16_t(((count-1)<<3)|(state.pending&7));
      if(count==1){dispatch(p,s,state.pending&7,owners);deliver();}
    }
  }
  auto result=std::move(events_);events_.clear();return result;
}
const PropSandboxState& PropSandbox::state(std::uint32_t handle)const{return props_.at(handle).state;}
const SourceInitialModelSelection *PropSandbox::region_selection(std::uint32_t handle)const{
  const auto at=props_.find(handle);return at==props_.end()?nullptr:&at->second.state.regions.selection;
}
std::vector<PropRegionSnapshot> PropSandbox::region_snapshot()const{
  std::vector<PropRegionSnapshot> result;
  for(const auto &[handle,p]:props_)result.push_back({handle,std::uint32_t(p.state.placement),p.state.definition,p.state.regions});
  return result;
}
void PropSandbox::apply_regions(std::span<const PropRegionSnapshot> rows){
  if(rows.size()!=props_.size())throw FormatError("native prop region snapshot is not a complete live bank");
  std::set<std::uint32_t> handles;
  for(const auto &row:rows){
    const auto at=props_.find(row.handle);
    if(at==props_.end() || !handles.insert(row.handle).second)
      throw FormatError("native prop region snapshot has an unknown/duplicate generation");
    const auto &p=at->second;const auto &selection=row.regions.selection;
    if(row.placement!=p.state.placement || row.definition!=p.state.definition ||
        !selection.resolved() || selection.variant_index!=p.definition->initial_regions.variant_index ||
        selection.states.size()!=p.definition->region_definition.regions.size() ||
        selection.permutations.size()!=selection.states.size() ||
        row.regions.looping_effects.size()!=selection.states.size())
      throw FormatError("native prop region snapshot differs from its authored owner");
    for(unsigned i=0;i<selection.states.size();++i){
      const int variant=selection.states[i].variant_permutation;
      const auto vi=selection.variant_index;
      const int mapped=vi<0?-1:p.definition->region_definition.variants.at(unsigned(vi)).regions[i];
      if(variant!=-1 && (mapped<0 || std::size_t(variant)>=p.definition->region_definition.variants.at(unsigned(vi)).definitions.at(unsigned(mapped)).permutations.size()))
        throw FormatError("native prop region variant permutation exceeds authored bank");
    }
    (void)source_initial_model_sections(p.definition->region_definition,selection,5);
  }
  for(const auto &row:rows){auto &p=props_.at(row.handle);
    collision_dirty_|=p.state.regions.selection.permutations!=row.regions.selection.permutations;
    p.state.regions=row.regions;
  }
}
const std::vector<PropSandboxState> PropSandbox::states()const{
  std::vector<PropSandboxState> result;for(const auto& [_,p]:props_)result.push_back(p.state);return result;
}
float prop_area_fraction(const PropAreaDamageDefinition& d,float distance){
  float fraction=1;
  if(d.radius_maximum-d.radius_minimum>0)
    fraction=std::clamp(1.f-(distance-d.radius_minimum)/(d.radius_maximum-d.radius_minimum),0.f,1.f);
  return fraction;
}
float prop_area_candidate_radius(const PropAreaDamageDefinition& d){
  return std::max(std::max(d.radius_maximum,d.source_58),d.source_68);
}
bool prop_area_occluded(Vec3 origin,Vec3 target,bool biped,float core,const PropAreaQuery& query){
  const auto delta=target-origin;
  if(!biped || core<=.0001f)return query(origin,delta,0x14a08c2d).hit;
  Vec3 side;
  // Original11E720 chooses the least-magnitude axis without a clip name or
  // velocity input;30BD0 owns both subsequent normalizations.
  if(std::abs(delta.x)<=std::abs(delta.y) && std::abs(delta.x)<=std::abs(delta.z))
    side={0,delta.z,0.f-delta.y};
  else if(std::abs(delta.y)<=std::abs(delta.z))side={0.f-delta.z,0,delta.x};
  else side={delta.y,0.f-delta.x,0};
  side=pickup_side_normalizer(side).output;
  const auto other=pickup_side_normalizer(cross(delta,side)).output;
  for(const auto offset:{side*core,side*(0.f-core),other*core,other*(0.f-core)}){
    const auto clipped=query(origin,offset,0x04a08c2d);
    if(!query(clipped.point,target-clipped.point,0x14a08c2d).hit)return false;
  }
  return true;
}
}
