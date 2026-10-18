#pragma once
#include "halo2/inventory.hpp"
#include "halo2/player_training.hpp"
#include "halo2/native_action_prompt.hpp"
#include "halo2/rumble.hpp"
#include "halo2/native_weapon_projectile.hpp"
#include "halo2/melee_impact.hpp"
#include "halo2/weapon_effect.hpp"
#include "halo2/collision.hpp"
#include "halo2/kernels.hpp"
#include "halo2/navigation.hpp"
#include "halo2/scene.hpp"
#include "halo2/cinematic_animation.hpp"
#include "halo2/campaign_frame_clock.hpp"
#include "halo2/source_unit_interaction_target.hpp"
#include "halo2/source_world_pose.hpp"
#include "halo2/scripts.hpp"
#include "halo2/source_script_lists.hpp"
#include "halo2/sound_classes.hpp"
#include "halo2/damage_history.hpp"
#include "halo2/unit_damage.hpp"
#include "halo2/damage_impact.hpp"
#include "halo2/object_garbage.hpp"
#include "halo2/source_weather.hpp"
#include "halo2/owner_counter.hpp"
#include "halo2/debug_game.hpp"
#include "halo2/prop_sandbox.hpp"
#include "halo2/static_scenery_collision.hpp"
#include "halo2/native_grenade.hpp"
#include "halo2/bullet_impact.hpp"
#include "halo2/player_action_edges.hpp"
#include "halo2/source_device_world.hpp"
#include "halo2/source_camouflage.hpp"
#include "halo2/source_object_body_storage.hpp"
#include "halo2/source_weapon_shader_input.hpp"
#include "halo2/native_weapon_projectile.hpp"
#include <memory>
namespace halo2 {
class CampaignSourceRuntime;
constexpr float tick_seconds = 1.f / 30.f;
constexpr float retail_gravity =
    4.1712594f; // XBE 0x4455D8, initialized by 0x1EB550.
struct Controls {
  float forward{}, right{}, yaw{}, pitch{};
  bool jump{}, crouch{}, fire{}, reload{};
  bool skip_cinematic{};
  std::int16_t zoom = -1; // Retained source local-view selector, command+24.
  // Native authority acknowledgement, not an original Xbox command field.
  std::uint32_t zoom_reset_serial{};
  // Native tick pulse. Delivery serials preserve short taps over UDP.
  bool grenade{};
  bool pickup{}, dual_wield{}, switch_weapon{}, fire_secondary{},switch_grenade{};
  std::uint16_t pickup_milliseconds{}, dual_wield_milliseconds{};
  bool accept{},cancel{},melee{},flashlight{},start{},back{};
  float look_stick{};
  // Delivered TU1.5 command+28/+2C vehicle intent. An engaged neutral target
  // means the sender selected no action; absence retains other input owners.
  std::optional<SourcePlayerActionTarget> vehicle_interaction_target;
};
struct WeaponState {
  std::uint32_t definition = null_datum;
  int loaded{}, reserve{}, burst_left{};
  float delay{}, reload{};
  std::uint64_t last_shot{}, reload_started{};
  bool reload_empty{};
  bool trigger_armed{}; // Native single-trigger latch; original trigger+0 bit0.
  SmgState smg;
  std::uint32_t source_datum = null_datum;
  // Zeroed allocation/FDAD0 fields plus the recovered native attachment and
  // ready consumers. Full raw item/power lifetime remains separate.
  SourceWeaponShaderState shader;
  std::int16_t ready_countdown{}; // Original weapon+19E.
  bool ready_clock{}; // Native ownership of the recovered ready fragment.
  float cooling_inhibitor{}; // Original weapon+188; heat lives in shader+180.
  std::uint32_t overheat_handle=null_datum; // Actual owning effect result, +244.
  std::int16_t energy_blocking_ticks{}; // Original weapon+17C.
  std::int16_t firing_effect_index{},firing_effect_remaining{};
  std::uint16_t firing_effect_visited{};
  WeaponMagazineState magazine;
  SmgBarrelState alternate_barrel;
  WeaponTriggerInputState trigger_input; // Original weapon+170..174.
  std::uint8_t charge_fraction{}; // Literal weapon+175 accumulator.
  WeaponTriggerTarget trigger_target;
  // Read-only publication of the actual fixed TU1.5 runtime records. These
  // records never supply a second weapon update or synthesized ammunition.
  std::array<SmgBarrelState,2> source_barrels;
  std::array<WeaponTriggerState,2> source_triggers;
  std::array<WeaponMagazineState,2> source_magazines;
  bool source_runtime_published{};
};
struct NativeWeaponPickup {
  std::uint32_t handle=null_datum, definition=null_datum, unlock_tick{};
  Vec3 position,forward{1,0,0},up{0,0,1};
  std::optional<WeaponState> weapon;
  bool visible=true,scene_owned{};
  int bsp=-1;
};
struct SourceInventory {
  // C52C0 initializes the four full datums and both hand selectors to NONE.
  std::array<std::uint32_t, 4> slots{
      null_datum, null_datum, null_datum, null_datum}; // unit+218..224
  std::array<std::int8_t, 2> current{-1, -1}; // unit+212/+213
  std::array<std::int8_t, 2> requested{-1, -1}; // unit+216/+217
  std::uint16_t inventory_sequence{}, replicated_sequence{}; // +210/+214
  std::array<std::uint32_t, 4> ready_ticks{}; // +228..234: timestamps
  // Unit.weapon owns current[0]. Only inactive slots store a WeaponState here;
  // copying the active state would create a second authoritative ammo clock.
  std::array<std::optional<WeaponState>, 4> inactive_weapons;
};
struct Unit {
  std::uint32_t source_datum = null_datum;
  std::uint32_t source_player = null_datum;
  std::uint32_t source_previous_player = null_datum;
  std::int16_t source_membership = -1;
  DamageHistory source_damage_history;
  ObjectGarbageState source_garbage;
  std::uint32_t source_death_tick = null_datum;
  OwnerCounterState source_owner_counter;
  std::uint32_t source_forward_9 = null_datum, source_forward_other = null_datum;
  Body body;
  BipedDefinition definition;
  RechargeState vitality;
  DamageFeedbackState damage_feedback;
  WeaponState weapon;
  SourceInventory source_inventory;
  std::array<std::uint8_t,2> source_weapon_hand_feedback{}; // Unit+1FA/+1FB, shared by all actual weapons.
  SourcePlayerActionEdges source_player_actions;
  UnitZoomState source_zoom;
  // Cold C52C0/BD270 fields. Only verified action owners may mutate them;
  // native Idle/grounded enums never stand in for the original action word.
  std::uint32_t source_parent = null_datum, source_action_word{};
  std::int16_t source_seat=-1;
  struct SeatAction {enum Phase {None,Entering,Seated,Boarding,Exiting} phase=None;std::uint64_t started{};Vec3 entry_position;} seat_action;
  // Genuine zeroed object allocation plus whole C52C0 retain these cold
  // values. Camouflage uses bits3/4 in the existing +134 action word.
  SourceCamouflageAmounts source_camouflage;
  std::uint8_t source_unit_10a{};
  std::uint8_t source_unit_10b{};
  struct Command {
    bool active{},moving=true,looking=true,pathfinding_failsafe{},ignore_obstacles{};
    std::int16_t combat_status=-1;
    std::optional<Vec3> goal,face_point,look_point;
    std::optional<std::uint32_t> goal_reference,face_reference,look_reference;
    std::optional<std::uint32_t> face_object,look_object;
    bool face_player{};
  } command;
  std::shared_ptr<const AnimationClip> custom_animation;
  std::uint64_t custom_animation_tick{};
  bool custom_animation_loop{};
  std::uint32_t emotional_state{};float emotional_weight{};std::int16_t emotional_transition{};
  std::uint32_t zoom_reset_serial{}; // Native delivery of C9A40 view resets.
  std::array<std::uint8_t, 2> grenades{};
  std::int8_t grenade_current = -1, grenade_requested = -1;
  NativeGrenadeAction grenade_action;
  NativeMeleeAction melee_action;
  float yaw{}, crouch{}, awareness{};
  JumpState jump;
  SourceLandingState landing;
  std::uint32_t character = null_datum;
  int target = -2;
  std::vector<Vec3> route;
  std::size_t waypoint{};
  int team{};
  bool visible = true, impervious{}, cannot_take_damage{};
  bool bsp_hidden{};
  bool exists = true;
  int bsp = -1;
  std::size_t squad{};
  bool controlled{};
  float pitch{};
  int last_damager=-2;
  enum State { Idle, Alert, Attack, Dead } state = Idle;
};
struct SoundEvent {
  std::uint32_t datum;
  Vec3 position;
  float gain = 1;
  enum Action {
    Impulse,
    LoopStart,
    LoopStop,
    LoopAlternate,
    ImpulseStop,
    Reset,
    // An HSC NONE object is a centered impulse, independent of the current
    // gameplay/cinematic camera. Keep it distinct through queued delivery.
    NonSpatialImpulse
  } action = Impulse;
  bool alternate{};
  // Full, salted speaker identity from the actual HSC object argument.
  // A position alone cannot bind a voice to its original facial controller.
  std::uint32_t owner = null_datum;
};
class Simulation {
public:
  using BspLoader = std::function<BspContent(std::size_t)>;
  struct Prediction {
    enum Kind { Sound, Model, Structure, Barrier, Object } kind;
    std::uint32_t datum{};
    int section{};
    bool instances{};
  };
  using Predictor = std::function<void(const Prediction &)>;
  using ScriptTickRate = std::function<std::int16_t()>;
  using ScriptSetMaxRumble = std::function<void(float,float)>;
  Simulation(const LevelContent &, std::size_t bsp, Spawn,
             Presentation::AnimationLoader = {}, BspLoader = {}, Predictor = {},
             std::optional<SourceSceneGameContext> = {},
             std::optional<NativeObjectLifetimes> = {},
             SourceSceneObjectOwner = {},std::shared_ptr<CampaignSourceRuntime> = {});
  Simulation(const LevelContent &, std::size_t bsp, Spawn,
             Presentation::AnimationLoader, BspLoader, Predictor,
             std::optional<SourceSceneGameContext>,
             std::optional<NativeObjectLifetimes>, SourceSceneObjectOwner,
             std::shared_ptr<CampaignSourceRuntime>,ScriptTickRate);
  Simulation(const LevelContent &, std::size_t bsp, Spawn,
             Presentation::AnimationLoader, BspLoader, Predictor,
             std::optional<SourceSceneGameContext>,
             std::optional<NativeObjectLifetimes>, SourceSceneObjectOwner,
             std::shared_ptr<CampaignSourceRuntime>,ScriptTickRate,ScriptSetMaxRumble);
  // Transfer an already loaded BSP without retaining a second collision bank.
  // Metadata and navigation in LevelContent remain available to their owners.
  Simulation(const LevelContent &, BspContent initial_geometry, std::size_t bsp, Spawn,
             Presentation::AnimationLoader = {}, BspLoader = {}, Predictor = {},
             std::optional<SourceSceneGameContext> = {},
             std::optional<NativeObjectLifetimes> = {},
             SourceSceneObjectOwner = {},std::shared_ptr<CampaignSourceRuntime> = {});
  Simulation(const LevelContent &, BspContent initial_geometry, std::size_t bsp, Spawn,
             Presentation::AnimationLoader, BspLoader, Predictor,
             std::optional<SourceSceneGameContext>,
             std::optional<NativeObjectLifetimes>, SourceSceneObjectOwner,
             std::shared_ptr<CampaignSourceRuntime>,ScriptTickRate);
  Simulation(const LevelContent &, BspContent initial_geometry, std::size_t bsp, Spawn,
             Presentation::AnimationLoader, BspLoader, Predictor,
             std::optional<SourceSceneGameContext>,
             std::optional<NativeObjectLifetimes>, SourceSceneObjectOwner,
             std::shared_ptr<CampaignSourceRuntime>,ScriptTickRate,ScriptSetMaxRumble);
  Simulation(const Simulation &)=delete;
  Simulation &operator=(const Simulation &)=delete;
  Simulation(Simulation &&)=delete;
  Simulation &operator=(Simulation &&)=delete;
  void step(const Controls &);
  void step_players(std::span<const Controls>);
  NativeActionPrompt control_action_prompt()const;
  NativeActionPrompt action_prompt()const;
  NativeActionPrompt unit_action_prompt(const Unit &,NativeActionPrompt current={},Vec3 current_root={})const;
  bool take_unit_action(Unit &,const NativeActionPrompt &);
  void step_unit_interactions(const Controls &);
  void configure_vehicle_rows(int engine,int timer_option=0){NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;vehicle_engine_=engine;vehicle_timer_option_=timer_option;}
  void configure_vehicle_allegiance(std::function<bool(const Unit&,const Unit&)> fn){NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;vehicle_allied_=std::move(fn);}
  std::span<const NativeWeaponPickup> weapon_pickups()const{return campaign_weapon_pickups_;}
  void respawn(Spawn);
  void equip(std::uint32_t);
  std::size_t place_squad(const std::string &);
  void run_script(const std::string &name) {NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;scripts_->start(name);}
  void start_level() {NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;scripts_->start_level();}
  bool level_finished()const{return level_finished_;}
  bool reset_requested()const{return reset_requested_;}
  bool has_checkpoint() const { return bool(checkpoint_); }
  bool cinematic_skippable() const {
    return cinematic_skip_enabled_ && bool(checkpoint_);
  }
  void request_checkpoint();
  void request_revert();
  std::uint64_t checkpoint_restores() const { return checkpoint_restores_; }
  void advance_audio(float seconds) {NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;sound_classes_.advance(seconds);}
  const SoundClassFades &sound_classes() const { return sound_classes_; }
  const std::array<int,5> &objectives() const { return objectives_; }
  bool flashlights_allowed() const { return flashlights_allowed_; }
  std::uint64_t flashlights_changed_tick() const { return flashlights_changed_tick_; }
  bool ambience_details_enabled() const { return ambience_details_enabled_; }
  void set_difficulty(unsigned value);
  void preview_camera(std::uint32_t graph, std::uint32_t name, int flag) {
    NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
    presentation_.camera_animation(graph, name, flag);
    presentation_.scripted = presentation_.cinematic = true;
    presentation_.field_of_view(60, 0);
    presentation_.letterbox(true, true);
  }
  bool allied(int a,int b)const{return a>=0 && a<16 && b>=0 && b<16 && peace_[a][b];}
  void set_team_allegiance(int,int,bool);
  const Unit &player() const { return player_; }
  CampaignSourceRuntime *campaign_source()const{return source_campaign_.get();}
  float player_pitch() const { return player_pitch_; }
  const std::vector<Unit> &actors() const { return actors_; }
  const CollisionWorld &collision() const;
  std::size_t bsp() const { return bsp_; }
  std::uint64_t bsp_revision() const { return bsp_revision_; }
  void request_bsp(int);
  const ScriptVm &scripts() const { return *scripts_; }
  const SceneObjects &scene() const { return scene_; }
  Vec3 object_center(std::uint32_t)const;
  const SourceDeviceWorld &devices()const{return devices_;}
  std::span<const SourceDeviceWorldEvent> device_events()const{return device_events_;}
  void replicate_devices(const SourceDeviceWorldSnapshot &);
  void replicate_source_scene(std::span<const SourceSceneRoot>,std::span<const SourceWorldPose>);
  // Original action dispatch calls this only after target/action admission.
  SourceControlActivation activate_admitted_control(std::uint32_t h){NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;return devices_.activate_admitted_control(h);}
  const NativeObjectLifetimes &object_lifetimes() const {return object_lifetimes_;}
  const SourceObjectBodyStorage &source_object_bodies() const {return object_bodies_;}
  // Raw construction storage, never a completed native/source game object.
  // Complete constructor/teardown services remain the caller's responsibility.
  std::uint32_t allocate_source_body_storage(std::int16_t extent) {
    NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
    return object_bodies_.allocate(object_lifetimes_,extent);}
  bool append_source_body_storage(std::uint32_t h,std::int16_t field,std::int16_t bytes,std::uint8_t alignment) {
    NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
    return object_bodies_.append(object_lifetimes_,h,field,bytes,alignment);}
  std::span<std::byte> source_body_storage(std::uint32_t h) {NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;return object_bodies_.body(object_lifetimes_,h);}
  void release_source_body_storage(std::uint32_t h) {NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;object_bodies_.release(object_lifetimes_,h);}
  const PropSandbox& props()const{return props_;}
  std::vector<PropSandboxEvent> take_prop_events(){NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;auto result=std::move(prop_events_);prop_events_.clear();return result;}
  std::span<const PropRemoval> prop_removals()const{return prop_removals_;}
  std::span<const BulletImpact> bullet_impacts()const{return bullet_impacts_;}
  std::span<const MeleeImpact> melee_impacts()const{return melee_impacts_;}
  std::span<const UnitDamageEffectEvent> unit_damage_effects()const{return unit_damage_effects_;}
  std::span<const UnitDamageVoiceEvent> unit_damage_voices()const{return unit_damage_voices_;}
  void replicate_prop_removals(std::span<const PropRemoval>);
  void replicate_prop_regions(std::span<const PropRegionSnapshot>);
  // Source spherical damage/visibility consumer composed with the declared
  // host geometry backend. Call only from an admitted authored area event.
  void area_damage(const PropAreaDamageDefinition&,Vec3,const DamageOwner&,int native_owner,
      std::uint32_t ignored_scene_prop=null_datum,std::uint32_t projectile_definition=null_datum);
  const SourceWeatherState &weather() const { return weather_; }
  const SourceWeatherClock &weather_clock() const { return weather_clock_; }
  std::uint32_t &source_primary_random() {NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;return random_state_;}
  // Render-time weather advancement belongs to source view0. Callers supply
  // absolute realtime; game ticks and additional local views do not advance it.
  void advance_weather_view(double realtime,std::int16_t view_slot) {
    NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
    source_weather_view_tick(weather_,weather_clock_,realtime,view_slot,&weather_record_);
  }
  const Presentation &presentation() const { return presentation_; }
  const PlayerTrainingState &training() const {return training_;}
  void training_definitions(std::span<const TrainingEventDefinition> definitions) {
    NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
    training_definitions_.assign(definitions.begin(),definitions.end());
  }
  const CampaignSubtitle &subtitle() const {return subtitle_;}
  const auto &titles() const {return titles_;}
  struct ObjectAnimation {
    std::shared_ptr<const AnimationClip> clip;std::uint64_t tick{};bool loop{};
    std::shared_ptr<CinematicAnimationAssets> assets;
    std::optional<CinematicAnimationChannel> channel;
    std::uint32_t anchor=null_datum;
    // Actual source timing/event packets, retained until this channel retires.
    std::vector<std::array<std::byte,20>> events;
  };
  void cinematic_animation_assets(std::shared_ptr<CinematicAnimationAssets>);
  const CampaignCinematicState *cinematic_state()const;
  CampaignFrameTime advance_frame_time(float seconds,bool paused);
  float frame_fraction()const;
  void cinematic_screen_consumer(bool);
  void advance_cinematic_render_time(float seconds,int requested,int completed);
  // Once per outer source frame, after effects, using the caller delta.
  // Headless callers own the same explicit phase; no render borrow advances it.
  void advance_cinematic_camera(float raw_seconds);
  CinematicCrossfadeFrame cinematic_crossfade_frame()const;
  void cinematic_crossfade_capture_complete();
  struct VehicleFlip {std::uint32_t object;std::uint8_t direction{},ticks{};Vec3 velocity;};
  struct ImpulseClock {std::uint32_t datum=null_datum;std::uint64_t start{},duration{};};
  struct Projectile {
    Vec3 position, velocity;
    float traveled{};
    std::uint32_t weapon{};
    int owner{};
    int bsp = -1;
    std::uint32_t source_datum = null_datum;
    DamageOwner captured_owner;
    Vec3 forward,up;
    std::optional<SourceNeedlerProjectileBirth> source_birth;
    std::int16_t barrel_index=-1;
  };
  using AuthorityDigest=std::array<std::byte,32>;
  struct AuthorityCheckpoint; // Value definition in simulation_authority.hpp.
  class PreparedAuthority {
  public:
    ~PreparedAuthority();
    PreparedAuthority(PreparedAuthority &&) noexcept;
    PreparedAuthority &operator=(PreparedAuthority &&) noexcept;
    bool for_owner(const Simulation &)const noexcept;
  private:
    friend class Simulation;
    PreparedAuthority();
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };
  // Parent admission binds complete immutable map/image/resources/schema and
  // quiesces the match, raw borrowers and callbacks through publication.
  // Source-backed pools require the exact staged complete world allocation.
  // This native simulation transaction is not a complete match or wire save.
  AuthorityCheckpoint authority_checkpoint(AuthorityDigest admitted_binding,bool qualified_source_world=false)const;
  PreparedAuthority prepare_authority(const AuthorityCheckpoint &,AuthorityDigest admitted_binding,
      const SourceRuntimeMemory *replacement_source_world=nullptr);
  bool commit_authority(PreparedAuthority &&) noexcept;
  const ObjectAnimation *object_animation(std::uint32_t handle) const {
    const auto at=object_animations_.find(handle);return at==object_animations_.end()?nullptr:&at->second;
  }
  void cinematic_text_services(std::function<bool(std::uint32_t)> localized,std::function<float(std::uint32_t)> duration) {
    NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
    subtitle_localized_=std::move(localized);sound_duration_=std::move(duration);
  }
  std::vector<RumbleEvent> take_rumble(){NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;auto result=std::move(rumble_events_);rumble_events_.clear();return result;}
  void controller_look_settings(std::function<bool()> get,std::function<void(bool)> set){NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;look_invert_get_=std::move(get);look_invert_set_=std::move(set);}

