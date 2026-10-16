#include "halo2/native_unit_interaction.hpp"
#include "halo2/content.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/math.hpp"
#include "halo2/first_person_ik.hpp"
#include <cmath>
#include <tuple>
namespace halo2 {
std::shared_ptr<const NativeUnitInteractionAssets> load_native_unit_interaction_assets(const Cache &cache,const LevelContent &content) {
  auto out=std::make_shared<NativeUnitInteractionAssets>();
  using ClipIdentity=std::tuple<std::uint32_t,std::uint32_t,std::string>;
  std::map<ClipIdentity,std::weak_ptr<const AnimationClip>> clips;
  const auto retain_clip=[&](AnimationClip clip)->std::shared_ptr<const AnimationClip> {
    if(clip.source_graph==null_datum || clip.source_index==null_datum)
      return std::make_shared<const AnimationClip>(std::move(clip));
    // Only immutable decoded resources share storage. Unit clocks and poses
    // remain independent, and the selected action name is part of identity.
    const ClipIdentity identity{clip.source_graph,clip.source_index,clip.name};
    auto &entry=clips[identity];
    if(auto retained=entry.lock())return retained;
    auto retained=std::make_shared<const AnimationClip>(std::move(clip));
    entry=retained;return retained;
  };
  const auto add=[&](std::uint32_t id) {
    if(id==null_datum || out->units.contains(id))return;
    const auto tag=cache.load_source_tag_metadata(id);const Bytes b(tag.bytes_at(tag.source_address(),tag.size()));
    if(b.u32(0x38)==null_datum)return;
    const auto model=cache.load_source_tag_metadata(b.u32(0x38));const Bytes m(model.bytes_at(model.source_address(),model.size()));
    if(m.u32(4)==null_datum)return;
    NativeUnitInteractionDefinition d;
    d.unit_flags=b.u32(0xbc);
    if(tag.group()=="vehi")d.vehicle_flags=b.u32(0x1ec);
    d.bound_offset={b.f32(8),b.f32(12),b.f32(16)};d.bound_radius=b.f32(4);
    if(tag.group()=="vehi") {d.flip_message=b.u32(0x21c);d.flip_minimum=b.f32(0x23c);d.flip_maximum=b.f32(0x240);}
    const auto count=b.u32(0x1c8);if(count>64)throw FormatError("Too many authored seats");
    if(count){const Bytes seats(tag.bytes_at(b.u32(0x1cc),count*0xb0));
      for(unsigned i=0;i<count;++i){const auto p=i*0xb0;NativeVehicleSeatDefinition s;
        s.flags=seats.u32(p);s.mode=seats.u32(p+4);s.marker=seats.u32(p+8);s.entry_marker=seats.u32(p+12);
        s.grenade_marker=seats.u32(p+0x10);s.grenade_message=seats.u32(p+0x14);s.melee_message=seats.u32(p+0x18);s.boarding_seat=std::int16_t(seats.u16(p+0x3e));
        s.entry_message=seats.u32(p+0x84);s.radius=seats.f32(p+0x98);s.facing=seats.f32(p+0x9c);s.aim=seats.f32(p+0xa0);s.maximum_velocity=seats.f32(p+0xa4);d.seats.push_back(s);
      }
    }
    const auto selected=cache.load_initial_model_selection(id);
    d.hierarchy=cache.load_model(m.u32(4),false);d.hierarchy.animation=m.u32(0x14);d.permutations=selected.permutations;
    const auto mode=cache.load_source_tag_metadata(m.u32(4));
    d.markers.emplace(0x04000095u,source_model_markers(mode,0x04000095u));
    for(const auto &seat:d.seats)for(const auto name:{seat.marker,seat.entry_marker,seat.grenade_marker})
      d.markers.try_emplace(name,source_model_markers(mode,name));
    try{d.idle=retain_clip(cache.load_action(d.hierarchy.animation,"combat","missile","any","idle"));pose_model(d.hierarchy,d.idle.get());}catch(const FormatError &){d.idle.reset();}
    d.hierarchy.mesh={};
    // Interaction queries retain the hierarchy, not expanded render vertices.
    // clear() leaves their potentially large allocations resident per unit.
    std::vector<SkinBinding>().swap(d.hierarchy.skin);
    std::vector<RenderModel::VertexOrigin>().swap(d.hierarchy.vertex_origins);
    out->units.emplace(id,std::move(d));
  };
  add(content.player.datum);for(const auto &r:content.player_representations)add(r.biped.datum);for(const auto &[id,c]:content.characters)add(c.biped.datum);
  for(const auto &o:content.objects)if(o.type==1)add(o.definition);
  for(std::size_t i=0;i<content.objects.size();++i)if(content.objects[i].netgame_row>=0)
    out->netgame_placements.emplace(unsigned(content.objects[i].netgame_row),i);
  for(const auto &object:content.objects)if(object.model_attachment) {
    const auto add_marker=[&](std::uint32_t id,std::uint32_t name){auto &d=out->units.at(id);
      const auto t=cache.load_source_tag_metadata(id);const Bytes b(t.bytes_at(t.source_address(),t.size()));
      const auto m=cache.load_source_tag_metadata(b.u32(0x38));const Bytes mb(m.bytes_at(m.source_address(),m.size()));
      d.markers.try_emplace(name,source_model_markers(cache.load_source_tag_metadata(mb.u32(4)),name));};
    add_marker(content.objects.at(object.model_attachment->parent).definition,object.model_attachment->parent_marker);
    add_marker(object.definition,object.model_attachment->child_marker);
  }
  std::map<std::uint32_t,bool> modes;
  for(const auto &[id,d]:out->units)for(const auto &seat:d.seats)modes.try_emplace(seat.mode,!(seat.flags&32));
  for(auto &[id,d]:out->units)for(const auto &[mode,blocks]:modes) {
    NativeUnitSeatAnimations a;
    const auto load=[&](std::uint32_t name){return retain_clip(cache.load_unit_weapon_action(d.hierarchy.animation,mode,blocks?0x07000001u:0x07000101u,0x07000001u,name));};
    for(const auto name:{0x05000049u,0x07000679u,0x0700067au,0x0700067bu,0x0700067cu}) {
      try {auto clip=load(name);Transform initial;
        // The retail first-frame cache is built from this decoded authored
        // root. Native cache files retain the resource rather than that mutable table.
        if(!clip->nodes.empty()) {const auto pose=sample_animation(*clip,0,0);initial=source_marker_transform(pose.translation,pose.rotation);initial.scale=pose.scale;}
        a.entries.push_back({std::move(clip),initial});
      }catch(const FormatError &){}
    }
    try{a.idle=load(0x0400000cu);}catch(const FormatError &){}
    try{a.board=load(0x050000c3u);}catch(const FormatError &){}
    try{a.exit=load(0x0400004au);}catch(const FormatError &){}
    if(!a.entries.empty())d.seat_animations.emplace(mode,std::move(a));
  }
  const auto globals=cache.query_tag({.group=tag_groups::globals});
  if(globals){const auto t=cache.load_source_tag_metadata(globals->datum);const Bytes g(t.bytes_at(t.source_address(),t.size()));
    if(g.u32(0xf0)){const Bytes control(t.bytes_at(g.u32(0xf4),0x80));out->flipped_up_threshold=std::cos(1.5707964f-control.f32(0x70));}}
  return out;
}
}
