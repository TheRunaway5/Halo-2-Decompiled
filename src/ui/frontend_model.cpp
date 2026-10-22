#include "halo2/frontend_model.hpp"
#include "halo2/frontend_emblem.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2 {
float frontend_model_initial_shield_fraction(float maximum_shield) {
  return maximum_shield>0.f?1.f:0.f;
}
float Cache::load_frontend_model_initial_shield_fraction(std::uint32_t unit) const {
  const Bytes data(cache_bytes());
  const auto root_for=[&](std::uint32_t datum,const char *group,std::size_t size) {
    const auto tag=query_tag({.handle=TagHandle{datum}, .group=TagGroup::from_string(group), .require_address=true});
    if(!tag)throw FormatError("frontend initial shield source tag missing");
    return metadata_offset(tag->address,size);
  };
  const auto object=root_for(unit,"bipd",0x3c);
  const auto model=data.u32(object+0x38);
  if(model==null_datum)return 0.f;
  const auto definition=root_for(model,"hlmt",0x68);
  const auto count=std::bit_cast<std::int32_t>(data.u32(definition+0x60));
  if(count<=0)return 0.f;
  const auto damage=metadata_offset(data.u32(definition+0x64),0x90);
  return frontend_model_initial_shield_fraction(std::bit_cast<float>(data.u32(damage+0x8c)));
}
std::optional<float> frontend_model_material_input(const FrontendAppearance *appearance,
    float shield_fraction,std::uint32_t name) {
  if(name==0x0f000559u)return std::clamp(shield_fraction,0.f,1.f);
  if(appearance)return frontend_model_emblem_input(*appearance,name);
  return {};
}
void animate_frontend_model_material(Material &material,double seconds,const ShaderInput &input) {
  animate_forward_material(material,seconds,input);
  animate_material_bitmaps(material,seconds,input);
  const auto child=[&](std::shared_ptr<Material> &pass) {
    if(!pass)return;
    if(pass.use_count()!=1)pass=std::make_shared<Material>(*pass);
    animate_frontend_model_material(*pass,seconds,input);
  };
  // These are the separately prepared object layers submitted by2BCD0/404B0.
  // BSP fog, projected shadows and bloom use their own execution contexts.
  child(material.environment_pass);
  for(auto &pass:material.world_overlays_before)child(pass);
  for(auto &pass:material.world_overlays_after)child(pass);
}
std::optional<std::uint32_t> Cache::load_frontend_model_weapon(std::uint32_t unit)const {
  const Bytes data(cache_bytes());
  const auto tag=query_tag({.handle=TagHandle{unit}, .group=tag_groups::biped, .require_address=true});
  if(!tag)throw FormatError("frontend starting weapon unit definition missing");
  const auto root=metadata_offset(tag->address,0x1c8);
  const auto count=data.u32(root+0x1c0);
  if(count>1)throw FormatError("frontend unit has unresolved initial weapon slot");
  if(!count)return {};
  const auto at=metadata_offset(data.u32(root+0x1c4),8);
  const auto weapon=data.u32(at+4);
  return weapon==null_datum?std::nullopt:std::optional(weapon);
}
std::optional<AnimationClip> Cache::load_frontend_model_animation(std::uint32_t object,bool request_ui)const {
  const Bytes data(cache_bytes());
  const auto root_for=[&](std::uint32_t id,const char*group,std::size_t size) {
    const auto tag=query_tag({.handle=TagHandle{id}, .group=TagGroup::from_string(group), .require_address=true});
    if(!tag)throw FormatError("frontend unit animation source tag missing");
    return metadata_offset(tag->address,size);
  };
  const auto object_root=root_for(object,"bipd",0x1c8);
  const auto model=data.u32(object_root+0x38);
  if(model==null_datum)return {};
  const auto model_root=root_for(model,"hlmt",0x7c);
  const auto graph=data.u32(model_root+0x14);
  if(graph==null_datum)return {};
  const auto root=root_for(graph,"jmad",0x5c);
  if(std::int16_t(data.u16(root+0xc))!=std::bit_cast<std::int32_t>(data.u32(model_root+0x78)))return {};

  const auto find=[&](TagBlockRange b,std::size_t stride,std::uint32_t name)->std::optional<std::size_t> {
    int lo=0,hi=int(b.count)-1;
    while(lo<=hi) {
      const auto k=(lo+hi)>>1;const auto at=b.offset+std::size_t(k)*stride;
      const auto candidate=data.u32(at);if(candidate==name)return at;
      if(std::bit_cast<std::int32_t>(name)<std::bit_cast<std::int32_t>(candidate))hi=k-1;
      else lo=k+1;
    }
    return {};
  };
  constexpr std::uint32_t any=0x030000d9u,idle=0x0400000cu,
      combat=0x06000086u,ui=0x020001b3u;
  std::uint32_t weapon_class=0x07000083u,weapon_type=any;
  if(const auto weapon=load_frontend_model_weapon(object)) {
    const auto identifiers=load_weapon_animation_identifiers(*weapon);
    weapon_class=identifiers[0];weapon_type=identifiers[1];
  }
  if(const auto alias=find(metadata_block(root+0x54, 8),8,weapon_type)) {
    const auto replacement=data.u32(*alias+4);
    if(replacement!=null_datum && replacement!=any && weapon_class!=0x0400054bu)
      weapon_class=replacement;
  }
  const auto select=[&](std::uint32_t mode)->std::optional<AnimationClip> {
    const auto modes=metadata_block(root+0x34, 20);
    for(unsigned step=0;step<8;++step) {
      if(((step&4)&&mode==any)||((step&2)&&weapon_class==any)||((step&1)&&weapon_type==any))continue;
      const auto m=find(modes,20,step&4?any:mode);if(!m)continue;
      const auto c=find(metadata_block(*m+4, 20),20,step&2?any:weapon_class);if(!c)continue;
      const auto t=find(metadata_block(*c+4, 0x34),0x34,step&1?any:weapon_type);if(!t)continue;
      const auto a=find(metadata_block(*t+4, 8),8,idle);if(!a)continue;
      const auto index=std::int16_t(data.u16(*a+6));if(index==-1)continue;
      if(index<0)throw FormatError("frontend idle source animation index invalid");
      auto selected_graph=graph;
      const auto inherited=std::int16_t(data.u16(*a+4));
      if(inherited!=-1) {
        const auto parents=metadata_block(root+0x4c, 32);
        if(inherited<0 || unsigned(inherited)>=parents.count)
          throw FormatError("frontend idle inherited graph outside source bank");
        selected_graph=data.u32(parents.offset+std::size_t(inherited)*32+4);
        (void)root_for(selected_graph,"jmad",0x34);
      }
      return load_animation(selected_graph,unsigned(index));
    }
    return {};
  };
  // Equipping the authored starting weapon updates the combat class/type.
  // Model widget2B0B96 requests ui/current/current/current with flags2;
  // failure retains the prior combat idle, as on this disc's Elite graph.
  if(request_ui)if(auto selected=select(ui))return selected;
  if(auto selected=select(combat))return selected;
  return load_initialized_object_idle(object);
}
}
