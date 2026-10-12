#pragma once
#include "halo2/vista_loading.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <optional>

namespace halo2 {
// Original game-engine+E4[4],+F4,+F5[4],+FA. This is a consumed projection;
// it is not the complete original engine bank or the audio game-over owner.
struct VistaGameplayFadeState {
  std::array<std::uint32_t,4> opacity_bits{};
  std::uint8_t activation_mask{};
  std::array<std::uint8_t,4> rendered_frames{};
  std::int16_t phase_counter{};
  bool operator==(const VistaGameplayFadeState&) const = default;
};
struct VistaGameplayFadePlayer {
  // The actual45343F/4513BE output user and45141D player mapping are required.
  // An absent player is a genuine NONE result, not an omitted owner callback.
  bool player_present{},unit_present{};
  std::int32_t respawn_countdown{};
  std::function<bool()> active_player; // Actual46C54F, only reached without unit.
};
struct VistaGameplayFadeTick {
  bool engine_present{},playback{}; //45B15E,4498B7
  std::int16_t phase{},tick_rate{}; // PC+6C and47C096
  std::int32_t pending_phase{}; // PC+C44
  std::uint32_t tick{},round_start_tick{}; //47BFE9,engine+70
  std::array<std::optional<VistaGameplayFadePlayer>,4> users;
};
// Actual46FE78/46A2BC activation writes for a real output user.
void vista_gameplay_fade_activate(VistaGameplayFadeState&,unsigned user);
// Literal475B40..475D10 after unrelated game-engine work. Invoke once per
// actual simulation tick, before the later post-world phase application.
void vista_gameplay_fade_tick(VistaGameplayFadeState&,const VistaGameplayFadeTick&,
    const VistaLoadingFloatingPoint&);
struct VistaGameplayFadeRender {
  bool engine_present{},cubemap_capture_in_progress{}; //45B15E,620FBD→6736AA
  std::uint32_t user=0xffffffffu;
  //664F3C reads geometry-cache failed sbsp/ltmp preload count, not view count.
  std::int32_t unresolved_world_geometry_count{},tick_rate{}; //664F3C,47C096
};
// The47554D black-quad branch, before its separate score-display branch.
// Actual672D24/47602F ARGB packing. Destination rectangle is the actual
// render viewport (8E66F8). Caller must composite this BEFORE the scoreboard.
std::optional<std::uint32_t> vista_gameplay_fade_render(VistaGameplayFadeState&,
    const VistaGameplayFadeRender&,const VistaLoadingFloatingPoint&);
} // namespace halo2
