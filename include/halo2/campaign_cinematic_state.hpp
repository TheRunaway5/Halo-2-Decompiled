#pragma once
#include "halo2/bloom.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/cinematic_crossfade.hpp"
#include <map>
#include <set>

namespace halo2 {
// The original cinematic interface allocates a zeroed lighting bank. The
// bloom override has its separate 22590 defaults. Full salted object handles
// keep a reused scene slot from inheriting a retired object's lighting flag.
struct CampaignCinematicState {
  ObjectLightingState lighting;
  std::set<std::uint32_t> lit_objects;
  CinematicBloomOverride bloom;
  // TU1.5 2B4F19 activates the zeroed screen-output bank. Screen start
  // itself applies no filter; later crossfade/DOF commands own their effects.
  bool screen_active{},screen_initialized{};
  // Native frame/presentation ownership. The original skip/save helper may
  // yield before admitting its first camera; that interval must stay covered.
  bool awaiting_camera{};
  float frame_fraction{};
  struct Attachment {
    std::uint32_t parent=0xffffffffu;
    unsigned node{};
    ObjectAttachmentFrame local;
  };
  // BAF10 aligns once. BB3E0 retains a canonical frame relative to the
  // selected parent node; later poses do not realign the child marker.
  std::map<std::uint32_t,Attachment> attachments;
  CinematicCrossfadeState crossfade;
  //4C6860/4920C0 and the native GPU binding are outside the saved game bank.
  // Restoring timed controls must preserve these current external owners.
  double render_clock{};
  bool screen_consumer{};
};
// Residency only. Runtime lighting selection always uses the live flag above.
// A computed HS object expression conservatively requests every definition.
std::set<std::uint32_t> cinematic_lighting_definitions(const LevelContent &);
}
