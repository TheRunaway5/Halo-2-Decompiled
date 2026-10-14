#include "halo2/frontend_model_ik.hpp"
#include "halo2/content.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
FrontendModelWeaponIk Cache::load_frontend_model_weapon_ik(std::uint32_t unit,
                                                          std::uint32_t mode)const {
  return load_unit_weapon_ik(unit,load_frontend_model_weapon(unit).value_or(null_datum),mode);
}
FrontendModelWeaponIk Cache::load_unit_weapon_ik(std::uint32_t unit,
    std::uint32_t weapon,std::uint32_t mode)const {
  const Bytes data(cache_bytes());
  const auto root_for=[&](std::uint32_t id,const char*group,std::size_t size) {
    const auto tag=query_tag({.handle=TagHandle{id}, .group=TagGroup::from_string(group), .require_address=true});
    if(!tag)throw FormatError("frontend world IK source tag missing");
    return metadata_offset(tag->address,size);
  };
  const auto object=root_for(unit,"bipd",0xc0);
  FrontendModelWeaponIk out;
  out.world_callback=!(data.u32(object+0xbc)&0x800);
  if(weapon==null_datum || !out.world_callback)return out;
  const auto model=root_for(data.u32(object+0x38),"hlmt",0x18);
  const auto graph=data.u32(model+0x14);
  if(graph==null_datum)return out;
  const auto graph_root=root_for(graph,"jmad",0x5c);
  const auto identifiers=load_weapon_animation_identifiers(weapon);
  auto weapon_class=identifiers[0];
  const auto aliases=data.u32(graph_root+0x54);
  if(aliases>65536)throw FormatError("frontend IK class alias count exceeds source limit");
  if(aliases) {
    const auto first=metadata_offset(data.u32(graph_root+0x58),aliases*8);
    for(unsigned i=0;i<aliases;++i)
      if(data.u32(first+i*8)==identifiers[1]) {
        const auto replacement=data.u32(first+i*8+4);
        if(replacement!=null_datum && replacement!=0x030000d9u && weapon_class!=0x0400054bu)
          weapon_class=replacement;
        break;
      }
  }
  const auto weapon_object=root_for(weapon,"weap",0x3c);
  const auto weapon_model=root_for(data.u32(weapon_object+0x38),"hlmt",8);
  out.definition=load_first_person_ik(graph,data.u32(weapon_model+4),
      data.u32(model+4),mode,weapon_class);
  return out;
}
void apply_frontend_model_weapon_ik(const FrontendModelWeaponIk&state,
    const RenderModel&model,std::span<Transform> body,
    std::span<const Transform> weapon,float weight) {
  if(!std::isfinite(weight))throw FormatError("nonfinite original frontend weapon IK weight");
  if(!state.world_callback || !(weight>0.f) || std::abs(weight)<.0001f)return;
  if(body.size()!=model.bones.size())throw FormatError("frontend IK body node bank differs from model");
  std::vector<int> parents;parents.reserve(model.bones.size());
  for(const auto&bone:model.bones)parents.push_back(bone.parent);
  std::vector<bool> visited(body.size());
  for(const auto&constraint:state.definition.constraints) {
    if(constraint.arms_node>=body.size() || constraint.weapon_node>=weapon.size())
      throw FormatError("frontend IK marker node outside original bank");
    if(visited[constraint.arms_node])continue;
    FirstPersonIkDefinition selected;selected.constraints.push_back(constraint);
    apply_first_person_ik(selected,parents,body,weapon,weight);
    visited[constraint.arms_node]=true;
  }
}
}
