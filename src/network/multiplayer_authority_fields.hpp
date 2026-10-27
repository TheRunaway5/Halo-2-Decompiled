#pragma once
// Ordinary owning native values. Immutable definitions, callbacks, the
// external source world, player-pool views, Simulation and retained Unit clip
// owners have separate admission/publication paths.
#define HALO2_MULTIPLAYER_AUTHORITY_VALUES(X) \
 X(rumble_events) X(rumble_serial) X(source_hud_sequence) X(king_state) X(king_hills) \
 X(players) X(native_presentation_controls) X(corpses) \
 X(world_weapon_rows) X(world_weapon_objects) X(world_weapon_configured) \
 X(weapon_pickups_configured) X(weapon_pickups) X(pickup_rows) X(garbage_queue) \
 X(first_person_views) X(first_person_clock_origins) \
 X(replicated_fp_views) X(replicated_fp_origins) \
 X(weapon_effect_events) X(weapon_effect_journal) X(weapon_effect_serial) \
 X(source_engine_events) X(source_engine_event_serial) X(source_gameplay_fade) \
 X(physics_impulses) X(physics_impulse_serial) \
 X(infection_zombies) X(infection_started) X(infection_start_tick) \
 X(defusal) X(defusal_bomb) X(defusal_definition) \
 X(finished) X(results_finalized) X(end_stats) X(retained_stats) \
 X(retained_recording) X(result_events) X(result_events_started) \
 X(source_engine_started) X(round_lifecycle) X(game_over) X(winner) X(winner_team) \
 X(retained_team_mask) X(team_scores) X(team_mapping)
