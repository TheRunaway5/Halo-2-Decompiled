#pragma once
#include "halo2/source_scenario_birth.hpp"
#include "halo2/source_live_objects_tu15.hpp"
#include "halo2/death_startup_native.hpp"
#include "halo2/source_runtime_cache_tags.hpp"
#include "halo2/transform.hpp"
#include "halo2/source_player_profile.hpp"
#include "halo2/observer_world.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/damage_feedback.hpp"
#include "halo2/damage_impact.hpp"
#include "halo2/native_grenade.hpp"
#include "halo2/native_melee_animation.hpp"
#include "halo2/source_unit_interaction_target.hpp"
#include "halo2/source_attached_effect.hpp"
#include <map>

namespace halo2 {
struct WeaponEffectInput;
struct ScriptValue;
class Cache;
struct Vec3;
struct ShadowCasterSphere;
struct SourceGameVariant;
struct ScenarioObject;
class PhysicsLiveWorldTu15;
struct FirstPersonViewBinding;
struct SourceObjectDrawCollector;
struct DamageOwner;
struct UnitDamageResult;
struct MultiplayerSourceMotion {
  Vec3 position,velocity,forward;
  float crouch;
  std::uint8_t physical_state;
  std::uint32_t parent;
  std::int16_t seat;
  std::uint32_t action_word;
  std::uint8_t unit_flags;
  std::uint16_t support_flags;
};
struct MultiplayerSourceVitality {
  float body{},shield{};
  DamageFeedbackState feedback;
};
// One retained source bank for the native multiplayer object world. Native
// process/cache bindings own host resources; complete original constructors
// produce pool, heap, physics and object state. This is not Xbox boot emulation.
class MultiplayerObjectRuntime {
public:
  // Cache owns resource I/O and must outlive this runtime.
  MultiplayerObjectRuntime(const std::filesystem::path &source_image,
      const Cache &,std::size_t bsp,const std::filesystem::path &savegame_directory);
  MultiplayerObjectRuntime(const std::filesystem::path &source_image,
      const Cache &,std::size_t bsp,const std::filesystem::path &savegame_directory,
      std::shared_ptr<PhysicsLiveClockSource>);
  ~MultiplayerObjectRuntime();
  MultiplayerObjectRuntime(const MultiplayerObjectRuntime &)=delete;
  MultiplayerObjectRuntime &operator=(const MultiplayerObjectRuntime &)=delete;
  SourceRuntimeMemory &memory();
  NativeRuntimeAddress object_pool() const;
  NativeRuntimeAddress player_pool() const;
  bool owns_physics() const;
  std::optional<std::uint32_t> physical_body_count() const;
  NativeRuntimeAddress source_clock() const;
  NativeRuntimeAddress object_body(std::uint32_t object) const;
  // Retained solely for original-ABI consumers and differential probes.
  std::uint32_t source_object_pool() const;
  std::uint32_t source_player_pool() const;
  std::uint32_t source_object_body(std::uint32_t object) const;
  std::uint32_t object_definition(std::uint32_t object) const;
  std::vector<SourceLiveObjectTu15> live_objects() const;
  std::vector<SourceAttachedEffect> attached_effects(std::uint32_t object) const;
  SourceModelRegionBank object_model_regions(std::uint32_t object) const;
  std::uint32_t object_parent(std::uint32_t object) const;
  Transform object_transform(std::uint32_t object) const;
  // Complete BA1D0/TU1.5 BC200 resolves the ultimate parent and actual
  // physics component, including its stationary and absent-body branches.
  Vec3 object_linear_velocity(std::uint32_t object);
  SourceVehicleSeatTarget vehicle_seat_target(std::uint32_t unit,std::uint32_t vehicle);
  // Native script builtin inputs enter complete original TU1.5 device/group
  // APIs on this same retained source bank; getters preserve source bits.
  ScriptValue script_device(const std::string &,const std::vector<ScriptValue> &);

