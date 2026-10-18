#include "halo2/fp_replication.hpp"
#include "halo2/simulation.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <tuple>

namespace halo2 {
FirstPersonChannelState first_person_channel_state(const FirstPersonHandBinding &hand) {
  return {hand.graph,hand.animation_identifier,hand.channel_delay,hand.channel_flags,
      hand.channel_options,hand.events,hand.future_events,hand.rate,hand.frame,
      hand.activity,hand.fire_ticks};
}
bool same_first_person_channel(const FirstPersonChannelState &a,const FirstPersonChannelState &b) {
  return std::bit_cast<std::uint32_t>(a.rate)==std::bit_cast<std::uint32_t>(b.rate) &&
      std::bit_cast<std::uint32_t>(a.frame)==std::bit_cast<std::uint32_t>(b.frame) &&
      std::tie(a.graph,a.animation_identifier,a.delay,a.flags,a.options,a.events,a.future_events,
          a.activity,a.fire_ticks)==
      std::tie(b.graph,b.animation_identifier,b.delay,b.flags,b.options,b.events,b.future_events,
          b.activity,b.fire_ticks);
}
namespace {
void restore(FirstPersonHandBinding &hand,const FirstPersonChannelState &state,
             const FirstPersonBindingDefinition &definition) {
  const auto clips=first_person_clips(hand,definition);
  const auto clip=std::find_if(clips.begin(),clips.end(),
      [&](const auto &c){return c.activity==state.activity;});
  if(state.graph!=definition.graph || clip==clips.end() || clip->frames<=0 ||
      state.animation_identifier!=first_person_clip_identifier(*clip) ||
      !std::isfinite(state.rate) || !std::isfinite(state.frame) || state.frame<0 ||
      state.frame>float(clip->frames)-std::bit_cast<float>(0x38d1b717u))
    throw FormatError("Replicated first-person clock disagrees with authored SMG channel");
  hand.graph=state.graph;hand.animation_identifier=state.animation_identifier;
  hand.channel_delay=state.delay;hand.channel_flags=state.flags;hand.channel_options=state.options;
  hand.events=state.events;hand.future_events=state.future_events;hand.rate=state.rate;
  hand.frame=state.frame;hand.activity=state.activity;hand.fire_ticks=state.fire_ticks;
}
}
bool apply_replicated_first_person_clock(FirstPersonViewBinding &origin,
    FirstPersonViewBinding &current,const FirstPersonReplicatedClock &clock,
    const FirstPersonBindingDefinition &definition) {
  if(!clock.present) {origin=current;return false;}
  if(current.unit==0xffffffffu || current.hands[0].weapon==0xffffffffu ||
      current.representation!=definition.representation || (current.hands[0].flags&6)!=6)
    throw FormatError("Replicated first-person clock has no resolved current binding");
  auto before=current,after=current;
  restore(before.hands[0],clock.origin,definition);
  restore(after.hands[0],clock.current,definition);
  auto completed=before.hands[0];
  first_person_animation_clock(completed,definition,tick_seconds);
  if(!same_first_person_channel(first_person_channel_state(completed),clock.current))
    throw FormatError("Replicated first-person clock is not one completed source step");
  origin=std::move(before);current=std::move(after);
  return true;
}
}
