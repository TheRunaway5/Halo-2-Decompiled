#pragma once
#include "halo2/multiplayer.hpp"
#include "halo2/simulation_authority.hpp"

namespace halo2 {
// Native in-process continuation values. This is not the display snapshot,
// an authenticated wire format or a complete original player-departure owner.
// Local presentation roster/definitions must match before preparing; moving
// authority to a PC with different local views still needs a session rebind.
struct MultiplayerMatch::AuthorityCheckpoint {
  std::uint32_t version=1,service_roles{};
  AuthorityDigest admitted_binding{},configuration_binding{};
  bool source_backed{};
  std::optional<DatumPool> owned_players;
  std::optional<DatumPool::SourceBinding> source_players;
  Simulation::AuthorityCheckpoint simulation;
  std::map<std::uint32_t,Simulation::AuthorityCheckpoint::UnitRecord> retained_units;
  std::vector<RumbleJournalEvent> rumble_events;
  std::uint64_t rumble_serial{};
  SourceKingState king_state;
  SourceKingHills king_hills;
  std::array<MultiplayerPlayer,multiplayer_capacity> players;
  std::array<Controls,multiplayer_capacity> native_presentation_controls;
  std::map<std::uint32_t,MultiplayerState::Corpse> corpses;
  std::vector<NetgameWeaponState> world_weapon_rows;
  std::vector<SourceNativeWorldWeapon> world_weapon_objects;
  bool world_weapon_configured{},weapon_pickups_configured{};
  std::vector<NativeWeaponPickup> weapon_pickups;
  std::vector<NetgameWeaponState> pickup_rows;
  ObjectGarbageQueue garbage_queue;
  std::array<FirstPersonViewBinding,4> first_person_views,first_person_clock_origins;
  std::array<FirstPersonViewBinding,multiplayer_capacity> replicated_fp_views,replicated_fp_origins;
  std::array<NativeWeaponEffectEvent,multiplayer_capacity> weapon_effect_events;
  std::vector<NativeWeaponEffectEvent> weapon_effect_journal;
  std::uint32_t weapon_effect_serial{};
  std::uint16_t infection_zombies{};
  bool infection_started{};
  std::uint32_t infection_start_tick{};
  bool finished{},results_finalized{};
  SourceGameEndStats end_stats;
  SourceResultStatistics retained_stats;
  SourceResultRecording retained_recording;
  SourceResultEventState result_events;
  bool result_events_started{};
  SourceRoundLifecycle round_lifecycle;
  SourceGameOverState game_over;
  int winner=-1,winner_team=-1;
  std::uint8_t retained_team_mask{};
  std::array<std::int16_t,8> team_scores;
  SourceTeamMapping team_mapping;
};
}
