#pragma once
#include "halo2/source_hud_messages.hpp"
#include "halo2/source_player_status.hpp"
#include "halo2/source_gameplay_fade_tu15.hpp"
#include "halo2/source_multiplayer_event_tu15.hpp"
#include "halo2/source_control_target.hpp"
#include "halo2/source_execution_inputs.hpp"
#include "halo2/source_main_camera_receipt_tu15.hpp"
#include "halo2/source_scenario_birth.hpp"
#include "halo2/weapon_hud.hpp"
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_object_audio_views_tu15.hpp"
#include "halo2/source_loop_sound_request_tu15.hpp"
#include "halo2/source_object_loop_emitter_tu15.hpp"
#include "halo2/source_effect_light_birth_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_multiplayer_engine_entity_tu15.hpp"
#include <filesystem>
#include <memory>
#include <thread>
#include <utility>
#include "halo2/transform.hpp"
#include "halo2/source_player_profile.hpp"
#include "halo2/death_camera.hpp"
#include "halo2/source_vehicle_director_tu15.hpp"
#include "halo2/source_original_camera_seed.hpp"
#include "halo2/native_tu15_input_devices.hpp"
#include "halo2/melee_impact.hpp"
#include "halo2/source_effect_location.hpp"
#include "halo2/source_effect_replica_render_tu15.hpp"
#include "halo2/unit_damage.hpp"
#include "halo2/particle_collision.hpp"
#include "halo2/source_weapon_shader_input.hpp"
#include "halo2/weapon_effect.hpp"
#include "halo2/source_runtime_resources.hpp"
#include "halo2/physics_game_arena_native.hpp"
#include "halo2/source_runtime_host_virtual_memory.hpp"
#include "halo2/physics_pool_host_storage.hpp"
#include "halo2/physics_platform_locks.hpp"
#include "halo2/source_object_draw_collector.hpp"
#include "halo2/source_object_admission_memory.hpp"

#include "halo2/source_object_attachment_tu15.hpp"
#include "halo2/source_round_lifecycle.hpp"
#include "halo2/source_game_over.hpp"

