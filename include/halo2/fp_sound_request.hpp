#pragma once
#include "halo2/fp_registration.hpp"

namespace halo2 {
// Represented TU1677E5 hand+1008/+100C fields. The identifier is returned by
// the required source scheduler provider; it is not an SDL voice identifier.
struct FirstPersonSoundState {
  std::uint32_t identifier=0xffffffffu;
  std::uint16_t activity=0xffffu;
  friend bool operator==(const FirstPersonSoundState &,const FirstPersonSoundState &)=default;
};
struct FirstPersonSoundConstants {
  std::array<std::byte,12> marker_position, marker_forward;
  std::uint32_t ordinary_callbacks, weapon_callbacks;
};
// The original owners leave marker+2/+3/+1C..+23 and location fields
// untouched until their consumers write them. Caller backing is explicit;
// zero padding is not an original constructor result.
struct FirstPersonSoundBacking {
  std::array<std::byte,40> marker;
  std::array<std::byte,48> location;
};
struct SoundWeaponAssociation {
  std::uint8_t flags_12c;
  std::uint32_t object_154;
};
// Semantic 18BA10 -> 21F220 request. Unowned stack bytes and pointers to
// temporary x86 stack records are omitted; the owned marker is copied here.
struct SourceSoundPlayRequest {
  std::uint32_t sound, flags, priority, owner, callbacks, platform_argument;
  std::array<std::byte,48> location;
  std::array<std::byte,40> marker;
  std::uint16_t data_size;
};
struct FirstPersonSoundServices {
  std::function<std::array<std::byte,20>(std::uint32_t)> sound_definition;
  std::function<std::array<std::byte,92>(std::int8_t)> sound_class;
  std::function<std::uint8_t(std::uint32_t)> object_header_type;
  std::function<SoundWeaponAssociation(std::uint32_t)> weapon_association;
  std::function<std::uint32_t(std::uint32_t)> object_parent;
  std::function<std::uint32_t(unsigned)> local_player;
  std::function<std::uint32_t(std::uint32_t)> player_current_unit;
  std::function<std::int32_t(unsigned)> camera_mode;
  // Complete 18DDE0 owner. Requires actual valid object, current node/world
  // transform, location/PVS and velocity producers at this event phase.
  // False is the real owner rejection; missing service is an unavailable seam.
  std::function<bool(std::uint32_t owner,std::uint32_t sound,
      std::uint32_t callbacks,std::span<const std::byte,40> marker,
      std::span<std::byte,48> location)> owner_request;
  // Complete 21F220 admission/selection/scheduler adapter. No default result,
  // seed, listener, permutation, voice or initialized flag is supplied here.
  std::function<std::uint32_t(const SourceSoundPlayRequest &)> submit;
  // Conditional 18BA10 successful-class attention/time owner. This callback
  // is mandatory only after actual success and class+8 has both bits1 and2.
  std::function<void(const SourceSoundPlayRequest &,
      std::span<const std::byte,20> definition)> successful_attention;
};
// Complete 18EF20/BBEA0 represented scalar/object-parent traversal. The
// optional resolved_view receives source NONE before any local scan.
std::uint8_t source_sound_audibility(std::uint32_t owner,std::uint8_t requested,
    const FirstPersonSoundServices &,std::uint32_t *resolved_view=nullptr);
bool source_sound_uses_weapon_callbacks(std::uint32_t sound,std::uint32_t owner,
    const FirstPersonSoundServices &);
// 18ACD0 with the actual FP marker-index NONE input; byte+1 is caller-owned.
void source_first_person_sound_marker(std::uint32_t sound,std::uint32_t owner,
    const FirstPersonSoundConstants &,std::array<std::byte,40> &,
    const FirstPersonSoundServices &);
// Bounded 1687C2 event, 18AA90/18AA40/18ACD0 marker, 18AD70 and 18BA10
// producer. Real lower-owner and scheduler services remain required. Returns
// false only for the original event/storage/camera gates. A completed rejected
// request still writes NONE and the low16 activity to the retained hand state.
bool source_first_person_sound_event(FirstPersonSoundState &,
    bool source_view_storage_present,unsigned view,const FirstPersonHandBinding &,
    const FirstPersonAnimationEvent &,std::uint16_t current_source_bsp,
    const FirstPersonSoundConstants &,
    FirstPersonSoundBacking &,const FirstPersonSoundServices &);
} // namespace halo2
