#pragma once
#include "halo2/model_markers.hpp"
#include "halo2/model.hpp"
#include "halo2/native_action_prompt.hpp"
#include <map>
namespace halo2 {
struct NativeVehicleSeatDefinition {
  std::uint32_t flags{},mode{},marker{},entry_marker{},grenade_marker{},entry_message{},grenade_message{},melee_message{};
  std::int16_t boarding_seat=-1;
  float radius{},facing{},aim{},maximum_velocity{};
};
struct NativeUnitSeatAnimations {
  struct Entry {std::shared_ptr<const AnimationClip> clip;Transform initial;};
  std::vector<Entry> entries;
  std::shared_ptr<const AnimationClip> idle,board,exit;
};
struct NativeUnitInteractionDefinition {
  std::uint32_t unit_flags{},vehicle_flags{}; // Authored+BC/+1EC, door eligibility.
  std::vector<NativeVehicleSeatDefinition> seats;
  Vec3 bound_offset;float bound_radius{};
  std::uint32_t flip_message{};
  float flip_minimum{},flip_maximum{};
  std::map<std::uint32_t,NativeUnitSeatAnimations> seat_animations;
  RenderModel hierarchy;
  std::vector<std::uint8_t> permutations;
  std::map<std::uint32_t,std::vector<ObjectAttachmentMarker>> markers;
  std::shared_ptr<const AnimationClip> idle;
};
struct NativeUnitInteractionAssets {
  std::map<std::uint32_t,NativeUnitInteractionDefinition> units;
  float flipped_up_threshold{};
  std::map<unsigned,std::size_t> netgame_placements;
};
std::shared_ptr<const NativeUnitInteractionAssets> load_native_unit_interaction_assets(const Cache &,const LevelContent &);
bool native_can_trade_weapon(const Unit &,const Unit &,const LevelContent &,bool allied,std::uint64_t tick=0);
bool native_trade_weapons(Unit &,Unit &,const LevelContent &,std::uint32_t tick,bool allied);
NativeActionPrompt native_ally_action_prompt(const Unit &,std::span<const Unit>,const LevelContent &,
    NativeActionPrompt current,Vec3 current_root,const std::function<bool(int,int)> &allied,std::uint64_t tick=0);
}