namespace halo2 {
// Diagnostic layout is needed only by its consumers. Keep changes to native
// coverage counters from recompiling unrelated live-world users.
struct NativeTu15PhysicsFrameExecutionStats;
class Cache;
struct SourceTextureCameraScenePrefixTu15;
class SdlSoundPlatform;
class MccSoundVariationProvider;
class MccScriptSoundWorldAccess;
class RendererPaletteFactory;
struct WeaponEffectInput;
struct FirstPersonViewBinding;
struct SourceGameVariant;
struct ScenarioObject;
struct SourceObjectDrawCollector;
struct SourceObjectBuildRequest;
struct SourcePortalVisibilitySet;
struct SourceObjectAdmission;
struct SourceObjectAdmissionInput;
// Local native input owner. Callback objects never cross a world checkpoint.
// Monotonic domains rebase on import; UTC remains an absolute external input.
class PhysicsLiveClockSource {
public:
    virtual ~PhysicsLiveClockSource()=default;
    virtual std::uint32_t milliseconds() const noexcept=0;
    virtual std::uint64_t cycles() const noexcept=0;
    virtual std::uint64_t utc_filetime() const noexcept=0;
};
// Read-only diagnostics around the real168CA1 frame phase. CPU estimates and
// timestamp samples are external platform inputs, never gameplay call returns.
struct SourceFirstPersonCpuInputTu15 {
    std::uint32_t kind,input;
    std::uint64_t value;
};
// Read-only before/after the actual original1DA750 child call. The retained
// ABI and memory belong to the running world; no force or contact is supplied.
using SourceBodyVelocityInputObserverTu15=std::function<void(std::uint32_t,
    const SourcePhysicsWorldRuntimeAbi &,const SourceRuntimeMemory &)>;
using SourcePhysicsStoreObserverTu15=std::function<void(std::uint32_t,
    const SourcePhysicsContactRecordStore &)>;
using SourcePhantomPhaseObserverTu15=std::function<void(bool,
    const SourcePhysicsWorldRuntimeAbi &,const SourceRuntimeMemory &)>;
using SourceFirstPersonPhaseObserverTu15=std::function<void(bool,
    const SourcePhysicsWorldRuntimeAbi &,const SourceRuntimeMemory &,
    std::span<const std::uint32_t>,std::span<const SourceFirstPersonCpuInputTu15>)>;
// Read-only18CEFC ->223B20 submit boundary over the actual160-byte request.
// Before/after callbacks retain the true source ABI/result; observers never
// supply an audio admission, source voice or successful platform return.
using SourceSoundSubmitObserverTu15=std::function<void(bool,std::uint32_t,
    std::uint32_t,const SourcePhysicsWorldRuntimeAbi &,const SourceRuntimeMemory &,
    std::span<const std::uint32_t>,std::span<const SourceFirstPersonCpuInputTu15>)>;
// Completed original caller intent, copied before admission mutates its
// backing. A native output adapter may consume it without supplying a source
// admission/voice result. Independent of diagnostic instruction tracing.
using SourceSoundRequestObserverTu15=std::function<void(std::uint32_t,
    std::span<const std::byte,160>)>;
using SourceLoopSoundRequestObserverTu15=std::function<void(const SourceLoopSoundRequestTu15 &)>;
struct SourceTeleporterActivationTu15 {
    std::uint32_t player{},sound=0xffffffffu;
};
struct SourceTeleporterEventsTu15 {
    std::vector<SourceTeleporterActivationTu15> activations;
    std::vector<SourceHudMessageEvent> notices;
};
// The source world owns its engine/player phases by default. Existing native
// match adapters own that interval until their state and consumers migrate;
// callers select exactly one owner for a tick, without checkpointed policy.
enum class SourceEngineFrameOwnerTu15 { World, MatchAdapter };
// Completed original engine bank, read without advancing any owner.
struct SourceMultiplayerEngineStateTu15 {
    SourceRoundLifecycle round;
    SourceGameEndStats statistics;
    SourceResultRecording recording;
    SourceGameOverState game_over;
    std::uint16_t retained_teams{};
    std::uint16_t available_teams{};
    bool finalized{};
};
// Actual15DDC0 input and completed outcome. NONE is a real tied/no-winner
// result; consumers must not infer a winner from accumulated win counters.
struct SourceMultiplayerRoundEndTu15 {
    std::uint32_t winner=0xffffffffu,tick{};
    std::int16_t round{};
    bool team{},game_over{};
};
struct SourceBspPartCommandState;
// Read-only publication of the actual101BC0 ->178E30 explosion request.
// Locations are the completed source effect's retained3C records, including
// their full34-byte world frames. No trajectory or effect frame is inferred.
// effect_instance is the actual constructed identity; a finite effect may
// already retire during its first update before the creation call returns.
struct SourceProjectileDetonationTu15 {
    std::uint32_t projectile,definition,effect,effect_instance,player,unit;
    std::array<std::byte,0x60> request;
    std::array<std::byte,4*0x1c> request_locations;
    std::vector<SourceEffectLocation> effect_locations;
    SourceEffectColors colors;
    SourceEffectControls controls;
    std::optional<SourceEffectReplicaRenderTu15> replica_render;
};
// Actual168DED draw records: model, owner, mode and original model-order
// world matrices. Skinning consumes these completed records without sampling
// another animation/IK owner. The native caller supplies its real view camera.
struct SourceFirstPersonModelTu15 {
    std::uint32_t render_model,owner,mode;
    std::vector<Transform> nodes;
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
// Completed original108A70 request made by the live source weapon scheduler.
// Observers receive presentation data after the original callee returns.
struct SourceWeaponEffectEventTu15 {
    std::uint32_t tick{},unit=0xffffffffu;
    WeaponEffectInput input;
    SourceWeaponEffectPresentation presentation;
    Vec3 velocity;
    // A one-shot can retire during its original initial update and return
    // NONE. Preserve its actual completed birth bank before that update.
    std::optional<std::vector<SourceEffectLocation>> birth_locations;
};
// Actual17D8DA ->D91D0 jpt! dispatch. The request address is scoped to the
// callback, and observers only read the retained bank before/after the real
// callee. They cannot admit damage, replace a result or allocate an effect.
struct SourceEffectDamageCallTu15 {
    std::uint32_t tick,instance,effect,owner,damage,request;
};
using SourceEffectDamageObserverTu15=std::function<void(bool,
    const SourceEffectDamageCallTu15 &,const SourceRuntimeMemory &)>;
// Native netcode lag-compensation boundary; not an original owner. Actual
// FF100 moves an instantaneous projectile (proj+BC bit5) once, through
// FF15C -> FDEB0, whose path test 101030 calls 16C620 at 1010AB (center ray)
// and 1012B0/10130C (radius side rays). The literal stdcall arguments are
// published here. run() executes the complete original query from its entry
// ABI; an owner may change native target state around it and must restore it
// exactly, and may run it again after rolling back a rejected attempt. The
// final run's result and writes are what the original caller receives.
struct SourceHitscanQueryTu15 {
    // owner_player is the projectile's +C4 owner player; ignore_object is its
    // +140 exclusion (the firing unit) as passed to 16C620.
    std::uint32_t projectile=0xffffffffu,definition=0xffffffffu,owner_player=0xffffffffu;
    std::uint32_t site{},flags{},ignore_object=0xffffffffu,ignore_unit=0xffffffffu,result{};
    std::uint32_t stack{}; // Entry ESP: bytes below it are dead callee scratch.
    Vec3 origin,delta;
};
class SourceHitscanRewindOwnerTu15 {
public:
    virtual ~SourceHitscanRewindOwnerTu15()=default;
    // Per-call gate. False leaves FF15C/16C620 on the unchanged original path.
    virtual bool armed()const noexcept=0;
    virtual void query(const SourceHitscanQueryTu15 &,SourceRuntimeMemory &,
        const std::function<void()> &run)=0;
};
// One fresh TU1.5 bank and its native process/arena/pool resources. Source
// constructors own all allocator, renderer, world and body state. The host
// process binding is explicit; it is not a claim of complete Xbox boot.
class PhysicsLiveWorldTu15 {
public:
    // Ordinary native startup uses compiled, individually declared TU1.5
    // operands and cold storage. No original executable path is required.
    PhysicsLiveWorldTu15(const Cache &,const std::filesystem::path &savegame_directory,
                        std::size_t bsp=0);
    PhysicsLiveWorldTu15(const Cache &,const std::filesystem::path &savegame_directory,
                        std::size_t bsp,std::shared_ptr<PhysicsLiveClockSource>);
    // Explicit offline-reference image startup for differential probes.
    PhysicsLiveWorldTu15(const Cache &,const std::filesystem::path &xbe,
                        const std::filesystem::path &savegame_directory,std::size_t bsp=0);
    PhysicsLiveWorldTu15(const Cache &,const std::filesystem::path &xbe,
                        const std::filesystem::path &savegame_directory,std::size_t bsp,
                        std::shared_ptr<PhysicsLiveClockSource>);
    // Isolated diagnostic operand tape, bound before native startup and kept
    // through world teardown. A null clock selects the actual platform source.
    // The creator must release this world before sealing the owning tape.
    // Complete-call journals and foreign-executor inputs are not admitted.
    PhysicsLiveWorldTu15(const Cache &,const std::filesystem::path &xbe,
                        const std::filesystem::path &savegame_directory,std::size_t bsp,
                        std::shared_ptr<PhysicsLiveClockSource>,std::shared_ptr<SourceExecutionInputs>);
    ~PhysicsLiveWorldTu15();
    PhysicsLiveWorldTu15(const PhysicsLiveWorldTu15 &)=delete;
    PhysicsLiveWorldTu15 &operator=(const PhysicsLiveWorldTu15 &)=delete;
    SourceRuntimeMemory &memory();
    // Immutable resource/codec owner of this actual World. A borrower must
    // retain the World and its admitted Cache for the reference's lifetime.
    const SourceRuntimeResources &runtime_resources() const;
    // Per-world execution evidence. Residual calls keep native-only closure
    // open; these counters never supply simulation or checkpoint state.
    NativeTu15PhysicsFrameExecutionStats native_physics_frame_activity() const;
    // Resolve this Cache's admitted resident bytes through the retained
    // canonical delivery and actual native allocation. No host pointer is
    // narrowed; foreign Cache owners or unloaded extents are rejected.
    NativeRuntimeAddress resident_cache_address(const Cache& expected_owner,
        std::uint32_t cache_address,std::size_t bytes) const;
    // Opaque native metadata ownership. Teardown/restore retires the old
    // provider even for identical bytes; Cache keeps actual DAT/PCM ownership.
    MccSoundVariationProvider sound_variation_provider(const Cache& expected_owner) const;
    // Epoch-qualified actual script-sound policy and checkpointed metadata.
    MccScriptSoundWorldAccess script_sound_access(const Cache& expected_owner);
    // Explicit caller input, including retained native stack scratch. The
    // journal owns a copy; arbitrary memory writes remain unrecorded work.
    void write_source_call_input(std::uint32_t,std::span<const std::byte>);
    // Semantic native cinematic mutations retain the world's executor and
    // publication barriers. Neither advances an original-address CPU owner.
    void stop_campaign_screen();
    void acknowledge_campaign_screen_capture();
    void completed_main_camera_observer(SourceCompletedMainCameraObserverTu15);
    // The slot is exclusive: active scoped/unscoped owners cannot be replaced.
    SourceCompletedMainCameraObserverScopeTu15 scoped_completed_main_camera_observer(SourceCompletedMainCameraObserverTu15);
    void inspect_render_memory(const SourceRenderMemoryInspectorTu15&) const;
    // Native original-ABI caller frame using this world's actual retained
    // stack/TLS/floating-point owner. Does not execute or admit a source entry.
    SourcePhysicsWorldRuntimeAbi prepare_source_call(std::span<const std::uint32_t> arguments={});
    // A separately typed source2B770/2B9F0 CPU-prefix scope. The genuine
    //132B0 boundary yields a retained packet/ABI; no completed graphics call
    // or GPU result is supplied. Whole13C00 restores source context afterward.
    void with_texture_camera_scene_prefix_tu15(std::span<const std::byte,0x118>,
        bool texture_camera,unsigned actual_local_players,
        const std::function<void(const SourceTextureCameraScenePrefixTu15 &)> &);
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
    // Opaque157EB0 default captured locally before any authority import.
    // Incoming checkpoint commits never replace this executor-owned value.
    SourceOriginalCameraSeed original_camera_seed()const;
    // Native physical-input owner, separate from received commands and from
    // the camera's cycle-only input binding. The original startup/sample/
    // preset/mapping/close routines execute on this world's retained bank.
    // This independent owner does not import/export hosting checkpoints.
    void initialize_native_input(NativeTu15InputSelection);
    void publish_native_input_profile(unsigned device,const SourceFullPlayerProfile &,
                                      const SourceMachineIdentity &);
    void sample_native_input();
    void close_native_input();
    // Optional actual native graphics owner, admitted before session options.
    // Headless execution supplies no graphics success or palette bindings.
    void admit_renderer_palette(RendererPaletteFactory);
    // Native campaign BSP delivery over the same source object/actor bank.
    void switch_campaign_bsp(std::size_t);
    // Actual currently bound Cache/BSP image; independent of cold game+2.
    std::size_t source_bsp_index() const;
    // Read-only native catalog selection from this World's actual campaign
    // game-options keys. Completion/profile-save/request services are separate.
    std::optional<std::string> next_campaign_scenario() const;
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
    bool request_render_geometry(std::uint32_t flags,std::uint32_t resource_header);
    void begin_model_frame();
    // Publish a completed native173130 portal result into the actual render
    // collector, then run whole3F3E0. Calls union the local views; the literal
    //2B630..2B63A reset occurs once through begin_model_frame().
    void publish_render_visibility(const SourcePortalVisibilitySet&);
    // Original per-view12D19 -> 22830 PRT cache reset and SH preparation.
    void begin_model_view();
    // Effective render pose after BF640/gates, copied before scratch is reused.
    // This differs from the collision WORLD bank (notably the finger nodes).
    // Present only after this frame's actual model builder requests it.
    std::optional<std::vector<Transform>> rendered_world_nodes(std::uint32_t object) const;
    // Native BF640 completion for network body publication. Returns an owned
    // WORLD copy while restoring every native write, including lazy BFB90
    // finger/LOCAL/cache work. No geometry/resource collector is constructed.
    std::vector<Transform> completed_body_world_nodes(std::uint32_t object);
    // Original41C70/41C10/D7230 traversal over the actual attachment/child
    // chains. Deep unowned attachment services propagate their real boundary.
    void append_render_attachments(SourceBspPartCommandState&,
        const SourceObjectAdmission&,std::uint32_t mode);
    SourceObjectAdmission admit_render_objects(const SourcePortalVisibilitySet&,
        const SourceObjectAdmissionInput&,std::span<const std::array<std::uint32_t,4>> prior={})const;
    SourceObjectClusterMemberships render_cluster_memberships()const;
    SourceObjectDrawCollector build_world_model(std::uint32_t object,unsigned view,
        unsigned local_players,bool view_modifier,float geometry_metric,float lighting_metric);
    // Complete builder for a current render-entry/child request. The caller
    // owns the mounted collector storage and its lifetime through this call.
    void build_render_model(SourceObjectDrawCollector&,const SourceObjectBuildRequest&,
        unsigned view,unsigned local_players,bool view_modifier);
    // The native roster ordinal disambiguates offline identities on this
    // machine; it does not bind the original four local input/view records.
    // The platform address produces the player ID. The separate simulation
    // machine key comes from this world's already constructed local owner.
    void construct_player(std::uint32_t player,unsigned native_identity_ordinal,
                          const SourcePlayerConfiguration &,const SourceMachineIdentity &);
    // Match the caller's start bank to this cache's authored scenario. A
    // separately declared native placement keeps its existing input owner.
    bool uses_scenario_player_spawns(std::span<const Spawn>) const;
    // Complete151DA0, with real14FDC0 activation when needed. The caller owns
    // respawn admission; the source factory owns selection, birth, binding,
    // equipment and completion stores. NONE preserves its actual failure.
    std::uint32_t birth_player(std::uint32_t player);
    SourceMultiplayerEngineStateTu15 multiplayer_engine_state() const;
    SourceMultiplayerEngineEntityTu15 multiplayer_engine_entity() const;
    void observe_multiplayer_round_end(std::function<void(const SourceMultiplayerRoundEndTu15 &)>);
    // Copies actual common event dispatch inputs. The callback is read-only
    // and supplies no event, RNG, audio admission or gameplay service result.
    void observe_multiplayer_events(std::function<void(const SourceMultiplayerEventTu15 &)>);
    // Native roster input, before player allocation/configuration. Original
    // team-mask and existing-player configuration owners consume the change.
    void publish_roster_team(unsigned user,std::int8_t team);
    SourcePlayerConfiguration update_player_configuration(std::uint32_t,const SourcePlayerConfiguration &);
    void bind_player(std::uint32_t player,std::uint32_t object);
    // Complete88C50 identity gate and155920/15A860 ordinary departure.
    // Retains the allocated player/profile/results; unit death is deferred
    // through the source world update. A stale/already-left identity returnsfalse.
    bool depart_player(std::uint32_t player);
    // Native selects its real local view; original151C90 owns the player,
    // view map, input binding and first-person unit transition. -1 releases.
    void set_local_player(std::uint32_t player,std::int16_t view);
    // Original151D30 owns player+24 and the reverse logical input-user table.
    // Native local users are distinct from views and physical controller ports.
    void set_player_input_user(std::uint32_t player,std::int16_t user);
    // Native presentation's actual camera position, published to172200's
    // retained observer record.1755B0 uses it for source effect admission.
    // This supplies a native camera input, not the original camera scheduler.
    void publish_native_observer_position(unsigned view,Vec3 position);
    FirstPersonViewBinding first_person_view(unsigned view) const;
    std::vector<SourceFirstPersonModelTu15> prepare_first_person_models(unsigned view,
        Vec3 position,Vec3 forward,Vec3 up,const SourceFirstPersonPhaseObserverTu15 & = {});
    void observe_first_person_phase(SourceFirstPersonPhaseObserverTu15);
    void observe_sound_submissions(SourceSoundSubmitObserverTu15);
    void observe_sound_requests(SourceSoundRequestObserverTu15);
    void observe_loop_sound_requests(SourceLoopSoundRequestObserverTu15);
    // Native presentation's actual local camera inputs. Complete18EE50,
    //18F900 and190460 compute every scheduling/PVS/distance result. The
    // legacy listener bank and Xbox driver/map availability stay untouched.
    void step_native_object_audio(const SourceObjectAudioViewsTu15&,float elapsed);
    // Native pre-view18D5A0/18F9B0 inputs for unparented scen/bloc/ssce
    // ordinary lsnd attachments. Actual function/marker/cluster owners run
    // on this World; host camera/PAS and playback-progress bits are unused.
    // Execution and completed result are covered by the source call journal.
    std::vector<SourceObjectLoopEmitterTu15> object_loop_emitters();
    void observe_effect_light_births(SourceEffectLightBirthObserverTu15);
    // Bind an actual owned EEPROM resource before source audio construction.
    // This retains only its immutable non-key UserSettings section; absence
    // never becomes a speaker/configuration or initialized-device default.
    void bind_sound_user_settings(const std::filesystem::path &);
    // Retain real native output and the existing application speaker policy.
    // Original SDK construction/reset/DMA code still owns every source result.
    // A bound device cannot be exported through a world authority checkpoint.
    void bind_native_sound_output(std::shared_ptr<SdlSoundPlatform>,const std::filesystem::path &);
    // Explicit observed boot-SMC transaction inputs, before source audio
    // construction. The original kernel expression publishes immutable data;
    // this does not select a mode or supply a device-initialization result.
    void bind_sound_boot_smc_input(const std::filesystem::path &);
    // Original152078..152098 post-birth activation and full1613B0 equipment
    // setup.1A8B50 selects authored loadouts and creates/attaches real weapons.
    SourcePlayerEquipmentTu15 initialize_player_equipment(std::uint32_t player,
                                                          std::uint32_t object);
    SourcePlayerEquipmentTu15 player_equipment(std::uint32_t object) const;
    // Explicit native/debug equipment input through the same complete
    // CF4D0/B9960/B9B70/CF5A0 owners used by original1A8B50.
    std::uint32_t equip_weapon(std::uint32_t unit,std::uint32_t definition);
    // Native controls queue the original command record in14FEC0's frame.
    // The complete player scheduler applies actions and153070/C92C0 once,
    // before PVS and the same source world's object/physics update.
    void deliver_command(std::uint32_t player,std::uint32_t object,
                         std::span<const std::byte,0x5c> command);
    // Actual13AAC0's player scheduler and five object/world phases, followed
    // by its13AB49 effect update and13ABD8 clock increment. Other game-frame
    // systems, including the preceding audio phase, are separate owners.
    // Complete TU1.5 CE440 admits an external unit controller request.
    // Actor/player ownership and the original control exclusion still apply.
    void set_unit_controller_active(std::uint32_t unit,bool requested);
    // Original C93D0 constructs the 7C command; C92C0 delivers declared
    // throttle/facing inputs synchronously. The actual controller solves motion.
    void deliver_unit_input(std::uint32_t unit,Vec3 throttle,Vec3 facing);
    void step_objects(SourceEngineFrameOwnerTu15 = SourceEngineFrameOwnerTu15::World);
    // Actual15EBA0 teleporter phase: registered engine, raw mode !=4,
    // whole1A6830 before player callbacks and world movement. No clock advance.
    std::vector<SourceTeleporterActivationTu15> step_teleporters(
        std::vector<SourceHudMessageEvent> *notices=nullptr);
    // Completed outputs at actual15EBEE ->1A6830, before later player/world
    // work. The world rebases its retained input; the observer only copies
    // presentation data and cannot advance another source owner.
    void observe_multiplayer_teleporters(std::function<void(const SourceTeleporterEventsTu15 &)>);
    std::array<float,2> hud_message_timing()const;
    // Completed source phases only:161610's descriptor at15ED4F and the raw
    // counter inputs at15E6BC. Snapshot readers do not execute either query.
    // Pointer-free observations of the actual pre-fade tick and genuine
    // player-init/unit-bind lifecycle, before host-local-view filtering.
    void observe_gameplay_fade(std::function<void(const SourceGameplayFadeFrameTu15&)>,
        std::function<void(const SourceGameplayFadeActivationTu15&)> activation);
    void observe_player_status(std::function<void(const SourcePlayerStatusPhaseTu15&)>);
    void observe_player_scoreboard(std::function<void(const SourcePlayerScoreboardEventTu15&)>);
    // Rebase the native player's retained source input through18A2A0 using
    // its completed unit forward. Raw native look deltas keep their baseline.
    void reorient_player_look(std::uint32_t player,std::uint32_t unit);
    // Original13AA28's complete179DF0 particle phase. The caller delivers its
    // actual frame delta once per frame. Other13A9B0 owners remain separate.
    void advance_effect_particles(float frame_seconds,const SourceFirstPersonPhaseObserverTu15& observer={});
    std::vector<SourceProjectileDetonationTu15> take_projectile_detonations();
    void observe_body_impulses(std::function<void(const SourceBodyImpulseTu15 &)>);
    // Presentation publication of the same completed 324F80 point impulse.
    // Unlike the diagnostic observer above, a caller/ownership shape it cannot
    // qualify is left to the ordinary dispatch instead of failing the tick.
    // Read-only: the original call runs unchanged either way.
    void publish_body_impulses(std::function<void(const SourceBodyImpulseTu15 &)>);
    void observe_phantom_phase(SourcePhantomPhaseObserverTu15);
    void observe_physics_stores(SourcePhysicsStoreObserverTu15);
    // Explicit diagnostic contract: the collector may inspect completed stores
    // and retained bytes, but may not mutate source state, FP policy or input.
    // General store observers retain their existing mutable registration.
    void observe_physics_stores_read_only(SourcePhysicsStoreObserverTu15);
    void observe_body_velocity_inputs(SourceBodyVelocityInputObserverTu15);
    void observe_weapon_effects(std::function<void(const SourceWeaponEffectEventTu15 &)>);
    // Read-only retained ABI/bank snapshots around the real scheduled108A70.
    void observe_weapon_effect_phase(SourceFirstPersonPhaseObserverTu15);
    void observe_effect_damage(SourceEffectDamageObserverTu15);
    // Borrowed until replaced or cleared with nullptr; see SourceHitscanQueryTu15.
    // Configuration only: install before stepping and clear before the owner dies.
    void set_hitscan_rewind_owner(SourceHitscanRewindOwnerTu15 *);
    std::vector<MeleeImpact> take_melee_impacts();
    std::vector<UnitDamageEffectEvent> take_unit_damage_effects();
    SourceWeaponEffectPresentation request_weapon_effect(const WeaponEffectInput &);
    // Completed original spatial/melee and weapon targeting owners over the
    // retained world. Only their actual command fields are published.
    void populate_weapon_targeting(std::uint32_t player,std::uint32_t object,
        std::span<std::byte,0x5c> command);
    // Convert native accumulated look into per-tick deltas, then run original
    //18A300 before aiming and command delivery. State belongs to the player.
    void prepare_player_look(std::uint32_t player,std::uint32_t object,
        std::span<std::byte,0x5c> command);
    std::optional<Vec2> player_look(std::uint32_t player,std::uint32_t unit) const;
    // Read the completed153BA0 result retained by the actual player's input
    // producer. No query executes during presentation;full generations match.
    std::optional<SourcePlayerActionTarget> player_action_target(std::uint32_t player,std::uint32_t unit) const;
    void populate_weapon_selection(std::uint32_t unit,std::span<std::byte,0x5c> command,bool switch_weapon);
    // Complete original TU1.5 13DC44 target-mask owner over the actual unit.
    std::uint32_t weapon_hud_target_flags(std::uint32_t unit);
    WeaponHudTarget weapon_hud_target(std::uint32_t weapon);
    SourceWeaponHudState weapon_hud_state(std::uint32_t weapon);
    VehicleWeaponHudStatus vehicle_weapon_hud(std::uint32_t unit);
    std::optional<ParticleCollisionHit> particle_collision(std::uint32_t flags,Vec3 origin,Vec3 displacement);
    bool objective_waypoint_occluded(Vec3 origin,Vec3 endpoint,std::uint32_t flags,
        std::uint32_t ignore_viewer,std::uint32_t ignore_target);
    // Complete original scheduler over a private native view context. Retains
    // authored entry/exit timers, producer state and observer registration.
    DeathCameraPacket player_camera_packet(std::uint32_t player,float seconds,
      SourceCameraControllerState &,SourceObserverState &,std::uint32_t &camera_random,
      bool cycle_pressed=false,std::uint8_t *camera_mode=nullptr);
    DeathCameraPacket unit_camera_packet(std::uint32_t unit,float seconds,
        SourceCameraControllerState &,SourceObserverState &,std::uint8_t *camera_mode=nullptr);
    NativeRuntimeAddress object_body(std::uint32_t full_datum) const;
    void configure_session(const SourceGameVariant &,std::uint16_t simulation_mode,
                           std::int16_t tick_rate,std::uint32_t random_seed);
    // Deliver campaign kind1 and its actual signed difficulty field. The
    // source clock/cache/engine owners consume the same native session inputs.
    void configure_campaign_session(std::int16_t difficulty,
        std::uint16_t simulation_mode,std::int16_t tick_rate,std::uint32_t random_seed);
    // Original ai_place overloads on this world's actual actor/object/script
    // banks. References retain original packed AI identities; an absent count
    // selects authored placement. Reached missing owners propagate as errors.
    void place_ai(std::uint32_t reference,std::optional<std::int16_t> count={});
    // Resolve an AI reference through the original actor iterator.
    // Return its full unit datum, including packed actor/location references.
    std::uint32_t ai_object(std::uint32_t reference);
    // Original scenario kill-volume state on this world. Containment uses
    // the actual source object pose and clock, one scheduled volume per tick;
    // fatal damage stays in the
    // original biped/vehicle update rather than this query.
    // Original B95D0 owns pose/physics delivery for this external position input.
    void set_object_position(std::uint32_t object,Vec3 position);
    void set_kill_volume_enabled(std::int16_t trigger,bool enabled);
    bool object_in_kill_volume(std::uint32_t object);
    // Actual11EBA0/11EAB0, including attached named-object/node resolution,
    // source WORLD composition, scale and strict open box boundaries.
    bool trigger_contains(std::size_t trigger,Vec3 point);
    // One complete original game physics frame (actual1CC8D0), using the
    // source clock configured for this session and the owner's retained ABI.
    // Command delivery and game-clock advancement belong to the game tick.
    // A constructed world and admitted session are required; missing callees
    // propagate with their real source state rather than falling back to host
    // collision movement.
    SourcePhysicsWorldRuntimeAbi step_physics(
        const SourcePhysicsContactRecordObserver & = {});
    std::uint32_t create_biped(std::uint32_t definition,const Vec3 &position,
                               const Vec3 &forward,std::uint32_t player=0xffffffffu);
    // Actual C1120 receives the original placement and palette addressed by
    // the source type table. D75A0 owns request construction, including UID,
    // frozen state, authored orientation, scale and type-specific fields.
    std::uint32_t create_scenario_object(const ScenarioObject &,SourceScenarioBirth = SourceScenarioBirth::automatic);
    // Actual15D4F0 consumes the original netgame row and admitted variant,
    // selects its collection, constructs its body and retains its row owner.
    std::uint32_t create_netgame_object(unsigned row);
    std::uint32_t netgame_object(unsigned row) const;
    // Presentation adopts the original model-created child in this salted
    // parent/body chain instead of constructing a second body.
    std::uint32_t child_object(std::uint32_t parent,std::uint32_t definition) const;
    // Exact original C0790/BAF10 model-child provenance, including duplicate definitions.
    std::uint32_t model_child_object(std::uint32_t parent,std::uint32_t definition,
        std::uint32_t parent_marker,std::uint32_t child_marker) const;
    // Complete BA3E0 deactivation and BA490 deletion precede header retirement.
    void destroy_object(std::uint32_t full_datum);
    struct AuthorityCheckpoint {
        struct Allocation {std::uint32_t payload,bytes,flags;};
        struct ResourceRead {std::uint32_t descriptor,destination,bytes;};
        std::uint32_t version=3,bsp{},main_thread_token{},last_instruction{},delivered_players{};
        std::uint64_t next_thread_token{};
        std::uint32_t logical_milliseconds{};
        std::uint64_t logical_cycles{};
        std::uint64_t logical_utc_filetime{};
        SourceRuntimeResources::Identity resources;
        SourceRuntimeMemory::Checkpoint bank;
        SourceRuntimeHostVirtualMemory::OwnershipCheckpoint virtual_memory;
        SourcePhysicsPoolHostStorage::OwnershipCheckpoint pool_storage;
        SourcePhysicsGameArenaNative::OwnershipCheckpoint arena;
        PhysicsPlatformLocks::OwnershipCheckpoint locks;
        NativeSynchronization::Checkpoint events;
        std::map<std::uint32_t,std::uint32_t> lock_events;
        std::vector<Allocation> allocations;
        std::map<std::uint32_t,ResourceRead> resource_reads;
        std::uint64_t next_resource_read{},next_model_collector{};
        std::map<std::pair<unsigned,std::uint32_t>,SourceObjectDrawCollector> model_collectors;
        std::map<std::uint32_t,SourceObjectAttachmentTu15> model_attachments;
        bool session_configured{},model_scratch_mounted{};
        std::vector<std::uint32_t> exit_callbacks;
        std::vector<SourceProjectileDetonationTu15> projectile_detonations;
        std::vector<UnitDamageEffectEvent> unit_damage_effects;
        std::vector<MeleeImpact> melee_impacts;
    };
    class PreparedAuthority {
    public:
        ~PreparedAuthority();
        PreparedAuthority(PreparedAuthority &&) noexcept;
        PreparedAuthority &operator=(PreparedAuthority &&) noexcept;
        bool for_owner(const PhysicsLiveWorldTu15 &) const noexcept;
        // Read-only access for enclosing owners to validate and stage their
        // native-address bindings to this exact replacement allocation.
        // The reference expires when preparation commits or is cancelled.
        const SourceRuntimeMemory &replacement_memory() const;
        // Executor-owned ABI for validating this staged bank's native owners.
        // Does not execute a kernel or write either live or replacement bytes.
        SourcePhysicsWorldRuntimeAbi replacement_caller() const;
        // Fixed campaign renderer-static adoption under this preparation
        // lock/barrier; never used by default full-RAM authority imports.
        void retain_live_campaign_lens();
        // A dedicated recipient presentation world retains its real native
        // cache/history while importing the next accepted simulation cut.
        void retain_live_multiplayer_render_history();
        void retain_live_multiplayer_render_history_from(const PhysicsLiveWorldTu15 &);
    private:
        friend class PhysicsLiveWorldTu15;
        PreparedAuthority();
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
    // Completed-call barrier on the retained executor thread; the enclosing
    // match must also quiesce raw memory borrowers and presentation consumers.
    // This captures the source physics owner, not the outer match/simulation.
    AuthorityCheckpoint authority_checkpoint() const;
    // Completed, externally quiescent executor boundary, outside any tape.
    // These adjust external domains only; native clock/RNG bytes stay intact.
    SourceExecutionClocks execution_clocks() const;
    void anchor_execution_clocks(const SourceExecutionClocks &);
    void require_execution_boundary() const;
    // Same native call/borrow exclusion, but permits the explicitly tracked
    // Begin/EndPresentedModelPoses operation inside a recorded frame.
    void require_presentation_boundary() const;
    // Executor-owned backing publication identity, independent of original
    // game clocks and journal bytes. Derived renderer owners cannot survive
    // a successful native bank replacement without a fresh activation.
    std::uint64_t authority_backing_revision() const;
    PreparedAuthority prepare_authority(const AuthorityCheckpoint &);
    // No-throw publication of all staged backing/resources/sidecars. Callers
    // must rebind every borrowed view/native address afterward. Services and
    // observers remain local; no host pointers or native thread IDs import.
    // This owner must outlive preparation; commit/cancel on its executor thread.
    bool commit_authority(PreparedAuthority &&) noexcept;
private:
    friend class NativeVehicleCamera;
    std::array<std::byte,0x5c> collect_native_input(std::uint32_t player,unsigned view,
        float frame_seconds,float game_seconds,SourceCameraControllerState &,SourceObserverState &);
    friend class MccScriptSoundWorldAccess;
    friend class SourceNativePublication;
    friend class MultiplayerObjectRuntime;
    void require_publication_provider(const MccSoundVariationProvider&,const Cache&)const;
    // Borrowed component only: the enclosing owning native publication must
    // retain this World, and must release timing/SDL before destroying it.
    class NativePublicationHold {
    public:
        ~NativePublicationHold();
        NativePublicationHold(const NativePublicationHold &)=delete;
        NativePublicationHold(NativePublicationHold &&)=delete;
        bool matches(const PhysicsLiveWorldTu15 &)const noexcept;
    private:
        friend class PhysicsLiveWorldTu15;
        explicit NativePublicationHold(PhysicsLiveWorldTu15 &);
        struct Impl;
        const std::thread::id executor_=std::this_thread::get_id();
        std::unique_ptr<Impl> impl_;
    };
    std::unique_ptr<NativePublicationHold> hold_native_publication();
    // One closed metadata publication owns the existing mutation-fault
    // barrier across its separately journaled source-input writes.
    void write_script_sound_inputs(
        std::span<const std::pair<std::uint32_t,std::span<const std::byte>>>);
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace halo2
