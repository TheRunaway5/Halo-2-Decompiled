#include "halo2/fp_registration.hpp"
#include <algorithm>
#include <bit>
#include <limits>

namespace halo2 {
namespace {
const FirstPersonClockClip &clip_for(const FirstPersonHandBinding &hand,const FirstPersonBindingDefinition &definition,
                                    std::uint32_t activity) {
  const auto clips=first_person_clips(hand,definition);
  const auto clip=std::find_if(clips.begin(),clips.end(),
      [&](const auto &c){return c.activity==activity;});
  if(clip==clips.end() || clip->frames<=0 || clip->loop_frame<0 || clip->loop_frame>=clip->frames)
    throw FormatError("First-person activity has no valid authored clip");
  return *clip;
}
void future_events(FirstPersonHandBinding &hand,const FirstPersonClockClip &clip) {
  hand.future_events=0;
  const float end=hand.rate>0 ? float(clip.frames)+1.f : 0.f;
  for(const auto &event:clip.events)
    if(event.type==0 && (hand.frame<=end ? hand.frame<event.frame && event.frame<=end
                                       : end<=event.frame && event.frame<hand.frame))
      hand.future_events|=std::uint16_t(1u<<(event.id&31));
}
void traverse(FirstPersonHandBinding &hand,const FirstPersonClockClip &clip,float from,float to,
              const FirstPersonEventCallback &callback) {
  const auto lower=std::min(from,to),upper=std::max(from,to);
  for(const auto &event:clip.events) {
    if(!(lower<=event.frame && event.frame<upper))continue;
    if(event.type==0) {
      if(std::bit_cast<std::int8_t>(std::uint8_t(hand.channel_options))<0)continue;
      hand.events|=std::uint16_t(1u<<(event.id&31));
    } else {
      if(hand.channel_options&(event.type==1?0x100:0x200))continue;
      if((event.reference_flags&2) && !(hand.channel_options&0x2000))continue;
      if((event.reference_flags&4) && !(hand.channel_options&0x1000))continue;
      if((event.reference_flags&0x10) && from>to)continue;
      if((event.reference_flags&0x20) && to>from)continue;
    }
    if(callback)callback(event);
  }
}
float next_frame(float frame,float rate,float seconds) {
  const float step=seconds*rate;
  const float frames=step*30.f;
  return frames+frame;
}
}
void first_person_activity(FirstPersonHandBinding &hand,const FirstPersonBindingDefinition &definition,
                          std::uint32_t activity) {
  if(hand.weapon==0xffffffffu || hand.graph==0xffffffffu)return;
  if(activity==0x06000006)hand.fire_ticks=4; // Original16A356 before168483 suppression.
  if((activity==0x06000006 && activity==hand.activity && definition.continuous_fire_animation) ||
      (activity==0x05000024 && activity==hand.activity))return;
  // Retail166992 leaves the current channel intact when animation_set fails.
  // Elite body graphs have throw_overheated, but neither authored SMG FP
  // graph has that action. Do not substitute the ordinary throw or crash.
  const auto clips=first_person_clips(hand,definition);
  if(std::none_of(clips.begin(),clips.end(),
      [&](const auto &clip){return clip.activity==activity;}))return;
  const auto &clip=clip_for(hand,definition,activity);
  hand.animation_identifier=(clip.index<<16)|0xffffu;hand.activity=activity;
  hand.channel_delay=0;hand.channel_flags=0;
  hand.channel_options=hand.dual_wield?(hand.secondary_hand?0x203f:0x103f):0x3f;
  hand.rate=1;hand.frame=0;hand.events=0;future_events(hand,clip);
}
void first_person_animation_clock(FirstPersonHandBinding &hand,const FirstPersonBindingDefinition &definition,
                                 float seconds,const FirstPersonEventCallback &callback) {
  if(hand.fire_ticks>0)--hand.fire_ticks; // Original168866 before channel update.
  if(hand.weapon==0xffffffffu || hand.graph==0xffffffffu || hand.animation_identifier==0xffffffffu)return;
  const auto &clip=clip_for(hand,definition,hand.activity);
  if((hand.animation_identifier>>16)!=clip.index || hand.graph!=definition.graph)
    throw FormatError("First-person primary clock graph/clip mismatch");
  hand.events=hand.future_events=0; // Original1CA2B0 also clears these during delay.
  if(hand.channel_delay) {--hand.channel_delay;return;}
  if((hand.channel_options&1) && !(hand.channel_flags&9)) {
    float frame=next_frame(hand.frame,hand.rate,seconds);
    hand.channel_flags&=0xf1;
    if(frame<0)frame=0;
    const float threshold=float(clip.frames)-std::bit_cast<float>(0x38d1b717u);
    traverse(hand,clip,hand.frame,frame,callback);
    if(frame>=threshold) {
      traverse(hand,clip,hand.frame,std::numeric_limits<float>::max(),callback);
      if(!(hand.channel_options&2)) {hand.channel_flags|=8;frame=threshold;}
      else {
        // Original1CEDA0 retains the selected clip in all executed stock cases.
        if((hand.channel_options&0x20) && (hand.channel_options&8))hand.rate=1;
        const float wrapped=frame-threshold;
        frame=wrapped+float(clip.loop_frame);
        traverse(hand,clip,float(clip.loop_frame),frame,callback);hand.channel_flags|=4;
      }
    }
    hand.frame=std::clamp(frame,0.f,threshold);
    if((hand.channel_options&1) && !(hand.channel_flags&9)) {
      if(next_frame(hand.frame,hand.rate,seconds)>=threshold)hand.channel_flags|=2;
      else hand.channel_flags&=0xfd;
    }
    future_events(hand,clip);
  }
  if((hand.channel_flags&0x0a) &&
      (hand.activity!=0x06000006 || !definition.continuous_fire_animation || hand.fire_ticks<1))
    first_person_activity(hand,definition,0x0400000c);
}
}
