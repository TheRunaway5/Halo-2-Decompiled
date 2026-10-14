#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <filesystem>
#include <memory>
#include "halo2/transform.hpp"
#include "halo2/source_player_profile.hpp"
#include "halo2/death_camera.hpp"
#include "halo2/melee_impact.hpp"
#include "halo2/source_effect_location.hpp"
#include "halo2/unit_damage.hpp"
#include "halo2/particle_collision.hpp"
#include "halo2/source_weapon_shader_input.hpp"

namespace halo2 {
class Cache;
struct WeaponEffectInput;
struct SourceGameVariant;
struct ScenarioObject;
struct SourceObjectDrawCollector;
struct SourcePortalVisibilitySet;
struct SourceObjectAdmission;
struct SourceObjectAdmissionInput;
// Read-only publication of the actual101BC0 ->178E30 explosion request.
// Locations are the completed source effect's retained3C records, including
// their full34-byte world frames. No trajectory or effect frame is inferred.
struct SourceProjectileDetonationTu15 {
    std::uint32_t projectile,definition,effect,effect_instance,player,unit;
    std::array<std::byte,0x60> request;
    std::array<std::byte,4*0x1c> request_locations;
    std::vector<SourceEffectLocation> effect_locations;
    SourceEffectColors colors;
    SourceEffectControls controls;
};
struct SourcePlayerEquipmentTu15 {
    std::array<std::uint32_t,4> weapons,definitions,ready_ticks;
    std::array<SourceWeaponShaderState,4> shaders;
    std::array<std::int8_t,2> current,requested;
    std::array<std::uint8_t,2> grenades;
    std::uint16_t inventory_sequence,replicated_sequence;
};
// Read-only bytes around the actual1D94CE point-impulse dispatch.
struct SourceBodyImpulseTu15 {
    std::uint32_t object,tick,body_index;
    std::array<std::byte,0x100> before,after;
    std::array<std::byte,16> impulse,point;
};
// One fresh TU1.5 bank and its native process/arena/pool resources. Source
// constructors own all allocator, renderer, world and body state. The host
// process binding is explicit; it is not a claim of complete Xbox boot.
class PhysicsLiveWorldTu15 {
public:
    PhysicsLiveWorldTu15(const Cache &,const std::filesystem::path &xbe,
                        const std::filesystem::path &savegame_directory,std::size_t bsp=0);
    ~PhysicsLiveWorldTu15();
    PhysicsLiveWorldTu15(const PhysicsLiveWorldTu15 &)=delete;
    PhysicsLiveWorldTu15 &operator=(const PhysicsLiveWorldTu15 &)=delete;
    SourceRuntimeMemory &memory();
    // Complete source entries over this same bank, with a retained full ABI.
    // Correspondence IDs select authenticated TU1.5 code only.
    void run(std::uint32_t canonical_entry,SourcePhysicsWorldRuntimeAbi &,
             const SourcePhysicsContactRecordObserver & = {});
    void run_actual(std::uint32_t tu15_entry,SourcePhysicsWorldRuntimeAbi &,
                    const SourcePhysicsContactRecordObserver & = {});
    std::uint32_t call(std::uint32_t canonical_entry,
                       std::span<const std::uint32_t> arguments={});
    std::uint32_t call_actual(std::uint32_t tu15_entry,
                             std::span<const std::uint32_t> arguments={});
    NativeRuntimeAddress world() const;
    void initialize_map();
    NativeRuntimeAddress object_pool() const;
    NativeRuntimeAddress player_pool() const;
    // Explicit original-ABI boundary for authenticated source routines/probes.
    std::uint32_t source_world() const;
    std::uint32_t source_object_pool() const;
    std::uint32_t source_player_pool() const;
    std::uint32_t source_object_body(std::uint32_t full_datum) const;
    // Original1717B0 requests authored sections;130980 owns allocation,
    // real cache-sector I/O and22A680 pointer publication on this same bank.
    void preload_render_model(std::uint32_t render_model);
    void begin_model_frame();
    SourceObjectAdmission admit_render_objects(const SourcePortalVisibilitySet&,
        const SourceObjectAdmissionInput&,std::span<const std::array<std::uint32_t,4>> prior={})const;
    SourceObjectDrawCollector build_world_model(std::uint32_t object,unsigned view,
        unsigned local_players,bool view_modifier,float geometry_metric,float lighting_metric);
    // The native roster ordinal disambiguates offline identities on this
    // machine; it does not bind the original four local input/view records.
    void construct_player(std::uint32_t player,unsigned native_identity_ordinal,
                          const SourcePlayerConfiguration &,const SourceMachineIdentity &);
    void bind_player(std::uint32_t player,std::uint32_t object);
    // Original152078..152098 post-birth activation and full1613B0 equipment
    // setup.1A8B50 selects authored loadouts and creates/attaches real weapons.
    SourcePlayerEquipmentTu15 initialize_player_equipment(std::uint32_t player,
                                                          std::uint32_t object);
    SourcePlayerEquipmentTu15 player_equipment(std::uint32_t object) const;
    // Native controls queue the original command record in14FEC0's frame.
    // The complete player scheduler applies actions and153070/C92C0 once,
    // before PVS and the same source world's object/physics update.
    void deliver_command(std::uint32_t player,std::uint32_t object,
                         std::span<const std::byte,0x5c> command);
    // Actual13AAC0's player scheduler and five object/world phases, followed
    // by its13AB49 effect update and13ABD8 clock increment. Other game-frame
    // systems, including the preceding audio phase, are separate owners.
    void step_objects();
    std::vector<SourceProjectileDetonationTu15> take_projectile_detonations();
    void observe_body_impulses(std::function<void(const SourceBodyImpulseTu15 &)>);
    std::vector<MeleeImpact> take_melee_impacts();
    std::vector<UnitDamageEffectEvent> take_unit_damage_effects();
    SourceWeaponEffectPresentation request_weapon_effect(const WeaponEffectInput &);
    // Completed original spatial/melee and weapon targeting owners over the
    // retained world. Only their actual command fields are published.
    void populate_weapon_targeting(std::uint32_t player,std::uint32_t object,
        std::span<std::byte,0x5c> command);
    std::optional<ParticleCollisionHit> particle_collision(std::uint32_t flags,Vec3 origin,Vec3 displacement);
    // Native local-view input delivery; the original158340 gate and245EA0
    // packet producer consume the retained source body, seat and authored
    // camera track. The native caller retains the producer's28-byte state.
    std::optional<DeathCameraPacket> vehicle_camera_packet(unsigned view,
        std::uint32_t unit,float yaw,float pitch,std::array<std::uint32_t,7> &);
    NativeRuntimeAddress object_body(std::uint32_t full_datum) const;
    void configure_session(const SourceGameVariant &,std::uint16_t simulation_mode,
                           std::int16_t tick_rate,std::uint32_t random_seed);
    // One complete original game physics frame (actual1CC8D0), using the
    // source clock configured for this session and the owner's retained ABI.
    // Command delivery and game-clock advancement belong to the game tick.
    // A constructed world and admitted session are required; missing callees
    // propagate with their real source state rather than falling back to host
    // collision movement.
    SourcePhysicsWorldRuntimeAbi step_physics(
        const SourcePhysicsContactRecordObserver & = {});
    std::uint32_t create_biped(std::uint32_t definition,const Vec3 &position,
                               const Vec3 &forward);
    // Actual C1120 receives the original placement and palette addressed by
    // the source type table. D75A0 owns request construction, including UID,
    // frozen state, authored orientation, scale and type-specific fields.
    std::uint32_t create_scenario_object(const ScenarioObject &);
    // Actual15D4F0 consumes the original netgame row and admitted variant,
    // selects its collection, constructs its body and retains its row owner.
    std::uint32_t create_netgame_object(unsigned row);
    std::uint32_t netgame_object(unsigned row) const;
    // Presentation adopts the original model-created child in this salted
    // parent/body chain instead of constructing a second body.
    std::uint32_t child_object(std::uint32_t parent,std::uint32_t definition) const;
    // Complete BA3E0 deactivation and BA490 deletion precede header retirement.
    void destroy_object(std::uint32_t full_datum);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace halo2
