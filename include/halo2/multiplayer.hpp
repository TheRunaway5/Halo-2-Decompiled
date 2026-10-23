#pragma once
#include "halo2/source_hud_messages.hpp"
#include "halo2/source_hud_objectives.hpp"
#include "halo2/source_dynamic_object.hpp"
#include "halo2/source_effect_random.hpp"
#include "halo2/custom_slayer.hpp"
#include "halo2/bomb_defusal.hpp"
#include "halo2/frontend.hpp"
#include "halo2/simulation.hpp"
#include "halo2/datum_pool.hpp"
#include "halo2/fp_registration.hpp"
#include "halo2/fp_replication.hpp"
#include "halo2/renderer_timing.hpp"
#include "halo2/source_netgame_weapon.hpp"
#include "halo2/source_contrail_snapshot.hpp"
#include "halo2/source_game_end.hpp"
#include "halo2/source_game_over.hpp"
#include "halo2/source_result_statistics.hpp"
#include "halo2/source_result_capture.hpp"
#include "halo2/source_player_lives.hpp"
#include "halo2/source_round_lifecycle.hpp"
#include "halo2/source_result_event.hpp"
#include "halo2/source_king.hpp"
#include "halo2/source_multiplayer_engine_entity_tu15.hpp"
#include "halo2/source_world_pose.hpp"
#include "halo2/native_death_camera.hpp"
#include "halo2/native_vehicle_camera.hpp"
#include "halo2/weapon_hud.hpp"
namespace halo2 {
class SourceDynamicObjectScene;
class MultiplayerSnapshotOrigin;
struct EffectPresentationObservation;
struct SourceLiveShadowRootTu15;
struct SourceLiveModelBaseDrawTu15;
struct SourceShadowRawPlanStats;
struct RendererCompletedMaterialUpload;
struct RendererObservedBspDraw;
struct SourceEffectLightCommandObservationTu15;
class PhysicsLiveWorldTu15;
struct SourceTeleporterEventsTu15;
struct SourceLightVolumeDraw;
struct SourceContrailDraw;
struct FullScoreboardLayout;
struct FullScoreboardPaintResult;
struct AnimationRenderObservation;
struct MultiplayerAudioSubmission;
struct ParticleDraw;
struct SourcePhysicsContactRuntimeAbi;
using SourcePhysicsWorldRuntimeAbi = SourcePhysicsContactRuntimeAbi;
class SourceRuntimeMemory;
struct SourceFirstPersonCpuInputTu15;
struct EffectLightRawDraw;
struct WorldEffectLight;
struct SourceBspShaderView;
struct SourceBspShaderPart;
struct SourceBspResidentViewPlan;
struct SourcePortalMaskState;
struct ShadowCasterSphere;
struct ObjectLightingState;
struct MultiplayerFrameTiming {
  struct View {double camera_lighting_ms{},static_world_ms{},objects_ms{},body_ms{},first_person_ms{},hud_ms{};};
  // Per-frame aggregate observations; no owners or unbounded event history.
  // A CPU sample requires two successful Linux current-thread clock reads.
  // Fewer CPU samples than wall samples explicitly means unavailable/partial.
  struct Work {double wall_ms{},thread_cpu_ms{};unsigned samples{},thread_cpu_samples{};};
  struct ReceiptCapture {
    unsigned attempts{},completed{},retained{};
    Work values,replica,pose,scene,hud,locomotion,sounds,retention,total;
  };
  // Passive same-clock boundaries, populated only with frame_observer.
  // CPU stamps are current-thread observations, never process/GPU clocks.
  struct Stamp {std::int64_t wall_ns{};double thread_cpu_ms{};bool thread_cpu_available{};};
  struct Schedule {
    bool measured{},had_pending{},has_pending{},terminal_drain{};
    std::uint64_t loop{},first_observer{},last_observer{};
    Stamp iteration_begin,timed_start,first_command_tail,last_command_tail,input_end,renderer_end;
    // client_send_calls counts returned send() calls, not emitted/arrived packets.
    unsigned command_tails{},client_send_calls{},input_observer_calls{},receive_calls{};
    std::size_t bank_size{},cursor_begin{},cursor_end{},projected_states{};
    Work receive,projection;
  };
  unsigned frame{},views{},bodies_drawn{},body_pose_builds{};
  unsigned vehicle_skin_builds{},vehicle_skin_reuses{};
  unsigned unit_skin_builds{},unit_skin_reuses{};
  unsigned selected_skin_requests{},selected_skin_reuses{};
  double input_ms{},common_ms{},renderer_ms{},wall_ms{};
  bool submitted{};
  std::array<View,4> view;
  RendererFrameTiming renderer;
  ReceiptCapture receipt_capture;
  Schedule schedule;
};
constexpr unsigned multiplayer_capacity = 16;
struct SlayerRules {
  // Validated configuration that produced these rules. Handwritten fixtures
  // can leave it absent; discovery must never invent settings for them.
  std::optional<UiVariant> session_configuration;
  // These projections require the complete retained source engine, including
  // settings not implemented by the older native-only adapters.
  bool requires_source_engine{};
  // Original registered engine type, independent of UI/catalog engine IDs.
  std::int32_t source_engine=2;
  std::string name = "Slayer";
  int score_limit = 25;
  std::int32_t round_mode{}; // Original variant+4C, consumed by limit-one HUD.
  std::int32_t life_limit{};
  unsigned time_ticks = 480 * 30, respawn_ticks = 5 * 30,
           suicide_ticks = 5 * 30;
  bool suicide_loss = true, death_loss = false, starting_grenades = true;
  bool maximum_one_grenade{}; // Source runtime engine flag; lifecycle is separate.
  bool team_mode{};
  std::int32_t team_scoring{}, respawn_modifier = 2;
  unsigned betrayal_ticks = 10 * 30;
  unsigned maximum_players = 16;
  unsigned starting_weapon{};
  unsigned weapon_set{}; // Source parameter83, variant+D4/options+210.
  unsigned secondary_starting_weapon=1;
  CustomSlayer custom=CustomSlayer::none;
  std::optional<BombDefusalRules> defusal;
  std::optional<BombDefusalLayout> defusal_layout;
  std::array<std::uint32_t,18> weapon_choices{};
  int primary_light_vehicle{}; // Original parameter76, variant+CD/options+209.
  int primary_heavy_vehicle{}; // Original parameter78, variant+CF/options+20B.
  std::uint32_t weapon = null_datum;
  // Original options initialization 139880+5C; 139540 copies options+10
  // into the shared deterministic RNG. Later world consumers are separate.
  std::uint32_t random_seed = 0x78a8;
  std::uint32_t spawn_salt = 0xffffffff; // Original default options +14.
  std::uint32_t king_flags{}; // Original variant+F0, not named-variant switches.
  std::int16_t king_move_seconds{}; // Original signed variant+F4.
  std::uint32_t player_flags=0xf9a; // Original default variant+48 word.
};
// Team sessions require explicit selected membership inputs from the profile
// handoff or another declared session provider.
SlayerRules slayer_rules(const UiVariant &, bool team_session = false);
SlayerRules multiplayer_source_rules(const UiVariant &,bool team_session=false);
// Bounded native binding entry point. Normal menu admission remains separate
// until the actual King presentation/traits/replication consumers are installed.
SlayerRules king_rules(const UiVariant &, bool team_session = false);
void validate_slayer_map_options(const LevelContent &,const SlayerRules &);
int slayer_parameter_default(unsigned);
bool slayer_spawn(const Spawn &, std::size_t bsp, bool initial);
// Retail14F040: appearance0/1 selects multiplayer representation2/3.
unsigned multiplayer_representation(const std::optional<SourcePlayerConfiguration> &);
unsigned source_session_player_representation(const std::optional<SourcePlayerConfiguration> &,int game_type);
std::vector<unsigned> source_session_player_representations(const LevelContent &,int game_type);
const BipedDefinition &multiplayer_biped(const LevelContent &,unsigned representation);
struct MultiplayerPlayer {
  std::uint32_t source_datum = null_datum;
  // Present allocated roster row. An ordinary TU1.5 departure retains this
  // row with source_flags bit2; gameplay/voice eligibility must exclude it.
  bool connected{}, death_counted{};
  std::string name;
  std::optional<SourcePlayerConfiguration> source_profile_request,source_configuration;
  int score{}, kills{}, deaths{};
  int assists{}, suicides{};
  std::uint16_t nonenemy_credit{};
  bool attribution_counted{}, incoming_suicide{};
  RespawnTimer respawn;
  bool source_current_unit{};
  std::uint32_t source_current_object = null_datum,
      source_previous_unit = null_datum, source_watched_unit = null_datum;
  bool source_initial_spawn = true;
  std::int16_t source_spawn_count{};
  std::uint8_t source_ffa_identity{};
  std::int8_t source_membership = -1; // Source player+C0, separate from FFA identity.
  std::uint16_t source_flags = 1; // Original player+2; other flag producers remain open.
  // Native publication of completed original teleports. Retained across lost
  // snapshots so clients cut pose/view interpolation even for nearby portals.
  std::uint32_t source_teleport_serial{};
  std::uint32_t source_teleport_sound=null_datum;
  SourceHudMessages source_messages;
  std::int16_t source_lives_remaining=-1; // Original signed player+1AC.
  std::int16_t source_king_inside_ticks{}; // Original King player+1B8.
  std::uint8_t source_strength{}; // Sanitized configuration+7D -> player+C1.
  std::int32_t source_field_194{}; // Birth0, successful spawn tick rate, pre-world countdown.
  std::array<std::int16_t,2> source_powerup_timers{}; // Original player+198/+19A.
};
// Authoritative weapon input/charge fields, retained independently per hand.
struct NativeWeaponTriggerPublication {
  std::uint16_t input_flags{};
  std::int16_t ticks{};
  std::uint8_t mode{},charge{};
  bool target_acquired{};
  std::optional<std::array<float,3>> target_position;
  bool operator==(const NativeWeaponTriggerPublication &)const=default;
};
struct NativeWeaponEffectEvent {
  std::uint32_t serial{},tick{},unit=null_datum,weapon=null_datum,effect=null_datum,a_bits{},b_bits{};
  SourceEffectColors colors;
  Vec3 velocity;
  std::uint32_t source_instance=null_datum;
  bool source_presentation{},source_scheduled{},source_locations_observed{};
  std::vector<SourceEffectLocation> source_locations;
};
struct WorldWeaponPresentation {
  std::uint32_t handle=null_datum,definition=null_datum;
  Vec3 position,forward{1,0,0},up{0,0,1};
  int loaded{},reserve{};
  SourceWeaponShaderState shader;
};
struct WorldVehiclePresentation {
  std::uint32_t handle=null_datum;std::uint32_t placement{};Transform transform;
  // Completed original ultimate-root lighting inputs, distinct from this
  // model's own posed coverage sphere. NONE retains the native static scene
  // boundary; it is not presented as a source physics/cache owner.
  std::uint32_t lighting_owner=null_datum,lighting_definition=null_datum;
  Vec3 lighting_origin,lighting_velocity;
  std::uint32_t lighting_flags{};
  bool lighting_frozen{};
  // Own object+4 / object+10A; model geometry LOD never inherits root flags.
  std::uint32_t geometry_flags{};
  std::uint16_t geometry_control_flags{};
  // Actual original body region choices, including random construction and later damage.
  std::optional<SourceModelRegionBank> regions;
};
struct MultiplayerState {
  std::optional<BombDefusalState> defusal;
  std::optional<BombDefusalRules> defusal_rules;
  struct Corpse {
    std::uint32_t object=null_datum,definition=null_datum,player=null_datum,death_tick=null_datum;
    std::optional<SourcePlayerConfiguration> configuration;
    Vec3 velocity;
    std::uint8_t physical_state=0xff;
    float health{},shield{};
    DamageFeedbackState damage_feedback;
  };
  struct Player {
    bool connected{}, dead{};
    Vec3 position;
    float yaw{}, pitch{}, crouch{}, health{}, shield{};
    std::uint32_t weapon = null_datum;
    int loaded{}, reserve{}, score{}, kills{}, deaths{};
    std::uint32_t shot_tick{}, reload_tick{};
    float reload_seconds{};
    bool reload_empty{};
    Vec3 velocity;
    // Native mover presentation inputs, not retail physical-state bytes.
    float native_forward{},native_right{};
    bool native_grounded{},native_crouching{};
    SourceLandingState landing;
    // Global material from an admitted BSP support triangle; FFFF means NONE.
    std::uint16_t native_support_material=0xffff;
    std::array<std::uint8_t, 2> grenades{};
    std::int8_t grenade_current = -1;
    NativeGrenadeAction grenade_action;
    NativeMeleeAction melee_action;
    int assists{}, suicides{};
    std::int16_t total_score{}, wins{};
    std::int16_t source_lives_remaining=-1;
    std::int16_t source_king_inside_ticks{};
    std::int32_t source_respawn_countdown{};
    // Full native snapshot identities for every roster participant.
    // Current unit/weapon are NONE after ownership release, distinct from a corpse.
    std::uint32_t source_player=null_datum, source_unit=null_datum, source_weapon=null_datum;
    std::string name;
    std::optional<SourcePlayerConfiguration> source_configuration;
    std::uint16_t source_flags{};
    std::uint32_t source_teleport_serial{};
    std::uint32_t source_teleport_sound=null_datum;
    SourceHudMessages source_messages;
    std::int16_t source_membership = -1;
    std::uint8_t source_ffa_identity{};
    UnitZoomState source_zoom;
    std::int16_t source_zoom_magazine_mode{};
    std::uint32_t source_parent=null_datum, source_action_word{};
    std::int16_t source_seat=-1;
    std::optional<NativeDeathCameraView> source_vehicle_camera;
    NativeActionPrompt action_prompt;
    // Authoritative current unit fade values; clients do not simulate timers.
    SourceCamouflageAmounts source_camouflage;
    std::uint8_t source_unit_10a{};
    std::uint32_t zoom_reset_serial{};
    std::uint16_t source_hud_zoom_mask = 0x40;
    std::uint32_t source_hud_target_flags{}; //13C8D4/TU1.5 13DC44; original aim mask.
    bool source_hud_autoaimed{}; //13C4B8 selector18: unit+1E4 confidence >=1.
    SourceHudObjectives source_hud_objectives;
    VehicleWeaponHudStatus source_vehicle_hud;
    // Unit+14A bits0/5 are primary/secondary trigger holds (C6C50), read by
    // 1A20E6. Grenade visibility uses the retained component+0C throw phase.
    std::uint8_t motion_sensor_control{};
    FirstPersonReplicatedClock first_person_clock;
    NativeWeaponEffectEvent weapon_effect;
    std::uint32_t source_previous_unit=null_datum, source_death_tick=null_datum;
    // Native host-computed observer view for this peer's own dead player.
    // It is omitted for living units and participants on other peers.
    std::optional<NativeDeathCameraView> source_death_camera;
    struct SecondaryWeapon {
      std::uint32_t definition=null_datum,datum=null_datum,shot_tick{},reload_tick{};
      int loaded{},reserve{};
      float reload_seconds{};
      bool reload_empty{};
      FirstPersonReplicatedClock clock;
      SourceWeaponShaderState shader;
      NativeWeaponTriggerPublication trigger;
      int ammunition_available{}; // Authoritative104CF0 inventory read for FP reload pose.
      std::int16_t magazine_mode{};
    } secondary_weapon;
    DamageFeedbackState damage_feedback;
    bool shield_recharging{};
    SourceWeaponShaderState weapon_shader;
    NativeWeaponTriggerPublication weapon_trigger;
    int ammunition_available{};
  };
  std::uint32_t tick{};
  std::int32_t source_game_type=2;
  std::int32_t source_engine=2;
  std::optional<SourceMultiplayerEngineEntityTu15> source_engine_entity;
  struct KingObjective {
    std::int32_t hill=-1;
    std::uint16_t occupants{};
  };
  std::optional<KingObjective> king;
  bool finished{};
  bool results_finalized{};
  std::int32_t game_over_countdown{};
  bool game_over_fade_started{};
  bool game_over_complete()const{return finished && game_over_countdown==0;}
  int winner = -1;
  int score_limit = 25;
  std::int32_t round_mode{};
  std::int32_t life_limit{};
  unsigned time_ticks = 14400;
  std::int16_t clock_seconds{};
  std::string variant_name = "Slayer";
  bool team_mode{};
  std::uint8_t retained_team_mask{};
  int winner_team = -1;
  std::array<std::int16_t, 8> team_scores{};
  std::array<std::int16_t, 8> team_total_scores{}, team_wins{};
  // Actual source engine team records, including extended rank fields2/3/6.
  std::array<std::array<std::int16_t,9>,8> team_statistics{};
  std::array<Player, multiplayer_capacity> players;
  std::vector<PropRemoval> prop_removals; // Persistent authoritative native scene tombstones.
  std::vector<PropRegionSnapshot> prop_regions;
  SourceDeviceWorldSnapshot devices; // Complete authoritative source state over native LAN.
  std::vector<SourceDeviceWorldEvent> device_events;
  std::vector<RumbleJournalEvent> rumble_events;
  std::vector<NativeGrenadeProjectile> grenade_projectiles;
  std::vector<NativeGrenadeDetonation> grenade_detonations;
  std::vector<BulletImpact> bullet_impacts;
  std::vector<MeleeImpact> melee_impacts;
  std::vector<UnitDamageEffectEvent> unit_damage_effects;
  std::vector<NativeWeaponEffectEvent> weapon_effects;
  std::vector<UnitDamageVoiceEvent> unit_damage_voices;
  std::vector<WorldWeaponPresentation> world_weapons;
  std::vector<WorldVehiclePresentation> world_vehicles;
  bool source_scene_roots_authoritative{};
  std::vector<SourceSceneRoot> source_scene_roots;
  std::vector<SourceDynamicObject> source_dynamic_objects;
  std::vector<NativeWeaponProjectile> weapon_projectiles;
  SourceContrailSnapshot source_contrails;
  bool source_body_poses_authoritative{};
  std::vector<SourceWorldPose> source_body_poses;
  // Detached bodies retain appearance and lifetime independently of a later
  // player spawn, profile change, disconnect or second death.
  std::vector<Corpse> corpses;
};
void observe_multiplayer_first_person(FirstPersonViewBinding &,
    const MultiplayerState::Player &,const FirstPersonBindingDefinition &);
class MultiplayerMatch;
class MultiplayerObjectRuntime;
struct SourcePlayerEquipmentTu15;
struct MultiplayerKingServices {
  // Optional observation after the actual internal result-event recorder.
  std::function<void(const MultiplayerMatch&,std::uint32_t,std::int32_t,std::uint32_t)> record;
  // Actual external score/objective presentation consumers remain required.
  std::function<void(const MultiplayerMatch&,std::uint32_t,std::int32_t)> score_changed;
  std::function<void(const MultiplayerMatch&,const SourceKingNotice&)> notification;
};
class MultiplayerMatch {
  friend class MultiplayerPreparedWorld;
public:
  MultiplayerMatch(const LevelContent &, std::size_t, std::span<const Spawn>,
                   SlayerRules,MultiplayerKingServices = {},
                   std::function<std::uint32_t()> result_wall_clock = {},
                   MultiplayerObjectRuntime *source_object_world = nullptr,
                   std::optional<SourceSceneGameContext> game_context = {},
                   std::function<std::optional<Spawn>(unsigned,bool)> source_spawn_input = {},
                   bool source_replica = false);
  MultiplayerMatch(const MultiplayerMatch &) = delete;
  ~MultiplayerMatch();
  MultiplayerMatch &operator=(const MultiplayerMatch &) = delete;
  MultiplayerMatch(MultiplayerMatch &&) = delete;
  MultiplayerMatch &operator=(MultiplayerMatch &&) = delete;
  using AuthorityDigest=Simulation::AuthorityDigest;
  struct AuthorityCheckpoint;
  class PreparedAuthority {
  public:
    ~PreparedAuthority();
    PreparedAuthority(PreparedAuthority &&) noexcept;
    PreparedAuthority &operator=(PreparedAuthority &&) noexcept;
    bool for_owner(const MultiplayerMatch &) const noexcept;
  private:
    friend class MultiplayerMatch;
    PreparedAuthority();
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };
  // The session executor must quiesce Match, its external source world and
  // every borrowed reference. The binding is supplied by that executor; it
  // is not peer authentication. Local view/service roles must already match.
  AuthorityCheckpoint authority_checkpoint(AuthorityDigest admitted_binding,
                                          bool qualified_source_world=false) const;
  PreparedAuthority prepare_authority(const AuthorityCheckpoint &,AuthorityDigest,
                                      const SourceRuntimeMemory *replacement_source_world=nullptr);
  // For source-backed sessions publish the exact staged external world first.
  // A premature call leaves the transaction intact. Publication allocates
  // nothing; borrowed spans/references expire on successful publication.
  bool commit_authority(PreparedAuthority &&) noexcept;
  unsigned join(std::string name, std::int8_t selected_team = -1,
      std::optional<SourcePlayerConfiguration> = std::nullopt,
      std::optional<unsigned> admitted_slot = std::nullopt);
  void validate_player_configuration(const SourcePlayerConfiguration&,std::int8_t selected_team)const;
  bool team_mode() const { return rules_.team_mode; }
  std::int32_t source_engine() const { return rules_.source_engine; }
  bool source_engine_owned() const { return source_engine_owned_; }
  std::int32_t source_game_type() const { return source_game_context_.game_type; }
  const SourceKingState* source_king_state()const {
    return rules_.source_engine==4 ? &king_state_ : nullptr;
  }
  unsigned available_player_slots() const;
  // Ordinary TU1.5 departure retains the allocated roster/profile/results.
  // Its original unit-death request is adopted after a later world phase.
  bool depart(unsigned);
  // Failed native transport admission uses the existing allocation rollback;
  // it is distinct from an admitted source-world participant's departure.
  void cancel_join(unsigned);
  void leave(unsigned);
  // Explicit native service input to original1537F0. Authored pickup and
  // variant admission have separate owners; this API does not supply them.
  void grant_powerup(unsigned player,std::int16_t index,std::int16_t ticks);
  void step(const std::array<Controls, multiplayer_capacity> &);
  // Optional source-qualified local activation policy, installed before joins.
  // Real allocation/configuration reaches birth before the first unit exists;
  // the first step runs activation before original respawn eligibility.
  void set_source_player_activation_owner(bool defer_initial_spawn,
      std::function<void(MultiplayerPlayer&,unsigned)> birth,
      std::function<void(MultiplayerMatch&)> pre_player_update);
  void apply_source_player_activation(std::uint32_t actual_datum,
      std::uint16_t expected_flags,std::uint16_t after_flags);
  const Unit &unit(unsigned) const;
  const SourceInventory &source_inventory(unsigned) const;
  const WeaponState *inventory_weapon(unsigned, std::int16_t slot) const;
  const WeaponState *weapon_from_datum(std::uint32_t) const;
  const MultiplayerPlayer &player(unsigned) const;
  const SourceGameEndStats& source_match_statistics()const{return end_stats_;}
  const SourceRoundLifecycle& source_round_lifecycle()const{return round_lifecycle_;}
  // Optional presentation owner at the real pre-world engine update boundary,
  // after end checks and before object updates and post-world phase application.
  void set_game_engine_presentation_owner(std::function<void(const MultiplayerMatch&)> owner) {
    NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
    game_engine_presentation_owner_=std::move(owner);
  }
  // Raw producer bank indexed by native live slots. Retained identity remapping,
  // participant properties and completion publication still need their
  // original owners before a PCR can consume it.
  const SourceResultStatistics* recorded_statistics()const;
  SourceResultRecording result_recording()const{return retained_recording_;}
  const SourceResultEventState* recorded_events()const{return recorded_statistics() ? &result_events_ : nullptr;}
  bool result_recording_started()const{return result_events_started_;}
  const MultiplayerPlayer *player_from_datum(std::uint32_t) const;
  const Unit *unit_from_datum(std::uint32_t) const;
  const ObjectGarbageQueue &garbage_queue() const { return garbage_queue_; }
  void configure_local_views(std::span<const unsigned>,const FirstPersonBindingDefinition &);
  void configure_local_views(std::span<const unsigned>,std::span<const FirstPersonBindingDefinition>);
  // Native presentation replica for remote participants; does not register
  // local observers or alter the weapon's original hand-feedback ownership.
  void configure_replicated_first_person(const FirstPersonBindingDefinition &);
  // Invoked by the actual registered local hand clock, with its full owner
  // and original authored event. The consumer owns camera/sound/effect gates,
  // request construction and real playback; remote replicas emit no duplicate.
  using FirstPersonEventOwner = std::function<void(
      unsigned view, unsigned hand, const FirstPersonViewBinding &,
      const FirstPersonAnimationEvent &)>;
  void configure_first_person_event_owner(FirstPersonEventOwner);
  // Presentation callbacks capture session-local render/audio owners.
  // Detach them before those owners leave scope; the actual Match can survive.
  void release_presentation_owners();
  // Declared original effect service boundary. A live owner must resolve
  // actual attachment/PVS/event/particle state; a shot counter is insufficient.
  void configure_weapon_effect_owner(
      std::function<std::uint32_t(const Unit &,const WeaponEffectInput &)>);
  void weapon_effect_presentation(std::uint32_t unit,std::uint32_t weapon,
      const SourceWeaponEffectPresentation &,Vec3 velocity,std::span<const SourceEffectLocation> locations={});
  void publish_source_projectile_contact(const SourceProjectileDetonationTu15 &);
  void configure_world_weapon_effect_owner(
      std::function<std::uint32_t(const WeaponEffectInput &)>);
  void configure_grenades(std::shared_ptr<const Cache>,NativeGrenadeConstants);
  void configure_melee_animation(std::shared_ptr<const Cache>,NativeMeleeConstants);
  std::size_t native_object_capacity()const{return std::size_t(simulation_.object_lifetimes().pool().capacity());}
  std::span<const NativeGrenadeProjectile> grenade_projectiles()const {
    return simulation_.grenade_sandbox_?simulation_.grenade_sandbox_->projectiles():std::span<const NativeGrenadeProjectile>{};
  }
  std::vector<NativeGrenadeEvent> take_grenade_events();
  const AnimationClip* grenade_clip(std::uint32_t graph,std::uint32_t clip)const{return simulation_.grenade_sandbox_?simulation_.grenade_sandbox_->action_clip(graph,clip):nullptr;}
  // Actual native salted object allocation, with explicit source-admitted
  // stock levitated rows. Ground/controller/widget services are separate.
  // Native typed gameplay adapter; the raw source inventory kernels remain independent.
  void configure_weapon_pickups();
  std::span<const NativeWeaponPickup> weapon_pickups()const{return weapon_pickups_;}
  NativeActionPrompt action_prompt(unsigned player)const;
  void configure_world_weapon_rows(std::span<const SourceNetgameWeaponRegistration>);
  std::span<const SourceNativeWorldWeapon> world_weapon_objects()const{return world_weapon_objects_;}
  std::span<const NetgameWeaponState> world_weapon_rows()const{return world_weapon_rows_;}
  const FirstPersonViewBinding &first_person_view(unsigned view) const { return first_person_views_.at(view); }
  // Read-only actual channel input immediately before the last tick's source
  // animation clock. Presentation can sample that interval after action selection.
  const FirstPersonViewBinding &first_person_clock_origin(unsigned view) const { return first_person_clock_origins_.at(view); }
  MultiplayerState state() const;
  bool finished() const { return finished_; }
  const Simulation &simulation() const { return simulation_; }
  void run_script(const std::string &name) {
    NativeAuthorityBoundary::Call active(authority_calls_);
    if(simulation_.devices().replicated())throw FormatError("Client cannot start an authoritative script");
    ++authority_calls_.mutations;simulation_.run_script(name);
  }