  Camera camera(const Camera &player) const {
    return presentation_.camera(player);
  }
  std::vector<SoundEvent> take_sounds();
  std::vector<Vertex>
  debug_mesh(std::span<const std::uint32_t> rendered_models = {},
      std::span<const Unit> previous_actors = {},
      std::span<const NativeWeaponProjectile> previous_projectiles = {},float fraction=1) const;
  std::vector<NativeWeaponProjectile> presentation_projectiles() const;
  std::uint64_t ticks() const { return ticks_; }
  std::uint64_t shots() const { return shots_; }
  std::uint64_t hits() const { return hits_; }
  std::string status() const;
  // Explicit optional workbench state, excluded from retail checkpoints.
  void debug_settings(DebugGameSettings value) {NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;debug_settings_=std::move(value);}
  const DebugGameSettings& debug_settings() const { return debug_settings_; }
  const Unit& debug_player(unsigned slot) const;
  void debug_heal(unsigned slot);
  void debug_teleport(unsigned slot,const Spawn&);
  void debug_save_snapshot(const std::string&);
  void debug_load_snapshot(const std::string&);
  void debug_delete_snapshot(const std::string&);
  std::vector<std::string> debug_snapshot_names() const;

private:
  friend class MultiplayerMatch;
  friend class CampaignSourceRuntime;
  mutable NativeAuthorityBoundary authority_calls_;
  std::uint32_t authority_service_roles()const;
  void step_impl(const Controls &, std::span<const Controls>);
  void step_devices();
  void disrupt_camouflage(Unit&,float amount,float limit);
  const LevelContent &content_;
  unsigned difficulty_ = 1;
  std::size_t bsp_;
  struct BspWorld;
  std::shared_ptr<const BspWorld> world_;
  BspLoader bsp_loader_;
  Predictor predictor_;
  std::optional<std::size_t> pending_bsp_;
  std::uint64_t bsp_revision_{};
  void commit_bsp();
  std::shared_ptr<CampaignSourceRuntime> source_campaign_;
  NativeObjectLifetimes object_lifetimes_;
  SourceObjectBodyStorage object_bodies_;
  SceneObjects scene_;
  SourceDeviceWorld devices_;
  std::vector<NativeWeaponPickup> campaign_weapon_pickups_;
  std::vector<VehicleFlip> vehicle_flips_;
  std::vector<NetgameWeaponState> vehicle_rows_;
  int vehicle_engine_=2,vehicle_timer_option_{};
  std::function<bool(const Unit&,const Unit&)> vehicle_allied_;
  void synchronize_weapon_pickups();
  void step_campaign_pickups(const Controls &);
  void equip_inventory(Unit &,std::uint32_t);
  std::vector<SourceDeviceWorldEvent> device_events_;
  std::uint64_t device_event_serial_{};
  PropSandbox props_;
  StaticSceneryCollision scenery_collision_;
  std::shared_ptr<const CollisionWorld> static_collision_;
  std::shared_ptr<const CollisionWorld> prop_collision_;
  std::vector<PropSandboxEvent> prop_events_;
  std::vector<PropRemoval> prop_removals_;
  void synchronize_props(bool force=false);
  Presentation presentation_;
  PlayerTrainingState training_;
  std::vector<TrainingEventDefinition> training_definitions_;
  CampaignSubtitle subtitle_;
  std::array<CampaignTitle,4> titles_;
  std::function<bool(std::uint32_t)> subtitle_localized_;
  std::function<float(std::uint32_t)> sound_duration_;
  Presentation::AnimationLoader animation_loader_;
  std::map<std::uint32_t,ObjectAnimation> object_animations_;
  std::vector<RumbleEvent> rumble_events_;
  bool ai_dialogue_enabled_=true;
  std::array<ImpulseClock,16> impulse_clocks_;
  // Native object-list bank. Source20A620 conversion allocates/appends through
  // these actual owners; interpreter roots retain lists over sleeps/saves.
  std::map<std::uint32_t,std::vector<std::uint32_t>> script_lists_;
  std::optional<SourceHsListBank> multiplayer_script_lists_;
  std::uint32_t next_script_list_=0x80000001u;
  std::function<bool()> look_invert_get_;
  std::function<void(bool)> look_invert_set_;
  std::array<std::array<bool, 16>, 16> peace_{};
  Unit player_;
  std::vector<Unit> actors_;
  std::unique_ptr<ScriptVm> scripts_;
  std::vector<SoundEvent> sounds_;
  std::vector<Projectile> projectiles_;
  std::vector<BulletImpact> bullet_impacts_;
  std::vector<MeleeImpact> melee_impacts_;
  std::uint64_t bullet_impact_serial_{};
  std::vector<UnitDamageEffectEvent> unit_damage_effects_;
  std::vector<UnitDamageVoiceEvent> unit_damage_voices_;
  std::uint64_t unit_damage_voice_serial_{};
  std::uint64_t unit_damage_effect_serial_{};
  void record_unit_damage_effect(const Unit &,const UnitDamageEffect &,const DamageOwner &);
  std::uint64_t ticks_{}, shots_{}, hits_{};
  float player_pitch_{};
  bool player_input_enabled_ = true;
  bool player_movement_disabled_{}, skip_held_{};
  std::map<std::uint32_t, SoundEvent> active_loops_;
  std::string mission_segment_;
  std::uint32_t random_state_ = 0x12345678;
  SourceWeatherState weather_;
  SourceWeatherRecord weather_record_;
  SourceWeatherClock weather_clock_;
  SoundClassFades sound_classes_;
  std::array<int,5> objectives_{};
  bool flashlights_allowed_ = true, ambience_details_enabled_ = true;
  bool suppress_ambience_rebuild_{};
  bool suppress_automatic_objects_{};
  bool multiplayer_weapon_steps_{};
  std::function<SourceVehicleSeatTarget(std::uint32_t,std::uint32_t)> source_vehicle_seat_query_;
  bool source_physics_movement_{};
  ScriptVm::Host source_device_script_;
  std::function<std::uint32_t(std::uint8_t)> allocate_object_;
  std::function<void(std::uint32_t,std::uint32_t)> publish_object_;
  std::function<void(std::uint32_t)> release_object_;
  std::function<void(Unit &, const DamageOwner &, const UnitDamageResult &,
      const WeaponDefinition &,std::optional<DamageImpactContext>,const DamageFeedbackState &)> damage_hook_;
  std::function<float(const DamageOwner &,std::uint32_t)> player_damage_scale_;
  float player_damage_scale(const DamageOwner &,std::uint32_t resolved_victim)const;
  std::function<void(const Unit &,const WeaponState &,std::uint32_t)> first_person_activity_hook_;
  std::function<std::int16_t(const Unit &,const WeaponState &,std::uint32_t,std::int16_t)> first_person_timing_hook_;
  std::function<std::uint32_t(const Unit &,const WeaponEffectInput &)> weapon_effect_hook_;
  std::function<std::uint32_t(const WeaponEffectInput &)> world_weapon_effect_hook_;
  std::shared_ptr<NativeGrenadeSandbox> grenade_sandbox_;
  std::shared_ptr<NativeMeleeAnimation> melee_animation_;
  std::uint64_t flashlights_changed_tick_{};
  struct SavedState;
  std::shared_ptr<const SavedState> checkpoint_;
  DebugGameSettings debug_settings_;
  std::map<std::string,std::pair<std::shared_ptr<const SavedState>,std::shared_ptr<const SavedState>>> debug_snapshots_;
  Unit& debug_player_mutable(unsigned slot);
  const DebugPlayerSettings& debug_settings_for(const Unit&) const;
  void debug_refill_weapon(Unit&);
  bool save_pending_{}, revert_pending_{}, cinematic_skip_enabled_{};
  bool level_finished_{},reset_requested_{};
  std::optional<std::uint64_t> reverted_tick_;
  std::uint64_t checkpoint_restores_{};
  void capture_checkpoint();
  void restore_checkpoint();
  CampaignCinematicState &mutable_cinematic_state();
  void move_unit(Unit &, Vec3 desired, bool jump, bool crouch,
                 bool player_control = false);
  Unit &script_unit(std::uint32_t);
  std::optional<Vec3> reference_point(Vec3,std::int16_t)const;
  Vec3 script_point(std::uint32_t)const;
  std::vector<std::uint32_t> script_objects(std::uint32_t)const;
  void update_command(Unit &);
  void update_weapon(Unit &, bool fire, bool reload, int owner, float pitch,
                     const Controls *source_control = nullptr, unsigned hand = 0);
  void update_charged_weapon(Unit &,WeaponState &,const WeaponDefinition &,bool fire,bool blocked,unsigned hand,int owner);
  void update_passive_weapon(WeaponState &,Unit *);
  void clear_weapon_for_holster(Unit &,WeaponState &);
  float weapon_hand_blend(const Unit &,const WeaponState &) const;
  void weapon_activity(Unit &,const WeaponState &,std::uint32_t);
  std::optional<unsigned> select_weapon_firing_effect(WeaponState &,const WeaponDefinition &);
  void shot(Unit &, int owner, float pitch, const Vec3 *prepared_direction = nullptr,
            const Vec3 *prepared_velocity = nullptr, WeaponState *weapon = nullptr,
            bool effects_prepared = false, std::int16_t barrel_index = -1);
  bool trace(Vec3 origin, Vec3 direction, float distance,
             const WeaponDefinition &, int owner, const DamageOwner &,
             std::uint32_t projectile=null_datum,const Vec3 *velocity=nullptr,
             float traveled=0);
  void damage(Unit &, const WeaponDefinition &, const DamageOwner &, int native_owner,
      std::optional<float> amount={},bool radius=false,
      std::optional<DamageImpactContext> impact={});
  ScriptValue host(const std::string &, const std::vector<ScriptValue> &);
  bool start_cinematic_object_animation(std::uint32_t object,std::uint32_t graph,
      std::uint32_t name,std::uint32_t anchor,bool unit,bool loop,bool interpolate);
  void step_cinematic_object_animations();
  void refresh_cinematic_object_poses(std::optional<std::uint32_t> requested = {});
  void attach_cinematic_objects(std::uint32_t parent,std::uint32_t parent_marker,
                                std::uint32_t child,std::uint32_t child_marker);
};
} // namespace halo2