  MultiplayerSourceMotion biped_motion(std::uint32_t object) const;
  NativeGrenadeAction grenade_action(std::uint32_t unit) const;
  NativeMeleeAction melee_action(std::uint32_t unit) const;
  std::vector<MeleeImpact> take_melee_impacts();
  std::vector<UnitDamageEffectEvent> take_unit_damage_effects();
  SourceWeaponEffectPresentation request_weapon_effect(const WeaponEffectInput &);
  void populate_weapon_targeting(std::uint32_t player,std::uint32_t object,
      std::span<std::byte,0x5c> command);
  std::uint32_t weapon_hud_target_flags(std::uint32_t unit);
  WeaponHudTarget weapon_hud_target(std::uint32_t weapon);
  std::optional<ParticleCollisionHit> particle_collision(std::uint32_t flags,Vec3 origin,Vec3 displacement);
  std::int8_t current_grenade(std::uint32_t unit) const;
  // Read-only actual type5 bodies whose definitions occur in the retained
  // globals grenade block. No parallel projectile header or motion is made.
  std::vector<NativeGrenadeProjectile> grenade_projectiles() const;
  std::vector<SourceProjectileDetonationTu15> take_projectile_detonations();
  void observe_body_impulses(std::function<void(const SourceBodyImpulseTu15 &)>);
    void observe_phantom_phase(SourcePhantomPhaseObserverTu15);
    void observe_physics_stores(SourcePhysicsStoreObserverTu15);
    void observe_body_velocity_inputs(SourceBodyVelocityInputObserverTu15);
  void observe_weapon_effects(std::function<void(const SourceWeaponEffectEventTu15 &)>);
  void observe_weapon_effect_phase(SourceFirstPersonPhaseObserverTu15);
  void observe_effect_damage(SourceEffectDamageObserverTu15);
  MultiplayerSourceVitality object_vitality(std::uint32_t object) const;
  // Deliver the completed native D5DE0/D9110/BCC20 owner state before the
  // retained source body's fatal/physical consumer. Maxima and flags remain
  // owned by their constructors and transition routines.
  void deliver_vitality(std::uint32_t object,const MultiplayerSourceVitality &);
  // Original request/report/reaction owners followed by DBB90's physical
  // stage, before attribution and cleanup. Radius geometry and melee aim/
  // bounds come from this retained world. No contact/velocity is forced.
  void deliver_direct_impact(std::uint32_t object,std::uint32_t damage,
      const DamageOwner &,const UnitDamageResult &,Vec3 direction,Vec3 position);
  void deliver_damage_reaction(std::uint32_t object,std::uint32_t damage,
      const DamageOwner &,const UnitDamageResult &,const DamageImpactContext &);
  void deliver_command(std::uint32_t player,std::uint32_t object,
                       std::span<const std::byte,0x5c> command);
  // Native presentation owns local views; the source simulation receives the
  // actual allocated player, OS machine identity and selected configuration.
  // TU1.5's already-allocated new-player continuation14EAD0 owns record
  // construction14EBE0, active census and engine/results join15A950. The
  // earlier-image14D800/14D600 adapter remains a separate compatibility path.
  void construct_player(std::uint32_t player,unsigned native_identity_ordinal,
                        const SourcePlayerConfiguration &);
  // Native roster input, before player allocation/configuration. Original
    // team-mask and existing-player configuration owners consume the change.
    void publish_roster_team(unsigned user,std::int8_t team);
    void bind_player(std::uint32_t player,std::uint32_t object);
  // TU1.5 ordinary departure retains the allocated identity/profile. The
  // original world consumes its deferred unit-death request on a later tick.
  bool depart_player(std::uint32_t player);
  void set_local_player(std::uint32_t player,std::int16_t view);
  void publish_native_observer_position(unsigned view,Vec3 position);
  FirstPersonViewBinding first_person_view(unsigned view) const;
  std::vector<SourceFirstPersonModelTu15> prepare_first_person_models(unsigned view,
      Vec3 position,Vec3 forward,Vec3 up,const SourceFirstPersonPhaseObserverTu15 & = {});
  void observe_first_person_phase(SourceFirstPersonPhaseObserverTu15);
  void observe_sound_submissions(SourceSoundSubmitObserverTu15);
  SourcePlayerEquipmentTu15 initialize_player_equipment(std::uint32_t player,
                                                        std::uint32_t object);
  SourcePlayerEquipmentTu15 player_equipment(std::uint32_t object) const;
  // Native confirmed-fatal stimulus. D7CD0 produces its actual damage record;
  // D7E00/CBDC0 own the ordinary death transition and player detachment.
  void kill_biped(std::uint32_t object);
  // Explicit native session inputs at the original game-options delivery
  // boundary. Clock reset consumes this rate; no captured options are loaded.
  void configure_session(const SourceGameVariant &,std::uint16_t simulation_mode,
      std::int16_t tick_rate,std::uint32_t random_seed);
  // Native placement input, complete B8820/B8A30 admission and construction.
  // Returns the actual salted source handle; never reserves a parallel header.
  std::uint32_t create_biped(std::uint32_t definition,const Vec3 &position,
                             const Vec3 &forward,std::uint32_t player=0xffffffffu);
  std::uint32_t create_scenario_object(const ScenarioObject &,SourceScenarioBirth = SourceScenarioBirth::automatic);
  std::uint32_t create_netgame_object(unsigned row);
  std::uint32_t netgame_object(unsigned row) const;
  std::uint32_t child_object(std::uint32_t parent,std::uint32_t definition) const;
  void destroy_object(std::uint32_t full_datum);
  // Actual source clock increment followed by the five complete object/world
  // phases. Retains the same bank; does not supply contacts or death admission.
  void step_objects();
  // Read-only presentation of the current source WORLD matrices. A stale
  // full handle or unavailable node backing is an error, never an idle pose.
  std::vector<Transform> world_nodes(std::uint32_t object) const;
  //133790 consumes the retained object+30/+3C sphere. Resolve the same full
  //salted owner as WORLD nodes; preview-model bounds are not this bank.
  ShadowCasterSphere object_sphere(std::uint32_t object) const;
  // Native presentation inputs over this same retained TU1.1 bank. Resources
  // finish through the original queue/read/fixup graph before their first draw.
  void preload_render_model(std::uint32_t render_model);
  void begin_model_frame();
  void publish_render_visibility(const SourcePortalVisibilitySet&);
  void advance_effect_particles(float,const SourceFirstPersonPhaseObserverTu15& observer={});
  // Keep native presentation and all its readbacks within one public-call
  // lifetime. The callback must not retain a world/memory reference.
  void render_model_view(const std::function<void(PhysicsLiveWorldTu15&)>&);
  std::optional<std::vector<Transform>> rendered_world_nodes(std::uint32_t object) const;
  SourceObjectAdmission admit_render_objects(const SourcePortalVisibilitySet&,
      const SourceObjectAdmissionInput&,std::span<const std::array<std::uint32_t,4>> prior={})const;
  SourceObjectDrawCollector build_world_model(std::uint32_t object,unsigned view,
      unsigned local_players,bool view_modifier,float geometry_metric,float lighting_metric);
  std::optional<DeathCameraTarget> camera_target(std::uint32_t object) const;
  SourceObserverWorldServices observer_world_services();
  std::optional<DeathCameraPacket> vehicle_camera_packet(unsigned view,
      std::uint32_t unit,float yaw,float pitch,std::array<std::uint32_t,7> &);
  SourceObserverWorldFrame observer_world_frame(std::int32_t view,
      std::uint32_t current_unit,std::uint16_t player_flags,
      const DeathCameraSavedClusters &) const;
  // Original full-ABI entry on the same bank, including reached native resource
  // owners. Scratch return/arguments are native caller storage, never snapshots.
  std::uint32_t call(std::uint32_t entry,std::span<const std::uint32_t> arguments={},
      std::optional<std::uint32_t> eax={},std::optional<std::uint32_t> ecx={},
      std::optional<std::uint32_t> edx={});
  struct RetainedDeathReport {
    std::int16_t damage_class=-1;
    Vec3 direction;
    std::uint32_t force{};
  };
  struct AuthorityCheckpoint {
    std::uint32_t version=1;
    PhysicsLiveWorldTu15::AuthorityCheckpoint world;
    std::map<std::uint32_t,RetainedDeathReport> death_reports;
  };
  class PreparedAuthority {
  public:
    ~PreparedAuthority();
    PreparedAuthority(PreparedAuthority &&) noexcept;
    PreparedAuthority &operator=(PreparedAuthority &&) noexcept;
    bool for_owner(const MultiplayerObjectRuntime &) const noexcept;
    const SourceRuntimeMemory &replacement_memory() const;
  private:
    friend class MultiplayerObjectRuntime;
    PreparedAuthority();
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };
  // Completed TU1.5 source owner plus the bridge's pending native damage
  // reports. This is not the outer Simulation/MultiplayerMatch or a wire save.
  // Quiesce raw memory borrowers and returned services through publication;
  // stage their source pool views against replacement_memory() and rebind.
  AuthorityCheckpoint authority_checkpoint() const;
  PreparedAuthority prepare_authority(const AuthorityCheckpoint &);
  // Owner must outlive preparation; commit/cancel on its retained executor.
  // Native observers/services stay local. Publication performs no allocation.
  bool commit_authority(PreparedAuthority &&) noexcept;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
  std::unique_ptr<PhysicsLiveWorldTu15> tu15_;
  struct Continuation;
  std::unique_ptr<Continuation> continuation_;
  using DeathReport=RetainedDeathReport;
  std::map<std::uint32_t,DeathReport> death_reports_;
};
} // namespace halo2
