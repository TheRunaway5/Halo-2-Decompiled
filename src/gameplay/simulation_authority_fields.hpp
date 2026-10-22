#pragma once
// Every ordinary mutable Simulation value bank. Component/identity/geometry/
// Unit/animation owners are staged separately. Campaign saved-state owners
// are explicitly excluded by admission, never silently discarded.
#define HALO2_SIMULATION_AUTHORITY_VALUES(X) \
 X(difficulty) X(bsp) X(pending_bsp) X(bsp_revision) X(object_bodies) \
 X(campaign_weapon_pickups) X(vehicle_flips) X(vehicle_rows) X(vehicle_engine) X(vehicle_timer_option) \
 X(device_events) X(device_event_serial) X(prop_events) X(prop_removals) \
 X(training) X(training_definitions) X(subtitle) X(titles) X(rumble_events) X(ai_dialogue_enabled) \
 X(impulse_clocks) X(script_lists) X(multiplayer_script_lists) X(next_script_list) X(peace) X(sounds) \
 X(projectiles) X(bullet_impacts) X(melee_impacts) X(bullet_impact_serial) \
 X(unit_damage_effects) X(unit_damage_voices) X(unit_damage_voice_serial) X(unit_damage_effect_serial) \
 X(ticks) X(shots) X(hits) X(player_pitch) X(player_input_enabled) X(player_movement_disabled) X(skip_held) \
 X(active_loops) X(mission_segment) X(random_state) X(weather) X(weather_record) X(weather_clock) X(sound_classes) \
 X(objectives) X(flashlights_allowed) X(ambience_details_enabled) X(suppress_ambience_rebuild) X(suppress_automatic_objects) \
 X(multiplayer_weapon_steps) X(source_physics_movement) X(flashlights_changed_tick) \
 X(cinematic_skip_enabled) X(level_finished) X(reset_requested) X(reverted_tick) X(checkpoint_restores)
