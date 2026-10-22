#include "halo2/native_player_animation.hpp"
#include "gameplay/weapons/weapon_animation_compiled_symbols.hpp"
#include "halo2/frontend_model_locals.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/unit_animation.hpp"
#include "halo2/original_data.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2 {
namespace {
void apply_sampled_overlay(const RenderModel &model,std::span<BonePose> locals,
    const AnimationClip &clip,std::span<const BonePose> sampled,float weight,
    std::span<const std::uint32_t> admitted_nodes={},bool masked=false) {
  if(locals.size()!=model.bones.size() || (clip.animation_type!=1&&clip.animation_type!=2) ||
      !std::isfinite(weight) || weight<0 || weight>1 || (clip.animation_type==2&&weight!=1))
    throw FormatError("Invalid native body overlay");
  for(std::size_t n=0;n<model.bones.size();++n) {
    if(masked && std::find(admitted_nodes.begin(),admitted_nodes.end(),model.bones[n].name)==admitted_nodes.end())continue;
    const auto found=std::find(clip.nodes.begin(),clip.nodes.end(),model.bones[n].name);
    if(found==clip.nodes.end())continue;
    const auto components=clip.components.at(std::size_t(found-clip.nodes.begin()));
    auto &a=locals[n];const auto &b=sampled[n];
    if(components&1) {
      if(clip.animation_type==1) {
        auto q=b.rotation;
        if(weight!=1) {
          for(unsigned k=0;k<3;++k)q[k]*=weight;
          const float sign=std::copysign(1.f,q[3]);q[3]=(q[3]-sign)*weight+sign;
        }
        const auto r=a.rotation;
        a.rotation={((r[3]*q[0]+q[3]*r[0])+q[2]*r[1])-r[2]*q[1],
                    ((r[3]*q[1]+q[3]*r[1])+q[0]*r[2])-r[0]*q[2],
                    ((r[3]*q[2]+q[3]*r[2])+q[1]*r[0])-r[1]*q[0],
                    ((r[3]*q[3]-(q[0]*r[0]))-(q[1]*r[1]))-r[2]*q[2]};
      } else {
        // This caller installs full replacement channels; no invented
        // transition duration or normalized quaternion is introduced.
        if(weight!=1)throw FormatError("Unrecovered weighted body replacement");
        a.rotation=b.rotation;
      }
    }
    if(components&2)a.translation=clip.animation_type==1?
        a.translation+b.translation*weight:b.translation;
    if(components&4)a.scale=clip.animation_type==1?
        a.scale*(weight==1?b.scale:(b.scale-1.f)*weight+1.f):b.scale;
  }
}
// Literal114BD0 state switch. Full-body specials/ready/throws that do not
// select an aim activity must retain their authored torso and weapon pose.
std::optional<bool> aim_moving(std::uint32_t activity) {
  switch(activity) {
  case 0x0400000cu:case 0x0400004au:case 0x05000049u:
  case 0x0700005cu:case 0x0700005du:case 0x0800001eu:
  case 0x0900000eu:case 0x0900001fu:case 0x09000020u:case 0x090006b2u:
  case 0x0a00000fu:case 0x0b000059u:case 0x0b00005au:
  case 0x0c000058u:case 0x0c00005bu:return false;
  case 0x09000015u:case 0x09000016u:case 0x0a000014u:case 0x0a000017u:
  case 0x0a000019u:case 0x0a00001au:case 0x0a00002du:
  case 0x0b000018u:case 0x0b00001bu:case 0x0b00002eu:
  case 0x0c000033u:case 0x0c000034u:case 0x0d000032u:case 0x0d000035u:return true;
  default:return {};
  }
}
}
void apply_native_player_overlay(const RenderModel &model,std::span<BonePose> locals,
    const AnimationClip &clip,float frame,float weight) {
  if(!clip.frame_count)throw FormatError("Empty native body overlay");
  const auto sampled=frontend_model_locals(model,clip,
      std::clamp(frame,0.f,float(clip.frame_count-1)),{},nullptr,true);
  apply_sampled_overlay(model,locals,clip,sampled,weight);
}
NativePlayerAnimation::NativePlayerAnimation(const Cache &cache,std::uint32_t graph,std::uint32_t weapon)
    :cache_(cache),graph_(graph),identifiers_(cache.load_weapon_animation_identifiers(weapon)) {
  const auto tag=cache.load_source_tag_metadata(graph);
  const Bytes root(tag.bytes_at(tag.source_address(),0x9c));
  if(root.u32(0xc)>256)throw FormatError("Body skeleton exceeds original hand masks");
  for(unsigned hand=0;hand<2;++hand)for(unsigned n=0;n<root.u32(0xc);++n)
    if(root.u32((hand?0x5c:0x7c)+(n/32)*4)&(1u<<(n%32)))
      hand_nodes_[hand].push_back(Bytes(tag.bytes_at(root.u32(0x10)+n*32,4)).u32(0));
}
const AnimationClip *NativePlayerAnimation::action(std::uint32_t activity,bool crouch,bool dual) {
  const std::array key{crouch?0x06000087u:0x06000086u,dual?0x0400054bu:identifiers_[0],activity};
  auto [it,inserted]=clips_.try_emplace(key);
  if(inserted) {
    // Authored omissions are legitimate (e.g. dual lower aim). Cache lookup
    // retains the source any-class/any-weapon fallback and inherited graph.
    const detail::WeaponAnimationCompiledSymbols symbols(cache_);
    try {it->second=cache_.load_unit_weapon_action(graph_,key[0],dual?symbols.authored(0x0400054bu):identifiers_[0],
        identifiers_[1],symbols.authored(activity),true);}
    catch(const FormatError &e) {
      if(std::string_view(e.what()).starts_with("authored unit weapon action missing:"))return nullptr;
      clips_.erase(it);throw;
    }
  }
  return it->second?&*it->second:nullptr;
}
std::vector<BonePose> NativePlayerAnimation::pose(const RenderModel &model,
    const AnimationClip &primary,float frame,float pitch,bool crouch,bool dual,bool moving,
    const AnimationClip *secondary,float secondary_frame,const AnimationClip *offhand,float offhand_frame,float relative_yaw,float lowered_weight,
    NativePlayerAnimation *offhand_animation,std::uint32_t primary_activity,float aim_weight,bool support_hand_grip) {
  if(!std::isfinite(lowered_weight) || lowered_weight<0 || lowered_weight>1 ||
      !std::isfinite(aim_weight) || aim_weight<0 || aim_weight>1)
    throw FormatError("Invalid native weapon lowering weight");
  auto locals=frontend_model_locals(model,primary,
      std::clamp(frame,0.f,float(primary.frame_count-1)),{},nullptr,true);
  //113830 installs active action channels at113928..113A1A before its
  // dual offsets, aim screens and final grip. Putting actions last lets a
  // reload/fire replacement erase the weapon-specific fingers and aiming.
  const auto hand_overlay=[&](const AnimationClip *clip,float sample,unsigned hand) {
    if(!clip)return;
    const auto sampled=frontend_model_locals(model,*clip,std::clamp(sample,0.f,float(clip->frame_count-1)),{},nullptr,true);
    apply_sampled_overlay(model,locals,*clip,sampled,1,hand_nodes_[hand],true);
  };
  if(dual) {
    hand_overlay(secondary,secondary_frame,0);
    hand_overlay(offhand,offhand_frame,1);
  } else if(secondary)apply_native_player_overlay(model,locals,*secondary,secondary_frame);
  auto &left=offhand_animation?*offhand_animation:*this;
  //113A22..113AC6 installs each actual held weapon's dual_screen with
  //400/800. These one-frame offsets precede aim and have no guessed timing.
  if(dual) {
    hand_overlay(action(0x0b00054au,crouch,true),0,0);
    hand_overlay(left.action(0x0b00054au,crouch,true),0,1);
  }
  // 114BD0 selects aim_still/move_up and aim_still/move_down. These are
  // raised/lowered alternatives, not disjoint upper/lower skeleton masks.
  // 113830 at114028..114056 gives them complementary weights. Applying
  // both at one compounds torso yaw and lowers the held weapon twice.
  const auto aim=primary_activity==null_datum?std::optional(moving):aim_moving(primary_activity);
  const auto *raised=aim?action(*aim?0x0b000027u:0x0c000026u,crouch,dual):nullptr;
  const auto *lowered=aim?action(*aim?0x0d000029u:0x0e000028u,crouch,dual):nullptr;
  const auto aim_screen=[&](const AnimationClip *aim,float weight) {
   if(aim && weight>0) {
    auto [entry,inserted]=screens_.try_emplace(std::pair(aim->source_graph,aim->source_index));
    if(inserted) {
    const auto tag=cache_.load_source_tag_metadata(aim->source_graph);
    const Bytes root(tag.bytes_at(tag.source_address(),0x34));
    const Bytes definition(tag.bytes_at(root.u32(0x30)+aim->source_index*0x6c,0x14));
    const auto index=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(definition.slice(0x12,1)[0]));
    if(index<0 || unsigned(index)>=root.u32(0x24))throw FormatError("Body aim has no authored screen");
    const Bytes screen(tag.bytes_at(root.u32(0x28)+unsigned(index)*0x1c+4,0x18));
    const int yaw_negative=std::int16_t(screen.u16(8)),yaw_positive=std::int16_t(screen.u16(10));
    const int pitch_negative=std::int16_t(screen.u16(20)),pitch_positive=std::int16_t(screen.u16(22));
    entry->second={yaw_negative,yaw_negative+1+yaw_positive,pitch_negative,pitch_positive,
        screen.f32(12),screen.f32(16),screen.f32(0),screen.f32(4)};
    }
    const auto &screen=entry->second;
    const float step=pitch<0?screen.negative_step:screen.positive_step;
    const float yaw_step=relative_yaw<0?screen.yaw_negative_step:screen.yaw_positive_step;
    if(!std::isfinite(step) || !std::isfinite(yaw_step) || !std::isfinite(pitch) || !std::isfinite(relative_yaw))throw FormatError("Invalid body aim screen angle");
    const auto image=cache_.original_data();
    if(!image || !image->unit_animation_constants)throw FormatError("Missing original aim screen epsilon");
    const float epsilon=std::bit_cast<float>(image->unit_animation_constants->frame_subtractor_bits);
    const float row=float(screen.pitch_negative)+std::clamp(step>=epsilon?pitch/step:0.f,-float(screen.pitch_negative),float(screen.pitch_positive));
    const float column=float(screen.yaw_negative)+std::clamp(yaw_step>=epsilon?relative_yaw/yaw_step:0.f,-float(screen.yaw_negative),float(screen.width-1-screen.yaw_negative));
    // Bilinear screen sampling keeps yaw columns separate from pitch rows.
    const float low=std::floor(row),high=std::ceil(row),row_weight=row-low;
    const float stride=float(screen.width);
    auto sampled=frontend_model_locals(model,*aim,column+stride*low,{},nullptr,true);
    const auto high_pose=frontend_model_locals(model,*aim,column+stride*high,{},nullptr,true);
    for(std::size_t i=0;i<sampled.size();++i) {
      auto &a=sampled[i];const auto &b=high_pose[i];
      for(unsigned k=0;k<4;++k)a.rotation[k]=a.rotation[k]*(1-row_weight)+b.rotation[k]*row_weight;
      a.translation=a.translation*(1-row_weight)+b.translation*row_weight;
      a.scale=a.scale*(1-row_weight)+b.scale*row_weight;
    }
    apply_sampled_overlay(model,locals,*aim,sampled,weight);
   }
  };
  aim_screen(raised,(lowered?1-lowered_weight:1)*aim_weight);
  aim_screen(lowered,(raised?lowered_weight:1)*aim_weight);
  //113830 composes the weapon grip after weighted aim (11447B..114568).
  if(dual) {
    hand_overlay(action(0x0400060bu,crouch,true),0,0);
    hand_overlay(left.action(0x0400060bu,crouch,true),0,1);
  }else if(const auto *grip=action(0x0400060bu,crouch,false)) {
    //11443E..114493 retains only400 when the80 IK blend has reached zero.
    // Reload/throw fingers on the released support hand must remain free.
    if(support_hand_grip)apply_native_player_overlay(model,locals,*grip,0);
    else hand_overlay(grip,0,0);
  }
  return locals;
}
void blend_native_player_locals(std::span<BonePose> current,std::span<const BonePose> saved,float weight) {
  if(current.size()!=saved.size() || !std::isfinite(weight) || weight<0 || weight>1)
    throw FormatError("Invalid native local transition bank");
  for(unsigned n=0;n<current.size();++n) {
    auto &a=current[n];const auto &b=saved[n];
    const float d=((b.rotation[0]*a.rotation[0]+a.rotation[1]*b.rotation[1])+a.rotation[2]*b.rotation[2])+b.rotation[3]*a.rotation[3];
    const float signed_weight=d<0?-weight:weight;
    for(unsigned k=0;k<4;++k)a.rotation[k]=b.rotation[k]*(1.f-weight)+a.rotation[k]*signed_weight;
    const float norm=((a.rotation[0]*a.rotation[0]+a.rotation[1]*a.rotation[1])+a.rotation[2]*a.rotation[2])+a.rotation[3]*a.rotation[3];
    if(norm<=0)a.rotation={0,0,0,1};
    else {
      // Source spills binary32 norm, then x87 FSQRT/FDIVR/FMUL
      // at the calling control word precision before each binary32 store.
      const double inverse=1./std::sqrt(double(norm));
      for(auto &v:a.rotation)v=float(inverse*double(v));
    }
    a.translation=(a.translation-b.translation)*weight+b.translation;
    a.scale=(a.scale-b.scale)*weight+b.scale;
  }
}
void NativePlayerTransition::tick(std::uint32_t activity,bool crouch,std::span<const BonePose> locals,
    const UnitConstructorTailConstants &constants) {
  advance_source_unit_animation_transitions(manager_);
  if(!current_.empty() && (activity!=activity_ || crouch!=crouch_)) {
    saved_.resize(current_.size());
    SourceUnitLocalTransitionServices services{};
    services.one_bits=constants.one_bits;services.zero_bits=constants.zero_bits;
    services.clock=[] {
      SourceLocalTransitionClock c{{},0x27f,0x1f80};
      c.record[2]=std::byte(30);
      const auto dt=std::bit_cast<std::uint32_t>(1.f/30.f);
      for(unsigned i=0;i<4;++i)c.record[4+i]=std::byte(dt>>(i*8));
      return c;
    };
    const auto seconds=source_unit_transition_duration(activity,activity_,crouch!=crouch_,0,0,constants);
    static_assert(sizeof(BonePose)==32);
    save_source_unit_local_transition(manager_,int(current_.size()),seconds,
        std::as_writable_bytes(std::span(saved_)),std::as_bytes(std::span(current_)),services);
  }
  current_.assign(locals.begin(),locals.end());
  apply(current_);activity_=activity;crouch_=crouch;
}
void NativePlayerTransition::apply(std::span<BonePose> locals,float fraction)const {
  if(std::to_integer<unsigned>(manager_[0x65]) && !saved_.empty()) {
    const float current=float(std::to_integer<unsigned>(manager_[0x64]));
    const float duration=float(std::to_integer<unsigned>(manager_[0x65]));
    blend_native_player_locals(locals,saved_,std::clamp((current-1.f+fraction)/duration,0.f,1.f));
  }
}
} // namespace halo2
