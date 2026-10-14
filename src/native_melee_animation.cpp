#include "halo2/native_melee_animation.hpp"
#include "world_continuation_bindings.hpp"
#include "halo2/fp_registration.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/prop_sandbox.hpp"
#include "halo2/source_runtime_resources.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2 {
int select_source_melee_candidate(std::array<bool,4> available,std::uint32_t &rng) {
  std::array<int,4> indices;unsigned count=0;
  for(unsigned i=0;i<4;++i)if(available[i])indices[count++]=int(i);
  if(!count)return -1;
  rng=rng*0x19660du+0x3c6ef35fu;
  return indices[((rng>>16)*count)>>16];
}
bool advance_source_melee_animation(NativeMeleeAction &s) {
  if(!s.active())return false;
  const bool impact=s.progress==s.impact;
  s.progress=std::bit_cast<std::int8_t>(std::uint8_t(std::uint8_t(s.progress)+1));
  if(s.progress>=s.duration)s.activity=0xffffffffu;
  return impact;
}
std::uint32_t source_melee_activity(std::uint32_t current,int kind,bool airborne,
    std::uint32_t ordinary,bool weapon_fallback) {
  switch(current) {
  case 0x0c000073:return kind==1?0x0c000075:0x11000074;
  case 0x0c000075:return kind==1?0x0c000077:0x11000076;
  case 0x0a0005bb:return 0x0b0005b2;
  case 0x130005bc:return 0x140005b3;
  case 0xffffffff:case 0x0e000607:case 0x0e000608:case 0x0e000609:case 0x0e00060a:break;
  default:return null_datum;
  }
  if(kind==2)return airborne?0x130005bc:0x0a0005bb;
  if(kind!=1)return null_datum;
  return ordinary!=null_datum?ordinary:weapon_fallback?0x0c000073:0x0500000a;
}
bool valid_native_melee_activity(std::uint32_t a) {
  switch(a) {
  case 0x0c000073:case 0x0c000075:case 0x0c000077:case 0x11000074:case 0x11000076:
  case 0x0a0005bb:case 0x0b0005b2:case 0x130005bc:case 0x140005b3:case 0x0500000a:
  case 0x0e000607:case 0x0e000608:case 0x0e000609:case 0x0e00060a:return true;
  default:return false;
  }
}
NativeMeleeHit query_native_melee(Vec3 origin,Vec3 aim,Vec3 pyramid,
    const std::function<std::optional<NativeMeleeContact>(Vec3,Vec3,std::uint32_t)> &cast) {
  // Original11E720 selects an orthogonal vector by the smallest absolute
  // component;30BD0 normalizes it before D0830 forms the second cross axis.
  Vec3 axis;
  if(std::abs(aim.x)<=std::abs(aim.y) && std::abs(aim.x)<=std::abs(aim.z))axis={0,aim.z,-aim.y};
  else if(std::abs(aim.y)<=std::abs(aim.z))axis={-aim.z,0,aim.x};
  else axis={aim.y,-aim.x,0};
  const float norm=length(axis);if(!(norm>0) || !cast)throw FormatError("Invalid native melee query basis");
  axis=axis/norm;
  const auto second=cross(aim,axis)*(pyramid.y*.5f);
  axis=axis*(pyramid.x*.5f);
  NativeMeleeHit result;result.point=origin;result.normal={0,0,1};
  for(int i=-2;i<3;++i)for(int j=-2;j<3;++j) {
    const auto delta=axis*float(i)+second*float(j)+aim*pyramid.z;
    const auto contact=cast(origin,delta,0x2480000fu);if(!contact)continue;
    result.point=contact->point;result.normal=contact->normal;
    if(contact->object==null_datum) {
      if(!result.target)result.material=contact->material;
    } else if(!result.target || (contact->type==0 &&
        (result.target->type!=0 || contact->fraction<result.target->fraction))) {
      result.target=contact;result.material=contact->material;
    }
  }
  return result;
}
NativeMeleeAnimation::NativeMeleeAnimation(std::shared_ptr<const Cache> cache,const LevelContent &content,NativeMeleeConstants c)
    :NativeMeleeAnimation(cache,content,c,cache&&!cache->is_vista()&&!cache->is_mcc()?
        std::make_shared<const SourceRuntimeResources>(*cache):nullptr) {}
