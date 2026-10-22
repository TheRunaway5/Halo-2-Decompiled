#include "halo2/animation_presentation.hpp"
#include "halo2/simulation.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <tuple>

namespace halo2 {
namespace {
void validate_fraction(float fraction) {
  if (!std::isfinite(fraction) || fraction < 0 || fraction > 1)
    throw FormatError("Invalid animation presentation fraction");
}
bool same_hand(const FirstPersonHandBinding &a, const FirstPersonHandBinding &b) {
  return std::bit_cast<std::uint32_t>(a.rate)==std::bit_cast<std::uint32_t>(b.rate) &&
    std::bit_cast<std::uint32_t>(a.frame)==std::bit_cast<std::uint32_t>(b.frame) &&
    std::tie(a.flags,a.weapon,a.weapon_node_count,a.arms_node_count,
      a.weapon_nodes,a.arms_nodes,a.graph,a.animation_identifier,a.channel_flags,
      a.channel_options,a.activity,a.channel_delay,a.events,
      a.future_events,a.fire_ticks) ==
    std::tie(b.flags,b.weapon,b.weapon_node_count,b.arms_node_count,
      b.weapon_nodes,b.arms_nodes,b.graph,b.animation_identifier,b.channel_flags,
      b.channel_options,b.activity,b.channel_delay,b.events,
      b.future_events,b.fire_ticks);
}
}
FirstPersonRenderFrame first_person_render_frame(
    const FirstPersonViewBinding &previous, const FirstPersonViewBinding &current,
    const FirstPersonBindingDefinition &definition, float fraction, bool consecutive_tick) {
  validate_fraction(fraction);
  const auto &before=previous.hands[0], &after=current.hands[0];
  const FirstPersonRenderFrame snap{after.frame,false};
  if (!consecutive_tick || previous.unit==0xffffffffu || previous.unit!=current.unit ||
      previous.representation!=current.representation || before.weapon==0xffffffffu ||
      before.weapon!=after.weapon || before.graph!=after.graph ||
      before.animation_identifier!=after.animation_identifier || before.activity!=after.activity ||
      !std::isfinite(before.frame) || !std::isfinite(after.frame)) return snap;
  auto predicted=before;
  first_person_animation_clock(predicted,definition,tick_seconds);
  if (!same_hand(predicted,after)) return snap;
  // The existing source clock accepts only its recovered stock clips with
  // loop_frame0. Its literal end threshold is also the wrap span here.
  const auto clips=first_person_clips(after,definition);
  const auto clip=std::find_if(clips.begin(),clips.end(),
      [&](const auto &c){return c.activity==after.activity;});
  if (clip==clips.end()) return snap;
  float span=after.frame-before.frame;
  const float end=float(clip->frames)-std::bit_cast<float>(0x38d1b717u);
  // The source can reselect the same idle activity on its completion path.
  // A decreasing positive-rate frame without the retained wrap flag is a cut,
  // even if the completed successor happened to have the same clip identifier.
  if (span<0 && before.rate>=0 && !(after.channel_flags&4)) return snap;
  if (span<0 && (after.channel_flags&4) && before.rate>0) span+=end;
  float frame=before.frame+span*fraction;
  if (frame>=end && (after.channel_flags&4) && before.rate>0) frame-=end;
  // Endpoint copies avoid another rounding operation at exact tick captures.
  if (fraction==0) frame=before.frame;
  if (fraction==1) frame=after.frame;
  return {frame,true};
}
double animation_render_tick(std::uint32_t previous, std::uint32_t current, float fraction) {
  validate_fraction(fraction);
  // Do not blend snapshot gaps, clock rewind or the unsigned clock rollover.
  if (current<=previous || current-previous!=1) return double(current);
  return double(previous)+double(fraction);
}
} // namespace halo2
