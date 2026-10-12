#include "halo2/prop_sandbox.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/source_prop_scene.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <bit>
#include <cmath>

namespace halo2 {
namespace {
struct Reader {
  SourceTagMetadata tag;
  const Cache* cache{};
  std::span<const std::byte> read(std::uint32_t p,std::size_t extent)const{
    if(p>=tag.source_address() && std::uint64_t(p)-tag.source_address()<=tag.size() &&
        extent<=tag.size()-(std::uint64_t(p)-tag.source_address()))return tag.bytes_at(p,extent);
    const Tag* owner=nullptr;
    for(const auto& t:cache->select_tags({.contains=TagAddressRange{p,extent}, .require_address=true}))if(t.group!="sbsp" && t.group!="ltmp"){
      if(owner && extent)throw FormatError("Ambiguous forwarded prop metadata");owner=&t;
    }
    if(!owner)throw FormatError("Prop metadata pointer has no owned resident backing: "+std::to_string(tag.datum())+"/"+std::to_string(p));
    (void)cache->tag_payload(owner->handle());
    return cache->read_tag_bytes(p,extent);
  }
  std::uint32_t u(std::uint32_t p)const{return Bytes(read(p,4)).u32(0);}
  std::int16_t s(std::uint32_t p)const{return std::bit_cast<std::int16_t>(Bytes(read(p,2)).u16(0));}
  float f(std::uint32_t p)const{const auto v=std::bit_cast<float>(u(p));if(!std::isfinite(v))throw FormatError("Nonfinite prop metadata");return v;}
  Vec3 point(std::uint32_t p)const{return {f(p),f(p+4),f(p+8)};}
  std::vector<std::uint32_t> block(std::uint32_t p,unsigned stride)const{
    const auto n=u(p);if(n>65536)throw FormatError("Excessive prop metadata block");
    std::vector<std::uint32_t> result;if(!n)return result;
    const auto base=u(p+4);read(base,std::size_t(n)*stride);
    for(unsigned i=0;i<n;++i)result.push_back(base+i*stride);return result;
  }
};
}
std::shared_ptr<const PropSandboxDefinition> load_prop_sandbox_definition(
    const Cache& cache,const ScenarioObject& placement){
  if(placement.type!=11)return {};
  const auto definition=cache.load_source_prop_definition(placement.definition);
  if(!definition.constructor_admitted || definition.render_model==null_datum ||
      cache.load_initialized_object_idle(placement.definition))return {};
  Reader hlmt{cache.load_source_tag_metadata(definition.model),&cache};
  const auto h=hlmt.tag.source_address();if(hlmt.u(h+0xc)==null_datum)return {};
  const auto selected=cache.load_initial_model_selection(placement.definition,placement.variant);
  if(!selected.resolved())return {};
  auto out=std::make_shared<PropSandboxDefinition>();out->datum=placement.definition;
  out->permutations=selected.permutations;
  out->collision=cache.load_model_collision(definition.model);
  Reader mode{cache.load_source_tag_metadata(definition.render_model),&cache};
  out->model.name=definition.name;
  for(const auto p:mode.block(mode.tag.source_address()+0x48,0x60)){
    ModelBone bone;bone.name=mode.u(p);bone.parent=mode.s(p+4);
    bone.bind.translation=mode.point(p+12);
    for(unsigned i=0;i<4;++i)bone.bind.rotation[i]=mode.f(p+24+i*4);
    bone.inverse_scale=mode.f(p+40);
    for(unsigned i=0;i<12;++i)bone.inverse[i]=mode.f(p+44+i*4);
    out->model.bones.push_back(bone);
  }
  const auto infos=hlmt.block(h+0x60,0xf8);
  if(!infos.empty()){
    const auto p=infos.front();auto& v=out->vitality;
    v.health=hlmt.f(p+0x28);v.shield=hlmt.f(p+0x8c);
    v.damage_flags=hlmt.u(p);v.default_section=hlmt.s(p+8);
    v.minimum_body_stun_damage=hlmt.f(p+0x2c);v.body_delay=hlmt.f(p+0x30);
    v.body_max=hlmt.f(p+0x38);v.body_material=hlmt.s(p+0xce);
    v.minimum_shield_stun_damage=hlmt.f(p+0x94);v.shield_delay=hlmt.f(p+0x98);
    v.shield_damaged_threshold=hlmt.f(p+0xa0);v.shield_material=hlmt.s(p+0xcc);
    v.shield_rate=hlmt.f(p+0xd0);v.body_rate=hlmt.f(p+0xd4);
    v.shield_damaged_first_person_shader=hlmt.u(p+0x80);
    v.shield_damaged_shader=hlmt.u(p+0x88);
    v.shield_damaged_effect=hlmt.u(p+0xa8);
    v.shield_depleted_effect=hlmt.u(p+0xb0);
    v.shield_recharging_effect=hlmt.u(p+0xb8);
    v.damage_info_count=static_cast<std::int32_t>(infos.size());
    v.overshield_first_person_shader=hlmt.u(p+0xec);
    v.overshield_shader=hlmt.u(p+0xf4);
    for(const auto s:hlmt.block(p+0xbc,0x38)){
      v.sections.push_back({hlmt.u(s),hlmt.u(s+4),hlmt.f(s+8)});
      PropDamageSection section{hlmt.u(s+4),hlmt.f(s+8),{}};
      for(const auto r:hlmt.block(s+12,0x50)){
        section.responses.push_back({hlmt.u(r+4),hlmt.u(r+0x10),hlmt.u(r+0x18),
            hlmt.u(r+0x28),hlmt.u(r+0x34),hlmt.s(r+0x20),hlmt.s(r+0x22),
            hlmt.f(r+8),hlmt.f(r+0x2c),hlmt.f(r+0x44),hlmt.f(r+0x4c)});
        section.responses.back().effect_marker=hlmt.u(r+0x24);
        for(const auto marker:{hlmt.u(r+0x28),hlmt.u(r+0x24)})
          if(marker && marker!=null_datum && !out->markers.contains(marker))
            out->markers.emplace(marker,source_model_markers(mode.tag,marker));
      }
      out->sections.push_back(std::move(section));
    }
  }
  return out;
}
std::shared_ptr<const PropAreaDamageDefinition> load_prop_area_damage_definition(
    const Cache& cache,std::uint32_t datum){
  Reader r{cache.load_source_tag_metadata(datum),&cache};
  if(r.tag.group()!="jpt!")throw FormatError("Prop area damage is not jpt!");
  const auto p=r.tag.source_address();
  // Directional cone needs its owning branch. Independent acceleration and
  // camouflage radii do not suppress the separately available health leaf.
  if(r.f(p+0x2c)!=0)
    throw FormatError("Area definition requires an unported directional/radius branch");
  auto out=std::make_shared<PropAreaDamageDefinition>();auto& w=out->damage;
  w.datum=w.damage=datum;w.name="authored area damage";
  w.damage_min=r.f(p+0x1c);w.damage_max=r.f(p+0x24);
  w.damage_amount=DamageAmountDefinition{w.damage_min,r.f(p+0x20),w.damage_max};
  w.damage_flags=r.u(p+0x14);w.damage_group=r.u(p+0x50);w.damage_group_b=r.u(p+0x54);
  w.damage_side_effect=r.s(p+0x10);w.damage_category=r.s(p+0x12);
  w.damage_camouflage_amount=r.f(p+0x30);
  out->radius_minimum=r.f(p);out->radius_maximum=r.f(p+4);
  if(!(out->radius_maximum>out->radius_minimum))
    throw FormatError("Area definition requires original equal-radius candidate admission");
  out->core_radius=r.f(p+0x18);out->acceleration=r.f(p+0x40);out->flags=r.u(p+0xc);
  out->source_58=r.f(p+0x58);out->source_64=r.f(p+0x64);out->source_68=r.f(p+0x68);
  return out;
}
}