NativeMeleeAnimation::NativeMeleeAnimation(std::shared_ptr<const Cache> cache,const LevelContent &content,
    NativeMeleeConstants c,std::shared_ptr<const SourceRuntimeResources> resources)
    :cache_(std::move(cache)),resources_(std::move(resources)),constants_(c),movement_forward_(content.movement.forward) {
  for(unsigned i=2;i<content.player_representations.size();++i)
    if(content.player_representations[i].biped.datum!=null_datum) {
      const auto datum=content.player_representations[i].biped.datum;
      representations_.emplace(datum,int(i));graphs_.emplace(datum,cache_->load_model_animation_graph(datum));
    }
}
NativeMeleeAnimation::AuthorityDigest NativeMeleeAnimation::authority_binding()const {
  if(!resources_)throw FormatError("Melee source continuation requires pinned Xbox resources");
  return world_continuation::melee_binding(resources_->identity(),constants_,movement_forward_,representations_,graphs_);
}
std::shared_ptr<NativeMeleeAnimation> NativeMeleeAnimation::prepare_authority_owner(const LevelContent &content,AuthorityDigest expected)const {
  auto next=std::shared_ptr<NativeMeleeAnimation>(new NativeMeleeAnimation(cache_,content,constants_,resources_));
  if(next->authority_binding()!=expected)throw FormatError("Melee continuation has different local immutable inputs");
  return next;
}
Vec3 NativeMeleeAnimation::pyramid(const Unit &u)const {
  if(u.weapon.definition==null_datum)return {};
  const auto tag=cache_->load_source_tag_metadata(u.weapon.definition);
  const Bytes d(tag.bytes_at(tag.source_address(),0x1bc));return {d.f32(0x1b0),d.f32(0x1b4),d.f32(0x1b8)};
}
std::shared_ptr<const PropAreaDamageDefinition> NativeMeleeAnimation::damage_definition(const Unit &u,std::uint32_t activity) {
  std::uint32_t damage=null_datum;
  if(u.weapon.definition!=null_datum) {
    const auto tag=cache_->load_source_tag_metadata(u.weapon.definition);
    const Bytes d(tag.bytes_at(tag.source_address(),0x1fc));
    unsigned slot=0;
    switch(activity) {
      case 0x0c000075:slot=0x1cc;break;
      case 0x0c000077:slot=0x1dc;break;
      case 0x0b0005b2:case 0x140005b3:slot=0x1ec;break;
      case 0x0500000a:case 0x0c000073:case 0x0e000607:case 0x0e000608:case 0x0e000609:case 0x0e00060a:slot=0x1bc;break;
    }
    if(slot)damage=d.u32(slot+4);
    if(damage==null_datum)damage=d.u32(0x190);
  }
  if(damage==null_datum) {
    const auto tag=cache_->load_source_tag_metadata(u.definition.datum);
    const Bytes d(tag.bytes_at(tag.source_address(),0x16c));damage=d.u32(0x168);
  }
  if(damage==null_datum)return {};
  auto [it,fresh]=damage_.try_emplace(damage);
  if(fresh) {
    // Direct melee damage shares jpt! vitality fields with area damage, but
    // has no radius/cone admission. Do not send it through an area loader.
    const auto tag=cache_->load_source_tag_metadata(damage);
    if(tag.group()!="jpt!")throw FormatError("Melee damage is not jpt!");
    const Bytes d(tag.bytes_at(tag.source_address(),0x58));
    auto definition=std::make_shared<PropAreaDamageDefinition>();auto &w=definition->damage;
    w.datum=w.damage=damage;w.damage_min=d.f32(0x1c);w.damage_max=d.f32(0x24);
    w.damage_amount=DamageAmountDefinition{w.damage_min,d.f32(0x20),w.damage_max};
    w.damage_flags=d.u32(0x14);w.damage_group=d.u32(0x50);w.damage_group_b=d.u32(0x54);
    w.damage_side_effect=std::int16_t(d.u16(0x10));w.damage_category=std::int16_t(d.u16(0x12));
    w.damage_camouflage_amount=d.f32(0x30);it->second=std::move(definition);
  }
  return it->second;
}
bool NativeMeleeAnimation::begin(Unit &u,int kind,std::uint32_t tick,std::uint32_t &rng,
    const std::function<void(std::uint32_t)> &publish) {
  const Cache::AnimationResourceReader resources=[this](auto pointer,auto bytes){
    return resources_?resources_->load(pointer,bytes):cache_->load_source_runtime_resource(pointer,bytes);};
  const auto representation=representations_.find(u.definition.datum);
  if(representation==representations_.end())return false;
  const bool dual=native_hand_weapon(u,1)!=nullptr;
  const FirstPersonBindingDefinition *binding=nullptr;
  if(u.weapon.definition!=null_datum) {
    auto [entry,inserted]=bindings_.try_emplace(std::tuple(u.weapon.definition,representation->second,dual));
    if(inserted)entry->second=cache_->load_first_person_binding(u.weapon.definition,representation->second,resources);
    binding=&entry->second;
  }
  const auto find=[&](std::uint32_t activity)->const FirstPersonClockClip * {
    if(binding)for(const auto &c:dual?binding->dual_clock_clips:binding->clock_clips)
      if(c.activity==activity)return &c;
    return nullptr;
  };
  std::uint32_t ordinary=null_datum;
  const auto previous=u.melee_action.activity;
  if(kind==1 && (previous==null_datum || (previous>=0x0e000607 && previous<=0x0e00060a))) {
    std::array<bool,4> available{};for(unsigned i=0;i<4;++i)available[i]=find(constants_.candidates[i][2]);
    const int selected=select_source_melee_candidate(available,rng);
    if(selected>=0)ordinary=constants_.candidates[unsigned(selected)][0];
  }
  // Native contact classification is explicit here; this does not construct
  // E5680's retained Havok/support/air-time record.
  const auto activity=source_melee_activity(previous,kind,!u.body.grounded,ordinary,find(0x0c000073));
  if(activity==null_datum)return false;
  const auto identifiers=u.weapon.definition==null_datum?std::array{0x030000d9u,0x030000d9u}:
      cache_->load_weapon_animation_identifiers(u.weapon.definition);
  std::optional<AnimationClip> body;
  for(const auto requested:{activity,0x0e000607u,0x0500000au}) {
    try {
      body=cache_->load_unit_weapon_action(graphs_.at(u.definition.datum),u.crouch>.5f?0x06000087:0x06000086,
          dual?0x0400054b:identifiers[0],identifiers[1],requested,true,resources);break;
    }catch(const FormatError &e) {
      if(!std::string_view(e.what()).starts_with("authored unit weapon action missing:"))throw;
    }
    if(activity!=0x0c000073 && activity!=0x0c000075 && activity!=0x0c000077 &&
        !(activity>=0x0e000607 && activity<=0x0e00060a))break;
  }
  const auto *fp=find(activity);
  if(!fp && !body)return false;
  const float frame_seconds=std::bit_cast<float>(constants_.frame_seconds_bits);
  const auto ticks=[&](int frames) {
    volatile float seconds=float(frames)*frame_seconds;
    volatile float result=seconds*30.f;
    return std::bit_cast<std::int8_t>(std::uint8_t(std::lrint(result)));
  };
  auto &s=u.melee_action;
  s={activity,tick,0,-1,-1,ticks(fp?fp->frames:body->frame_count),s.modifiers,0};
  if(activity==0x0a0005bb || activity==0x130005bc) {
    s.duration=std::int8_t(std::lrint(30.f*std::bit_cast<float>(constants_.special_seconds_bits)));
  }else if(fp) {
    for(const auto &event:fp->events)if(event.type==0) {
      if(event.id==0 && s.impact==-1)s.impact=ticks(event.frame);
      if(event.id==1 && s.recovery==-1)s.recovery=ticks(event.frame);
    }
  }else for(const auto &event:body->frame_events) {
    if(event.type==0 && s.impact==-1)s.impact=ticks(event.frame);
    if(event.type==1 && s.recovery==-1)s.recovery=ticks(event.frame);
  }
  if(movement_forward_>0 && dot(u.body.velocity,Vec3{std::cos(u.yaw),std::sin(u.yaw),0})/movement_forward_>
      std::bit_cast<float>(constants_.moving_threshold_bits))s.speed_modifier=127;
  u.source_action_word|=1u<<27;
  source_zoom_reset(u.source_zoom);++u.zoom_reset_serial;
  if(publish)publish(activity);
  return true;
}
void NativeMeleeAnimation::update(Unit &u,bool pressed,std::uint32_t tick,std::uint32_t &rng,
    const std::function<void(std::uint32_t)> &publish,const std::function<void(const NativeMeleeAction&)> &impact,int kind) {
  auto &s=u.melee_action;
  if(u.vitality.dead){s.activity=null_datum;u.source_action_word&=~(1u<<27);return;}
  bool admitted=pressed && !(u.source_action_word&((1u<<25)|(1u<<22)|(1u<<29))) &&
      (!s.active() || (s.recovery!=-1 && s.progress>=s.recovery));
  if(admitted && u.weapon.definition!=null_datum) {
    const auto weapon=cache_->load_source_tag_metadata(u.weapon.definition);
    const Bytes d(weapon.bytes_at(weapon.source_address(),0x130));
    const auto state=std::bit_cast<std::int8_t>(std::uint8_t(u.weapon.smg.weapon_state));
    admitted=!(d.u32(0x12c)&(1u<<9)) && state!=1 && state!=2;
  }
  if(admitted) {
    // Native interruption of the retained reload action. No magazine transfer
    // occurs here; the interrupted item keeps its actual loaded/reserve counts.
    for(unsigned hand=0;hand<2;++hand)if(auto *w=native_hand_weapon(u,hand)) {
      w->reload=0;w->burst_left=0;
    }
    u.source_action_word&=~1u;
    begin(u,kind,tick,rng,publish);
  }
  if(!s.active())return;
  if(s.progress==s.impact && impact)impact(s);
  const bool special=s.activity==0x0a0005bb || s.activity==0x130005bc;
  // Without an admitted target/physical-mode6 owner, EB700 releases the
  // windup into its authored ground/air strike through EB080(kind0).
  s.progress=std::bit_cast<std::int8_t>(std::uint8_t(std::uint8_t(s.progress)+1));
  if(s.progress>=s.duration || special) {
    if(!begin(u,0,tick,rng,publish)){s.activity=null_datum;u.source_action_word&=~(1u<<27);}
  }
}
} // namespace halo2
