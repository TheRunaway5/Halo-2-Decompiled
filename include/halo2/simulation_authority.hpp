#pragma once
#include "halo2/simulation.hpp"

namespace halo2 {
// Native owning values, without imported callbacks or process resources.
// Unit animation pointers in these records must be null; clips travel by value.
struct Simulation::AuthorityCheckpoint {
  std::uint32_t version=2;
  AuthorityDigest admitted_binding{},bsp_binding{},unit_interaction_binding{};
  std::uint32_t service_roles{};
  bool source_backed{};
  std::optional<NativeObjectLifetimes> owned_lifetimes;
  std::optional<NativeObjectLifetimes::SourceAuthorityCheckpoint> source_lifetimes;
  SourceObjectBodyStorage object_bodies;
  SceneObjects::AuthorityCheckpoint scene;
  SourceDeviceWorld::AuthorityCheckpoint devices;
  PropSandbox::AuthorityCheckpoint props;
  StaticSceneryCollision::AuthorityCheckpoint scenery;
  Presentation::AuthorityCheckpoint presentation;
  ScriptVm::AuthorityCheckpoint scripts;
  std::optional<NativeGrenadeSandbox::AuthorityCheckpoint> grenades;
  std::optional<AuthorityDigest> melee_binding;
  struct UnitRecord {Unit value;std::optional<AnimationClip> animation;};
  struct AnimationRecord {std::optional<AnimationClip> clip;std::uint64_t tick{};bool loop{};};
  UnitRecord player;
  std::vector<UnitRecord> actors;
  std::map<std::uint32_t,AnimationRecord> object_animations;
  // Exact views used by the native collision solver. The BSP/nav definition
  // rebinds locally; fixed/all prefixes retain original contact indices/order.
  BspContent bsp_geometry;
  CollisionWorld::AuthorityCheckpoint bsp_collision;
  std::optional<CollisionWorld::AuthorityCheckpoint> fixed_collision,all_collision;
  unsigned difficulty{};
  std::size_t bsp{};
  std::optional<std::size_t> pending_bsp;
  std::uint64_t bsp_revision{};
  std::vector<NativeWeaponPickup> campaign_weapon_pickups;
  std::vector<VehicleFlip> vehicle_flips;
  std::vector<NetgameWeaponState> vehicle_rows;
  int vehicle_engine{},vehicle_timer_option{};
  std::vector<SourceDeviceWorldEvent> device_events;
  std::uint64_t device_event_serial{};
  std::vector<PropSandboxEvent> prop_events;
  std::vector<PropRemoval> prop_removals;
  PlayerTrainingState training;
  std::vector<TrainingEventDefinition> training_definitions;
  CampaignSubtitle subtitle;
  std::array<CampaignTitle,4> titles;
  std::vector<RumbleEvent> rumble_events;
  bool ai_dialogue_enabled{};
  std::array<ImpulseClock,16> impulse_clocks;
  std::map<std::uint32_t,std::vector<std::uint32_t>> script_lists;
  std::optional<SourceHsListBank> multiplayer_script_lists;
  std::uint32_t next_script_list{};
  std::array<std::array<bool,16>,16> peace;
  std::vector<SoundEvent> sounds;
  std::vector<Projectile> projectiles;
  std::vector<BulletImpact> bullet_impacts;
  std::vector<MeleeImpact> melee_impacts;
  std::uint64_t bullet_impact_serial{};
  std::vector<UnitDamageEffectEvent> unit_damage_effects;
  std::vector<UnitDamageVoiceEvent> unit_damage_voices;
  std::uint64_t unit_damage_voice_serial{},unit_damage_effect_serial{};
  std::uint64_t ticks{},shots{},hits{};
  float player_pitch{};
  bool player_input_enabled{},player_movement_disabled{},skip_held{};
  std::map<std::uint32_t,SoundEvent> active_loops;
  std::string mission_segment;
  std::uint32_t random_state{};
  SourceWeatherState weather;
  SourceWeatherRecord weather_record;
  SourceWeatherClock weather_clock;
  SoundClassFades sound_classes;
  std::array<int,5> objectives;
  bool flashlights_allowed{},ambience_details_enabled{},suppress_ambience_rebuild{},suppress_automatic_objects{};
  bool multiplayer_weapon_steps{},source_physics_movement{};
  std::uint64_t flashlights_changed_tick{};
  bool cinematic_skip_enabled{},level_finished{},reset_requested{};
  std::optional<std::uint64_t> reverted_tick;
  std::uint64_t checkpoint_restores{};
};
}
