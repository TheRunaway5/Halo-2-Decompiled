#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_object_velocity_read_only_tu15.hpp"
#include "halo2/source_object_sound_tu15.hpp"
#include "halo2/source_physics_deactivation_tu15.hpp"
#include "halo2/source_collision_instance_storage_tu15.hpp"
#include "halo2/source_collision_query_tu15.hpp"
#include "halo2/source_vehicle_occupancy_tu15.hpp"
#include "halo2/source_model_render_tu15.hpp"
#include "halo2/source_object_render_entry.hpp"
#include "halo2/source_object_admission_memory.hpp"
#include "halo2/source_model_build.hpp"
#include "halo2/source_bsp_part_commands.hpp"
#include "halo2/source_render_cache.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/source_scene_device_constructor_tu15.hpp"
#include "halo2/source_map_world_support_tu15.hpp"
#include "halo2/physics_phantom_world_tu15.hpp"
#include "halo2/physics_game_arena_native.hpp"
#include "halo2/physics_early_initializer_runtime.hpp"
#include "halo2/physics_renderer_storage_runtime.hpp"
#include "halo2/physics_main_heap_runtime.hpp"
#include "halo2/physics_platform_locks.hpp"
#include "halo2/source_process_heap_platform.hpp"
#include "halo2/native_tu15_heap_data.hpp"
#include "halo2/source_runtime_image.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/native_threads.hpp"
#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/source_runtime_cache_tags.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_player_departure_tu15.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/player_command.hpp"
#include "halo2/source_damage_delivery_runtime.hpp"
#include "halo2/death_cycle_runtime_tu15.hpp"
#include "halo2/source_combat_action_tu15.hpp"
#include "halo2/source_contrail_geometry_tu15.hpp"
#include "halo2/source_effect_particle_frame_tu15.hpp"
#include "halo2/source_particle_swarm_tu15.hpp"
#include "halo2/source_effect_particle_draw_tu15.hpp"
#include "halo2/source_ai_actor_tu15.hpp"
#include "halo2/source_ai_frame_tu15.hpp"
#include "halo2/source_kill_volumes.hpp"
#include "halo2/source_state_context_startup_tu15.hpp"
#include "halo2/source_sandbox_special_tu15.hpp"
#include "halo2/source_sound_startup_tu15.hpp"
#include "halo2/source_runtime_dsp_section.hpp"
#include "halo2/source_runtime_sound_sdk_resources.hpp"
#include "halo2/source_sound_sdk_tu15.hpp"
#include "halo2/weapon_effect.hpp"
#include "halo2/fp_registration.hpp"
#include "halo2/math.hpp"
#include "physics_live_entries_tu15.hpp"
#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <mutex>
#include <sstream>
#include <thread>
#include <exception>
#include <atomic>
#if defined(__i386__) || defined(__x86_64__)
#include <immintrin.h>
#endif

namespace halo2 {
namespace {
constexpr auto image=SourcePhysicsContactImage::TitleUpdate15;
constexpr SourcePhysicsFactoryFloatingPoint fp{0x27f,0x1f80};
constexpr std::uint32_t stack=0x600f0000u,tls=0x60200000u,
    parameters=0x60201000u,process_context=0x60210000u,
    notifications=0x60211000u,allocation_callback=0xf7000001u,
    native_tick_count=0x60212000u,native_thread_context=0x60213000u;
class NativeWorldClock final:public PhysicsLiveClockSource {
public:
    std::uint32_t milliseconds() const noexcept override {
        return std::uint32_t(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    }
    std::uint64_t cycles() const noexcept override {
#if defined(__i386__) || defined(__x86_64__)
        return __rdtsc();
#else
        return std::uint64_t(std::chrono::steady_clock::now().time_since_epoch().count());
#endif
    }
    std::uint64_t utc_filetime() const noexcept override {
        return std::uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()/100)+116444736000000000ull;
    }
};
struct WorldMutationObserver {
    SourceRuntimeMemory &bank;
    std::shared_ptr<std::atomic<std::uint64_t>> serial=std::make_shared<std::atomic<std::uint64_t>>(0);
    explicit WorldMutationObserver(SourceRuntimeMemory &bank):bank(bank) {
        const auto observed=serial;
        source_runtime_register_write_guard(bank,this,[observed](auto,auto){observed->fetch_add(1,std::memory_order_relaxed);});
    }
    ~WorldMutationObserver(){source_runtime_unregister_write_guard(bank,this);}
};
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned bytes=4) {
    if(!bytes || bytes>4)throw FormatError("Invalid native source scalar read size");
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(bytes));std::uint32_t v=0;
    for(unsigned i=0;i<bytes;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);
    return v;
}
void put(SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t v) {
    std::array<std::byte,4>b;
    for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8*i));m.write(p,b);
}
}
struct PhysicsLiveWorldTu15::Impl {
    const Cache&cache_owner;
    const std::shared_ptr<PhysicsLiveClockSource> clocks;
    SourceRuntimeResources resources;
    std::uint32_t tick_adjustment{};
    std::uint64_t cycle_adjustment{};
    mutable std::recursive_mutex call_gate;
    std::size_t public_calls{},execution_depth{};
    bool staging{},continuation_fault{};
    struct PublicCall {
        Impl &owner;
        int exceptions=std::uncaught_exceptions();
        bool mutating;
        std::uint64_t writes{};
        explicit PublicCall(Impl &owner,bool mutating=true):owner(owner),mutating(mutating) {
            std::lock_guard gate(owner.call_gate);
            if(owner.staging || (mutating && std::this_thread::get_id()!=owner.main_thread))
                throw FormatError("TU1.5 world mutation requires its available retained executor");
            writes=owner.mutations.serial->load(std::memory_order_relaxed);
            ++owner.public_calls;
        }
        ~PublicCall(){std::lock_guard gate(owner.call_gate);if(mutating && std::uncaught_exceptions()>exceptions &&
            owner.mutations.serial->load(std::memory_order_relaxed)!=writes)owner.continuation_fault=true;--owner.public_calls;}
    };
    void require_executor() const {
        if(std::this_thread::get_id()!=main_thread || staging)
            throw FormatError("TU1.5 world requires its available retained executor");
    }
    struct Execution {
        Impl &owner;
        int exceptions=std::uncaught_exceptions();
        std::optional<SourceRuntimeDspSection::CallScope> dsp_scope;
        std::optional<SourceRuntimeSoundSdkResources::CallScope> sdk_scope;
        explicit Execution(Impl &owner):owner(owner){owner.require_executor();dsp_scope.emplace(owner.dsp_section.get());sdk_scope.emplace(owner.sound_sdk_resources.get());++owner.execution_depth;}
        ~Execution(){if(std::uncaught_exceptions()>exceptions)owner.continuation_fault=true;--owner.execution_depth;}
    };
    SourceRuntimeMemory bank;
    std::unique_ptr<SourceRuntimeDspSection> dsp_section;
    std::unique_ptr<SourceRuntimeSoundSdkResources> sound_sdk_resources;
    WorldMutationObserver mutations{bank};
    SourceRuntimeHostVirtualMemory virtual_memory;
    SourcePhysicsPoolHostStorage pool_storage;
    PhysicsPlatformLocks locks;
    NativeSynchronization events{{0xf6000001u,0xf6010000ull}};
    NativeSynchronizationServices event_services=events.services();
    std::map<std::uint32_t,std::uint32_t> lock_events;
    std::map<std::thread::id,std::uint32_t> thread_ids;
    std::uint64_t next_thread_token=1;
    std::mutex mutex;
    const std::thread::id main_thread=std::this_thread::get_id();
    std::uint32_t last_instruction{};
    using Allocation=AuthorityCheckpoint::Allocation;
    std::vector<Allocation> allocations;
    using NativeResourceRead=AuthorityCheckpoint::ResourceRead;
    std::map<std::uint32_t,NativeResourceRead> resource_reads;
    std::uint64_t next_resource_read=0xf8000001u;
    SourceRuntimeCacheBspImage bsp;
    std::map<std::uint32_t,std::uint32_t> bipeds;
    bool session_configured{};
    std::uint32_t delivered_players{};
    std::map<std::pair<unsigned,std::uint32_t>,SourceObjectDrawCollector> model_collectors;
    std::map<std::uint32_t,std::vector<Transform>> rendered_model_nodes;
    std::uint64_t next_model_collector=0x62000000u;
    static constexpr std::uint32_t model_scratch=0x60400000u;
    bool model_scratch_mounted{};
    std::vector<SourceProjectileDetonationTu15> projectile_detonations;
    std::function<void(const SourceBodyImpulseTu15 &)> body_impulse_observer;
    SourcePhantomPhaseObserverTu15 phantom_phase_observer;
    SourcePhysicsContactRecordObserver physics_store_observer;
    SourceBodyVelocityInputObserverTu15 body_velocity_input_observer;
    std::function<void(const SourceWeaponEffectEventTu15 &)> weapon_effect_observer;
    SourceFirstPersonPhaseObserverTu15 weapon_effect_phase_observer;
    bool weapon_effect_phase_active{};
    std::vector<std::uint32_t> weapon_effect_phase_trace;
    std::vector<SourceFirstPersonCpuInputTu15> weapon_effect_phase_cpu;
    SourceEffectDamageObserverTu15 effect_damage_observer;
    SourceFirstPersonPhaseObserverTu15 first_person_phase_observer;
    SourceSoundSubmitObserverTu15 sound_submit_observer;
    bool first_person_phase_active{};
    std::vector<std::uint32_t> first_person_phase_trace;
    std::vector<SourceFirstPersonCpuInputTu15> first_person_phase_cpu;
    std::vector<UnitDamageEffectEvent> unit_damage_effects;
    std::vector<MeleeImpact> melee_impacts;
    struct NativeExitRegistrations {
        Impl &owner;
        std::vector<std::uint32_t> callbacks;
        ~NativeExitRegistrations() {for(auto i=callbacks.rbegin();i!=callbacks.rend();++i)owner.call_actual(*i);}
    } native_exits{*this,{}};
    // Destroy and join before destroying callbacks, locks or retained memory.
    std::unique_ptr<SourcePhysicsGameArenaNative> arena;

