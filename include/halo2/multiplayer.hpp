#pragma once
#include "halo2/source_effect_random.hpp"
#include "halo2/frontend.hpp"
#include "halo2/simulation.hpp"
#include "halo2/datum_pool.hpp"
#include "halo2/fp_registration.hpp"
#include "halo2/fp_replication.hpp"
#include "halo2/renderer_timing.hpp"
#include "halo2/source_netgame_weapon.hpp"
#include "halo2/source_game_end.hpp"
#include "halo2/source_game_over.hpp"
#include "halo2/source_result_statistics.hpp"
#include "halo2/source_result_capture.hpp"
#include "halo2/source_player_lives.hpp"
#include "halo2/source_round_lifecycle.hpp"
#include "halo2/source_result_event.hpp"
#include "halo2/source_king.hpp"
#include "halo2/source_world_pose.hpp"
namespace halo2 {
struct FullScoreboardLayout;
struct FullScoreboardPaintResult;
struct AnimationRenderObservation;
struct MultiplayerAudioSubmission;
struct ParticleDraw;
struct SourceBspShaderView;
struct SourceBspShaderPart;
struct SourcePortalMaskState;
struct ShadowCasterSphere;
struct MultiplayerFrameTiming {
  struct View {double camera_lighting_ms{},static_world_ms{},objects_ms{},body_ms{},first_person_ms{},hud_ms{};};
  unsigned frame{},views{},bodies_drawn{},body_pose_builds{};
  double input_ms{},common_ms{},renderer_ms{},wall_ms{};
  bool submitted{};
  std::array<View,4> view;
  RendererFrameTiming renderer;
};
constexpr unsigned multiplayer_capacity = 16;
struct SlayerRules {
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
  int primary_light_vehicle{}; // Original parameter76, variant+CD/options+209.
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
// Bounded native binding entry point. Normal menu admission remains separate
// until the actual King presentation/traits/replication consumers are installed.
SlayerRules king_rules(const UiVariant &, bool team_session = false);
void validate_slayer_map_options(const LevelContent &,const SlayerRules &);
int slayer_parameter_default(unsigned);
bool slayer_spawn(const Spawn &, std::size_t bsp, bool initial);
// Retail14F040: appearance0/1 selects multiplayer representation2/3.
unsigned multiplayer_representation(const std::optional<SourcePlayerConfiguration> &);
const BipedDefinition &multiplayer_biped(const LevelContent &,unsigned representation);
struct MultiplayerPlayer {
  std::uint32_t source_datum = null_datum;
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
  std::int16_t source_lives_remaining=-1; // Original signed player+1AC.
  std::int16_t source_king_inside_ticks{}; // Original King player+1B8.
  std::uint8_t source_strength{}; // Sanitized configuration+7D -> player+C1.
  std::int32_t source_field_194{}; // Birth0, successful spawn tick rate, pre-world countdown.
  std::array<std::int16_t,2> source_powerup_timers{}; // Original player+198/+19A.
};
struct NativeWeaponEffectEvent {
  std::uint32_t serial{},tick{},unit=null_datum,weapon=null_datum,effect=null_datum,a_bits{},b_bits{};
};
struct WorldWeaponPresentation {
  std::uint32_t handle=null_datum,definition=null_datum;
  Vec3 position,forward{1,0,0},up{0,0,1};
  int loaded{},reserve{};
  SourceWeaponShaderState shader;
};
struct WorldVehiclePresentation {std::uint32_t handle=null_datum;std::uint32_t placement{};Transform transform;};
struct MultiplayerState {
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
    std::int16_t source_membership = -1;
    std::uint8_t source_ffa_identity{};
    UnitZoomState source_zoom;
    std::int16_t source_zoom_magazine_mode{};
    std::uint32_t source_parent=null_datum, source_action_word{};
    NativeActionPrompt action_prompt;
    // Authoritative current unit fade values; clients do not simulate timers.
    SourceCamouflageAmounts source_camouflage;
    std::uint8_t source_unit_10a{};
    std::uint32_t zoom_reset_serial{};
    std::uint16_t source_hud_zoom_mask = 0x40;
    // Unit+14A bits0/5 are primary/secondary trigger holds (C6C50), read by
    // 1A20E6. Grenade visibility uses the retained component+0C throw phase.
    std::uint8_t motion_sensor_control{};
    FirstPersonReplicatedClock first_person_clock;
    NativeWeaponEffectEvent weapon_effect;
    std::uint32_t source_previous_unit=null_datum, source_death_tick=null_datum;
    struct SecondaryWeapon {
      std::uint32_t definition=null_datum,datum=null_datum,shot_tick{},reload_tick{};
      int loaded{},reserve{};
      float reload_seconds{};
      bool reload_empty{};
      FirstPersonReplicatedClock clock;
      SourceWeaponShaderState shader;
      int ammunition_available{}; // Authoritative104CF0 inventory read for FP reload pose.
    } secondary_weapon;
    DamageFeedbackState damage_feedback;
    SourceWeaponShaderState weapon_shader;
    int ammunition_available{};
  };
  std::uint32_t tick{};
  std::int32_t source_engine=2;
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
  SourceDeviceWorldSnapshot devices; // Complete authoritative source state over native LAN.
  std::vector<SourceDeviceWorldEvent> device_events;
  std::vector<RumbleJournalEvent> rumble_events;
  std::vector<NativeGrenadeProjectile> grenade_projectiles;
  std::vector<NativeGrenadeDetonation> grenade_detonations;
  std::vector<BulletImpact> bullet_impacts;
  std::vector<UnitDamageEffectEvent> unit_damage_effects;
  std::vector<WorldWeaponPresentation> world_weapons;
  std::vector<WorldVehiclePresentation> world_vehicles;
  std::vector<NativeWeaponProjectile> weapon_projectiles;
  bool source_body_poses_authoritative{};
  std::vector<SourceWorldPose> source_body_poses;
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
public:
  MultiplayerMatch(const LevelContent &, std::size_t, std::span<const Spawn>,
                   SlayerRules,MultiplayerKingServices = {},
                   std::function<std::uint32_t()> result_wall_clock = {},
                   MultiplayerObjectRuntime *source_object_world = nullptr);
  MultiplayerMatch(const MultiplayerMatch &) = delete;
  MultiplayerMatch &operator=(const MultiplayerMatch &) = delete;
  MultiplayerMatch(MultiplayerMatch &&) = delete;
  MultiplayerMatch &operator=(MultiplayerMatch &&) = delete;
  unsigned join(std::string name, std::int8_t selected_team = -1,
      std::optional<SourcePlayerConfiguration> = std::nullopt,
      std::optional<unsigned> admitted_slot = std::nullopt);
  void validate_player_configuration(const SourcePlayerConfiguration&,std::int8_t selected_team)const;
  bool team_mode() const { return rules_.team_mode; }
  std::int32_t source_engine() const { return rules_.source_engine; }
  const SourceKingState* source_king_state()const {
    return rules_.source_engine==4 ? &king_state_ : nullptr;
  }
  unsigned available_player_slots() const;
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
  void release_presentation_owners() noexcept;
  // Declared original effect service boundary. A live owner must resolve
  // actual attachment/PVS/event/particle state; a shot counter is insufficient.
  void configure_weapon_effect_owner(
      std::function<std::uint32_t(const Unit &,const WeaponEffectInput &)>);
  void configure_grenades(std::shared_ptr<const Cache>,NativeGrenadeConstants);
  void configure_melee_animation(std::shared_ptr<const Cache>,NativeMeleeConstants);
  std::size_t native_object_capacity()const{return std::size_t(simulation_.object_lifetimes().pool().capacity());}
  std::span<const NativeGrenadeProjectile> grenade_projectiles()const {
    return simulation_.grenade_sandbox_?simulation_.grenade_sandbox_->projectiles():std::span<const NativeGrenadeProjectile>{};
  }
  std::vector<NativeGrenadeEvent> take_grenade_events(){return simulation_.grenade_sandbox_?simulation_.grenade_sandbox_->take_events():std::vector<NativeGrenadeEvent>{};}
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
  bool source_object_world_bound() const {return source_object_world_!=nullptr;}
  std::vector<Transform> source_world_nodes(std::uint32_t object) const;
  ShadowCasterSphere source_object_sphere(std::uint32_t object) const;
  // Only an admitted game-tick effect owner may bind this deterministic word.
  // Render-batched visual playback must not consume gameplay RNG here.
  SourceEffectRandom source_effect_random(std::uint32_t &auxiliary) {
    return {simulation_.source_primary_random(),auxiliary};
  }
  void advance_weather_view(double realtime,std::int16_t view_slot) {
    simulation_.advance_weather_view(realtime,view_slot);
  }
  void debug_settings(DebugGameSettings settings) { simulation_.debug_settings(std::move(settings)); }
  void debug_request(const DebugGameRequest& request,const Spawn* spawn=nullptr) {
    if(request.player>=players_.size() || !players_[request.player].connected)
      throw FormatError("debug player is no longer connected");
    if(request.kind==DebugGameRequest::Heal)simulation_.debug_heal(request.player);
    else if(request.kind==DebugGameRequest::Teleport && spawn)simulation_.debug_teleport(request.player,*spawn);
    else throw FormatError("this debug request needs a campaign state owner");
  }
  void advance_audio(float seconds){simulation_.advance_audio(seconds);}
  std::vector<SoundEvent> take_sounds() { return simulation_.take_sounds(); }
  std::vector<PropSandboxEvent> take_prop_events() { return simulation_.take_prop_events(); }
  void replicate_prop_removals(std::span<const PropRemoval> rows) { simulation_.replicate_prop_removals(rows); }
  void replicate_devices(const SourceDeviceWorldSnapshot &s){simulation_.replicate_devices(s);}
  SourceControlActivation activate_admitted_control(std::uint32_t h){return simulation_.activate_admitted_control(h);}

private:
  std::vector<RumbleJournalEvent> rumble_events_;
  std::uint64_t rumble_serial_{};
  const LevelContent &content_;
  MultiplayerObjectRuntime *source_object_world_{};
  Simulation simulation_;
  std::vector<Spawn> spawns_;
  SlayerRules rules_;
  std::vector<SourceKingMarker> king_markers_;
  SourceKingState king_state_;
  SourceKingHills king_hills_;
  MultiplayerKingServices king_services_;
  std::array<MultiplayerPlayer, multiplayer_capacity> players_;
  std::array<Controls,multiplayer_capacity> native_presentation_controls_;
  DatumPool player_pool_{"players", multiplayer_capacity, 0x21c};
  std::map<std::uint32_t, Unit> retained_units_;
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
  std::uint32_t weapon_effect_serial_{};
  std::function<std::uint32_t(const Unit&,const WeaponEffectInput&)> weapon_effect_owner_;
  bool finished_{};
  bool results_finalized_{};
  SourceGameEndStats end_stats_;
  SourceResultStatistics retained_stats_;
  SourceResultRecording retained_recording_;
  SourceResultEventState result_events_;
  bool result_events_started_{};
  std::function<std::uint32_t()> result_wall_clock_;
  void record_result_event(std::uint32_t player,std::uint32_t first,std::uint32_t second);
  SourceRoundLifecycle round_lifecycle_;
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
  bool spawn(unsigned, bool initial);
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
  bool debug_panel{};
  std::shared_ptr<DesktopSession> desktop;
  // Retained frontend music device/state; once per actual world presentation
  // update, independent of the optional source lifecycle/announcer adapters.
  std::function<void()> menu_audio_update;
  std::filesystem::path map = "assets/disc/maps/lockout.map", screenshot, shared_maps;
  std::filesystem::path gameplay_xbe; // Explicit user-owned update data source.
  // Development integration of the selected original world into actual Match
  // allocation/spawn/rendering. Missing live owners stop explicitly.
  bool source_object_world{};
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
  // Diagnostic observation after real input delivery. It never supplies or
  // changes controls; normal interactive sessions leave it unset.
  std::function<void(std::uint32_t, std::span<const Controls>,
                     const MultiplayerState &)> input_observer;
  // Read-only observation after live machine owners update the BSP portal
  // mask. This never supplies door state or changes visibility decisions.
  std::function<void(std::uint32_t,const SourcePortalMaskState&)> world_portal_observer;
  // Observe the actual camera submitted for a local view; this is never an
  // alternate camera/input provider. Used by rendered scope verification.
  std::function<void(unsigned,const Camera &,float,std::int16_t)> view_observer;
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
  // Read-only carried weapon material and its actual per-owner shader inputs,
  // after animation and before GPU upload. Never supplies a shader value.
  std::function<void(std::uint32_t,std::uint32_t,const Material&,
      const ShaderInput&)> weapon_material_observer;
  std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,float)> world_weapon_shader_input_observer;
  std::function<void(unsigned,std::uint32_t,std::uint32_t,std::uint32_t,
                     std::span<const Vertex>)> weapon_projectile_draw_observer;
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
  // Actual owner-separated world geometry, after source model/LOD/lighting
  // selection. Observation cannot create or change a projectile.
  std::function<void(unsigned,std::uint32_t,std::uint32_t,std::uint32_t,
                     std::span<const Vertex>)> grenade_draw_observer;
  unsigned capture_tick{}; // Exact local gameplay capture; no wall-clock substitution.
  std::optional<Camera> capture_camera;
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