  bool source_object_world_bound() const {return source_object_world_!=nullptr;}
  MultiplayerObjectRuntime *source_object_runtime() {return source_object_world_;}
  const std::optional<UiVariant> &session_configuration() const {return rules_.session_configuration;}
  // Explicit authored placement request through the retained source factory.
  // This does not stand in for automatic netgame-row scheduling.
  std::uint32_t create_world_vehicle(std::size_t placement);
  // Authored scene lifecycle requests use the same retained constructor and
  // destructor as initial placement. No solved root/velocity is supplied.
  std::uint32_t create_world_object(std::size_t placement);
  void destroy_world_object(std::uint32_t full_object);
  const OriginalData *original_data()const{return content_.original_data.get();}
  std::uint32_t presentation_seed()const{return rules_.random_seed;}
  std::vector<Transform> source_world_nodes(std::uint32_t object) const;
  ShadowCasterSphere source_object_sphere(std::uint32_t object) const;
  // Only an admitted game-tick effect owner may bind this deterministic word.
  // Render-batched visual playback must not consume gameplay RNG here.
  SourceEffectRandom source_effect_random(std::uint32_t &auxiliary) {
    NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
    return {simulation_.source_primary_random(),auxiliary};
  }
  void advance_weather_view(double realtime,std::int16_t view_slot);
  void advance_audio(float seconds);
  std::vector<SoundEvent> take_sounds();
  std::vector<PropSandboxEvent> take_prop_events();
  void replicate_prop_removals(std::span<const PropRemoval> rows) {NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;simulation_.replicate_prop_removals(rows); }
  void replicate_prop_regions(std::span<const PropRegionSnapshot> rows) {NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;simulation_.replicate_prop_regions(rows); }
  void replicate_source_scene(std::span<const SourceSceneRoot> roots,std::span<const SourceWorldPose> poses) {NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;simulation_.replicate_source_scene(roots,poses); }
  void replicate_devices(const SourceDeviceWorldSnapshot &s){NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;simulation_.replicate_devices(s);}
  Simulation::ReplicaCheckpoint replica_checkpoint(const MultiplayerSnapshotOrigin &origin)const {
    if(!source_replica_)throw FormatError("A local authority cannot export a display replica checkpoint");
    authority_calls_.capture();return simulation_.replica_checkpoint(origin);
  }
  bool owns_replica_checkpoint(const Simulation::ReplicaCheckpoint &saved)const noexcept {
    return simulation_.owns_replica_checkpoint(saved);
  }
  Simulation::PreparedReplica prepare_replica(const Simulation::ReplicaCheckpoint &saved) {
    return simulation_.prepare_replica(saved,&authority_calls_);
  }
  bool replica_ready(const Simulation::PreparedReplica &prepared)const noexcept {
    return simulation_.replica_ready(prepared);
  }
  bool commit_replica(Simulation::PreparedReplica &prepared)noexcept {
    return simulation_.commit_replica(prepared);
  }
  SourceControlActivation activate_admitted_control(std::uint32_t h){NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;return simulation_.activate_admitted_control(h);}

private:
  std::array<const std::uint64_t *,2> execution_call_counters()const;
  friend class SourceMatchAuthority;
  mutable NativeAuthorityBoundary authority_calls_;
  AuthorityDigest authority_configuration()const;
  std::uint32_t authority_service_roles()const;
  void adopt_source_prop_regions();
  void adopt_source_devices();
  std::vector<RumbleJournalEvent> rumble_events_;
  std::uint64_t rumble_serial_{};
  std::uint8_t source_hud_sequence_{};
  const LevelContent &content_;
  MultiplayerObjectRuntime *source_object_world_{};
  bool source_replica_{};
  SourceSceneGameContext source_game_context_;
  Simulation simulation_;
  std::vector<Spawn> spawns_;
  std::function<std::optional<Spawn>(unsigned,bool)> source_spawn_input_;
  SlayerRules rules_;
  std::vector<SourceKingMarker> king_markers_;
  SourceKingState king_state_;
  SourceKingHills king_hills_;
  MultiplayerKingServices king_services_;
  std::array<MultiplayerPlayer, multiplayer_capacity> players_;
  std::array<Controls,multiplayer_capacity> native_presentation_controls_;
  DatumPool player_pool_{"players", multiplayer_capacity, 0x21c};
  std::map<std::uint32_t, Unit> retained_units_;
  std::map<std::uint32_t, MultiplayerState::Corpse> corpses_;
  std::vector<SourceNetgameWeaponRegistration> world_weapon_definitions_;
  std::vector<NetgameWeaponState> world_weapon_rows_;
  std::vector<SourceNativeWorldWeapon> world_weapon_objects_;
  bool world_weapon_configured_{};
  bool weapon_pickups_configured_{};
  std::vector<NativeWeaponPickup> weapon_pickups_;
  std::vector<NetgameWeaponState> pickup_rows_;
  void step_weapon_pickups(const std::array<Controls,multiplayer_capacity> &);
  void drop_weapon(Unit &,int slot);
  void drop_secondary_weapon(Unit &);
  void drop_carried_weapons(Unit &);
  ObjectGarbageQueue garbage_queue_;
  std::array<FirstPersonViewBinding,4> first_person_views_;
  std::array<FirstPersonViewBinding,4> first_person_clock_origins_;
  std::vector<unsigned> first_person_players_;
  std::map<std::pair<unsigned,std::uint32_t>,FirstPersonBindingDefinition> first_person_definitions_;
  FirstPersonEventOwner first_person_event_owner_;
  std::map<std::pair<unsigned,std::uint32_t>,FirstPersonBindingDefinition> replicated_fp_definitions_;
  std::array<FirstPersonViewBinding,multiplayer_capacity> replicated_fp_views_,replicated_fp_origins_;
  std::array<NativeWeaponEffectEvent,multiplayer_capacity> weapon_effect_events_;
  std::vector<NativeWeaponEffectEvent> weapon_effect_journal_;
  std::uint32_t weapon_effect_serial_{};
  std::function<std::uint32_t(const Unit&,const WeaponEffectInput&)> weapon_effect_owner_;
  std::uint16_t infection_zombies_{};
  bool infection_started_{};
  std::uint32_t infection_start_tick_{};
  std::optional<BombDefusalState> defusal_;
  std::uint32_t defusal_bomb_=null_datum,defusal_definition_=null_datum;
  BombDefusalPlayer defusal_player(unsigned,const Controls &) const;
  void update_defusal(const std::array<Controls,multiplayer_capacity> &);
  void reset_defusal_players();
  void start_infection();
  void infect(unsigned);
  void update_infection();
  bool finished_{};
  bool results_finalized_{};
  SourceGameEndStats end_stats_;
  SourceResultStatistics retained_stats_;
  SourceResultRecording retained_recording_;
  SourceResultEventState result_events_;
  bool result_events_started_{};
  std::function<std::uint32_t()> result_wall_clock_;
  std::uint32_t result_wall_adjustment_{};
  std::uint32_t result_wall_time() const;
  bool prepare_result_clock(PreparedAuthority &) const noexcept;
  void record_result_event(std::uint32_t player,std::uint32_t first,std::uint32_t second);
  SourceRoundLifecycle round_lifecycle_;
  bool source_engine_owned_{};
  bool source_engine_started_{};
  // Prepared source worlds can run a real initial frame before callers attach
  // presentation definitions. This marks that setup boundary, not game time.
  std::uint64_t first_person_setup_tick_{};
  void synchronize_source_engine(bool bindings=false);
  void consume_source_teleporters(const SourceTeleporterEventsTu15 &);
  void expire_source_hud_messages();
  bool source_engine_query(unsigned slot,std::array<std::uint32_t,2> arguments);
  void adopt_source_player_units();
  std::function<void(const MultiplayerMatch&)> game_engine_presentation_owner_;
  bool defer_initial_player_spawn_{};
  std::function<void(MultiplayerPlayer&,unsigned)> player_birth_owner_;
  std::function<void(MultiplayerMatch&)> pre_player_update_owner_;
  SourceGameOverState game_over_;
  SourceGameOverConstants game_over_constants_;
  int winner_ = -1;
  int winner_team_ = -1;
  std::uint8_t retained_team_mask_{};
  std::array<std::int16_t, 8> team_scores_{};
  SourceTeamMapping team_mapping_;
  Unit &unit_mutable(unsigned);
  SourceCamouflageServices camouflage_services();
  SourcePlayerConfiguration prepare_player_configuration(const SourcePlayerConfiguration&,std::int8_t)const;
  bool spawn(unsigned, bool initial,std::uint32_t completed_source_unit=0xffffffffu);
  void apply_score(unsigned, std::int32_t delta);
  void update_game_engine();
  void game_engine_death(std::uint32_t attacker,std::uint32_t victim,bool relationship);
  std::optional<SourceResultPublication> result_publication();
  std::int16_t increment_stat(unsigned player,unsigned field,int result_stat);
  std::int16_t record_credited_kill(unsigned player,std::optional<std::uint8_t> cause);
  void record_victim_death(unsigned victim,std::uint32_t credited,bool self,bool betrayal,
      std::optional<std::uint8_t> cause);
  void end_match(int player_winner,int team_winner);
  void apply_round_phase();
  void finalize_results();
  void prepare_respawn(unsigned victim, std::uint32_t credited_player, std::uint32_t death_clock);
  std::uint32_t allocate_object(std::uint8_t type);
  void step_world_weapons(std::uint32_t tick);
  void retain_unit(Unit &);
  void synchronize_source_weapon_state(WeaponState &) const;
  void synchronize_source_equipment(Unit &,const SourcePlayerEquipmentTu15 &);
  void damage_event(Unit &, const DamageOwner &, const UnitDamageResult &, std::int16_t damage_class);
  void owner_counter_event(const Unit &, const DamageOwner &, std::int16_t damage_class);
  void finish_death(Unit &, MultiplayerPlayer &, std::uint32_t tick);
  ObjectGarbageState &object_garbage(std::uint32_t);
  void update_first_person_views();
  void update_replicated_first_person_views();
};
struct MultiplayerOptions {
  // A synchronous native session load can initialize this actual world once.
  // Runtime validates the request and reuses its Cache, Match and local roster.
  std::shared_ptr<class MultiplayerPreparedWorld> prepared_world;
  std::optional<std::uint32_t> session_random_seed;
  std::shared_ptr<class OnlineMatchSession> online_match;
  bool match_voice{};
  std::shared_ptr<DesktopSession> desktop;
  // Retained frontend music device/state; once per actual world presentation
  // update, independent of the optional source lifecycle/announcer adapters.
  std::function<void()> menu_audio_update;
  std::function<std::optional<FrontendInviteNotice>()> party_notice;
  std::function<bool(bool)> party_notice_action;
  std::filesystem::path map = "assets/disc/maps/lockout.map", screenshot, shared_maps;
  // Finite captures can pin unchanged compiled shader assets independently
  // from concurrent builds. Ordinary sessions retain their configured path.
  std::filesystem::path capture_shader_directory;
  std::filesystem::path gameplay_xbe; // Explicit user-owned update data source.
  // TU1.5 sessions automatically retain their original object/physics world.
  // This flag also requests the older source integration explicitly; an empty
  // directory selects session-private scratch storage.
  bool source_object_world{};
  // Original kind1 world and authored campaign content, shared by physical
  // player rosters. Empty retains ordinary kind2 multiplayer admission.
  std::optional<std::int16_t> campaign_difficulty;
  // Campaign activation supplies its actual initial spawn input. A missing
  // owner is an error; multiplayer scoring is not a campaign spawn algorithm.
  std::function<std::optional<Spawn>(unsigned,bool)> campaign_spawn_input;
  // Read-only diagnostics after original map/session binding, before automatic
  // object construction. No source data or physical result can be supplied.
  std::function<void(const SourceRuntimeMemory &)> source_world_observer;
  std::filesystem::path source_object_savegame_directory;
  // Explicit native placement inputs before object birth, for offline live
  // acceptance. Empty selects the map's authored spawn bank as usual.
  std::vector<Spawn> native_spawn_inputs;
  std::filesystem::path ui_assets; // Imported Vista/MCC HUD art; original geometry/logic retained.
  std::filesystem::path menu_ui_assets; // Independently selected pause/menu sampling pack.
  std::size_t bsp{};
  unsigned local_players = 1, host_port{}, frames{}, ticks{}, width = 1280,
           height = 720;
  unsigned starting_weapon{};
  // Explicit profile selections; team play requires one0..7 entry per user.
  // An empty span in FFA retains source player identities independently.
  std::vector<std::int8_t> selected_teams;
  std::vector<SourcePlayerConfiguration> selected_profiles;
  // Retained actual SDL controller identities in local-view order. -1 is KBM.
  // Empty retains the standalone command-line controller assignment.
  std::vector<std::int32_t> input_instances;
  std::string join;
  unsigned capture_players{}; // Finite test captures wait for actual joined participants.
  std::filesystem::path input_script;
  // Verification path using the unchanged full-copy BSP scene. Both paths
  // retain the same source selection, gameplay, camera and GPU programs.
  bool source_bsp_copy_reference{};
  // Paired finite capture shares actual cameras, time, vertices, materials and
  // view banks with a second draw on the same full-copy-capable renderer.
  std::filesystem::path bsp_reference_screenshot;
  std::function<void(std::uint32_t,unsigned,const SourceBspShaderView &,
      std::span<const SourceBspShaderPart>)> bsp_selection_observer;
  // Actual selected BSP parts and final material bank at GPU submission.
  // Read-only: the observer never supplies a material, visibility or LOD.
  std::function<void(unsigned,std::uint32_t,const SourceBspResidentViewPlan&,
      std::span<const Material>)> bsp_submission_observer;
  // Capture-only final per-view material upload. Zero/empty leaves ordinary
  // sessions unobserved; the observer cannot supply completed material data.
  unsigned completed_material_upload_tick{};
  std::function<void(unsigned,std::uint32_t,const RendererCompletedMaterialUpload&)>
      completed_material_upload_observer;
  unsigned completed_bsp_draw_tick{};
  std::function<void(unsigned,std::uint32_t,const RendererObservedBspDraw&)> completed_bsp_draw_observer;
  // Diagnostic observation after real input delivery. It never supplies or
  // changes controls; normal interactive sessions leave it unset.
  std::function<void(std::uint32_t, std::span<const Controls>,
                     const MultiplayerState &)> input_observer;
  // After a complete accepted replica update, before rendering. The owning
  // receipt identifies this exact body, including same-tick ownership refreshes.
  // Read-only diagnostics; false means no new game tick was advanced locally.
  std::function<void(const MultiplayerState &,const MultiplayerSnapshotOrigin &,bool)>
      replica_observer;
  // Read-only CPU effect barrier/history observation after each accepted
  // replica receipt. This does not supply an admission, cut or readiness vote.
  std::function<void(const MultiplayerSnapshotOrigin &,const EffectPresentationObservation &)>
      effect_history_observer;
  // Read-only observation after live machine owners update the BSP portal
  // mask. This never supplies door state or changes visibility decisions.
  std::function<void(std::uint32_t,const SourcePortalMaskState&)> world_portal_observer;
  // Observe the actual camera submitted for a local view; this is never an
  // alternate camera/input provider. Used by rendered scope verification.
  std::function<void(unsigned,const Camera &,float,std::int16_t)> view_observer;
  // Actual submitted world rectangle in the full 640x480 UI surface.
  // Read-only verification of local-player layout and live setting changes.
  std::function<void(unsigned,const UiRect &)> view_rectangle_observer;
  // Presentation snapshot and submitted camera in the same render frame.
  // Continuous fields use the render fraction; discrete state and original
  // source pose banks retain the current authoritative snapshot. Network
  // receive can advance between fixed-rate input observations and rendering.
  std::function<void(unsigned,const MultiplayerState &,const Camera &)> view_state_observer;
  // Read-only observation of actual per-view roster output submitted as UI.
  std::function<void(unsigned,bool,const FullScoreboardLayout&,
      const FullScoreboardPaintResult&)> scoreboard_observer;
  // Actual posed vertices submitted for drawing, never an animation provider.
  std::function<void(unsigned,unsigned,const AnimationRenderObservation&)> animation_observer;
  // Read-only measured host timings. renderer_ms includes GPU/fence/present
  // waits and is not a claim of CPU compute or measured GPU execution time.
  std::function<void(const MultiplayerFrameTiming&)> frame_observer;
  // Actual per-player body material after its original emblem bindings and
  // animation, before GPU upload. Observes only roots animated this frame.
  std::function<void(unsigned,const Material&)> player_armor_observer;
  // Accepted posed shield geometry: view, owner, FP, authored shader and render
  // model. This observes admission and cannot force damage or a visible layer.
  std::function<void(unsigned,unsigned,bool,std::uint32_t,std::uint32_t,
      std::span<const Vertex>)> shield_layer_observer;
  std::function<void(unsigned,std::uint32_t,unsigned,unsigned,bool,float,unsigned)> camouflage_layer_observer;
  // Read-only carried weapon material and its actual per-owner shader inputs,
  // after animation and before GPU upload. Never supplies a shader value.
  std::function<void(std::uint32_t,std::uint32_t,const Material&,
      const ShaderInput&)> weapon_material_observer;
  std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,float)> world_weapon_shader_input_observer;
  // Submitted loose-weapon nodes/vertices and the retained original main-view
  // selection. Observes the accepted draw without providing pose or admission.
  std::function<void(unsigned,unsigned,std::uint32_t,std::uint32_t,
      std::span<const Transform>,std::span<const Vertex>,
      std::span<const SourceLiveModelBaseDrawTu15>)> world_weapon_source_draw_observer;
  std::function<void(unsigned,std::uint32_t,std::uint32_t,const ObjectLightingState&,
      bool,unsigned,std::span<const Vertex>)> vehicle_draw_observer;
  // Read-only geometric evidence using actual posed WORLD triangles and the
  // source c0..6 bank submitted to this view. Does not prove surviving pixels.
  std::function<void(unsigned,std::uint32_t,std::span<const Vertex>,
      const std::array<ShaderVector,7>&,Vec2)> vehicle_projection_observer;
  // Completed ordinary BLOC source base geometry and its actual light cache.
  std::function<void(unsigned,std::uint32_t,std::uint32_t,const ObjectLightingState&,
      bool,unsigned,std::span<const Vertex>)> prop_source_draw_observer;
  // Submitted ordinary source-selected biped/corpse/held-weapon base geometry.
  // The observer borrows this view's actual final vertices only during the call.
  std::function<void(unsigned,std::uint32_t,std::uint32_t,const ObjectLightingState&,
      bool,unsigned,std::span<const Vertex>)> unit_source_draw_observer;
  // Read-only actual TU1.5 shadow candidate/history entry/return boundary.
  std::function<void(bool,unsigned,std::uint32_t,const SourcePhysicsWorldRuntimeAbi&,
      const SourceRuntimeMemory&)> shadow_selection_observer;
  // Synchronous completed-root handoff while actual palettes/contexts are
  // current. The later frame observer cannot safely borrow those live banks.
  std::function<void(unsigned,std::uint32_t,PhysicsLiveWorldTu15&,
      const SourceLiveShadowRootTu15&)> shadow_root_completed;
  std::function<void(unsigned,std::uint32_t,std::span<const SourceLiveShadowRootTu15>)> shadow_root_frame_observer;
  std::function<void(bool,unsigned,std::uint32_t,std::uint32_t,std::uint32_t,
      const SourcePhysicsWorldRuntimeAbi&,const SourceRuntimeMemory&)> shadow_admission_observer;
  std::function<void(unsigned,std::uint32_t,const PhysicsLiveWorldTu15&)> shadow_synchronization_observer;
  // Capture label; actual model-light native seed/40E80 ownership is unconditional.
  bool diagnostic_native_light_seed=false;
  std::function<void(unsigned,std::uint32_t,const SourceEffectLightCommandObservationTu15&)> effect_light_command_observer;
  std::function<void(unsigned,std::uint32_t,std::uint32_t,std::uint32_t,
                     std::span<const Vertex>)> weapon_projectile_draw_observer;
  // Actual recorded MGS2 GPU commands and their per-view projected inputs.
  std::function<void(unsigned,std::uint32_t,const SourceLightVolumeDraw &)> weapon_light_volume_observer;
  std::function<void(unsigned,std::uint32_t,const SourceContrailDraw &)> weapon_contrail_observer;
  // Application session owner runs after an authoritative world update and
  // returns true only when its real lifecycle has requested world teardown.
  // This is a control boundary, not an observer or a game-over shortcut.
  // Unbound standalone/network clients retain their existing termination.
  std::function<bool(const MultiplayerMatch&)> session_world_update;
  // Runs only after the actual local roster has been inserted into the real
  // source player pool. The application can bind its controller to these
  // returned slots; names and assumed slot zero are not identity joins.
  std::function<void(const MultiplayerMatch&,std::span<const unsigned>)> session_world_loaded;
  // Reached actual per-view HUD composition, after native geometry/GPU
  // initialization and before the scoreboard. Returns source-packed ARGB.
  std::function<std::optional<std::uint32_t>(unsigned)> session_view_black;
  // Separately owned PC announcer cache/device: consume actual host volume,
  // source class gains and completed packets without sharing Xbox datums.
  std::function<void(float,const std::array<float,sound_class_count>&)> session_audio_update;
  // Actual authored sound selections accepted by the real native SDL queue.
  // Observation supplies no event, clip, seed, voice or admission result.
  std::function<void(const MultiplayerAudioSubmission&)> audio_observer;
  // Read-only actual per-view particle submissions; never supplies an effect.
  std::function<void(unsigned,std::uint32_t,std::span<const ParticleDraw>)> effect_observer;
  std::function<void(bool,const SourcePhysicsContactRuntimeAbi&,const SourceRuntimeMemory&,
      std::span<const std::uint32_t>,std::span<const SourceFirstPersonCpuInputTu15>)> source_particle_phase_observer;
  // Completed source packets and their actual delivery provenance, before
  // diagnostic pixel controls. A true result requests capture of this frame
  // using the runtime-owned options; it cannot change simulation/light state.
  std::function<bool(unsigned,std::uint32_t,std::span<const EffectLightRawDraw>,
                     std::span<const WorldEffectLight>)> effect_light_capture_request;
  // Actual owner-separated world geometry, after source model/LOD/lighting
  // selection. Observation cannot create or change a projectile.
  std::function<void(unsigned,std::uint32_t,std::uint32_t,std::uint32_t,
                     std::span<const Vertex>)> grenade_draw_observer;
  // Read-only completed scene adoption before per-view model interpolation.
  std::function<void(const MultiplayerState &,const SceneObjects &)> source_scene_observer;
  // Actual per-view prop geometry submission; no lifetime/pose is supplied.
  std::function<void(unsigned,std::uint32_t,std::uint32_t,std::span<const Vertex>)> prop_draw_observer;
  std::function<void(const MultiplayerState &,const SourceDynamicObjectScene &)> source_dynamic_observer;
  std::function<void(unsigned,std::uint32_t,std::uint32_t,std::span<const Vertex>)> dynamic_draw_observer;
  unsigned capture_tick{}; // Exact local gameplay capture; no wall-clock substitution.
  // Explicit bounded reference submission only. Zero leaves live shadowGPU
  // disabled. The only admitted case is capture tick61,640x480,singleview.
  unsigned reference_live_shadow_gpu_tick{};
  std::function<void(unsigned,std::uint32_t,bool,const SourceShadowRawPlanStats&)>
      reference_live_shadow_gpu_observer;
  std::optional<Camera> capture_camera;
  // Finite pose-review capture only: show the current local source body and
  // held weapon from capture_camera, suppressing its first-person models/HUD.
  bool capture_third_person{};
  // Captured camera+28/+2C, radians and lighting/grid distance factor. This
  // diagnostic accompanies capture_camera and never supplies gameplay state.
  std::optional<Vec2> capture_projection;
  std::optional<std::array<Vec3,2>> capture_root; // Captured source origin and physics velocity.
  std::filesystem::path reference_player; // Bounded captured original-frame diagnostic inputs.
  std::optional<float> reference_render_fraction; // Explicit diagnostic sampling fraction, not a source clock.
  // Explicit controlled inputs for isolated finite input-script captures.
  // They do not represent an original startup RNG or presentation history.
  std::optional<std::uint32_t> reference_auxiliary_seed;
  bool reference_tick_cadence{};
  UiVariant variant{"Slayer", 0, {}};
  bool dedicated{}, hidden{}, validation{}, mute{}, replay{};
  // Unbound ordinary paths retain eager spawn. The local Vista owner supplies
  // actual initialized options/identity and both callbacks as one policy.
  bool defer_initial_player_spawn{};
  std::function<void(MultiplayerPlayer&,unsigned)> player_birth_owner;
  std::function<void(MultiplayerMatch&)> pre_player_update_owner;
};
int run_multiplayer(const MultiplayerOptions &);
} // namespace halo2