    Impl(const Cache&cache,const std::filesystem::path&xbe,const std::filesystem::path&files,std::size_t selected,
         std::shared_ptr<PhysicsLiveClockSource> clocks)
        :cache_owner(cache),clocks(std::move(clocks)),resources(cache),bank(load_source_runtime_image(xbe,image,SourceRuntimeKernelTickCount{
            native_tick_count,{},"native monotonic millisecond tick-count data import"})),virtual_memory(bank,0x08000000u),
         pool_storage(bank,0x10000000u),bsp(cache.load_source_runtime_cache_bsp_image(selected)) {
        if(!this->clocks)throw FormatError("TU1.5 world requires local native clock inputs");
        dsp_section=std::make_unique<SourceRuntimeDspSection>(bank,xbe);
        bank.mount(0x60000000u,std::vector<std::byte>(0x100000),
            SourceRuntimeMemory::Access::ReadWrite,"native TU1.5 multiplayer main call stack");
        bank.mount(tls,std::vector<std::byte>(0x2000),SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 exception chain and process caller parameters");
        put(bank,tls,0xffffffffu);
        sound_sdk_resources=std::make_unique<SourceRuntimeSoundSdkResources>(bank,tls);
        // Retained current-thread input for the original CRT/XAPI consumers.
        // This native executor has no Xbox TLS expansion; the original CRT
        // consequently constructs its main-thread state at573840 itself.
        bank.mount(native_thread_context+0x28u,std::vector<std::byte>(4),
            SourceRuntimeMemory::Access::ReadWrite,"native TU1.5 main-thread CRT TLS policy");
        bank.mount(native_thread_context+0x12cu,std::vector<std::byte>(4),
            SourceRuntimeMemory::Access::ReadWrite,"native TU1.5 retained executor thread identity");
        put(bank,tls+0x28u,native_thread_context);
        put(bank,native_thread_context+0x12cu,lock_services().current_thread());
        bank.mount(process_context+0x250,std::vector<std::byte>(4),SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 process allocation notification pointer");
        bank.mount(notifications+0x14,std::vector<std::byte>(4),SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 process allocation consumer");
        put(bank,tls+0x20,process_context);put(bank,process_context+0x250,notifications);
        put(bank,notifications+0x14,allocation_callback);
        // Import156 is data, not an address in the map's virtual range. Bind
        // a retained native DWORD so the encoded ordinal cannot alias cache
        // bytes at8000009C. The original33B590 loads and RET still execute.
        if(word(bank,0x41b774u)!=native_tick_count)
            throw FormatError("TU1.5 tick-count import identity differs");

        // The native process invokes actual2DE2EA with the literal caller's
        // 2DBA60..2DBA8C inputs. Publish its returned heap at the consumed CRT
        // binding. Unrun hardware/timer/XAPI startup is not reported complete.
        put(bank,parameters,0x30);
        const auto args=native_tu15_process_heap_arguments(parameters);
        auto a=frame(args);
        source_run_process_heap_with_native_platform(bank,0x2da83a,a,image,fp,
            services(),virtual_memory,locks,lock_services());
        if(!a.gp[0])throw std::runtime_error("TU1.5 native process heap construction failed");
        put(bank,0x56ee28,a.gp[0]);

        arena=std::make_unique<SourcePhysicsGameArenaNative>(bank,
            SourcePhysicsGameArenaConfiguration{files,0x60110000u,fp,{}},services());
        for(auto entry:{0x12cb70u,0x1221b0u,0x125280u}) {
            a=frame({});arena->run(entry,a);
        }
        // Main-thread source construction has exclusive access after the
        // startup worker has retired the real file jobs and is stopped/joined.
        arena->stop();
        // Actual CRT row46DA08 constructs the static Havok scratch owner.
        // Its real native exit registration is retained through bank lifetime.
        call_actual(0x3835f0u); // Actual CRT row46D7C8 Havok rigid-body deactivator.
        call_actual(0x381370u);
        call_actual(0x380fa0u); // Actual CRT row46D990 owns portal/object empty bounds.
        // Actual SDK accounting CRT rows46D7E4..46D7F0; source writes its
        // statistics pointers, not native-assigned pool/capability totals.
        for(auto entry:{0x387baau,0x387bb5u,0x38a988u,0x38a993u})call_actual(entry);
        call_actual(0x3825d0u); // Actual CRT game-engine class registration.
        initialize_game_storage();
        call_actual(0x148d40u); // Actual startup row3 constructs both RNG streams.
        call_actual(0x149050u); // Actual source clock storage owner, row17.
        call_actual(0x1e7710u); // Actual row18 constructs team relationship state.
        call_actual(0x1f3830u); // Actual source gravity/physics constants, row25.
        call_actual(0x1f4700u); // Actual44B308 scenario kill-volume state owner.
        call_actual(0x219080u); // Actual44B7AC BSP collision-instance mask allocation.
        call(0x133760); // actual134B00: full renderer raw/descriptor construction.
        call(0x1c5f00); // actual1CB130: real pools/main heap/renderer overflow join.
        call_actual(0xb87f0); // Actual source object header/body storage constructors.
        call_actual(0x10e3b0u); // Actual object activation bookkeeping owner, row28.
        call_actual(0x1789f0u); // Actual body/component datum owners, row40.
        call_actual(0x14dfe0u); // Actual source multiplayer player storage, row19.
        call_actual(0x159f80u); // Actual source game-engine storage, row22.
        call_actual(0x125c90u); // Actual12054 resource-cache construction.
        call_actual(0x12ebe0u); // Actual1281D texture-cache construction.
        call_actual(0x1303c0u); // Actual12818 model-resource construction.
        call_actual(0x1183f0u); // Actual473A00 antenna datum storage.
        call_actual(0x1800b0u); // Actual44B404 decal pool/global constructor, before44B44C.
        call_actual(0x17e640u); // Actual44B44C effect-instance/location pool storage.
        call_actual(0x186cb0u); // Actual44B398 breakable-surface effect storage.
        call_actual(0x18b5c0u); // Actual startup row44B548 object-looping-sound pool.
        initialize_object_looping_sound_storage();
        initialize_sound_storage();
        call_actual(0x22c440u); // Literal44B56C owns13A9B0's per-frame sound state.
        call_actual(0x2131b0u); // Actual44B524 sound-availability bitmap storage.
        // Actual44B350 owns cached object render states and the color/cache
        // records consumed by projectile attachment creation at17E4AA.
        call_actual(0x2b480u);
        call_actual(0x168ba3u); // Actual1410AC first-person view storage owner.
        call_actual(0x168bc5u); // Actual1410C2 resets its real four-view bank.
        call_actual(0x1888a0u); // Original local input/camera storage constructor.
        call_actual(0x1eecf0u); // Literal44B1C4 player/view action-cache constructor.
        call_actual(0x2293e0u); // Literal44B590 per-view feedback request storage.
        call_actual(0x1563a0u); // Literal44B5B4 local player effect/impulse storage.
        call_actual(0x157e10u); // Literal44B3BC original director storage.
        for(auto entry:{0x381710u,0x381730u,0x381750u,0x381770u})call_actual(entry);
        call_actual(0x1cfa40u); // Actual44B6F8 complete AI storage initializer.
        const auto tags=cache.load_source_runtime_cache_tag_image();
        reserve_map_pages(tags);
        source_bind_runtime_cache_tags_tu15(bank,tags);
        source_bind_runtime_cache_sound_gestalt_tu15(bank);
        // Original1918D9 prepares every resident collision, biped, creature,
        // physical model and vehicle definition. In particular11F02A passes
        // zero to1F2C70, resolving serialized shape references before birth.
        call_actual(0x11ef70u);
        for(const auto&tag:cache.select_tags({.group=tag_groups::biped}))bipeds.emplace(tag.datum,tag.address);
        a=frame({});a.gp[0]=bsp.mapping;
        source_run_physics_live_support_tu15(bank,0x1255c0,a,fp,services());
        if(!(a.gp[0]&255u))throw FormatError("TU1.5 source BSP publication failed");
        // Original11EA1D/11EA20 prepares the selected BSP's instanced
        // collision MOPP shapes after resource publication and before binding.
        // Their serialized vtables are not covered by the resident tag pass.
        a=frame({});a.gp[0]=bsp.bsp;run_actual(0x171120u,a);
        source_bind_runtime_cache_bsp_tu15(bank,bsp);
        // Actual44B7BC is the BSP-load slot (constructor+10), not a
        // substituted all-visible mask. Complete2190C0 owns initialization,
        // existing object associations and authored noncollidable exclusions.
        call_actual(0x2190c0u);
        call_actual(0x213260u); // Actual44B52C new-map availability reset.
        call_actual(0x10e410u);
        call_actual(0x178ab0u);
        call_actual(0x1cb200u);
        call_actual(0x14e0b0u);
        call_actual(0x125eb0u); // Actual191A57 cache activation/reset.
        call_actual(0x130600u); // Actual191A89 model-resource activation/reset.
        call_actual(0x1f4760u); // Actual44B310 resets scenario kill-volume state.
        call_actual(0x118430u); // Actual473A04 activates/resets antenna storage.
        call_actual(0x180170u); // Actual44B40C activates/resets the decal pool and per-cluster links.
        call_actual(0x17e700u); // Actual44B454 effect-instance/location new-map reset.
        call_actual(0x186d30u); // Actual44B3A0 activates breakable-surface effect storage.
        call_actual(0x2b520u); // Actual44B358 activates/resets cached render states.
    }
    void initialize_object_looping_sound_storage() {
        // Literal18B660..18B71C storage prefix. The original2279E0 audio
        // volume/reset and18BB50 profile consumers are separate native audio
        // composition; neither is replaced with a fabricated callee result.
        const auto pool=word(bank,0x4f9b5cu),state=word(bank,0x4f9b58u);
        if(!pool || !state)throw FormatError("TU1.5 looping-sound storage constructor failed");
        const std::array active{std::byte{1}};bank.write(pool+0x29u,active);
        auto a=frame({});a.gp[6]=pool;run_actual(0x16e600u,a);
        for(unsigned i=0;i<8;++i)put(bank,state+i*4u,0xffffffffu);
        put(bank,state+0x20u,0);put(bank,state+0x24u,0);
        for(unsigned i=0;i<0x40;++i)put(bank,state+0x28u+i*4u,0);
        for(unsigned i=0;i<0x40;++i)put(bank,state+0x128u+i*4u,0xffffffffu);
        const auto one=word(bank,0x46a0d0u);
        for(auto offset:{0x228u,0x22cu,0x230u,0x234u})put(bank,state+offset,one);
        put(bank,state+0x238u,0x1f40u);put(bank,state+0x23cu,0);
    }
    void initialize_sound_storage() {
        // Actual12008..12025 supplies this static arena through VirtualAlloc.
        // The native resource owner reserves/commits real zero-filled pages;
        // it does not claim the Xbox kernel's failure-register behavior.
        if(word(bank,0x51db00u) || word(bank,0x4f2c50u))
            throw FormatError("TU1.5 sound storage is already constructed");
        const auto reserved=virtual_memory.allocate({0,0,0x23000u,0x2000u,4u});
        const auto committed=virtual_memory.allocate({reserved.base,0,0x23000u,0x1000u,4u});
        if(reserved.status || committed.status || !committed.base)
            throw FormatError("TU1.5 sound static arena has no native backing");
        put(bank,0x51db00u,committed.base);put(bank,0x51db04u,0);
        const auto allocate=[&](std::uint32_t size) {
            const auto base=word(bank,0x51db00u),cursor=word(bank,0x51db04u);
            const auto address=(std::uint64_t(base)+cursor+3u)&~std::uint64_t(3u);
            const auto end=address+size;
            if(end<std::uint64_t(base) || end-base>0x23000u)
                throw FormatError("TU1.5 sound static arena request exceeds its original backing");
            bank.mutable_view(std::uint32_t(address),size);
            const auto increment=std::uint32_t(end-base)-cursor;
            put(bank,0x51db04u,std::uint32_t(end-base));put(bank,parameters+0x100u,increment);
            const std::array args{0x51db08u};auto a=frame(args);
            a.gp[0]=parameters+0x100u;a.gp[7]=4u;run_actual(0x166a80u,a);
            return std::uint32_t(address);
        };
        // Literal127EF1..12804B storage prefix. The subsequent224EC0 Xbox
        // DirectSound activation belongs to the separate audio device owner.
        // Its unactivated flags remain the constructor's actual zero bytes.
        const auto state=allocate(0x20cu);
        for(unsigned i=0;i<0x83;++i)put(bank,state+i*4u,0);
        put(bank,0x4f2c50u,state);bank.mutable_view(state+0x79u,1)[0]=std::byte{1};
        call_actual(0x21efe0u);
        put(bank,state+0x1b0u,0xffffffffu);put(bank,state+0x1b4u,0);
        put(bank,state+0x1b8u,word(bank,0x44923cu));put(bank,state+0x1ccu,0xffffffffu);
        put(bank,state+0x1d0u,0);put(bank,state+0x1d4u,0);
        const std::array pool_args{0x45f1f8u,0xbcu};auto a=frame(pool_args);a.gp[0]=0x180u;
        run_actual(0x11f5e0u,a);const auto sources=a.gp[0];
        put(bank,0x4f2c4cu,sources);const auto slots=allocate(0x1200u);put(bank,0x4f2c48u,slots);
        if(!sources)throw FormatError("TU1.5 sound source storage construction failed");
        bank.mutable_view(sources+0x29u,1)[0]=std::byte{1};
        a=frame({});a.gp[6]=sources;run_actual(0x16e600u,a);
        for(unsigned i=0;i<0x480;++i)put(bank,slots+i*4u,0);
        for(const auto entry:{0x220550u,0x220ad0u,0x223ea0u})
            if(!(call_actual(entry)&255u))throw FormatError("TU1.5 sound pool construction failed");
        // Original128039 supplies the voice-capacity configuration in EAX.
        a=frame({});a.gp[0]=0x455b20u;run_actual(0x2247b0u,a);
        if(!(a.gp[0]&255u))throw FormatError("TU1.5 sound voice storage construction failed");
    }
    void reserve_map_pages(const SourceRuntimeCacheTagImage &tags) {
        // Native map I/O supplies the literal header. Original125354..1253A2
        // reserves header+1C from the selected physical lower bound before
        // tag publication. Omitting this reservation lets12ECF0 give the map
        // pages to textures and the first resource read destroys the tag table.
        std::array<std::byte,0x20> header;
        std::ifstream input(cache_owner.path(),std::ios::binary);
        input.read(reinterpret_cast<char*>(header.data()),header.size());
        if(!input)throw FormatError("Native physical map reservation requires its actual cache header");
        const auto raw=[&](unsigned p) {
            std::uint32_t value{};
            for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(header[p+i])<<(8*i);
            return value;
        };
        const auto requested=raw(0x1c),selected=word(bank,0x4f2cf0u);
        if(raw(0)!=0x68656164u || raw(4)!=8u || !requested || requested>0xfffff000u || selected>=5)
            throw FormatError("Native physical map reservation input differs from an Xbox cache");
        const auto slot=0x4f2cfcu+selected*4u;
        const auto lower=word(bank,slot),upper=word(bank,0x4f2d10u+selected*4u);
        const auto bytes=(requested+0xfffu)&0xfffff000u;
        if(!lower || lower>=0x80000000u || bytes>0x7fffffffu-lower || lower+bytes>upper ||
            tags.index!=(lower|0x80000000u))
            throw FormatError("Native map does not fit its original selected physical reservation");
        for(const auto &region:tags.regions)
            if(region.address<tags.index || std::uint64_t(region.address)+region.bytes.size()>
                std::uint64_t(tags.index)+bytes)
                throw FormatError("Native map payload escapes its header's physical reservation");
        // Preserve original12538A's allocator store before12539D protection.
        put(bank,slot,lower+bytes);
        arena->protect_pages(tags.index,bytes,4u);
        put(bank,0x55658cu,tags.index); // Actual1253AB publishes the reserved cache base.
    }
    void initialize_game_storage() {
        // Actual13A708..13A756: native subsystem composition owns the later
        // 68 startup callbacks. Preserve this source storage prefix, including
        // the source CRC and null selected-BSP short, without publishing ready.
        const auto base=word(bank,0x4f2950u),cursor=word(bank,0x4f2954u);
        const auto game=base+cursor;
        if(!base || game<base || cursor>0xffffffffu-0x1200u)
            throw FormatError("TU1.5 game-options source arena allocation wraps");
        bank.mutable_view(game,0x1200);
        put(bank,0x4f2954u,cursor+0x1200u);
        put(bank,parameters+0x100u,0x1200u);
        const std::array checksum_args{0x4f295cu};
        auto a=frame(checksum_args);a.gp[0]=parameters+0x100u;a.gp[7]=4u;
        source_run_physics_game_arena_runtime(bank,0x1656a0u,a,image,fp,services());
        for(unsigned i=0;i<0x480;++i)put(bank,game+i*4u,0);
        put(bank,0x4f3218u,game);
        const std::array none{std::byte{0xff},std::byte{0xff}};bank.write(game+2u,none);
    }
    PhysicsPlatformLockServices lock_services() {
        const auto event=[this](std::uint32_t p) {
            std::lock_guard guard(mutex);
            const auto at=lock_events.find(p);if(at!=lock_events.end())return at->second;
            const auto created=event_services.create_semaphore(0,1);
            if(created.error)throw std::system_error(created.error);
            lock_events.emplace(p,created.handle);return created.handle;
        };
        return {
            [this] {
                std::lock_guard guard(mutex);const auto id=std::this_thread::get_id();
                const auto at=thread_ids.find(id);if(at!=thread_ids.end())return at->second;
                if(next_thread_token>UINT32_MAX)throw FormatError("TU1.5 native thread-token namespace exhausted");
                const auto token=std::uint32_t(next_thread_token);thread_ids.emplace(id,token);++next_thread_token;return token;
            },
            [this,event](auto p) {acquire_native_synchronization(event_services,event(p),0xffffffffu,{},5);},
            [this,event](auto p) {
                const auto r=event_services.release_semaphore(event(p),1);
                if(r.error)throw std::system_error(r.error);
            }
        };
    }
    SourcePhysicsWorldRuntimeServices services() {
        SourcePhysicsWorldRuntimeServices s;
        s.instruction=[this](auto p) {
            if(std::this_thread::get_id()==main_thread)last_instruction=p;
            if(first_person_phase_active && std::this_thread::get_id()==main_thread)
                first_person_phase_trace.push_back(p);
            if(weapon_effect_phase_active && std::this_thread::get_id()==main_thread)
                weapon_effect_phase_trace.push_back(p);

            if(p==0x33b595u) {
                put(bank,native_tick_count,clocks->milliseconds()+tick_adjustment);
            }
        };
#if defined(__i386__) || defined(__x86_64__)
        s.timestamp_counter=[this] {
            const auto value=clocks->cycles()+cycle_adjustment;
            if(first_person_phase_active)first_person_phase_cpu.push_back({1,0,value});
            if(weapon_effect_phase_active)weapon_effect_phase_cpu.push_back({1,0,value});
            return value;
        };
        // Explicit host CPU estimate policy; Xbox estimate bits remain a
        // separate hardware comparison. The original Newton steps still run.
        s.reciprocal_sqrt=[this](std::uint32_t bits) {
            const auto value=std::bit_cast<std::uint32_t>(_mm_cvtss_f32(
                _mm_rsqrt_ss(_mm_set_ss(std::bit_cast<float>(bits)))));
            if(first_person_phase_active)first_person_phase_cpu.push_back({2,bits,value});
            if(weapon_effect_phase_active)weapon_effect_phase_cpu.push_back({2,bits,value});
            return value;
        };
#endif
        s.required_owner=[this](auto&m,const auto&c,auto&a,const auto&o){required(m,c,a,o);};
        return s;
    }
    std::uint32_t retain_resource_read(NativeResourceRead read) {
        if(next_resource_read>UINT32_MAX)throw FormatError("TU1.5 native resource-read namespace exhausted");
        const auto job=std::uint32_t(next_resource_read);
        if(!resource_reads.emplace(job,read).second)throw FormatError("TU1.5 native resource-read alias reused");
        ++next_resource_read;return job;
    }
    void required(SourcePhysicsContactMemory&m,const SourcePhysicsContactRuntimeCall&c,
        SourcePhysicsWorldRuntimeAbi&a,const SourcePhysicsContactRecordObserver&o) {
        if(&m!=&bank)throw std::runtime_error("TU1.5 live world bank owner differs");
        if(c.target==0x1d9080u && c.instruction!=c.target && !c.tail && body_velocity_input_observer)
            body_velocity_input_observer(c.instruction,a,bank);
        if(c.target==0x1da750u && c.instruction!=c.target && !c.tail && phantom_phase_observer) {
            const auto observer=phantom_phase_observer;
            observer(true,a,bank);
            required(m,{c.target,c.target,false},a,o);
            observer(false,a,bank);return;
        }
        if(c.instruction==0x17d8dau && c.target==0xd91d0u && !c.tail && effect_damage_observer) {
            const auto sp=a.gp[4],effect=a.gp[3],part=a.gp[5];
            const auto pool=word(m,0x4f720cu),rows=word(m,pool+0x44u);
            if(word(m,sp)!=0x17d8dfu || word(m,sp+8u)!=0xffffffffu ||
                word(m,part+8u)!=0x6a707421u || word(m,pool+0x24u)!=400u ||
                effect<rows || (effect-rows)%400u || (effect-rows)/400u>=256u)
                throw FormatError("TU1.5 effect damage differs from its actual part caller");
            const auto index=(effect-rows)/400u,instance=((word(m,effect)&65535u)<<16u)|index;
            if(record(pool,instance)!=effect || !(word(m,word(m,pool+0x48u)+(index/32u)*4u)&(1u<<(index%32u))))
                throw FormatError("TU1.5 effect damage lost its actual effect allocation");
            const SourceEffectDamageCallTu15 event{word(m,word(m,0x51df54u)+8u),
                instance,word(m,effect+4u),word(m,effect+0x48u),word(m,part+0x10u),word(m,sp+4u)};
            effect_damage_observer(true,event,bank);
            required(m,{c.target,c.target,false},a,o);
            effect_damage_observer(false,event,bank);return;
        }
        if(c.target==0x108a70u && c.instruction!=c.target && !c.tail && (weapon_effect_observer || weapon_effect_phase_observer)) {
            const auto sp=a.gp[4];
            if(word(m,sp)!=c.instruction+5u)
                throw FormatError("TU1.5 scheduled weapon effect lost its original caller");
            const WeaponEffectInput input{a.gp[0],a.gp[1],word(m,sp+4u),word(m,sp+8u),word(m,sp+12u)};
            const auto tag=input.effect==0xffffffffu?nullptr:cache_owner.query_tag({.handle=TagHandle{input.effect}});
            if(tag && tag->group=="effe") {
                const auto body=object_body(input.weapon),unit=word(m,body+0x14u);
                SourceWeaponEffectEventTu15 event;event.tick=word(m,word(m,0x51df54u)+8u)+1u;
                event.unit=unit;event.input=input;
                // BC200 resolves inherited parent motion using private native
                // scratch. Observing this request cannot write gameplay stack
                // bytes, alter the outer ABI or enter another live call scope.
                event.velocity=source_object_linear_velocity_read_only_tu15(bank,input.weapon,fp);
                const SourcePhysicsContactRecordObserver observed=[&](const auto &store) {
                    if(o)o(store);
                    const auto pc=store.instruction;
                    const bool ca=pc==0x17b0a3u || pc==0x17e484u || pc==0x17e4d2u || pc==0x17e58eu;
                    const bool cb=pc==0x17b0a9u || pc==0x17e48du || pc==0x17e4dbu || pc==0x17e594u;
                    if((!ca && !cb) || store.written.size()!=4)return;
                    const auto color=Bytes(store.written).u32(0);
                    (ca?event.presentation.colors.a:event.presentation.colors.b)=color;
                    event.presentation.colors_observed=true;
                };
                const bool capture=weapon_effect_phase_observer && !weapon_effect_phase_active;
                if(capture) {
                    weapon_effect_phase_trace.clear();weapon_effect_phase_cpu.clear();
                    weapon_effect_phase_observer(true,a,bank,{},{});weapon_effect_phase_active=true;
                }
                try {required(m,{c.target,c.target,false},a,observed);}
                catch(...) {if(capture)weapon_effect_phase_active=false;throw;}
                if(capture) {
                    weapon_effect_phase_active=false;
                    weapon_effect_phase_observer(false,a,bank,weapon_effect_phase_trace,weapon_effect_phase_cpu);
                }
                event.presentation.instance=a.gp[0];
                if(a.gp[0]!=0xffffffffu) {
                    const auto effect=record(word(m,0x4f720cu),a.gp[0]);
                    if(!effect || word(m,effect+4u)!=input.effect)
                        throw FormatError("Scheduled TU1.5 weapon effect lost its completed allocation");
                    event.presentation.colors={word(m,effect+0x104u),word(m,effect+0x108u)};
                    event.presentation.colors_observed=true;
                }
                if(weapon_effect_observer)weapon_effect_observer(event);return;
            }
        }
        if(c.instruction==0x1d94ceu && c.target==0x324f80u && !c.tail && body_impulse_observer) {
            const auto sp=a.gp[4],list=a.gp[6],index=a.gp[7],motion=a.gp[1];
            if(word(m,sp)!=0x1d94d1u || index>=0x800u ||
                word(m,word(m,list+index*0x60u+0x40u)+0x3cu)!=motion)
                throw FormatError("TU1.5 point impulse differs from its original rigid-body caller");
            const auto pool=word(m,0x52be5cu),rows=word(m,pool+0x44u),stride=word(m,pool+0x24u);
            if(stride!=0xa0u)throw FormatError("TU1.5 point impulse physical-component stride differs");
            std::uint32_t object=0xffffffffu;
            for(unsigned i=0;i<word(m,pool+0x38u);++i) {
                if(!(word(m,word(m,pool+0x48u)+(i>>5)*4u)&(1u<<(i&31u))))continue;
                const auto component=rows+i*stride;
                if(word(m,component+0x70u)==list && index<word(m,component+0x74u)) {
                    if(object!=0xffffffffu)throw FormatError("TU1.5 point impulse has ambiguous component ownership");
                    object=word(m,component+8u);
                }
            }
            if(object==0xffffffffu)throw FormatError("TU1.5 point impulse lacks its actual object component");
            (void)object_body(object);
            SourceBodyImpulseTu15 event{object,word(m,word(m,0x51df54u)+8u),index,{},{},{},{}};
            m.read(motion,event.before);m.read(word(m,sp+4u),event.impulse);m.read(word(m,sp+8u),event.point);
            required(m,{c.target,c.target,false},a,o);
            m.read(motion,event.after);body_impulse_observer(event);return;
        }

        if(c.instruction==0x18cefcu && c.target==0x223b20u && !c.tail && sound_submit_observer) {
            const auto sp=a.gp[4],request=a.gp[0],sound=word(m,sp+4u);
            if(word(m,sp)!=0x18cf01u)throw FormatError("Original sound submit caller differs");
            (void)bank.view(request,160);
            const bool was_active=first_person_phase_active;
            if(!was_active){first_person_phase_trace.clear();first_person_phase_cpu.clear();}
            const auto trace_begin=first_person_phase_trace.size(),cpu_begin=first_person_phase_cpu.size();
            sound_submit_observer(true,sound,request,a,bank,{},{});
            // Forward the full unchanged request/ABI into the complete source
            // admission owner. Its real result, including NONE, remains final.
            first_person_phase_active=true;
            try {required(m,{c.target,c.target,false},a,o);}catch(...) {first_person_phase_active=was_active;throw;}
            first_person_phase_active=was_active;
            sound_submit_observer(false,sound,request,a,bank,
                std::span(first_person_phase_trace).subspan(trace_begin),
                std::span(first_person_phase_cpu).subspan(cpu_begin));return;
        }
        if((c.instruction==0x1a9da2u || c.instruction==0x1a82abu) && c.target==0x33a4a6u && !c.tail) {
            if(word(m,a.gp[4])!=c.instruction+5u)
                throw FormatError("TU1.5 native language caller differs");
            // Current native interface language is English (map language0;
            // campaign_text.cpp uses the same explicit supported locale).
            // XGetLanguage's raw English code is1. Original11F440 performs
            // its own conversion and1A9D90 owns the cached language field.
            // This is a native platform input, not Xbox locale discovery.
            a.gp[0]=1;a.gp[4]+=4u;return;
        }
        if(c.instruction==0x183e17u && c.target==0x439c0u && !c.tail) {
            source_native_cache_vertex_view_tu15(m,a);return;
        }
        if(c.instruction==0x1d2eau && c.target==0x407c00u && !c.tail) {
            source_native_cache_bitmap_view_tu15(m,a,fp,services(),o);return;
        }
        if((c.instruction==0xd8889u || c.instruction==0xd9016u || c.instruction==0xdb8b9u) && c.target==0x179610u && !c.tail) {
            const auto sp=a.gp[4],target=a.gp[2],owner=a.gp[3];
            if(word(m,sp)!=c.instruction+5u)
                throw FormatError("TU1.5 shield effect request caller differs");
            (void)object_body(target);
            UnitDamageEffectEvent event;event.target=target;
            event.effect={c.instruction==0xd9016u?UnitDamageEffectKind::ShieldDepleted:
                c.instruction==0xd8889u?UnitDamageEffectKind::ShieldRecharging:
                UnitDamageEffectKind::ShieldDamaged,word(m,sp+4u)};
            if(owner)event.owner={word(m,owner),word(m,owner+4u),
                std::bit_cast<std::int16_t>(std::uint16_t(word(m,owner+8u)))};
            // Run the complete original attached-effect allocator before
            // publishing its actual request, including an authored NONE.
            required(m,{c.target,c.target,false},a,o);
            if(a.gp[0]!=0xffffffffu) {
                const auto effect=record(word(m,0x4f720cu),a.gp[0]);
                if(!effect || word(m,effect+4u)!=event.effect.datum)
                    throw FormatError("TU1.5 shield effect lost its completed allocation");
                event.colors={word(m,effect+0x104u),word(m,effect+0x108u)};
                event.controls={std::bit_cast<float>(word(m,effect+0x6cu)),std::bit_cast<float>(word(m,effect+0x70u))};
            }
            unit_damage_effects.push_back(event);return;
        }
        if(c.instruction==0xd1fbfu && c.target==0xd13d0u && !c.tail) {
            const auto sp=a.gp[4],unit=word(m,sp+4u),body=object_body(unit);
            if(word(m,sp)!=0xd1fc4u)throw FormatError("TU1.5 melee contact caller differs");
            const auto selected=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(bank.view(body+0x212u,1)[0]));
            std::uint32_t weapon=0xffffffffu;
            if(selected>=0 && selected<4) {
                const auto carried=word(m,body+0x218u+std::uint32_t(selected)*4u);
                if(carried!=0xffffffffu)weapon=word(m,object_body(carried));
            }
            auto root=body;
            for(unsigned depth=0;word(m,root+0x14u)!=0xffffffffu;++depth) {
                if(depth>=0x800u)throw FormatError("TU1.5 melee root parent chain cycles");
                root=object_body(word(m,root+0x14u));
            }
            const auto scalar=[&](auto p){return std::bit_cast<float>(word(m,p));};
            const auto vector=[&](auto p){return Vec3{scalar(p),scalar(p+4u),scalar(p+8u)};};
            MeleeImpact event{0,0,unit,weapon,word(m,sp+8u),vector(word(m,sp+0x10u)),
                vector(word(m,sp+0x14u)),vector(root+0x70u),vector(root+0x64u),
                std::bit_cast<std::int16_t>(std::uint16_t(word(m,sp+0xcu)))};
            source_run_combat_action_tu15(m,c.target,a,fp,services(),o);
            melee_impacts.push_back(event);return;
        }
        if(c.instruction==0x101db8u && c.target==0x178e30u && !c.tail) {
            //101BC0 preserves its101330 caller's EBP projectile body. Its
            // source-selected effect and four named locations are already
            // complete at this literal call. Observe them without altering
            // the retained ABI, then execute the full original effect owner.
            const auto sp=a.gp[4],request=word(m,sp+4u),body=a.gp[5];
            if(word(m,sp)!=0x101dbdu || word(m,sp+0xd8u)!=0x101a07u ||
                word(m,request)!=4u || word(m,request+0x20u)!=4u)
                throw FormatError("TU1.5 projectile explosion effect caller differs");
            const auto pool=word(m,0x4ecbd0u),rows=word(m,pool+0x44u);
            if(word(m,pool+0x24u)!=12u)throw FormatError("TU1.5 explosion object pool stride differs");
            std::uint32_t projectile=0xffffffffu;
            for(unsigned i=0;i<word(m,pool+0x38u);++i) {
                if(!(word(m,word(m,pool+0x48u)+(i>>5)*4u)&(1u<<(i&31u))))continue;
                const auto header=rows+i*12u;
                if(word(m,header+8u)==body && bank.view(header+3u,1)[0]==std::byte{5}) {
                    if(projectile!=0xffffffffu)throw FormatError("TU1.5 explosion body has multiple live identities");
                    projectile=((word(m,header)&65535u)<<16)|i;
                }
            }
            if(projectile==0xffffffffu || !(word(m,body+0x12cu)&0x1000u))
                throw FormatError("TU1.5 explosion request has no detonating source projectile");
            SourceProjectileDetonationTu15 event{projectile,word(m,body),word(m,request+4u),
                0xffffffffu,word(m,body+0xc4u),word(m,body+0xc8u),{},{},{}};
            m.read(request,event.request);m.read(word(m,request+0x1cu),event.request_locations);
            required(m,{c.target,c.target,false},a,o);
            event.effect_instance=a.gp[0];
            if(event.effect_instance!=0xffffffffu) {
                const auto effect=record(word(m,0x4f720cu),event.effect_instance);
                if(!effect || word(m,effect+4u)!=event.effect)
                    throw FormatError("TU1.5 completed explosion lost its actual effect identity");
                event.colors={word(m,effect+0x104u),word(m,effect+0x108u)};
                event.controls={std::bit_cast<float>(word(m,effect+0x6cu)),std::bit_cast<float>(word(m,effect+0x70u))};
                const auto tag_rows=word(m,0x4f0414u);
                const auto definition=word(m,tag_rows+(event.effect&65535u)*16u+8u);
                const auto locations=word(m,definition+0xcu);
                if(locations>32u)throw FormatError("TU1.5 explosion effect exceeds its original location bank");
                const auto scalar=[&](auto p){return std::bit_cast<float>(word(m,p));};
                const auto vector=[&](auto p){return Vec3{scalar(p),scalar(p+4u),scalar(p+8u)};};
                for(unsigned i=0;i<locations;++i) {
                    const auto datum=word(m,effect+0x7cu+i*4u);
                    if(datum==0xffffffffu)continue;
                    const auto location=record(word(m,0x4f7208u),datum);
                    if(!location)throw FormatError("TU1.5 explosion effect lost a retained location");
                    Transform frame;frame.scale=scalar(location+8u);
                    frame.basis={vector(location+0xcu),vector(location+0x18u),vector(location+0x24u)};
                    frame.position=vector(location+0x30u);
                    const auto name=word(m,word(m,definition+0x10u)+i*4u);
                    event.effect_locations.push_back({name,frame});
                }
            }
            projectile_detonations.push_back(std::move(event));return;
        }
        if(source_try_native_process_heap_memory_call(bank,c,a,image,virtual_memory))return;
        if(source_try_native_physics_lock_call(m,c,a,image,locks,lock_services()))return;
        if(pool_storage.try_call(m,c,a))return;
        if(c.instruction==0x1301b0u && c.target==0x1a3b70u && !c.tail) {
            // Actual1300B0 has allocated the original texture cache block and
            // produced its hardware descriptor. Native file I/O fills that
            // block; Vulkan presentation owns its separate GPU resources.
            const auto sp=a.gp[4],tracking=a.gp[6];
            const auto descriptor=word(m,tracking+0x10u),level=word(m,tracking+4u);
            const auto size=word(m,sp+4u),destination=word(m,sp+8u);
            const auto pool=word(m,0x4f2d24u),rows=word(m,pool+0x44u);
            if(std::this_thread::get_id()!=main_thread || word(m,sp)!=0x1301b5u ||
                level>=3u || tracking<rows || (tracking-rows)%0x28u ||
                (tracking-rows)/0x28u>=word(m,pool+0x38u) ||
                word(m,sp+12u)!=0 || word(m,sp+16u)!=tracking+3u || word(m,sp+20u)!=3u ||
                a.gp[1]!=word(m,descriptor+0x1cu+level*4u) ||
                size!=((word(m,descriptor+0x34u+level*4u)+511u)&~511u) ||
                destination!=word(m,descriptor+0x40u+level*4u))
                throw FormatError("TU1.5 texture resource I/O differs from its actual source cache block");
            const auto bytes=resources.load(a.gp[1],size);
            bank.write(destination,bytes);
            const auto job=retain_resource_read({descriptor,destination,size});
            const std::array done{std::byte{1}};bank.write(tracking+3u,done);
            a.gp[0]=job;a.gp[4]+=28u;return;
        }
        if(c.instruction==0x130f68u && c.target==0x1a3b70u && !c.tail) {
            // Complete130EB0 has reserved its actual geometry-cache pages and
            // allocated the tracking record. This native I/O boundary reads
            // those real cache sectors, then publishes completion at the
            // exact caller-supplied byte. No GPU/residency result is supplied.
            const auto sp=a.gp[4],tracking=a.gp[6],descriptor=a.gp[5];
            const auto pool=word(m,0x4f2d5cu),rows=word(m,pool+0x44u);
            const auto cache=word(m,0x4f2d6cu),pages=word(m,cache+0x64u);
            const auto token=word(m,descriptor+0x20u),index=token&65535u;
            const auto page=word(m,pages+0x44u)+index*0x18u;
            const auto shift=word(m,cache+0x34u)&31u;
            const auto expected=word(m,0x4f2d64u)+(word(m,page+8u)<<shift);
            const auto size=word(m,sp+4u),destination=word(m,sp+8u);
            if(std::this_thread::get_id()!=main_thread || word(m,sp)!=0x130f6du ||
                tracking<rows || (tracking-rows)%12u ||
                (tracking-rows)/12u>=word(m,pool+0x38u) ||
                word(m,tracking+8u)!=descriptor || token==0xffffffffu ||
                index>=word(m,pages+0x38u) || destination!=expected ||
                a.gp[1]!=word(m,descriptor) || size!=((word(m,descriptor+4u)+511u)&~511u) ||
                word(m,sp+12u)!=0 || word(m,sp+16u)!=tracking+2u ||
                word(m,sp+20u)!=4u || word(m,sp+24u)!=a.gp[3])
                throw FormatError("TU1.5 geometry resource I/O differs from its actual source cache allocation");
            const auto bytes=resources.load(a.gp[1],size);
            bank.write(destination,bytes);
            const auto job=retain_resource_read({descriptor,destination,size});
            const std::array done{std::byte{1}};bank.write(tracking+2u,done);
            a.gp[0]=job;a.gp[4]+=28u;return;
        }
        if(c.instruction==0x126139u && c.target==0x1a3b70u && !c.tail) {
            // Explicit native file-I/O boundary of the original1260C0 cache
            // owner. It has already allocated the real cache block and job
            // tracking record. Deliver actual sector bytes, retain a native
            // completed request identity, and signal its I/O completion only
            // after the read and destination write succeed. Xbox async queue
            // scheduling is not asserted by this synchronous native policy.
            const auto sp=a.gp[4],tracking=a.gp[7];
            const auto descriptor=word(m,tracking+0xcu),size=word(m,sp+4),destination=word(m,sp+8);
            const auto resource=resources.animation(descriptor);
            if(std::this_thread::get_id()!=main_thread || word(m,sp)!=0x12613eu ||
                word(m,sp+12)!=0 || word(m,sp+16)!=tracking+4u || word(m,sp+20)!=6u ||
                !std::ranges::equal(bank.view(descriptor,12),std::span(resource.descriptor).first(12)) ||
                a.gp[1]!=word(m,descriptor+8u) || size!=((resource.bytes.size()+511u)&~511u))
                throw FormatError("TU1.5 animation resource I/O differs from its actual cache descriptor");
            auto bytes=resources.load(a.gp[1],size);
            bank.write(destination,bytes);
            const auto job=retain_resource_read({descriptor,destination,size});
            const std::array done{std::byte{1}};bank.write(tracking+4u,done);
            a.gp[0]=job;a.gp[4]+=28;return;
        }
        // Keep the existing external XAPI wrapper's platform/correspondence
        // owner. The special module calls its own complete wrapper internally
        // only from the authenticated2131F9 sound-bank constructor.
        if(dsp_section && dsp_section->dispatch(m,c,a))return;
        if(sound_sdk_resources && sound_sdk_resources->dispatch(m,c,a,o))return;
        if(source_sound_sdk_tu15_owns_entry(c.target)) {
            source_run_sound_sdk_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_sound_startup_tu15_owns_entry(c.target)) {
            source_run_sound_startup_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(c.target!=0x33b3acu && source_sandbox_special_tu15_owns_entry(c.target)) {
            source_run_sandbox_special_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_player_departure_tu15_owns_entry(c.target)) {
            source_run_player_departure_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_contrail_geometry_tu15_owns_entry(c.target)) {
            source_run_contrail_geometry_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_combat_action_tu15_owns_entry(c.target)) {
            source_run_combat_action_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_death_cycle_runtime_tu15_owns_entry(c.target)) {
            source_run_death_cycle_runtime_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_map_world_support_tu15_owns_entry(c.target)) {
            source_run_map_world_support_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_scene_device_constructor_tu15_owns_entry(c.target)) {
            source_run_scene_device_constructor_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_model_render_tu15_owns_entry(c.target)) {
            source_run_model_render_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_collision_instance_storage_tu15_owns_entry(c.target)) {
            source_run_collision_instance_storage_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_vehicle_occupancy_tu15_owns_entry(c.target)) {
            source_run_vehicle_occupancy_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_collision_query_tu15_owns_entry(c.target)) {
            source_run_collision_query_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_physics_deactivation_tu15_owns_entry(c.target)) {
            source_run_physics_deactivation_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_object_sound_tu15_owns_entry(c.target)) {
            source_run_object_sound_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_physics_live_support_tu15_owns_entry(c.target)) {
            source_run_physics_live_support_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_particle_swarm_tu15_owns_entry(c.target)) {
            source_run_particle_swarm_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_effect_particle_frame_tu15_owns_entry(c.target)) {
            source_run_effect_particle_frame_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_effect_particle_draw_tu15_owns_entry(c.target)) {
            source_run_effect_particle_draw_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_state_context_startup_tu15_owns_entry(c.target)) {
            source_run_state_context_startup_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_ai_actor_tu15_owns_entry(c.target)) {
            source_run_ai_actor_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_ai_frame_tu15_owns_entry(c.target)) {
            source_run_ai_frame_tu15(m,c.target,a,fp,services(),o);return;
        }
        const auto sp=a.gp[4];
        if(c.instruction==0x33a4d3u && c.target==0x80000080u && c.tail) {
            // NtQuerySystemTime receives the actual32BBAE local output cell.
            // Native UTC is converted to the original 100ns epoch; the real
            // CRT time conversion, rand and stream seed instructions execute.
            if(std::this_thread::get_id()!=main_thread || word(m,sp)!=0x32bbbcu ||
                word(m,sp+4u)!=a.gp[5]-8u)
                throw FormatError("TU1.5 native system-time caller differs");
            const auto ticks=clocks->utc_filetime();
            const auto output=word(m,sp+4u);
            put(m,output,std::uint32_t(ticks));put(m,output+4u,std::uint32_t(ticks>>32u));
            a.gp[4]+=8u;return;
        }
        if(c.instruction==0x2db4d1u && c.target==0x800000b6u && c.tail) {
            if(word(m,0x41b648u)!=c.target || !arena)
                throw FormatError("TU1.5 resource-cache physical protection import differs");
            arena->protect_pages(word(m,sp+4),word(m,sp+8),word(m,sp+12));
            a.gp[4]+=16;return;
        }
        if((c.instruction==0x381389u || c.instruction==0x3835ffu) &&
            c.target==0x32b0b4u && !c.tail) {
            const auto callback=c.instruction==0x381389u?0x383970u:0x383c90u;
            if(word(m,sp)!=c.instruction+5u || word(m,sp+4)!=callback)
                throw FormatError("TU1.5 Havok native exit registration frame differs");
            native_exits.callbacks.push_back(callback);
            // A native C runtime registration: actual retained callback and
            // failure propagation, not a claim that the Xbox CRT table ran.
            a.gp[0]=0;a.gp[4]+=4;return;
        }
        if(c.instruction==0x1255e7u && c.target==0x125870u && !c.tail) {
            if(std::this_thread::get_id()!=main_thread || word(m,sp)!=0x1255ecu ||
                word(m,sp+4)!=0xffffffffu || word(m,sp+8)!=bsp.file_offset ||
                a.gp[0]!=bsp.file_offset || a.gp[2]!=bsp.region.address ||
                a.gp[6]!=((bsp.region.bytes.size()+511u)&~511u))
                throw FormatError("TU1.5 BSP I/O frame differs from the actual cache resource");
            source_deliver_runtime_cache_bsp_tu15(bank,bsp);
            a.gp[0]=(a.gp[0]&0xffffff00u)|1u;a.gp[4]+=12;return;
        }
        // Actual fresh-segment and retained-segment commitment frames in the
        // unchanged TU1.5 support bodies. This is the explicit native resource
        // ABI: source owns all free lists, headers, splitting and publication.
        if(c.instruction==0x33b3bfu && c.target==0x800000b8u) {
            // Whole33B3AC is the shared XAPI raw allocation wrapper. Its
            // original callers include the sound bit bank and four-byte
            // decal storage. Preserve the actual requested size; only host
            // page/address selection belongs to this native resource boundary.
            const auto bp=word(m,sp+4u),np=word(m,sp+12u),frame_=a.gp[5];
            if(c.tail || word(m,sp)!=0x33b3c5u || bp!=frame_+8u || np!=frame_+0xcu ||
                word(m,bp)!=0 || !word(m,np) || word(m,sp+8u)!=0 ||
                word(m,sp+16u)!=0x101000u || word(m,sp+20u)!=4u)
                throw FormatError("TU1.5 raw virtual-memory request differs from its original XAPI wrapper");
            const auto requested=word(m,np);
            const auto reservation=virtual_memory.allocate({0,0,requested,0x2000u,4u});
            if(reservation.status) {a.gp[0]=reservation.status;a.gp[4]+=24u;return;}
            const auto result=virtual_memory.allocate({reservation.base,0,reservation.bytes,0x1000u,4u});
            put(m,bp,result.base);put(m,np,result.bytes);a.gp[0]=result.status;a.gp[4]+=24u;return;
        }
        struct VmFrame {std::uint32_t site,ret;int base,size;std::uint32_t type;};
        constexpr std::array vm_frames{
            VmFrame{0x2dd3d8u,0x2dd3deu,0xc,0,0x1000u},
            VmFrame{0x2ddf6fu,0x2ddf71u,-4,8,0x2000u},
            VmFrame{0x2ddf95u,0x2ddf97u,-4,8,0x2000u},
            VmFrame{0x2ddfcau,0x2ddfccu,-4,-0x10,0x1000u}};
        for(const auto&f:vm_frames)if(c.instruction==f.site && c.target==0x800000b8u) {
            const auto bp=word(m,sp+4),np=word(m,sp+12);
            const auto expected_size=f.size ? a.gp[5]+std::uint32_t(f.size) : a.gp[3];
            if(c.tail || word(m,sp)!=f.ret || bp!=a.gp[5]+std::uint32_t(f.base) ||
                np!=expected_size || word(m,sp+8)!=0 || word(m,sp+16)!=f.type || word(m,sp+20)!=4)
                throw std::runtime_error("TU1.5 live process growth resource frame differs");
            const auto result=virtual_memory.allocate({word(m,bp),0,word(m,np),f.type,4});
            put(m,bp,result.base);put(m,np,result.bytes);a.gp[0]=result.status;a.gp[4]+=24;return;
        }
        for(const auto&e:physics_live_entries_tu15::entries)if(c.target==e.actual) {
            using Module=physics_live_entries_tu15::Module;
            switch(e.module) {
            case Module::Damage:source_run_damage_delivery_runtime(m,e.canonical,a,image,fp,services(),o);return;
            case Module::World:source_run_physics_world_runtime(m,e.canonical,a,image,fp,services(),o);return;
            case Module::Pool:
                // The generated module already reports the selected image's
                // instruction PC. Mapping it again treats a TU1.5 PC as TU1.1.
                source_run_physics_pool_startup_runtime(m,e.canonical,a,image,fp,services(),o);return;
            case Module::Arena:source_run_physics_game_arena_runtime(m,e.canonical,a,image,fp,services(),o);return;
            case Module::MainHeap:source_run_physics_main_heap_runtime(m,e.canonical,a,image,fp,services(),o);return;
            case Module::Early:source_run_physics_early_initializer_runtime(m,e.canonical,a,image,fp,services(),o);return;
            case Module::Renderer:source_run_physics_renderer_storage_runtime(m,e.canonical,a,image,fp,services(),o);return;
            case Module::Process:
                source_run_process_heap_with_native_platform(bank,e.canonical,a,image,fp,
                    services(),virtual_memory,locks,lock_services(),o);return;
            case Module::Phantom: {
                auto control=a.floating_point.value_or(fp);
                source_run_physics_phantom_world_tu15_runtime(m,e.actual,a,control,services(),o);
                a.floating_point=control;return;
            }
            }
        }
        if(c.instruction==0x2db3b0u && c.target==allocation_callback && !c.tail) {
            if(std::this_thread::get_id()!=main_thread || word(m,sp)!=0x2dee63u || word(m,sp+4)!=10u)
                throw std::runtime_error("TU1.5 native allocation notification frame differs");
            const auto p=word(m,sp+8);allocations.push_back({word(m,p),word(m,p+4),word(m,p+8)});
            a.gp[4]+=12;return;
        }
        std::ostringstream message;message<<"TU1.5 live world owner missing at "<<std::hex
            <<c.instruction<<" -> "<<c.target<<" return "<<word(m,sp);
        throw std::runtime_error(message.str());
    }
    SourcePhysicsWorldRuntimeAbi frame(std::span<const std::uint32_t>args) {
        require_executor();
        if(std::this_thread::get_id()!=main_thread)
            throw std::runtime_error("TU1.5 live world caller thread differs");
        SourcePhysicsWorldRuntimeAbi a{};a.eflags=2;a.fs_base=tls;a.gp[4]=stack;
        a.floating_point=fp;put(bank,stack,0x60300000u);
        if(args.size()>0x1000)throw std::runtime_error("TU1.5 live call arguments exceed native frame");
        for(unsigned i=0;i<args.size();++i)put(bank,stack+4+i*4,args[i]);return a;
    }
    void run(std::uint32_t entry,SourcePhysicsWorldRuntimeAbi&a,
        const SourcePhysicsContactRecordObserver&o={}) {
        Execution execution(*this);
        try {
            if(entry==0x133760u || entry==0x1360f0u || entry==0x165740u)
                source_run_physics_renderer_storage_with_native_pages(bank,entry,a,fp,services(),pool_storage,o);
            else if(entry==0x1c5f00u || entry==0x1f15b0u)
                source_run_physics_early_initializer_with_native_storage(bank,entry,a,fp,services(),pool_storage,o);
            else source_run_physics_world_runtime(bank,entry,a,image,fp,services(),o);
        }catch(const std::exception&e) {
            std::ostringstream message;message<<"TU1.5 live root "<<std::hex<<entry
                <<" at "<<last_instruction<<": "<<e.what();
            throw std::runtime_error(message.str());
        }
    }
    std::uint32_t call(std::uint32_t entry,std::span<const std::uint32_t>args={}) {
        auto a=frame(args);run(entry,a);return a.gp[0];
    }
    std::uint32_t call_actual(std::uint32_t entry,std::span<const std::uint32_t>args={}) {
        auto a=frame(args);
        run_actual(entry,a);return a.gp[0];
    }
    void run_actual(std::uint32_t entry,SourcePhysicsWorldRuntimeAbi&a,
                    const SourcePhysicsContactRecordObserver&o={}) {
        Execution execution(*this);
        if(std::this_thread::get_id()!=main_thread)
            throw FormatError("TU1.5 live entry requires its retained executor thread");
        try {
            required(bank,{entry,entry,false},a,o?o:physics_store_observer);
        }catch(const std::exception&e) {
            std::ostringstream message;message<<"TU1.5 live actual root "<<std::hex<<entry
                <<" at "<<last_instruction<<": "<<e.what();
            message<<" [gp";
            for(const auto value:a.gp)message<<' '<<value;
            message<<']';
            if(a.gp[4]>=0x60000000u && a.gp[4]<=0x600fffc0u) {
                message<<" [stack";
                for(unsigned i=0;i<8;++i)message<<' '<<word(bank,a.gp[4]+i*4u);
                message<<']';
            }
            throw std::runtime_error(message.str());
        }
    }
    std::uint32_t record(std::uint32_t pool,std::uint32_t datum) const {
        SourceRawDatumPoolServices storage{.read=[this](auto p,auto n){return bank.view(p,n);}};
        return source_raw_datum_lookup(pool,datum,storage);
    }
    std::uint32_t object_body(std::uint32_t datum) const {
        const auto header=record(word(bank,0x4ecbd0u),datum);
        if(!header)throw FormatError("TU1.5 body requires a live full source object identity");
        const auto body=word(bank,header+8u);
        if(!body)throw FormatError("TU1.5 object has no source-created body");
        return body;
    }
    std::optional<DeathCameraPacket> vehicle_camera_packet(unsigned view,
        std::uint32_t unit,float yaw,float pitch,std::array<std::uint32_t,7> &state) {
        if(view>=4 || !std::isfinite(yaw) || !std::isfinite(pitch))
            throw FormatError("TU1.5 vehicle camera requires an actual native local view and finite input");
        constexpr std::uint32_t mode=0x60034000u,producer=0x60034020u,
            input=0x60034040u,output=0x60034100u;
        //158340 owns admission, including boarding/exit animation modes.
        const std::array mode_args{mode};auto a=frame(mode_args);a.gp[3]=unit;
        run_actual(0x158340u,a);if(!(a.gp[0]&0xffffu))return {};
        // Construct/bind the original input record without assigning a source
        // player to a local user. Native presentation owns that association.
        a=frame({});a.gp[0]=view;a.gp[1]=unit;run_actual(0x18a0c0u,a);
        const auto record=word(bank,0x4f9b54u)+0x14u+view*0x94u;
        // Native mouse/gamepad yaw/pitch are delivered at the same retained
        // input fields read at24604D/246062, after their source constructor.
        put(bank,record+0x10u,std::bit_cast<std::uint32_t>(yaw));
        put(bank,record+0x14u,std::bit_cast<std::uint32_t>(pitch));
        std::array<std::uint32_t,8> controls{};controls[0]=view;
        DeathCameraPacket packet{};
        bank.write(producer,std::as_bytes(std::span(state)));
        bank.write(input,std::as_bytes(std::span(controls)));
        bank.write(output,std::as_bytes(std::span(packet)));
        const std::array args{producer,input,output};call_actual(0x245ea0u,args);
        bank.read(producer,std::as_writable_bytes(std::span(state)));
        bank.read(output,std::as_writable_bytes(std::span(packet)));
        return packet;
    }
    void publish_roster_team(unsigned user,std::int8_t team) {
        if(!session_configured || user>=16 || team<0 || team>=8)
            throw FormatError("TU1.5 team roster input requires its admitted session and slot");
        const auto game=word(bank,0x4f3218u);
        if(std::to_integer<unsigned>(bank.view(game+0x184u,1)[0])&1u) {
            // Native roster inputs consumed by original15A2C0: active at
            // options+2DC and signed team at+374, with E4 bytes per slot.
            // Original15A1A0 produces the mask admitted by14E7B0 itself.
            const auto slot=game+0x2dcu+user*0xe4u;
            const std::array active{std::byte{1}},team_input{std::byte(std::uint8_t(team))};
            bank.write(slot,active);bank.write(slot+0x98u,team_input);
            call_actual(0x15a2c0u);call_actual(0x15a470u);
        }
    }
    void construct_player(std::uint32_t player,unsigned user,
        const SourcePlayerConfiguration&configuration,const SourceMachineIdentity&machine) {
        const auto pool=word(bank,0x4f54f4u);
        if(!session_configured || !pool || user>=word(bank,pool+0x20u) || !record(pool,player))
            throw FormatError("TU1.5 player construction requires an allocated source player and native roster identity ordinal");
        constexpr std::uint32_t creation=0x60020000u;
        std::array<std::byte,0xac> input{};
        input[2]=input[3]=std::byte{0xff};
        std::fill_n(input.begin()+4,4,std::byte{0xff});
        const auto identity=source_offline_player_identity(machine,std::int32_t(user));
        for(unsigned i=0;i<machine.size();++i)input[8u+i]=std::byte(machine[i]);
        for(unsigned i=0;i<identity.size();++i)input[0xeu+i]=std::byte(identity[i]);
        for(unsigned i=0;i<configuration.size();++i)input[0x1cu+i]=std::byte(configuration[i]);
        bank.write(creation,input);

        // The already allocated new-player continuation calls14EBE0 with
        // new-record mode, increments the separate active census and executes
        // the original engine/result join notification. Calling14EBE0 alone
        // is its record-update path and leaves that census at zero.
        auto a=frame({});a.gp[0]=creation;a.gp[6]=player;run_actual(0x14ead0u,a);
        // Literal14E798 is the new-player campaign AI notification. The
        // original owner consumes ESI's full player datum and owns its two
        // tracking slots; its game-type guard admits campaign registration.
        auto ai=frame({});ai.gp[6]=player;run_actual(0x1d03a0u,ai);
    }
    void bind_player(std::uint32_t player,std::uint32_t object) {
        if(!record(word(bank,0x4f54f4u),player))
            throw FormatError("TU1.5 player binding requires a live full source player identity");
        if(object!=0xffffffffu)object_body(object);
        const std::array args{player};auto a=frame(args);a.gp[3]=object;
        run_actual(0x14f620u,a);
    }
    bool depart_player(std::uint32_t player) {
        const auto address=record(word(bank,0x4f54f4u),player);
        if(!address)return false;
        const std::array packet{player&65535u,word(bank,address+4u),word(bank,address+8u),word(bank,address+12u)};
        // Native caller storage is the actual argument area of this owned
        // ABI frame. Original88C50 receives its packet pointer in EAX.
        auto a=frame(packet);a.gp[0]=a.gp[4]+4u;run_actual(0x88c50u,a);
        return (a.gp[0]&255u)!=0;
    }
    void set_local_player(std::uint32_t player,std::int16_t view) {
        if(!session_configured || view<-1 || view>=4 || !record(word(bank,0x4f54f4u),player))
            throw FormatError("TU1.5 local view requires a live source player and original view index");
        const auto locals=word(bank,0x4f54f0u);
        if(view>=0) {
            const auto prior=word(bank,locals+0xcu+unsigned(view)*4u);
            if(prior!=0xffffffffu && prior!=player)
                throw FormatError("TU1.5 local view still belongs to another player");
        }
        const std::array args{player,std::uint32_t(std::int32_t(view))};
        call_actual(0x151c90u,args);
    }
    FirstPersonViewBinding first_person_view(unsigned index) const {
        if(index>=4)throw FormatError("Original first-person view index exceeds its four-view bank");
        const auto bank_=word(bank,0x4f6498u);
        if(!bank_)throw FormatError("Original first-person storage was not constructed");
        const auto view=bank_+index*0x20ccu;
        FirstPersonViewBinding result;
        result.unit=word(bank,view+4u);
        result.representation=std::bit_cast<std::int32_t>(word(bank,view+8u));
        bool dual=false;
        if(result.unit!=0xffffffffu) {
            const auto body=object_body(result.unit);
            const auto slot=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(bank.view(body+0x213u,1)[0]));
            if(slot>=0) {
                if(slot>=4)throw FormatError("Original secondary hand selects an invalid inventory slot");
                dual=word(bank,body+0x218u+unsigned(slot)*4u)!=0xffffffffu;
            }
        }
        for(unsigned h=0;h<2;++h) {
            const auto p=view+0xcu+h*0x1010u;auto &hand=result.hands[h];
            hand.flags=word(bank,p);hand.weapon=word(bank,p+4u);
            hand.graph=word(bank,p+8u);hand.animation_identifier=word(bank,p+0xcu);
            hand.channel_delay=std::to_integer<std::uint8_t>(bank.view(p+0x18u,1)[0]);
            hand.channel_flags=std::to_integer<std::uint8_t>(bank.view(p+0x19u,1)[0]);
            const auto short_at=[&](unsigned offset) {return Bytes(bank.view(p+offset,2)).u16(0);};
            hand.channel_options=short_at(0x1au);hand.events=short_at(0x1cu);hand.future_events=short_at(0x1eu);
            hand.rate=std::bit_cast<float>(word(bank,p+0x20u));hand.frame=std::bit_cast<float>(word(bank,p+0x24u));
            hand.activity=word(bank,p+0x84u);hand.fire_ticks=std::bit_cast<std::int16_t>(short_at(0xf2u));
            hand.weapon_node_count=word(bank,p+0xf4u);hand.arms_node_count=word(bank,p+0xf8u);
            if(hand.weapon_node_count>64 || hand.arms_node_count>64)
                throw FormatError("Original first-person node map exceeds its authored64-node bank");
            for(unsigned n=0;n<64;++n) {
                hand.weapon_nodes[n]=std::bit_cast<std::int32_t>(word(bank,p+0xfcu+n*4u));
                hand.arms_nodes[n]=std::bit_cast<std::int32_t>(word(bank,p+0x1fcu+n*4u));
            }
            hand.dual_wield=dual;hand.secondary_hand=h!=0;
        }
        return result;
    }
    std::vector<SourceFirstPersonModelTu15> prepare_first_person_models(unsigned index,
        Vec3 position,Vec3 forward,Vec3 up,const SourceFirstPersonPhaseObserverTu15 &observer) {
        if(index>=4 || !session_configured || first_person_phase_active)
            throw FormatError("Source FP draw requires its actual available view/frame");
        for(const auto v:{position,forward,up})for(float x:{v.x,v.y,v.z})
            if(!std::isfinite(x))throw FormatError("Source FP native camera input is not finite");
        const auto binding=first_person_view(index);
        if(binding.unit==0xffffffffu)return {};
        // Literal3D313/3D31A admits the original director's first-person mode.
        auto camera_mode=frame({});camera_mode.gp[0]=index;run_actual(0x158290u,camera_mode);
        if(camera_mode.gp[0])return {};
        // Native view/camera input at original2BA58 and2BAA6..2BAAB.
        // Original168DED constructs the camera/control root and every matrix.
        put(bank,0x4c66f8u,index);
        for(const auto &[p,v]:std::array<std::pair<std::uint32_t,Vec3>,3>{{
            {0x4c65c0u,position},{0x4c65ccu,forward},{0x4c65d8u,up}}}) {
            put(bank,p,std::bit_cast<std::uint32_t>(v.x));put(bank,p+4u,std::bit_cast<std::uint32_t>(v.y));
            put(bank,p+8u,std::bit_cast<std::uint32_t>(v.z));
        }
        // This is caller scratch in the already-owned native ABI bank, not
        // a new persistent allocation or an independently constructed pose.
        constexpr std::uint32_t output=0x60050000u,capacity=4u,stride=0xd0cu;
        auto a=frame(std::array{index,binding.unit,capacity,output});
        if(observer) {
            first_person_phase_trace.clear();first_person_phase_cpu.clear();observer(true,a,bank,{},{});
            first_person_phase_active=true;
            try {run_actual(0x168dedu,a);}catch(...) {first_person_phase_active=false;throw;}
            first_person_phase_active=false;
            observer(false,a,bank,first_person_phase_trace,first_person_phase_cpu);
        } else run_actual(0x168dedu,a);
        if(a.gp[0]>capacity)throw FormatError("Original FP draw count escapes its caller capacity");
        std::vector<SourceFirstPersonModelTu15> models;
        static_assert(sizeof(Transform)==0x34);
        for(unsigned n=0;n<a.gp[0];++n) {
            const auto p=output+n*stride,model=word(bank,p);
            if(model==0xffffffffu)throw FormatError("Original FP draw has no actual render model");
            const auto metadata=word(bank,word(bank,0x4f0414u)+(model&65535u)*16u+8u);
            const auto count=word(bank,metadata+0x48u);
            if(count>64)throw FormatError("Original FP model exceeds its64-node record");
            SourceFirstPersonModelTu15 result{model,word(bank,p+4u),word(bank,p+8u),{}};
            result.nodes.resize(count);
            const auto bytes=bank.view(p+0xcu,count*sizeof(Transform));
            std::memcpy(result.nodes.data(),bytes.data(),bytes.size());models.push_back(std::move(result));
        }
        return models;
    }
    SourcePlayerEquipmentTu15 player_equipment(std::uint32_t object) const {
        const auto body=object_body(object);
        const auto bytes=bank.view(body+0x210u,0x30u);
        const auto short_at=[&](unsigned at) {
            return std::uint16_t(std::to_integer<unsigned>(bytes[at])|
                (std::to_integer<unsigned>(bytes[at+1])<<8));
        };
        SourcePlayerEquipmentTu15 equipment;
        equipment.inventory_sequence=short_at(0);
        equipment.replicated_sequence=short_at(4);
        for(unsigned i=0;i<2;++i) {
            equipment.current[i]=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(bytes[2+i]));
            equipment.requested[i]=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(bytes[6+i]));
            equipment.grenades[i]=std::to_integer<std::uint8_t>(bytes[0x2eu+i]);
        }
        for(unsigned i=0;i<4;++i) {
            equipment.weapons[i]=word(bank,body+0x218u+i*4u);
            equipment.ready_ticks[i]=word(bank,body+0x228u+i*4u);
            equipment.definitions[i]=equipment.weapons[i]==0xffffffffu?0xffffffffu:
                word(bank,object_body(equipment.weapons[i]));
            if(equipment.weapons[i]!=0xffffffffu) {
                const auto weapon=object_body(equipment.weapons[i]);
                // Complete1046C0 consumes these unchanged runtime fields.
                equipment.shaders[i]={word(bank,weapon+0x12cu),word(bank,weapon+0x16cu),
                    std::bit_cast<float>(word(bank,weapon+0x184u)),std::bit_cast<float>(word(bank,weapon+0x180u))};
            }
        }
        return equipment;
    }
    SourcePlayerEquipmentTu15 initialize_player_equipment(std::uint32_t player,std::uint32_t object) {
        const auto owner=record(word(bank,0x4f54f4u),player);
        if(!session_configured || !owner || word(bank,owner+0x2cu)!=object)
            throw FormatError("TU1.5 equipment requires the bound source player and unit");
        const std::array args{object};call_actual(0xb1a70u,args);
        const std::array action{0x400u};auto a=frame(action);a.gp[0]=object;
        run_actual(0xb1c30u,a);
        a=frame({});a.gp[1]=object;run_actual(0x1613b0u,a);
        return player_equipment(object);
    }
    void deliver_command(std::uint32_t player,std::uint32_t object,
                         std::span<const std::byte,0x5c> command) {
        const auto owner=record(word(bank,0x4f54f4u),player);
        if(!session_configured || !owner || word(bank,owner+0x2cu)!=object)
            throw FormatError("TU1.5 command requires the player's actual current source unit");
        object_body(object);
        if(!source_player_command_valid(command))
            throw FormatError("Native TU1.5 delivered command fails the original validator");
        const auto slot=player&0xffffu;
        if(slot>=16u)throw FormatError("TU1.5 delivered player escapes the original command frame");
        // Native delivered-input boundary of14FEC0: +8 is the admitted
        // player mask and +C contains sixteen literal5C command records.
        constexpr std::uint32_t input=0x60028000u;
        bank.write(input+0xcu+slot*0x5cu,command);
        delivered_players|=1u<<slot;put(bank,input+8u,delivered_players);
    }
    SourceWeaponEffectPresentation request_weapon_effect(const WeaponEffectInput &input) {
        (void)object_body(input.weapon);
        // Complete108A70 owns selection106F90, refresh17DFF0 and attached
        // creation179610. In particular178FF3 calls17E460 for definition
        // bits3/4; lighting colors are never inferred from the tag name.
        auto a=frame(std::array{input.previous_handle,input.a_bits,input.b_bits});
        a.gp[0]=input.effect;a.gp[1]=input.weapon;
        SourceWeaponEffectPresentation result;
        // Visibility may retire the original instance before the creator
        //returns. Retain only its actual completed color stores; this does
        //not replace allocation, visibility, or any original instruction.
        run_actual(0x108a70u,a,[&](const SourcePhysicsContactRecordStore &store) {
            const auto pc=store.instruction;
            const bool color_a=pc==0x17b0a3u || pc==0x17e484u || pc==0x17e4d2u || pc==0x17e58eu;
            const bool color_b=pc==0x17b0a9u || pc==0x17e48du || pc==0x17e4dbu || pc==0x17e594u;
            if((!color_a && !color_b) || store.written.size()!=4)return;
            std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<std::uint32_t>(store.written[n])<<(n*8);
            (color_a?result.colors.a:result.colors.b)=value;result.colors_observed=true;
        });
        result.instance=a.gp[0];
        if(result.instance!=0xffffffffu) {
            const auto effect=record(word(bank,0x4f720cu),result.instance);
            if(!effect || word(bank,effect+4u)!=input.effect)
                throw FormatError("TU1.5 weapon effect lost its actual allocation");
            result.colors={word(bank,effect+0x104u),word(bank,effect+0x108u)};
            result.colors_observed=true;
        }
        return result;
    }
    void populate_weapon_targeting(std::uint32_t player,std::uint32_t object,
        std::span<std::byte,0x5c> command) {
        const auto owner=record(word(bank,0x4f54f4u),player);
        if(!owner || word(bank,owner+0x2cu)!=object)
            throw FormatError("TU1.5 targeting requires the player's current unit");
        (void)object_body(object);
        //153BA0's complete12-byte result has an independent melee target
        //at+8. Native action-prompt presentation already owns the first8.
        constexpr std::uint32_t prompt=0x60041000u,parameters_=0x60041100u,
            eye=0x60041200u,velocity=0x60041220u,look=0x60041240u,result=0x60041300u;
        call_actual(0x153ba0u,std::array{player,prompt});
        const auto put_command=[&](unsigned at,std::uint32_t value) {
            for(unsigned n=0;n<4;++n)command[at+n]=std::byte(value>>(8*n));
        };
        put_command(0x30u,word(bank,prompt+8u));
        //1ADC90 produces the actual selected weapon's angle/range bank.
        //Native input supplies view direction;1582F0 supplies the actual
        //posed eye. No target, distance, hit or confidence is assigned.
        auto a=frame(std::array{0xffffffffu});a.gp[1]=object;a.gp[6]=parameters_;
        run_actual(0x1adc90u,a);if(!(a.gp[0]&255u))return;
        a=frame(std::array{eye,velocity});a.gp[0]=object;run_actual(0x1582f0u,a);
        const auto command_float=[&](unsigned at) {
            std::uint32_t bits{};for(unsigned n=0;n<4;++n)bits|=std::to_integer<std::uint32_t>(command[at+n])<<(8*n);
            return std::bit_cast<float>(bits);
        };
        const auto aim=direction(command_float(4),command_float(8));
        for(const auto &[at,v]:std::array<std::pair<unsigned,float>,3>{{{0,aim.x},{4,aim.y},{8,aim.z}}})
            put(bank,look+at,std::bit_cast<std::uint32_t>(v));
        a=frame({});a.gp[0]=object;run_actual(0xbcf70u,a);const auto root=a.gp[0];
        const auto team=std::int32_t(std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(bank.view(owner+0xc0u,1)[0])));
        a=frame(std::array{parameters_,look,root,std::bit_cast<std::uint32_t>(team),result});a.gp[1]=eye;
        run_actual(0x1adf60u,a);if(!(a.gp[0]&255u))return;
        // Literal1AD664..1AD7DD publishes these fields from the completed
        //query. The original constructor leaves the impact vector untouched.
        const auto primary=word(bank,result+0x4cu),secondary=word(bank,result+0x50u);
        put_command(0x50u,primary);put_command(0x54u,secondary);
        if(std::bit_cast<float>(primary)>0 || std::bit_cast<float>(secondary)>0 ||
            std::to_integer<unsigned>(bank.view(result+0x28u,1)[0])) {
            const auto target=word(bank,result);
            put_command(0x34u,target);put_command(0x38u,word(bank,result+4u));
            if(target!=0xffffffffu) {
                const auto pool=word(bank,0x4ecbd0u),headers=word(bank,pool+0x44u);
                const auto header=headers+(target&65535u)*12u;
                const auto type=std::to_integer<unsigned>(bank.view(header+3u,1)[0]);
                if(type<2u)put_command(0x3cu,word(bank,object_body(target)+0x13cu));
            }
        }
    }
    std::optional<ParticleCollisionHit> particle_collision(std::uint32_t flags,Vec3 origin,Vec3 delta) {
        constexpr std::uint32_t start_=0x60041400u,delta_=0x60041420u,result_=0x60041500u;
        for(const auto &[p,v]:std::array<std::pair<std::uint32_t,Vec3>,2>{{{start_,origin},{delta_,delta}}}) {
            put(bank,p,std::bit_cast<std::uint32_t>(v.x));put(bank,p+4u,std::bit_cast<std::uint32_t>(v.y));
            put(bank,p+8u,std::bit_cast<std::uint32_t>(v.z));
        }
        const auto hit=call_actual(0x16c620u,std::array{flags,start_,delta_,0xffffffffu,0xffffffffu,result_});
        if(!(hit&255u))return {};
        const auto scalar=[&](unsigned at){return std::bit_cast<float>(word(bank,result_+at));};
        const auto vector=[&](unsigned at){return Vec3{scalar(at),scalar(at+4),scalar(at+8)};};
        const auto type=word(bank,result_);
        // Original2C30A0,2C3199 and2C32E3 consume fraction+4, point+8
        //and plane+28. Both terrain1 and instanced terrain3 are structure.
        return ParticleCollisionHit{scalar(4),vector(8),vector(0x28),type==1u || type==3u,
            std::bit_cast<std::int16_t>(std::uint16_t(word(bank,result_+0x24u))),std::int32_t(type)};
    }
    void advance_effect_particles(float seconds,const SourceFirstPersonPhaseObserverTu15& observer) {
        if(!session_configured || !word(bank,0x52be48u) || !std::isfinite(seconds) || seconds<0)
            throw FormatError("TU1.5 particle frame requires a configured source world and finite frame delta");
        if(observer && (first_person_phase_active || weapon_effect_phase_active))
            throw FormatError("TU1.5 particle observation cannot nest a source phase");
        const std::array args{std::bit_cast<std::uint32_t>(seconds)};
        auto a=frame(args);
        if(observer) {
            first_person_phase_trace.clear();first_person_phase_cpu.clear();
            observer(true,a,bank,{},{});first_person_phase_active=true;
        }
        try {run_actual(0x179df0u,a);}
        catch(...) {if(observer)first_person_phase_active=false;throw;}
        if(observer) {
            first_person_phase_active=false;
            observer(false,a,bank,first_person_phase_trace,first_person_phase_cpu);
        }
    }
    void step_objects() {
        if(!session_configured || !word(bank,0x52be48u))
            throw FormatError("TU1.5 object tick requires the configured source world");
        // Native source consumers request textures during game ticks. Run the
        // complete original cache lock/load/scale owner at this boundary;
        // its ten-request counter must not accumulate across all game ticks.
        // This is native scheduling, not the complete Xbox render scheduler.
        call_actual(0x12f110u);
        // Original13A9CC runs cached-decal aging before13AAFF player/action
        // delivery. This composition runs that complete owner once per native
        // game tick against the actual source clock, pools and cache pages.
        // Actual13A9C7 advances all contrail marker chains immediately
        // before13A9CC decals. Object-local17EA60 alone cannot age/fade or
        // retire detached trails, and would leave impact-only point pairs.
        const auto tick_clock=word(bank,0x51df54u);
        if(!tick_clock)throw FormatError("Contrail tick requires its original clock owner");
        const std::array contrail_delta{word(bank,tick_clock+4u)};
        call_actual(0x17ead0u,contrail_delta);
        call_actual(0x180520u);
        // Original13AAFF applies the complete player/input/action scheduler
        // before PVS and the object/world phases. Its1500C0..1501A8 path
        // produces the player clusters consumed by13B6F0 itself.
        const std::array commands{0x60028000u};call_actual(0x14fec0u,commands);
        delivered_players=0;put(bank,0x60028008u,0);
        // Literal13AB0D owns AI decisions and the actual NPC unit-control
        // producer before PVS/object physics. Its own source AI-enabled byte
        // admits campaign work and returns immediately for an inactive bank.
        call_actual(0x1d0670u);
        for(auto entry:{0x13b6f0u,0xb8f40u,0x1cc8d0u,0xb9090u,0xb9180u})
            call_actual(entry);
        // Literal13AB49 advances source effect owners after object retirement.
        // Native presentation/audio remain separate consumers of completed
        // source requests; source instance lifetime and RNG cannot be skipped.
        call_actual(0x179cd0u);
        // Literal13AB80 observes actual local players and advances their
        // original FP managers/poses, before13ABD3 increments the clock.
        if(!first_person_phase_observer)call_actual(0x168ca1u);
        else {
            auto a=frame({});first_person_phase_trace.clear();first_person_phase_cpu.clear();
            first_person_phase_observer(true,a,bank,{},{});
            first_person_phase_active=true;
            try {run_actual(0x168ca1u,a);}catch(...) {first_person_phase_active=false;throw;}
            first_person_phase_active=false;
            first_person_phase_observer(false,a,bank,first_person_phase_trace,first_person_phase_cpu);
        }
        const auto clock=word(bank,0x51df54u);
        if(!clock)throw FormatError("TU1.5 frame has no source clock owner");
        // Literal13ABD3..13ABD8 follows these phases in the original frame.
        put(bank,clock+8u,word(bank,clock+8u)+1u);
    }
    void initialize_map() {
        if(word(bank,0x52be48u))throw FormatError("TU1.5 physics world is already constructed");
        call_actual(0x1e7760u); // Actual row18 new-map team relationship reset.
        for(auto entry:{0x37f110u,0x37f130u,0x37f150u,0x37f1e0u,0x37f240u,
            0x37fc70u,0x37fc90u,0x37fcb0u})call(entry);
        call_actual(0xb8a00u);
        call(0x1c5ff0u);
        if(!word(bank,0x52be48u))throw FormatError("TU1.5 new-map world was not published");
        call_actual(0xb8be0u);
        call_actual(0x1eed30u); // Literal44B1CC resets that cache for the new map.
        call_actual(0x229420u); // Literal44B598 resets feedback slots and original gains.
        call_actual(0x1563e0u); // Literal44B5BC new-map local player effect reset.
        call_actual(0x157eb0u); // Literal44B3C4 original director new-map reset.
        call_actual(0x22c480u); // Literal44B574 resets that frame state through its real owner.
    }
    std::uint32_t create_biped(std::uint32_t definition,const Vec3&position,const Vec3&forward,std::uint32_t player) {
        if(!session_configured)throw FormatError("TU1.5 object birth requires its actual native session options");
        const auto tag=bipeds.find(definition);
        if(tag==bipeds.end())throw FormatError("TU1.5 biped birth requires its actual cache definition");
        constexpr std::uint32_t request=0x60010000u;
        const std::array args{definition,0xffffffffu,0u};
        auto a=frame(args);a.gp[6]=request;
        source_run_physics_live_support_tu15(bank,0xb9960u,a,fp,services());
        for(const auto&[at,v]:std::array<std::pair<unsigned,Vec3>,2>{{{0x1c,position},{0x28,forward}}}) {
            put(bank,request+at,std::bit_cast<std::uint32_t>(v.x));
            put(bank,request+at+4,std::bit_cast<std::uint32_t>(v.y));
            put(bank,request+at+8,std::bit_cast<std::uint32_t>(v.z));
        }
        if(player!=0xffffffffu) {
            const auto owner=record(word(bank,0x4f54f4u),player);
            if(!owner)throw FormatError("TU1.5 player birth requires its live full source identity");
            // Original151FEC..15201F supplies these inputs before B9B70:
            // request+5C is the player, +64 its sign-extended team, +A8
            // the retained configuration value at player+89.
            put(bank,request+0x5cu,player);
            const auto team=std::int32_t(std::bit_cast<std::int8_t>(
                std::to_integer<std::uint8_t>(bank.view(owner+0xc0u,1)[0])));
            put(bank,request+0x64u,std::bit_cast<std::uint32_t>(team));
            put(bank,request+0xa8u,word(bank,owner+0x89u));
        }
        const std::array birth_args{request};return call_actual(0xb9b70u,birth_args);
    }
    void set_object_position(std::uint32_t object,Vec3 position) {
        if(!session_configured || !word(bank,0x52be48u) ||
            !std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
            throw FormatError("Object position input requires a configured source world and finite point");
        object_body(object);
        constexpr std::uint32_t point=0x60060000u;
        for(unsigned n=0;n<3;++n)put(bank,point+n*4u,std::bit_cast<std::uint32_t>(
            n==0?position.x:n==1?position.y:position.z));
        const std::array args{object,0u,0u};auto a=frame(args);a.gp[0]=point;
        run_actual(0xb95d0u,a);
    }
    std::uint32_t create_netgame_object(unsigned row) {
        const auto scenario=word(bank,0x4ecc20u),engine=word(bank,0x4f63b8u);
        if(!session_configured || !word(bank,0x52be48u) || !engine ||
            row>=word(bank,scenario+0x120u) || row>=100u)
            throw FormatError("TU1.5 netgame birth requires its admitted authored row and source world");
        // Literal15D92D..15D957 admits the original four row selectors.
        // The registered engine supplies its type through the real vtable.
        const auto owner=word(bank,0x56dfa0u+word(bank,engine+0xc14u)*4u);
        auto selector=frame({});selector.gp[0]=0xffffffffu;
        if(owner) {
            selector.gp[1]=owner;
            run_actual(word(bank,word(bank,owner)),selector);
        }
        const auto engine_type=selector.gp[0];
        const std::array count{4u};selector=frame(count);
        selector.gp[2]=engine_type;
        selector.gp[7]=word(bank,scenario+0x124u)+row*0x90u+4u;
        run_actual(0x15cab0u,selector);
        if(selector.gp[0]==0xffffffffu)return 0xffffffffu;
        const std::array args{engine+0x7dcu,row};call_actual(0x15d4f0u,args);
        const auto object=word(bank,engine+0x7dcu+row*8u);
        if(object!=0xffffffffu) {
            const auto body=object_body(object);
            if(bank.view(body+0xafu,1)[0]!=std::byte(row))
                throw FormatError("TU1.5 netgame constructor did not retain its actual row identity");
        }
        return object;
    }
    std::uint32_t netgame_object(unsigned row) const {
        const auto scenario=word(bank,0x4ecc20u),engine=word(bank,0x4f63b8u);
        if(!session_configured || !engine || row>=word(bank,scenario+0x120u) || row>=100u)
            throw FormatError("TU1.5 netgame lookup requires an actual retained authored row");
        const auto object=word(bank,engine+0x7dcu+row*8u);
        if(object!=0xffffffffu)object_body(object);
        return object;
    }
    std::uint32_t child_object(std::uint32_t parent,std::uint32_t definition) const {
        const auto body=object_body(parent);std::uint32_t result=0xffffffffu;
        unsigned count=0;
        for(auto child=word(bank,body+0x10u);child!=0xffffffffu;) {
            if(++count>0x800u)throw FormatError("TU1.5 source child chain is cyclic");
            const auto child_body=object_body(child);
            if(word(bank,child_body+0x14u)!=parent)
                throw FormatError("TU1.5 source child has another salted parent");
            if(word(bank,child_body)==definition) {
                if(result!=0xffffffffu)throw FormatError("TU1.5 child definition requires additional authored attachment identity");
                result=child;
            }
            child=word(bank,child_body+0xcu);
        }
        return result;
    }
    std::uint32_t create_scenario_object(const ScenarioObject &placement) {
        if(!session_configured || !word(bank,0x52be48u) ||
            placement.type<0 || placement.type>12 || placement.placement<0)
            throw FormatError("TU1.5 scene birth requires its live world and authored placement identity");
        const auto short_at=[&](std::uint32_t address) {
            const auto b=bank.view(address,2);
            return std::bit_cast<std::int16_t>(std::uint16_t(std::to_integer<unsigned>(b[0])|
                (std::to_integer<unsigned>(b[1])<<8)));
        };
        // Literal D7AA0..D7B48 consumes this same source type table, scenario
        // blocks, row stride and palette. Preserve the original placement bytes.
        const auto type=word(bank,0x474b90u+std::uint32_t(placement.type)*4u);
        const auto placed_offset=short_at(type+0xau),palette_offset=short_at(type+0xcu),stride=short_at(type+0xeu);
        if(placed_offset<0 || palette_offset<0 || stride<=0)
            throw FormatError("TU1.5 source type has no authored placement/palette");
        const auto scenario=word(bank,0x4ecc20u),placed=scenario+std::uint32_t(placed_offset),
            palette=scenario+std::uint32_t(palette_offset);
        if(std::uint32_t(placement.placement)>=word(bank,placed))
            throw FormatError("TU1.5 placement ordinal escapes its source block");
        const auto input=word(bank,placed+4u)+std::uint32_t(placement.placement)*std::uint32_t(stride);
        const auto choice=short_at(input);
        if(choice<0 || std::uint32_t(choice)>=word(bank,palette) ||
            word(bank,word(bank,palette+4u)+std::uint32_t(choice)*0x28u+4u)!=placement.definition)
            throw FormatError("TU1.5 placement definition differs from its original palette");
        const std::array args{std::uint32_t(placement.type),std::uint32_t(placement.placement),palette,1u,1u};
        auto a=frame(args);a.gp[0]=input;run_actual(0xc1120u,a);return a.gp[0];
    }
    void destroy_object(std::uint32_t object) {
        object_body(object);
        // Literal15C897..15C8A2 requests complete deactivation/deletion.
        const std::array deactivate{object,1u};call_actual(0xba3e0u,deactivate);
        const std::array erase{object,1u};call_actual(0xba490u,erase);
        if(record(word(bank,0x4ecbd0u),object))
            throw FormatError("TU1.5 complete deletion did not retire its full source identity");
    }
    void begin_model_frame() {
        rendered_model_nodes.clear();
        auto frame=word(bank,0x4c6854u),cursor=word(bank,0x494338u);
        source_render_frame_begin(frame,cursor);
        put(bank,0x4c6854u,frame);put(bank,0x494338u,cursor);
        // Literal2B630..2B63A clears the512 cluster union once per frame.
        std::ranges::fill(bank.mutable_view(0x4d1ee0u,64),std::byte{});
    }
    void publish_render_visibility(const SourcePortalVisibilitySet& visibility) {
        if(!session_configured || visibility.clusters.size()>128 ||
            visibility.groups.size()>6 || visibility.frustums.size()>512 ||
            visibility.sorted_cluster_indices.size()!=visibility.clusters.size())
            throw FormatError("TU1.5 render visibility requires a current bounded portal result");
        for(const auto &cluster:visibility.clusters)
            if(cluster.source_cluster<0 || cluster.source_cluster>=512)
                throw FormatError("TU1.5 render visibility cluster escapes its512-bit source bank");
        static_assert(sizeof(SourceClusterFrustumRanges)==0x1a);
        for(const auto index:visibility.sorted_cluster_indices)
            if(index>=visibility.clusters.size())
                throw FormatError("TU1.5 render visibility sort escapes its cluster entries");
        for(const auto &cluster:visibility.clusters)for(unsigned group=0;group<6;++group) {
            const auto count=cluster.counts[group],first=cluster.first_indices[group];
            if(count<0 || (count && (first<0 || std::size_t(first)+std::size_t(count)>visibility.frustums.size())))
                throw FormatError("TU1.5 render visibility range escapes its completed frustums");
        }
        const auto collector=word(bank,0x556e18u);
        if(!collector)throw FormatError("TU1.5 actual render visibility collector is absent");
        // Existing133760/1C5F00 render storage owns this collector. Native
        //173130 supplies the group/cluster/sort/frustum banks. Frustum+0C is
        // retained by that owner and survives native input publication too.
        const auto count=std::int16_t(visibility.clusters.size());
        const auto groups=std::int16_t(visibility.groups.size());
        const auto frustums=std::int16_t(visibility.frustums.size());
        bank.write(collector,std::as_bytes(std::span(&groups,1)));
        bank.write(collector+4u,std::as_bytes(std::span(visibility.groups)));
        bank.write(collector+0xa6eu,std::as_bytes(std::span(visibility.clusters)));
        bank.write(collector+0xa6cu,std::as_bytes(std::span(&count,1)));
        bank.write(collector+0x1770u,std::as_bytes(std::span(visibility.sorted_cluster_indices)));
        bank.write(collector+0x1970u,std::as_bytes(std::span(&frustums,1)));
        for(std::size_t n=0;n<visibility.frustums.size();++n) {
            const auto bytes=std::as_bytes(std::span(&visibility.frustums[n],1));
            const auto at=collector+0x1974u+std::uint32_t(n)*0x108u;
            bank.write(at,bytes.first(12));bank.write(at+16u,bytes.subspan(16));
        }
        call_actual(0x3f3e0u);
    }
    SourceModelBuildWorld model_build_world(unsigned view,unsigned players,bool modifier) {
        if(!session_configured || view>=4)
            throw FormatError("TU1.5 model builder requires a current session and valid view");
        const auto policy=source_geometry_lod_policy(players);
        put(bank,0x4c6870u,view);put(bank,0x4c683cu,std::uint32_t(policy.ordinary_maximum));
        bank.mutable_view(0x4c6839u,1)[0]=std::byte(modifier);
        if(!model_scratch_mounted) {
            bank.mount(model_scratch,std::vector<std::byte>(0x4000),SourceRuntimeMemory::Access::ReadWrite,
                "native TU1.5 model-builder caller scratch");model_scratch_mounted=true;
        }
        SourceModelBuildWorld w;auto& m=w.admission.memory;
        m.banks={word(bank,0x4f0414u),word(bank,0x4ecc18u),word(bank,0x4ecc14u),word(bank,0x4ecbd0u),
            [this](auto p,auto n){return bank.view(p,n);},[this](auto flags,auto p){
                auto a=frame({});a.gp[0]=flags;a.gp[1]=p;run_actual(0x130980u,a);return (a.gp[0]&255u)!=0;}};
        m.write=[this](auto p,auto n){return bank.mutable_view(p,n);};
        m.view=std::int32_t(view);m.render_frame=word(bank,0x4c6854u);m.render_cache_pool=word(bank,0x515f14u);
        m.context_callback=0xc08e0u;
        w.admission.first_person_owner=word(bank,0x4ce3f0u);
        w.admission.first_person_models=std::bit_cast<std::int16_t>(Bytes(bank.view(0x4ce3f4u,2)).u16(0));
        w.admission.ordinary_maximum=policy.ordinary_maximum;w.admission.view_modifier=modifier;
        w.admission.rasterizer=word(bank,0x4922a0u);
        w.admission.scalar=[this](auto object,auto function){
            const auto output=model_scratch+0x3f00u;const std::array args{object,function,output};
            const auto result=call_actual(0xbcb70u,args);
            return std::pair{(result&255u)!=0,std::bit_cast<float>(word(bank,output))};};
        w.nodes.resource_cache_present=word(bank,0x4f2d64u)!=0;
        w.nodes.deferred_nodes=[this](auto object,auto selected,auto nodes){
            const std::array args{object,selected,nodes};return call_actual(0xbf640u,args);};
        w.nodes.observed_world_nodes=[this](auto object,auto nodes,auto count){
            if(count<0 || count>256 || (!nodes && count))
                throw FormatError("Completed TU1.5 render pose has an invalid node bank");
            auto& retained=rendered_model_nodes[object];retained.resize(std::size_t(count));
            if(count)std::memcpy(retained.data(),bank.view(nodes,std::size_t(count)*sizeof(Transform)).data(),
                std::size_t(count)*sizeof(Transform));};
        w.view_owner=word(bank,0x4c66f8u);w.view_players=word(bank,0x4f9b54u);
        w.cached_models=0x4ce3f8u;w.arena_cursor=0x494338u;w.arena=0x49433cu;w.scratch=model_scratch;
        w.prt=[this](const std::array<std::uint32_t,7>& args){
            return (call_actual(0x23670u,args)&255u)!=0;
        };
        w.lighting=[this](auto object,float distance){
            const std::array args{std::bit_cast<std::uint32_t>(distance)};auto a=frame(args);a.gp[0]=object;
            run_actual(0x3daf0u,a);std::array<std::uint32_t,21> result;
            const auto bytes=bank.view(a.gp[0],sizeof(result));std::memcpy(result.data(),bytes.data(),sizeof(result));return result;};
        return w;
    }
    void build_render_model(SourceObjectDrawCollector& collector,const SourceObjectBuildRequest& request,
        unsigned view,unsigned players,bool modifier) {
        (void)object_body(request.owner);
        if(Bytes(collector.bytes).u32(0)!=request.owner)
            throw FormatError("TU1.5 model caller lost its full object owner");
        put(bank,collector.address,request.owner);
        const auto world=model_build_world(view,players,modifier);
        source_object_build_collector(collector,request,world);
    }
    SourceObjectDrawCollector build_world_model(std::uint32_t owner,unsigned view,
        unsigned players,bool modifier,float metric,float lighting_metric) {
        (void)object_body(owner);
        if(view>=4)throw FormatError("TU1.5 model collector view outside four-player bank");
        auto at=model_collectors.find({view,owner});
        if(at==model_collectors.end()) {
            if(next_model_collector>0xfffff000ull)throw FormatError("TU1.5 native model-collector namespace exhausted");
            SourceObjectDrawCollector collector{};collector.address=std::uint32_t(next_model_collector);
            std::memcpy(collector.bytes.data(),&owner,sizeof(owner));
            bank.mount(collector.address,std::vector<std::byte>(0x1000),SourceRuntimeMemory::Access::ReadWrite,
                "native TU1.5 per-view model collector caller storage");
            try{put(bank,collector.address,owner);at=model_collectors.emplace(std::pair{view,owner},collector).first;}
            catch(...){bank.release(collector.address);throw;}
            next_model_collector+=0x1000u;
        }
        auto& collector=at->second;
        build_render_model(collector,{owner,std::bit_cast<std::uint32_t>(metric),
            std::bit_cast<std::uint32_t>(lighting_metric),0u,0u,{},1u},view,players,modifier);
        return collector;
    }
    void append_render_attachments(SourceBspPartCommandState& state,
        const SourceObjectAdmission& queue,std::uint32_t mode) {
        if(mode!=1u && mode!=2u)
            throw FormatError("TU1.5 attachment command adapter requires a secondary light/shadow mode");
        if(queue.objects.size()>256u)throw FormatError("TU1.5 attachment queue exceeds current object bank");
        constexpr std::uint32_t context=0x64100000u,header=context+0x3000u,
            flags=context+0x3100u,owners=context+0x3300u;
        bank.mount(context,std::vector<std::byte>(0x4000),SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 secondary attachment queue caller");
        struct Release {SourceRuntimeMemory& b;~Release(){b.release(context);}} release{bank};
        put(bank,context+0x14u,header);put(bank,context+0x2a78u,mode);
        put(bank,header,256u);put(bank,header+4u,std::uint32_t(queue.objects.size()));
        put(bank,header+8u,flags);put(bank,header+12u,owners);
        for(unsigned n=0;n<queue.objects.size();++n) {
            (void)object_body(queue.objects[n].id);
            const auto value=queue.objects[n].flags;
            std::memcpy(bank.mutable_view(flags+n*2u,2).data(),&value,2);
            put(bank,owners+n*4u,queue.objects[n].id);
        }
        // Same original command allocations constructed by3B890. Project
        // typed caller contents while preserving those actual data pointers.
        for(unsigned n=0;n<3;++n) {
            const auto at=0x4ce268u+n*24u;const auto& source=state.banks[n];
            if(source.header[0]!=word(bank,at) || source.storage.size()!=source.header[0])
                throw FormatError("TU1.5 attachment command backing differs from original capacity");
            const auto data=word(bank,at+20u);
            std::memcpy(bank.mutable_view(at,20).data(),source.header.data(),20);
            std::memcpy(bank.mutable_view(data,source.storage.size()*24u).data(),
                source.storage.data(),source.storage.size()*24u);
        }
        auto a=frame({});a.gp[7]=context;a.gp[3]=mode;
        run_actual(0x41c70u,a);
        for(unsigned n=0;n<3;++n) {
            const auto at=0x4ce268u+n*24u;auto& target=state.banks[n];
            std::memcpy(target.header.data(),bank.view(at,20).data(),20);
            std::memcpy(target.storage.data(),bank.view(word(bank,at+20u),target.storage.size()*24u).data(),
                target.storage.size()*24u);
        }
    }
    void preload_render_model(std::uint32_t model) {
        if(!session_configured)
            throw FormatError("TU1.5 model loading requires delivered session options");
        const auto table=word(bank,0x4f0414u),index=word(bank,0x556d90u);
        if(model==0xffffffffu || (model&65535u)>=word(bank,index+24u))
            throw FormatError("TU1.5 model loading datum escapes the current cache");
        const auto row=table+(model&65535u)*16u;
        if(word(bank,row)!=0x6d6f6465u || word(bank,row+4u)!=model || word(bank,row+12u)<0x7cu)
            throw FormatError("TU1.5 model loading salt/group/payload differs");
        const auto mode=word(bank,row+8u);
        const std::array args{0u};auto request=frame(args);request.gp[0]=model;
        run_actual(0x1717b0u,request);
        const auto count=word(bank,mode+0x24u),sections=word(bank,mode+0x28u);
        for(unsigned i=0;i<count;++i) {
            const auto header=sections+i*0x5cu+0x38u;
            if(std::bit_cast<std::int32_t>(word(bank,header+8u))<=0)continue;
            auto load=frame({});load.gp[0]=2u;load.gp[1]=header;
            run_actual(0x130980u,load);
            // Native resource I/O completes synchronously. The original
            // admission/fixup owner must itself publish success; a header or
            // an uploaded preview mesh is not a successful source request.
            if(!(load.gp[0]&255u)) {
                load=frame({});load.gp[1]=header;run_actual(0x130980u,load);
            }
            if(!(load.gp[0]&255u))
                throw FormatError("TU1.5 original model-resource publication did not finish: model="+
                    std::to_string(model)+" section="+std::to_string(i)+" resource="+
                    std::to_string(word(bank,header))+" metadata_bytes="+std::to_string(word(bank,header+8u))+
                    " resources_pointer="+std::to_string(word(bank,header+0x14u))+" cache_token="+
                    std::to_string(word(bank,header+0x20u)));
        }
    }
    void configure_session(const SourceGameVariant&variant,std::uint16_t mode,
        std::int16_t rate,std::uint32_t seed,std::uint32_t kind=2,
        std::optional<std::int16_t> difficulty={}) {
        if(session_configured || rate<=0 || !source_game_variant_valid(variant) ||
            (kind!=1 && kind!=2) || (kind==1 && (!difficulty || *difficulty<0 || *difficulty>3)))
            throw FormatError("TU1.5 source session requires one admitted variant and positive clock rate");
        const auto game=word(bank,0x4f3218u);
        // Native session delivery binds the actual source game-options fields;
        // complete1490B0 consumes the delivered clock rate and resets the clock.
        put(bank,game+8u,kind);
        if(difficulty) {
            // Authenticated HS1D4/1D5 read this signed short when kind==1.
            const std::array value{std::byte(std::uint16_t(*difficulty)&255u),
                std::byte(std::uint16_t(*difficulty)>>8)};
            bank.write(game+0x132u,value);
        }
        put(bank,game+0xcu,std::uint32_t(mode)|(std::uint32_t(std::uint16_t(rate))<<16));
        put(bank,game+0x18u,seed);
        // Literal13A8F2..13A8FB copies the admitted options seed to the
        // already constructed gameplay stream before map object creation.
        const auto random=word(bank,0x4f3cd8u);
        if(!random)throw FormatError("TU1.5 gameplay random stream was not constructed");
        put(bank,random,word(bank,game+0x18u));
        bank.write(game+0x13cu,std::as_bytes(std::span(variant.bytes)));
        auto stage=frame({});stage.gp[6]=game+8u;
        run_actual(0x191ca0u,stage);
        const std::array capacity{stage.gp[0]};
        // Actual resource-stage callbacks45F944 rows6/7 consume the source
        // selector and carve their backing from the retained physical bank.
        call_actual(0x125d40u,capacity);
        call_actual(0x130500u,capacity);
        call_actual(0x12ecf0u); // Actual191AAF carves and activates texture backing.
        call_actual(0x1490b0u);
        call_actual(0x159ff0u); // Full source engine reset consumes the delivered variant.
        call_actual(0x1cfc90u); // Actual44B700 AI reset consumes delivered game options.
        session_configured=true;
    }
    void set_kill_volume_enabled(std::int16_t trigger,bool enabled) {
        if(trigger==-1)return;
        const auto scenario=word(bank,0x4ecc20u),state=word(bank,0x52be6cu);
        if(!session_configured || !scenario || !state)
            throw FormatError("Kill-volume control requires its actual configured source world");
        const auto count=word(bank,scenario+0x108u),rows=word(bank,scenario+0x10cu);
        if(trigger<0 || std::uint32_t(trigger)>=count || count>65536u)
            throw FormatError("Kill-volume trigger escapes the admitted source scenario");
        bank.view(rows,std::size_t(count)*0x44u);
        const auto selected=std::bit_cast<std::int16_t>(std::uint16_t(
            word(bank,rows+std::uint32_t(trigger)*0x44u+0x40u,2)));
        if(selected==-1)return;
        // Complete metadata admission precedes borrowing mutable storage, so
        // rejected input cannot mark a live continuation as partially written.
        if(selected<0 || selected>=256)
            throw FormatError("Kill-volume bit escapes the actual source state");
        std::vector<std::int16_t> indices;indices.reserve(count);
        for(std::uint32_t i=0;i<count;++i)indices.push_back(std::bit_cast<std::int16_t>(
            std::uint16_t(word(bank,rows+i*0x44u+0x40u,2))));
        SourceKillVolumeView(bank.mutable_view(state,0x24),indices,{}).set_enabled(trigger,enabled);
    }
    bool object_in_kill_volume(std::uint32_t object) {
        if(!session_configured || !word(bank,0x52be48u) || !word(bank,0x52be6cu))
            throw FormatError("Kill-volume containment requires its actual configured source world");
        if(object!=0xffffffffu)object_body(object);
        auto a=frame({});a.gp[7]=object;run_actual(0x1f47e0u,a);
        return (a.gp[0]&255u)!=0;
    }
};
PhysicsLiveWorldTu15::PhysicsLiveWorldTu15(const Cache&cache,const std::filesystem::path&xbe,const std::filesystem::path&files,std::size_t bsp)
    :PhysicsLiveWorldTu15(cache,xbe,files,bsp,std::make_shared<NativeWorldClock>()) {}
PhysicsLiveWorldTu15::PhysicsLiveWorldTu15(const Cache&cache,const std::filesystem::path&xbe,const std::filesystem::path&files,std::size_t bsp,
    std::shared_ptr<PhysicsLiveClockSource> clocks):impl_(std::make_unique<Impl>(cache,xbe,files,bsp,std::move(clocks))) {}
PhysicsLiveWorldTu15::~PhysicsLiveWorldTu15()=default;
SourceRuntimeMemory&PhysicsLiveWorldTu15::memory(){Impl::PublicCall active(*impl_,false);return impl_->bank;}
void PhysicsLiveWorldTu15::run(std::uint32_t p,SourcePhysicsWorldRuntimeAbi&a,
    const SourcePhysicsContactRecordObserver&o){Impl::PublicCall active(*impl_);impl_->run(p,a,o);}
void PhysicsLiveWorldTu15::run_actual(std::uint32_t p,SourcePhysicsWorldRuntimeAbi&a,
    const SourcePhysicsContactRecordObserver&o){Impl::PublicCall active(*impl_);impl_->run_actual(p,a,o);}
std::uint32_t PhysicsLiveWorldTu15::call(std::uint32_t p,std::span<const std::uint32_t>a){Impl::PublicCall active(*impl_);return impl_->call(p,a);}
std::uint32_t PhysicsLiveWorldTu15::call_actual(std::uint32_t p,std::span<const std::uint32_t>a){Impl::PublicCall active(*impl_);return impl_->call_actual(p,a);}
void PhysicsLiveWorldTu15::append_render_attachments(SourceBspPartCommandState& state,
    const SourceObjectAdmission& queue,std::uint32_t mode) {Impl::PublicCall active(*impl_);
    impl_->append_render_attachments(state,queue,mode);
}
void PhysicsLiveWorldTu15::preload_render_model(std::uint32_t model){Impl::PublicCall active(*impl_);impl_->preload_render_model(model);}
void PhysicsLiveWorldTu15::begin_model_frame(){Impl::PublicCall active(*impl_);impl_->begin_model_frame();}
void PhysicsLiveWorldTu15::publish_render_visibility(const SourcePortalVisibilitySet& visibility){Impl::PublicCall active(*impl_);impl_->publish_render_visibility(visibility);}
void PhysicsLiveWorldTu15::begin_model_view(){Impl::PublicCall active(*impl_);impl_->call_actual(0x22830u);}
std::optional<std::vector<Transform>> PhysicsLiveWorldTu15::rendered_world_nodes(std::uint32_t object)const {
    Impl::PublicCall active(*impl_,false);
    const auto found=impl_->rendered_model_nodes.find(object);
    return found==impl_->rendered_model_nodes.end()?std::nullopt:std::optional(found->second);
}
bool PhysicsLiveWorldTu15::request_render_geometry(std::uint32_t flags,std::uint32_t header) {Impl::PublicCall active(*impl_);
    auto a=impl_->frame({});a.gp[0]=flags;a.gp[1]=header;
    impl_->run_actual(0x130980u,a);return (a.gp[0]&255u)!=0;
}
SourceObjectAdmission PhysicsLiveWorldTu15::admit_render_objects(const SourcePortalVisibilitySet& visibility,
    const SourceObjectAdmissionInput& input,std::span<const std::array<std::uint32_t,4>> prior)const {Impl::PublicCall active(*impl_,false);
    const auto& b=impl_->bank;
    if(!impl_->session_configured)throw FormatError("TU1.5 object admission requires the current session");
    return source_object_admission_memory(visibility,{word(b,0x4ecbd0u),word(b,0x4eabb0u),
        word(b,0x4eabb4u),word(b,0x4eaba4u),word(b,0x4eaba8u),
        [&](auto address,auto size){return b.view(address,size);}},input,prior);
}
SourceObjectDrawCollector PhysicsLiveWorldTu15::build_world_model(std::uint32_t object,unsigned view,
    unsigned players,bool modifier,float metric,float lighting_metric){Impl::PublicCall active(*impl_);
    return impl_->build_world_model(object,view,players,modifier,metric,lighting_metric);
}
void PhysicsLiveWorldTu15::build_render_model(SourceObjectDrawCollector& collector,const SourceObjectBuildRequest& request,
    unsigned view,unsigned players,bool modifier) {Impl::PublicCall active(*impl_);
    impl_->build_render_model(collector,request,view,players,modifier);
}
SourceWeaponEffectPresentation PhysicsLiveWorldTu15::request_weapon_effect(const WeaponEffectInput&i){Impl::PublicCall active(*impl_);return impl_->request_weapon_effect(i);}
void PhysicsLiveWorldTu15::populate_weapon_targeting(std::uint32_t p,std::uint32_t o,std::span<std::byte,0x5c> c){Impl::PublicCall active(*impl_);impl_->populate_weapon_targeting(p,o,c);}
std::optional<ParticleCollisionHit> PhysicsLiveWorldTu15::particle_collision(std::uint32_t f,Vec3 p,Vec3 d){Impl::PublicCall active(*impl_);return impl_->particle_collision(f,p,d);}
std::uint32_t PhysicsLiveWorldTu15::source_world()const{Impl::PublicCall active(*impl_,false);return word(impl_->bank,0x52be48);}
NativeRuntimeAddress PhysicsLiveWorldTu15::world()const {Impl::PublicCall active(*impl_,false);
    const auto source=source_world();
    return source?impl_->bank.native_address(source,4):NativeRuntimeAddress{};
}
void PhysicsLiveWorldTu15::initialize_map(){Impl::PublicCall active(*impl_);impl_->initialize_map();}
std::uint32_t PhysicsLiveWorldTu15::source_object_pool()const{Impl::PublicCall active(*impl_,false);return word(impl_->bank,0x4ecbd0u);}
std::uint32_t PhysicsLiveWorldTu15::source_player_pool()const{Impl::PublicCall active(*impl_,false);return word(impl_->bank,0x4f54f4u);}
NativeRuntimeAddress PhysicsLiveWorldTu15::object_pool()const{Impl::PublicCall active(*impl_,false);return impl_->bank.native_address(source_object_pool(),0x4c);}
NativeRuntimeAddress PhysicsLiveWorldTu15::player_pool()const{Impl::PublicCall active(*impl_,false);return impl_->bank.native_address(source_player_pool(),0x4c);}
void PhysicsLiveWorldTu15::construct_player(std::uint32_t p,unsigned u,
    const SourcePlayerConfiguration&c,const SourceMachineIdentity&m){Impl::PublicCall active(*impl_);impl_->construct_player(p,u,c,m);}
void PhysicsLiveWorldTu15::set_local_player(std::uint32_t p,std::int16_t v){Impl::PublicCall active(*impl_);impl_->set_local_player(p,v);}
void PhysicsLiveWorldTu15::publish_native_observer_position(unsigned view,Vec3 position) {Impl::PublicCall active(*impl_);
    if(view>=4 || !std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
        throw FormatError("Native source observer requires a finite position and original local view");
    const auto locals=word(impl_->bank,0x4f54f0u);
    if(!locals || word(impl_->bank,locals+0xcu+view*4u)==0xffffffffu)
        throw FormatError("Native source observer has no actual local player");
    auto a=impl_->frame({});a.gp[1]=view;impl_->run_actual(0x172200u,a);
    if(!a.gp[0])throw FormatError("Native source observer has no selected BSP");
    for(unsigned n=0;n<3;++n)put(impl_->bank,a.gp[0]+n*4u,
        std::bit_cast<std::uint32_t>(n==0?position.x:n==1?position.y:position.z));
}
std::vector<SourceFirstPersonModelTu15> PhysicsLiveWorldTu15::prepare_first_person_models(unsigned view,
    Vec3 p,Vec3 f,Vec3 u,const SourceFirstPersonPhaseObserverTu15 &observer) {Impl::PublicCall active(*impl_);
    return impl_->prepare_first_person_models(view,p,f,u,observer);
}
FirstPersonViewBinding PhysicsLiveWorldTu15::first_person_view(unsigned v)const{Impl::PublicCall active(*impl_);return impl_->first_person_view(v);}
void PhysicsLiveWorldTu15::publish_roster_team(unsigned user,std::int8_t team){Impl::PublicCall active(*impl_);impl_->publish_roster_team(user,team);}
void PhysicsLiveWorldTu15::bind_player(std::uint32_t p,std::uint32_t o){Impl::PublicCall active(*impl_);impl_->bind_player(p,o);}
bool PhysicsLiveWorldTu15::depart_player(std::uint32_t p){Impl::PublicCall active(*impl_);return impl_->depart_player(p);}
SourcePlayerEquipmentTu15 PhysicsLiveWorldTu15::initialize_player_equipment(std::uint32_t p,std::uint32_t o){Impl::PublicCall active(*impl_);return impl_->initialize_player_equipment(p,o);}
SourcePlayerEquipmentTu15 PhysicsLiveWorldTu15::player_equipment(std::uint32_t o)const{Impl::PublicCall active(*impl_,false);return impl_->player_equipment(o);}
void PhysicsLiveWorldTu15::deliver_command(std::uint32_t p,std::uint32_t o,
    std::span<const std::byte,0x5c>c){Impl::PublicCall active(*impl_);impl_->deliver_command(p,o,c);}
void PhysicsLiveWorldTu15::bind_sound_user_settings(const std::filesystem::path &path) {Impl::PublicCall active(*impl_);
    impl_->sound_sdk_resources->bind_user_settings(SourceTu15UserSettings::load(path));
}
void PhysicsLiveWorldTu15::observe_sound_submissions(SourceSoundSubmitObserverTu15 observer) {Impl::PublicCall active(*impl_);
    impl_->sound_submit_observer=std::move(observer);
}
void PhysicsLiveWorldTu15::observe_first_person_phase(SourceFirstPersonPhaseObserverTu15 observer) {Impl::PublicCall active(*impl_);
    impl_->first_person_phase_observer=std::move(observer);
}
void PhysicsLiveWorldTu15::step_objects(){Impl::PublicCall active(*impl_);impl_->step_objects();}
void PhysicsLiveWorldTu15::advance_effect_particles(float seconds,const SourceFirstPersonPhaseObserverTu15& observer){Impl::PublicCall active(*impl_);impl_->advance_effect_particles(seconds,observer);}
std::vector<SourceProjectileDetonationTu15> PhysicsLiveWorldTu15::take_projectile_detonations() {Impl::PublicCall active(*impl_);
    auto result=std::move(impl_->projectile_detonations);impl_->projectile_detonations.clear();return result;
}
void PhysicsLiveWorldTu15::observe_physics_stores(SourcePhysicsStoreObserverTu15 observer){Impl::PublicCall active(*impl_);
  if(!observer) {impl_->physics_store_observer={};return;}
  impl_->physics_store_observer=[owner=impl_.get(),observer=std::move(observer)](const auto &store){observer(owner->last_instruction,store);};
}
void PhysicsLiveWorldTu15::observe_body_velocity_inputs(SourceBodyVelocityInputObserverTu15 observer){Impl::PublicCall active(*impl_);impl_->body_velocity_input_observer=std::move(observer);}
void PhysicsLiveWorldTu15::observe_phantom_phase(SourcePhantomPhaseObserverTu15 observer){Impl::PublicCall active(*impl_);impl_->phantom_phase_observer=std::move(observer);}
void PhysicsLiveWorldTu15::observe_body_impulses(std::function<void(const SourceBodyImpulseTu15 &)> observer) {Impl::PublicCall active(*impl_);
    impl_->body_impulse_observer=std::move(observer);
}
void PhysicsLiveWorldTu15::observe_effect_damage(SourceEffectDamageObserverTu15 observer) {Impl::PublicCall active(*impl_);
    impl_->effect_damage_observer=std::move(observer);
}
void PhysicsLiveWorldTu15::observe_weapon_effects(std::function<void(const SourceWeaponEffectEventTu15 &)> observer) {Impl::PublicCall active(*impl_);
    impl_->weapon_effect_observer=std::move(observer);
}
void PhysicsLiveWorldTu15::observe_weapon_effect_phase(SourceFirstPersonPhaseObserverTu15 observer) {Impl::PublicCall active(*impl_);
    impl_->weapon_effect_phase_observer=std::move(observer);
}
std::vector<MeleeImpact> PhysicsLiveWorldTu15::take_melee_impacts() {Impl::PublicCall active(*impl_);
    auto result=std::move(impl_->melee_impacts);impl_->melee_impacts.clear();return result;
}
std::vector<UnitDamageEffectEvent> PhysicsLiveWorldTu15::take_unit_damage_effects() {Impl::PublicCall active(*impl_);
    auto result=std::move(impl_->unit_damage_effects);impl_->unit_damage_effects.clear();return result;
}
std::optional<DeathCameraPacket> PhysicsLiveWorldTu15::vehicle_camera_packet(unsigned view,
    std::uint32_t unit,float yaw,float pitch,std::array<std::uint32_t,7> &state) {Impl::PublicCall active(*impl_);
    return impl_->vehicle_camera_packet(view,unit,yaw,pitch,state);
}
std::uint32_t PhysicsLiveWorldTu15::source_object_body(std::uint32_t d)const{Impl::PublicCall active(*impl_,false);return impl_->object_body(d);}
NativeRuntimeAddress PhysicsLiveWorldTu15::object_body(std::uint32_t d)const{Impl::PublicCall active(*impl_,false);return impl_->bank.native_address(source_object_body(d),4);}
std::uint32_t PhysicsLiveWorldTu15::create_biped(std::uint32_t d,const Vec3&p,const Vec3&f,std::uint32_t player){Impl::PublicCall active(*impl_);return impl_->create_biped(d,p,f,player);}
std::uint32_t PhysicsLiveWorldTu15::create_scenario_object(const ScenarioObject&p){Impl::PublicCall active(*impl_);return impl_->create_scenario_object(p);}
std::uint32_t PhysicsLiveWorldTu15::create_netgame_object(unsigned r){Impl::PublicCall active(*impl_);return impl_->create_netgame_object(r);}
std::uint32_t PhysicsLiveWorldTu15::netgame_object(unsigned r)const{Impl::PublicCall active(*impl_,false);return impl_->netgame_object(r);}
std::uint32_t PhysicsLiveWorldTu15::child_object(std::uint32_t p,std::uint32_t d)const{Impl::PublicCall active(*impl_,false);return impl_->child_object(p,d);}
void PhysicsLiveWorldTu15::destroy_object(std::uint32_t object){Impl::PublicCall active(*impl_);impl_->destroy_object(object);}
void PhysicsLiveWorldTu15::configure_session(const SourceGameVariant&v,std::uint16_t m,std::int16_t r,std::uint32_t s){Impl::PublicCall active(*impl_);impl_->configure_session(v,m,r,s);}
void PhysicsLiveWorldTu15::configure_campaign_session(std::int16_t d,std::uint16_t m,std::int16_t r,std::uint32_t s){Impl::PublicCall active(*impl_);impl_->configure_session(source_game_variant_default(0),m,r,s,1u,d);}
void PhysicsLiveWorldTu15::set_object_position(std::uint32_t o,Vec3 p){Impl::PublicCall active(*impl_);impl_->set_object_position(o,p);}
void PhysicsLiveWorldTu15::set_kill_volume_enabled(std::int16_t t,bool e){Impl::PublicCall active(*impl_);impl_->set_kill_volume_enabled(t,e);}
bool PhysicsLiveWorldTu15::object_in_kill_volume(std::uint32_t o){Impl::PublicCall active(*impl_);return impl_->object_in_kill_volume(o);}
SourcePhysicsWorldRuntimeAbi PhysicsLiveWorldTu15::step_physics(const SourcePhysicsContactRecordObserver&o) {Impl::PublicCall active(*impl_);
    if(!impl_->session_configured || world().is_null())
        throw FormatError("TU1.5 physics frame requires its constructed world and admitted session");
    auto a=impl_->frame({});
    impl_->run(0x1c7660u,a,o);
    return a;
}
PhysicsLiveWorldTu15::AuthorityCheckpoint PhysicsLiveWorldTu15::authority_checkpoint() const {
    std::unique_lock gate(impl_->call_gate,std::try_to_lock);impl_->require_executor();
    if(!gate.owns_lock() || impl_->public_calls || impl_->execution_depth || impl_->continuation_fault || !impl_->session_configured ||
        !word(impl_->bank,0x52be48u))throw FormatError("TU1.5 authority export requires a completed configured world");
    impl_->dsp_section->require_quiescent();
    impl_->sound_sdk_resources->require_quiescent();
    AuthorityCheckpoint saved;
    saved.resources=impl_->resources.identity();saved.bsp=impl_->bsp.index;
    saved.arena=impl_->arena->ownership_checkpoint();saved.locks=impl_->locks.ownership_checkpoint(impl_->bank);
    saved.virtual_memory=impl_->virtual_memory.ownership_checkpoint();saved.pool_storage=impl_->pool_storage.ownership_checkpoint();
    saved.events=impl_->events.checkpoint();saved.bank=impl_->bank.checkpoint();
    {
        std::lock_guard lock(impl_->mutex);
        const auto main=impl_->thread_ids.find(impl_->main_thread);
        if(main==impl_->thread_ids.end())throw FormatError("TU1.5 authority lacks its retained executor token");
        saved.main_thread_token=main->second;saved.next_thread_token=impl_->next_thread_token;
        saved.lock_events=impl_->lock_events;
    }
    saved.last_instruction=impl_->last_instruction;saved.delivered_players=impl_->delivered_players;
    saved.allocations=impl_->allocations;saved.resource_reads=impl_->resource_reads;
    saved.next_resource_read=impl_->next_resource_read;saved.model_collectors=impl_->model_collectors;
    saved.next_model_collector=impl_->next_model_collector;saved.model_scratch_mounted=impl_->model_scratch_mounted;
    saved.session_configured=impl_->session_configured;saved.exit_callbacks=impl_->native_exits.callbacks;
    saved.projectile_detonations=impl_->projectile_detonations;saved.unit_damage_effects=impl_->unit_damage_effects;
    saved.melee_impacts=impl_->melee_impacts;
    saved.logical_milliseconds=impl_->clocks->milliseconds()+impl_->tick_adjustment;
    saved.logical_cycles=impl_->clocks->cycles()+impl_->cycle_adjustment;
    return saved;
}
struct PhysicsLiveWorldTu15::PreparedAuthority::Impl {
    PhysicsLiveWorldTu15::Impl &owner;
    std::unique_lock<std::recursive_mutex> gate;
    struct Barrier {
        PhysicsLiveWorldTu15::Impl &owner;
        explicit Barrier(PhysicsLiveWorldTu15::Impl &owner):owner(owner){owner.staging=true;}
        ~Barrier(){owner.staging=false;}
    };
    std::optional<Barrier> barrier;
    SourceRuntimeMemory replacement;
    AuthorityCheckpoint fields;
    std::map<std::thread::id,std::uint32_t> thread_ids;
    std::optional<SourceRuntimeHostVirtualMemory::PreparedOwnership> virtual_memory;
    std::optional<SourcePhysicsPoolHostStorage::PreparedOwnership> pool_storage;
    std::optional<SourcePhysicsGameArenaNative::PreparedOwnership> arena;
    std::optional<PhysicsPlatformLocks::PreparedOwnership> locks;
    std::optional<NativeSynchronization::PreparedRestore> events;
    explicit Impl(PhysicsLiveWorldTu15::Impl &owner,const AuthorityCheckpoint &saved)
        :owner(owner),gate(owner.call_gate,std::try_to_lock) {
        owner.require_executor();
        if(!gate.owns_lock() || owner.public_calls || owner.execution_depth || saved.version!=1 ||
            !saved.session_configured || saved.resources!=owner.resources.identity() || saved.bsp!=owner.bsp.index)
            throw FormatError("TU1.5 authority image/resource/BSP or barrier mismatch");
        owner.dsp_section->require_quiescent();
        owner.sound_sdk_resources->require_quiescent();
        barrier.emplace(owner);replacement.restore(saved.bank);
        // The loader authenticated the original executable. Its immutable
        // regions must remain exactly those of the local admitted image.
        if(!owner.bank.matches_read_only_checkpoint(saved.bank))
            throw FormatError("TU1.5 authority changed immutable image bytes or topology");
        if(!saved.main_thread_token || saved.main_thread_token>=saved.next_thread_token || saved.next_thread_token>0x100000000ull ||
            word(replacement,native_thread_context+0x12cu)!=saved.main_thread_token ||
            word(replacement,tls+0x28u)!=native_thread_context || word(replacement,0x41b774u)!=native_tick_count ||
            !word(replacement,0x52be48u) || (saved.delivered_players&0xffff0000u) ||
            word(replacement,0x60028008u)!=saved.delivered_players)
            throw FormatError("TU1.5 authority executor/world/input binding differs");
        replacement.view(word(replacement,0x52be48u),4);
        if(saved.events.objects.size()!=saved.lock_events.size())throw FormatError("TU1.5 authority has orphaned lock events");
        std::set<std::uint32_t> handles;
        for(const auto &[critical,handle]:saved.lock_events) {
            const auto object=std::ranges::find(saved.events.objects,handle,&NativeSynchronization::ObjectCheckpoint::handle);
            if(!std::binary_search(saved.locks.critical_sections.begin(),saved.locks.critical_sections.end(),critical) ||
                !handles.insert(handle).second || object==saved.events.objects.end() ||
                object->kind!=NativeSynchronization::ObjectKind::Semaphore || object->count || object->limit!=1)
                throw FormatError("TU1.5 authority lock event lacks its released critical section");
        }
        if(std::uint64_t{0xf8000001u}+saved.resource_reads.size()!=saved.next_resource_read || saved.next_resource_read>0x100000000ull)
            throw FormatError("TU1.5 authority resource-read cursor differs");
        auto expected=std::uint64_t{0xf8000001u};
        for(const auto &[job,read]:saved.resource_reads)
            if(job!=expected++ || !read.bytes || std::uint64_t(read.destination)+read.bytes>0x100000000ull)
                throw FormatError("TU1.5 authority completed resource request differs");
        for(auto callback:saved.exit_callbacks)if(callback!=0x383970u && callback!=0x383c90u)
            throw FormatError("TU1.5 authority exit callback is not an admitted source owner");
        std::set<std::uint32_t> collectors;
        for(const auto &[key,collector]:saved.model_collectors) {
            if(key.first>=4 || collector.address<0x62000000u || collector.address%0x1000u ||
                std::uint64_t(collector.address)+0x1000u>saved.next_model_collector ||
                !collectors.insert(collector.address).second || Bytes(collector.bytes).u32(0)!=key.second ||
                word(replacement,collector.address)!=key.second)
                throw FormatError("TU1.5 authority model collector identity/cursor differs");
            const auto region=std::ranges::find(saved.bank.regions,collector.address,&SourceRuntimeMemory::CheckpointRegion::address);
            if(region==saved.bank.regions.end() || region->owner!="native TU1.5 per-view model collector caller storage" ||
                region->bytes.size()!=0x1000u || region->access!=SourceRuntimeMemory::Access::ReadWrite || region->page_bytes)
                throw FormatError("TU1.5 authority model collector backing differs");
        }
        if(std::count_if(saved.bank.regions.begin(),saved.bank.regions.end(),[](const auto &region){
                return region.owner=="native TU1.5 per-view model collector caller storage";})!=std::ptrdiff_t(collectors.size()) ||
            saved.next_model_collector<0x62000000u || saved.next_model_collector>0x100000000ull || saved.next_model_collector%0x1000u)
            throw FormatError("TU1.5 authority has orphaned model collector storage");
        const auto scratch=std::ranges::find(saved.bank.regions,PhysicsLiveWorldTu15::Impl::model_scratch,
            &SourceRuntimeMemory::CheckpointRegion::address);
        if(saved.model_scratch_mounted!=(scratch!=saved.bank.regions.end()) ||
            (saved.model_scratch_mounted && (scratch->owner!="native TU1.5 model-builder caller scratch" ||
                scratch->bytes.size()!=0x4000u || scratch->access!=SourceRuntimeMemory::Access::ReadWrite || scratch->page_bytes)))
            throw FormatError("TU1.5 authority model scratch binding differs");
        virtual_memory.emplace(owner.virtual_memory.prepare_ownership(saved.virtual_memory,saved.bank));
        pool_storage.emplace(owner.pool_storage.prepare_ownership(saved.pool_storage,saved.bank));
        arena.emplace(owner.arena->prepare_ownership(saved.arena,saved.bank,replacement));
        locks.emplace(owner.locks.prepare_ownership(owner.bank,saved.locks,saved.bank));
        events.emplace(owner.events.prepare_restore(saved.events));
        thread_ids.emplace(owner.main_thread,saved.main_thread_token);
        fields.last_instruction=saved.last_instruction;fields.delivered_players=saved.delivered_players;
        fields.main_thread_token=saved.main_thread_token;fields.next_thread_token=saved.next_thread_token;
        fields.logical_milliseconds=saved.logical_milliseconds;fields.logical_cycles=saved.logical_cycles;
        fields.lock_events=saved.lock_events;fields.allocations=saved.allocations;fields.resource_reads=saved.resource_reads;
        fields.next_resource_read=saved.next_resource_read;fields.next_model_collector=saved.next_model_collector;
        fields.model_collectors=saved.model_collectors;fields.model_scratch_mounted=saved.model_scratch_mounted;
        fields.session_configured=saved.session_configured;fields.exit_callbacks=saved.exit_callbacks;
        fields.projectile_detonations=saved.projectile_detonations;fields.unit_damage_effects=saved.unit_damage_effects;
        fields.melee_impacts=saved.melee_impacts;
    }
};
PhysicsLiveWorldTu15::PreparedAuthority::PreparedAuthority()=default;
PhysicsLiveWorldTu15::PreparedAuthority::~PreparedAuthority()=default;
PhysicsLiveWorldTu15::PreparedAuthority::PreparedAuthority(PreparedAuthority &&) noexcept=default;
PhysicsLiveWorldTu15::PreparedAuthority &PhysicsLiveWorldTu15::PreparedAuthority::operator=(PreparedAuthority &&) noexcept=default;
bool PhysicsLiveWorldTu15::PreparedAuthority::for_owner(const PhysicsLiveWorldTu15 &owner) const noexcept {
    return impl_ && &impl_->owner==owner.impl_.get();
}
const SourceRuntimeMemory &PhysicsLiveWorldTu15::PreparedAuthority::replacement_memory() const {
    if(!impl_)throw FormatError("TU1.5 authority preparation has no replacement bank");
    return impl_->replacement;
}
PhysicsLiveWorldTu15::PreparedAuthority PhysicsLiveWorldTu15::prepare_authority(const AuthorityCheckpoint &saved) {
    PreparedAuthority prepared;prepared.impl_=std::make_unique<PreparedAuthority::Impl>(*impl_,saved);return prepared;
}
bool PhysicsLiveWorldTu15::commit_authority(PreparedAuthority &&prepared) noexcept {
    if(!prepared.for_owner(*this) || std::this_thread::get_id()!=impl_->main_thread)return false;
    if(impl_->dsp_section->active_references() || !impl_->sound_sdk_resources->quiescent())return false;
    auto &state=*prepared.impl_;auto &fields=state.fields;
    if(!state.arena->matches_memory(state.replacement))return false;
    // Native clock domains are local inputs. Preserve their observed logical
    // values at the handoff; source game clocks/RNG remain in imported bytes.
    const auto tick_adjustment=fields.logical_milliseconds-impl_->clocks->milliseconds();
    const auto cycle_adjustment=fields.logical_cycles-impl_->clocks->cycles();
    impl_->bank.swap_backing(state.replacement);
    impl_->virtual_memory.commit_ownership(std::move(*state.virtual_memory));
    impl_->pool_storage.commit_ownership(std::move(*state.pool_storage));
    impl_->arena->commit_ownership(std::move(*state.arena));
    impl_->locks.commit_ownership(std::move(*state.locks));impl_->events.commit_restore(std::move(*state.events));
    impl_->thread_ids.swap(state.thread_ids);impl_->next_thread_token=fields.next_thread_token;
    impl_->lock_events.swap(fields.lock_events);impl_->allocations.swap(fields.allocations);
    impl_->resource_reads.swap(fields.resource_reads);impl_->next_resource_read=fields.next_resource_read;
    impl_->model_collectors.swap(fields.model_collectors);impl_->next_model_collector=fields.next_model_collector;
    impl_->rendered_model_nodes.clear();
    impl_->model_scratch_mounted=fields.model_scratch_mounted;impl_->session_configured=fields.session_configured;
    impl_->native_exits.callbacks.swap(fields.exit_callbacks);
    impl_->projectile_detonations.swap(fields.projectile_detonations);impl_->unit_damage_effects.swap(fields.unit_damage_effects);
    impl_->melee_impacts.swap(fields.melee_impacts);impl_->last_instruction=fields.last_instruction;
    impl_->delivered_players=fields.delivered_players;impl_->tick_adjustment=tick_adjustment;impl_->cycle_adjustment=cycle_adjustment;
    impl_->continuation_fault=false;
    prepared.impl_.reset();return true;
}
} // namespace halo2
