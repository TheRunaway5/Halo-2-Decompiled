#pragma once
#include "halo2/math.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>

namespace halo2 {
struct SourceObserverConstants;
struct SourceObserverWorldConstants;
// Immutable operands of the authenticated23FB00/23FF50/1732B0 owners.
struct DeathCameraConstants {
  float default_fov{},random_scale{},distance_scale{},distance_bias{};
  float full_turn{},pitch_scale{},pitch_bias{},transition_seconds{},watch_seconds{};
  float attached_seconds{},one{},half{},probe_scale{},probe_tolerance{},epsilon{};
  float minimum_coordinate{},maximum_coordinate{},maximum_distance{};
};
struct DeathCameraPlayer {
  std::uint32_t datum=0xffffffffu,current_unit=0xffffffffu,previous_unit=0xffffffffu;
  std::int8_t team=-1;
};
struct DeathCameraTarget {
  Vec3 node0; // Actual current WORLD node0 translation, not the object root/eye.
  std::uint8_t type{};
  std::uint16_t biped_flags{};
};
struct DeathCameraState {
  Vec3 initial_position{},anchor{};
  float yaw{},pitch{},distance{},fov{},remaining{};
  std::uint32_t player=0xffffffffu,watched_player=0xffffffffu,target=0xffffffffu;
  float watch_remaining{};
  std::uint8_t attached_ticks{};
  std::int8_t view=-1;
};
// Literal AC-byte caller-owned observer packet; preserves source field/flag layout.
using DeathCameraPacket=std::array<std::uint32_t,43>;
using DeathCameraTargetLookup=std::function<std::optional<DeathCameraTarget>(std::uint32_t)>;
struct DeathCameraSession {
  bool multiplayer=true;
  // Campaign23FE30's actual occupied type6 object iterator. Required only
  // when no current/previous unit or living participant can be selected.
  std::function<std::uint32_t()> fallback_target;
};
// Complete23FCA0 over occupied player records. A same-team player enables
// restriction even when that player's unit is NONE.
bool source_death_camera_team_restriction(std::span<const DeathCameraPlayer>,
    std::uint32_t own);
//23FD40 iterates actual occupied player slots in ascending source pool order.
std::uint32_t source_death_camera_next_player(std::span<const DeathCameraPlayer>,
    std::uint32_t own,std::uint32_t watched,bool team_only);
//23FEA0 target replacement, preserving a still-valid corpse when no new live
// target exists. The caller supplies the actual multiplayer team restriction.
void source_death_camera_replace_target(DeathCameraState&,
    std::span<const DeathCameraPlayer>,bool team_only,const DeathCameraConstants&,
    const DeathCameraSession& = {});
//23FB00 consumes exactly three draws from the caller's retained local RNG.
// Its23FEA0 fallback uses the same session team restriction as later cycling.
DeathCameraState source_death_camera_create(Vec3 observer_position,std::int8_t view,
    std::uint32_t player,std::span<const DeathCameraPlayer>,std::uint32_t& local_rng,
    const DeathCameraConstants&,bool team_only=false,const DeathCameraSession& = {});
struct DeathCameraFrame {
  float seconds{};
  std::int16_t tick_rate{};
  Vec3 observer_position{},observer_up{};
  bool cycle_pressed{},team_only{},clock_initialized{},clock_paused{};
};
// Complete23FF50 packet producer. Pool/target/observer/clock/input
// values are live owner inputs; no absent corpse pose or successful lookup is inferred.
DeathCameraPacket source_death_camera_packet(DeathCameraState&,
    std::span<const DeathCameraPlayer>,const DeathCameraTargetLookup&,
    const DeathCameraFrame&,const DeathCameraConstants&,
    const SourceObserverConstants&,const SourceObserverWorldConstants&,
    const DeathCameraSession& = {});
// Required16B240 owner: normalized source segment fraction, or an actual miss.
using DeathCameraCollision=std::function<std::optional<float>(
    std::uint32_t flags,Vec3 origin,Vec3 delta)>;
// Complete1732B0 four-probe and ten-refinement distance owner. fog_side is the
// original11D4A0 cluster/fog-plane classification, independently supplied.
float source_death_camera_distance(Vec3 forward,Vec3 up,Vec3 target,float distance,
    float margin,bool fog_side,const DeathCameraCollision&,const DeathCameraConstants&);
//170A70/170B60 saved-cluster bank used by172A50 while current ownership is NONE.
struct DeathCameraSavedClusters {
  std::int32_t bsp=-1;
  std::array<std::int32_t,4> clusters{-1,-1,-1,-1};
};
void source_death_camera_save_clusters(DeathCameraSavedClusters&,std::uint32_t flags,
    std::int16_t active_bsp,std::span<const std::uint32_t,4> local_players,
    std::span<const Vec3,4> positions,const std::function<std::int16_t(Vec3)>&);
} // namespace halo2
