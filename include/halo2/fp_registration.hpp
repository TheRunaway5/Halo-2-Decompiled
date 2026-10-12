#pragma once
#include "halo2/cache.hpp"
#include "halo2/model.hpp"
#include <functional>

namespace halo2 {
struct FirstPersonAnimationEvent {
  std::uint16_t type{};
  std::int16_t frame{};
  std::uint32_t tag=0xffffffffu, id=0xffffffffu, reference_flags{}, marker{};
};
struct FirstPersonClockClip {
  std::uint32_t activity{}, index{};
  std::int16_t frames{}, loop_frame{};
  std::vector<FirstPersonAnimationEvent> events;
};
// The represented fields of TU1677E5/1678C1/169D8A/169965/167D32.
// Native secondary hands reuse the same node map and clock with authored dual clips.
struct FirstPersonBindingDefinition {
  std::int32_t representation{};
  std::uint32_t weapon_definition=0xffffffffu, arms_model=0xffffffffu,
      weapon_model=0xffffffffu, graph=0xffffffffu;
  std::vector<std::uint32_t> arms_nodes, weapon_nodes, graph_nodes;
  std::vector<std::uint8_t> graph_node_flags;
  std::uint32_t idle_index=0xffffffffu;
  bool record_present{};
  bool continuous_fire_animation{}; // Authored weap+12C bit17.
  std::vector<FirstPersonClockClip> clock_clips,dual_clock_clips;
  std::array<std::optional<AnimationClip>,2> ammunition; // Single/dual original169965 overlay lookup.
};
// Original169728..169846 frame selector. available is104CF0 (loaded plus
// eligible shared reserve), elapsed is signed16 weapon+228 minus+226.
float first_person_ammunition_frame(std::int16_t loaded,std::int16_t available,
    std::int16_t capacity,std::int16_t elapsed,std::int16_t weapon_type,
    std::uint32_t activity,unsigned frames);
struct FirstPersonHandBinding {
  std::uint32_t flags{}, weapon=0xffffffffu;
  std::uint32_t weapon_node_count{}, arms_node_count{};
  std::array<std::int32_t,64> weapon_nodes{}, arms_nodes{};
  std::uint32_t graph=0xffffffffu, animation_identifier=0xffffffffu;
  std::uint8_t channel_flags{};
  std::uint16_t channel_options{};
  float rate=1, frame{};
  std::uint32_t activity=0xffffffffu;
  std::uint8_t channel_delay{};
  std::uint16_t events{}, future_events{};
  bool dual_wield{};
  bool secondary_hand{};
  std::int16_t fire_ticks{}; // Hand+F2, distinct from unit hand feedback timers.
};
struct FirstPersonViewBinding {
  std::uint32_t unit=0xffffffffu;
  std::int32_t representation=-1;
  std::array<FirstPersonHandBinding,2> hands;
};
struct FirstPersonBindingInput {
  std::uint32_t player=0xffffffffu, unit=0xffffffffu;
  std::int32_t representation=-1;
  // Resolved CD1A0 current indices (+212), never requested indices (+216).
  std::array<std::uint32_t,2> weapons{0xffffffffu,0xffffffffu};
};
using FirstPersonDefinitionLookup=std::function<const FirstPersonBindingDefinition *(
    std::uint32_t weapon, std::int32_t representation)>;
// Each current hand resolves its own weapon, node map and authored action channel.
void observe_first_person_binding(FirstPersonViewBinding &,const FirstPersonBindingInput &,
    const FirstPersonDefinitionLookup &);
void release_first_person_binding(FirstPersonViewBinding &);
inline std::span<const FirstPersonClockClip> first_person_clips(
    const FirstPersonHandBinding &hand,const FirstPersonBindingDefinition &definition) {
  return hand.dual_wield && !definition.dual_clock_clips.empty()?
      definition.dual_clock_clips:definition.clock_clips;
}
std::int32_t first_person_weapon_view(std::span<const FirstPersonViewBinding>,std::uint32_t weapon);
// Source SMG primary-channel kernel, also used by the native per-hand adapter.
// Full secondary blend channels, camera,
// feedback-response decay and resource/audio playback are separate consumers.
void first_person_activity(FirstPersonHandBinding &,const FirstPersonBindingDefinition &,
                          std::uint32_t activity);
using FirstPersonEventCallback=std::function<void(const FirstPersonAnimationEvent &)>;
void first_person_animation_clock(FirstPersonHandBinding &,const FirstPersonBindingDefinition &,
                                 float seconds,const FirstPersonEventCallback & = {});
}
