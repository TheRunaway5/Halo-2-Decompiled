#include "halo2/native_player_animation.hpp"
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
    const AnimationClip &clip,std::span<const BonePose> sampled,float weight) {
  if(locals.size()!=model.bones.size() || (clip.animation_type!=1&&clip.animation_type!=2) ||
      !std::isfinite(weight) || weight<0 || weight>1 || (clip.animation_type==2&&weight!=1))
    throw FormatError("Invalid native body overlay");
  for(std::size_t n=0;n<model.bones.size();++n) {
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
}
void apply_native_player_overlay(const RenderModel &model,std::span<BonePose> locals,
    const AnimationClip &clip,float frame,float weight) {
  if(!clip.frame_count)throw FormatError("Empty native body overlay");
  const auto sampled=frontend_model_locals(model,clip,
      std::clamp(frame,0.f,float(clip.frame_count-1)),{},nullptr,true);
  apply_sampled_overlay(model,locals,clip,sampled,weight);
}
NativePlayerAnimation::NativePlayerAnimation(const Cache &cache,std::uint32_t graph,std::uint32_t weapon)
    :cache_(cache),graph_(graph),identifiers_(cache.load_weapon_animation_identifiers(weapon)){}
const AnimationClip *NativePlayerAnimation::action(std::uint32_t activity,bool crouch,bool dual) {
  const std::array key{crouch?0x06000087u:0x06000086u,dual?0x0400054bu:identifiers_[0],activity};
  auto [it,inserted]=clips_.try_emplace(key);
  if(inserted) {
    // Authored omissions are legitimate (e.g. dual lower aim). Cache lookup
    // retains the source any-class/any-weapon fallback and inherited graph.
    try {it->second=cache_.load_unit_weapon_action(graph_,key[0],key[1],identifiers_[1],activity,true);}
    catch(const FormatError &e) {
      if(std::string_view(e.what()).starts_with("authored unit weapon action missing:"))return nullptr;
      clips_.erase(it);throw;
    }
  }
  return it->second?&*it->second:nullptr;
}
std::vector<BonePose> NativePlayerAnimation::pose(const RenderModel &model,
    const AnimationClip &primary,float frame,float pitch,bool crouch,bool dual,bool moving,
    const AnimationClip *secondary,float secondary_frame,const AnimationClip *offhand,float offhand_frame,float relative_yaw) {
  auto locals=frontend_model_locals(model,primary,
      std::clamp(frame,0.f,float(primary.frame_count-1)),{},nullptr,true);
  // 114BD0 publishes both standing/moving aim channels. The lower screen
  // must be composed too: several upper screens contain only pitch motion.
  // 1DE920 resolves each inherited screen;2800F0 maps its angle grid.
  const auto aim_screen=[&](std::uint32_t activity) {
   if(const auto *aim=action(activity,crouch,dual)) {
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
    const float low=std::floor(row),high=std::ceil(row),weight=row-low;
    const float stride=float(screen.width);
    auto sampled=frontend_model_locals(model,*aim,column+stride*low,{},nullptr,true);
    const auto high_pose=frontend_model_locals(model,*aim,column+stride*high,{},nullptr,true);
    for(std::size_t i=0;i<sampled.size();++i) {
      auto &a=sampled[i];const auto &b=high_pose[i];
      for(unsigned k=0;k<4;++k)a.rotation[k]=a.rotation[k]*(1-weight)+b.rotation[k]*weight;
      a.translation=a.translation*(1-weight)+b.translation*weight;
      a.scale=a.scale*(1-weight)+b.scale*weight;
    }
    apply_sampled_overlay(model,locals,*aim,sampled,1);
   }
  };
  aim_screen(moving?0x0b000027u:0x0c000026u);
  aim_screen(moving?0x0d000029u:0x0e000028u);
  //113BC0 composes the weapon grip after both aim screens (11447B..114568).
  if(const auto *grip=action(0x0400060bu,crouch,dual))
    apply_native_player_overlay(model,locals,*grip,0);
  if(secondary)apply_native_player_overlay(model,locals,*secondary,secondary_frame);
  if(offhand)apply_native_player_overlay(model,locals,*offhand,offhand_frame);
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
