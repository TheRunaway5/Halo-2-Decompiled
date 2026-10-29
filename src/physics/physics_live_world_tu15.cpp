#include "halo2/mcc_sound_duration_tu15.hpp"
#include "halo2/source_first_person_action_tu15.hpp"
#include "halo2/source_sound_timing_input_observation_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/native_campaign_ai_queries_tu15_binding.hpp"
#include "halo2/native_campaign_ai_behavior_reset_tu15_binding.hpp"
#include "halo2/native_campaign_ai_area_tu15_binding.hpp"
#include "halo2/native_campaign_ai_clump_retirement_tu15_binding.hpp"
#include "halo2/native_campaign_ai_combat_reset_tu15_binding.hpp"
#include "halo2/native_campaign_unit_action_support_tu15_binding.hpp"
#include "halo2/native_campaign_unit_animation_reset_tu15_binding.hpp"
#include "halo2/native_campaign_screen_lifecycle_tu15.hpp"
#include "halo2/native_campaign_screen_lifecycle_tu15_binding.hpp"
#include "halo2/native_campaign_screen_timed_tu15_binding.hpp"
#include "halo2/native_campaign_camera_commands_tu15_binding.hpp"
#include "halo2/native_campaign_camera_startup_tu15_binding.hpp"
#include "halo2/native_campaign_camera_control_tu15_binding.hpp"
#include "halo2/native_campaign_camera_reset_tu15_binding.hpp"
#include "halo2/native_tu15_render_camera_packet_binding.hpp"
#include "halo2/native_campaign_level_transition_tu15.hpp"
#include "halo2/native_campaign_scenario_lookup_tu15.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include "halo2/source_damage_model_children_tu15.hpp"
#include "halo2/source_runtime_mcc_preparation.hpp"
#include "halo2/source_effect_world_locations.hpp"
#include "halo2/source_receiver_request_tu15.hpp"
#include "halo2/source_vehicle_compiled_symbols_tu15.hpp"
#include "halo2/source_vehicle_bv_agent_tu15.hpp"
#include "halo2/mcc_cpu_bitmap_projection.hpp"
#include "rendering/models/source_model_inline_cpu_bitmap_tu15.hpp"
#include "halo2/source_ratio_threshold_crt_tu15.hpp"
#include "halo2/source_environment_crt_pair_tu15.hpp"
#include "audio/mcc_sound_variation_provider_internal.hpp"
#include "campaign/scripting/mcc_script_sound_world_access_internal.hpp"
#include "halo2/source_campaign_lens_restore_tu15.hpp"
#include "halo2/source_campaign_environment_tu15.hpp"
#include "halo2/source_dialogue_admission_tu15.hpp"
#include "halo2/source_targeting_input_tu15.hpp"
#include "halo2/source_input_collection_tu15.hpp"
#include "halo2/source_profile_binding_tu15.hpp"
#include "halo2/source_input_startup_tu15.hpp"
#include "halo2/source_campaign_audio_tu15.hpp"
#include "halo2/source_sound_dsp_image_tu15.hpp"
#include "halo2/simulation.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_ctf_activation_tu15.hpp"
#include "halo2/source_ai_actor_contact_selector_tu15.hpp"
#include "halo2/source_ctf_configuration_tu15.hpp"
#include "halo2/source_ctf_player_iterator_tu15.hpp"
#include "halo2/source_ctf_activation_spawn_tu15.hpp"
#include "halo2/source_ctf_flag_position_tu15.hpp"
#include "halo2/source_multiplayer_engines_tu15.hpp"
#include "halo2/source_player_status_phase_tu15.hpp"
#include "halo2/source_gameplay_fade_phase_tu15.hpp"
#include "halo2/source_teleporter_tu15.hpp"
#include "halo2/source_unit_view_reset_tu15.hpp"
#include "halo2/source_unit_animation_cancel_tu15.hpp"
#include "halo2/source_vehicle_evasion_tu15.hpp"
#include "halo2/source_havok_static_lifetime_tu15.hpp"
#include "halo2/source_shadow_frame_tu15.hpp"
#include "halo2/source_shadow_array_swap_tu15.hpp"
#include "halo2/source_transparent_group_tu15.hpp"
#include "halo2/source_transparent_collection_tu15.hpp"
#include "halo2/source_cinematic_shadow_control_tu15.hpp"
#include "halo2/source_effect_light_view_tu15.hpp"
#include "halo2/source_first_person_light_pose_tu15.hpp"
#include "halo2/pstencil/native_light_update_tu15.hpp"
#include "halo2/pstencil/native_scenario_lights_tu15.hpp"
#include "halo2/pstencil/native_light_shape_tu15.hpp"
#include "halo2/pstencil/native_light_constants_tu15.hpp"
#include "halo2/pstencil/native_light_portal_tu15.hpp"
#include "halo2/source_renderer_request_tu15.hpp"
#include "halo2/source_renderer_tail_storage_tu15.hpp"
#include "halo2/source_object_velocity_read_only_tu15.hpp"
#include "halo2/source_object_sound_tu15.hpp"
#include "halo2/source_object_loop_audio_tu15.hpp"
#include "halo2/source_weapon_hud_tu15.hpp"
#include "halo2/source_crt_float_tu15.hpp"
#include "campaign/campaign_bsp_callbacks_tu15.hpp"
#include "halo2/source_physics_deactivation_tu15.hpp"
#include "halo2/source_vehicle_auxiliary_tu15.hpp"
#include "halo2/source_cloth_tu15.hpp"
#include "halo2/source_constraint_instance_tu15.hpp"
#include "halo2/source_body_impulse_tu15.hpp"
#include "halo2/source_collision_instance_storage_tu15.hpp"
#include "halo2/source_collision_query_tu15.hpp"
#include "halo2/source_damage_receiver_tu15.hpp"
#include "halo2/source_conveyor_tu15.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "halo2/source_vehicle_occupancy_tu15.hpp"
#include "halo2/source_vehicle_look_tu15.hpp"
#include "halo2/source_vehicle_input_tu15.hpp"
#include "halo2/source_model_render_tu15.hpp"
#include "halo2/source_object_render_entry.hpp"
#include "halo2/source_object_admission_memory.hpp"
#include "halo2/source_model_build.hpp"
#include "halo2/source_bsp_part_commands.hpp"
#include "halo2/source_render_cache.hpp"
#include "halo2/source_multiplayer_render_history_tu15.hpp"
#include "halo2/source_presented_model_pose_tu15.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/source_scene_device_constructor_tu15.hpp"
#include "halo2/source_map_world_support_tu15.hpp"
#include "halo2/source_ui_script_startup_tu15.hpp"
#include "halo2/physics_phantom_world_tu15.hpp"
#include "halo2/physics_game_arena_native.hpp"
#include "halo2/physics_early_initializer_runtime.hpp"
#include "halo2/physics_renderer_storage_runtime.hpp"
#include "halo2/physics_main_heap_runtime.hpp"
#include "halo2/physics_platform_locks.hpp"
#include "halo2/source_process_heap_platform.hpp"
#include "halo2/native_tu15_heap_data.hpp"
#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/mcc_runtime_delivery.hpp"
#include "halo2/mcc_script_layout.hpp"
#include "halo2/native_mcc_script_builtin.hpp"
#include "halo2/source_runtime_image.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/source_runtime_undo_journal.hpp"
#include "halo2/source_view_projection.hpp"
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
#include "halo2/source_effect_compiled_symbols_tu15.hpp"
#include "halo2/source_weapon_animation_query_symbols_tu15.hpp"
#include "halo2/source_physics_store_observers_tu15.hpp"
#include "halo2/native_tu15_physics_frame_observation.hpp"
#include "halo2/native_lift_scalar_consumer_tu15.hpp"
#include "halo2/native_vehicle_rate_consumer_tu15.hpp"
#include "halo2/native_constraint_schema_emitter_consumer_tu15.hpp"
#include "halo2/native_tu15_world_integration_residuals.hpp"
#include "halo2/source_contrail_geometry_tu15.hpp"
#include "halo2/source_effect_particle_frame_tu15.hpp"
#include "halo2/source_particle_swarm_tu15.hpp"
#include "halo2/source_lens_flare_tu15.hpp"
#include "halo2/source_script_effect_tu15.hpp"
#include "halo2/source_campaign_cinematic_tu15.hpp"
#include "halo2/source_campaign_completion_tu15.hpp"
#include "halo2/source_campaign_activation_tu15.hpp"
#include "halo2/source_geometry_cache_frame_tu15.hpp"
#include "halo2/native_tu15_renderer_palette.hpp"
#include "halo2/source_campaign_camera_tu15.hpp"
#include "halo2/source_texture_camera_producer_tu15.hpp"
#include "halo2/source_texture_camera_frame_tu15.hpp"
#include "halo2/source_texture_camera_scene_prefix_tu15.hpp"
#include "halo2/source_campaign_screen_tu15.hpp"
#include "halo2/source_animation_composition_tu15.hpp"
#include "halo2/source_object_functions_tu15.hpp"
#include "halo2/source_unit_cinematic_tu15.hpp"
#include "halo2/source_unit_speech_tu15.hpp"
#include "halo2/source_resource_cache_pressure_tu15.hpp"
#include "halo2/source_lens_query_tu15.hpp"
#include "halo2/source_effect_particle_draw_tu15.hpp"
#include "halo2/source_ai_actor_tu15.hpp"
#include "halo2/source_ai_frame_tu15.hpp"
#include "halo2/source_campaign_ai_placement_settings_tu15.hpp"
#include "halo2/source_campaign_ai_conditions_tu15.hpp"
#include "halo2/source_campaign_ai_scalar_defaults_tu15.hpp"
#include "halo2/source_campaign_ai_object_decisions_tu15.hpp"
#include "halo2/source_campaign_ai_object_callbacks_tu15.hpp"
#include "halo2/source_campaign_birth_query_variants_tu15.hpp"
#include "halo2/source_campaign_ai_actor_behavior_tu15.hpp"
#include "halo2/source_level_handles_startup_tu15.hpp"
#include "halo2/source_level_handles_new_map_tu15.hpp"
#include "halo2/source_render_object_startup_tu15.hpp"
#include "halo2/source_object_material_tu15.hpp"
#include "halo2/source_damage_feedback_tu15.hpp"
#include "halo2/source_recorded_animation_tu15.hpp"
#include "halo2/source_ai_placement.hpp"
#include "halo2/source_campaign_runtime_tu15.hpp"
#include "halo2/source_kill_volumes.hpp"
#include "halo2/source_state_context_startup_tu15.hpp"
#include "halo2/source_sandbox_special_tu15.hpp"
#include "halo2/source_sound_startup_tu15.hpp"
#include "halo2/source_static_registration_tu15.hpp"
#include "halo2/source_runtime_dsp_section.hpp"
#include "halo2/source_runtime_sound_sdk_resources.hpp"
#include "halo2/native_tu15_audio_preference.hpp"
#include "halo2/source_runtime_sound_dma_resources.hpp"
#include "halo2/source_runtime_sound_reset_resources.hpp"
#include "halo2/source_sound_dma_storage_tu15.hpp"
#include "halo2/source_sound_hardware_storage_tu15.hpp"
#include "halo2/source_sound_cache_io_tu15.hpp"
#include "halo2/source_sound_cache_query_tu15.hpp"
#include "halo2/source_sound_scalar_tu15.hpp"
#include "halo2/native_tu15_sound_manager.hpp"
#include "halo2/source_sound_listener_selection_tu15.hpp"
#include "halo2/source_sound_gain_tu15.hpp"
#include "halo2/source_sound_random_scalar_tu15.hpp"
#include "halo2/source_sound_properties_tu15.hpp"
#include "halo2/source_sound_random_short_tu15.hpp"
#include "halo2/source_sound_range_selection_tu15.hpp"
#include "halo2/source_sound_permutation_tu15.hpp"
#include "halo2/source_sound_binding_tu15.hpp"
#include "halo2/source_sound_chunk_readiness_tu15.hpp"
#include "halo2/source_boot_smc_video_mode_input.hpp"
#include "halo2/source_network_session_startup_tu15.hpp"
#include "halo2/source_result_recording_support_tu15.hpp"
#include "halo2/source_sound_sdk_tu15.hpp"
#include "halo2/weapon_effect.hpp"
#include "halo2/fp_registration.hpp"
#include "halo2/math.hpp"
#include "physics/physics_live_entries_tu15.hpp"
#include "platform/native_startup_memory_hints.hpp"
#include <algorithm>
#include <bit>
#include <chrono>
#include <iostream>
#include <set>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <cerrno>
#if defined(__GLIBC__)
#include <malloc.h>
#endif
#include <mutex>
#include <sstream>
#include <thread>
#include <exception>
#include <atomic>
#include "halo2/source_execution_inputs.hpp"
#include "halo2/source_execution_calls.hpp"
#if defined(__i386__) || defined(__x86_64__)
#include <immintrin.h>
#endif

namespace halo2 {
namespace {
// Image and DSP input buffers have released their allocations. Return only
// unused native heap pages before constructing the original source arenas.
void release_unused_loader_memory() noexcept {
#if defined(__GLIBC__)
    const auto saved_errno=errno;
    (void)::malloc_trim(0);
    errno=saved_errno;
#endif
}
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
    // Counts every observed source write, as its per-store observer did.
    SourceRuntimeWriteSerial serial;
    explicit WorldMutationObserver(SourceRuntimeMemory &bank):serial(bank) {}
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
namespace detail {
struct MainWorldLife {std::atomic<bool> active{true};};
struct MainReceiptFrame {
    std::weak_ptr<const MainWorldLife> life;const void*owner{};std::atomic<bool> active{true};
    std::uint32_t frame{},tick{},bsp{},clock{};
    MainReceiptFrame(std::weak_ptr<const MainWorldLife> l,const void*o,std::uint32_t f,std::uint32_t t,std::uint32_t b,std::uint32_t c)
        :life(std::move(l)),owner(o),frame(f),tick(t),bsp(b),clock(c){}
};
struct MainCameraObserverBinding {
    SourceCompletedMainCameraObserverTu15 observer;
    std::weak_ptr<MainReceiptFrame> frame;
    explicit MainCameraObserverBinding(SourceCompletedMainCameraObserverTu15 observer):observer(std::move(observer)){}
    ~MainCameraObserverBinding(){if(const auto current=frame.lock())current->active=false;}
};
struct MainReadBorrow {
    std::weak_ptr<const MainWorldLife> life;std::weak_ptr<const MainReceiptFrame> frame;
    const void*owner{};const SourceRuntimeMemory*memory{};const std::thread::id executor;std::atomic<bool> active{true};
    MainReadBorrow(std::weak_ptr<const MainWorldLife> l,std::weak_ptr<const MainReceiptFrame> f,
        const void*o,const SourceRuntimeMemory*m,std::thread::id e)
        :life(std::move(l)),frame(std::move(f)),owner(o),memory(m),executor(e){}
};
struct MainReceiptAuthority {
    SourceRuntimeMemory& bank;
    std::shared_ptr<MainWorldLife> life=std::make_shared<MainWorldLife>();
    std::shared_ptr<MainReceiptFrame> frame;
    const void*owner{};
    MainReceiptAuthority(SourceRuntimeMemory& memory,const void*owner):bank(memory),owner(owner) {
        source_runtime_register_write_observer(bank,this,[this](auto p,auto n){if(anchors(p,n))retire();});
        try{source_runtime_register_topology_observer(bank,this,[this](auto p,auto n) noexcept {if((!p&&!n) || anchors(p,n))retire();});}
        catch(...){source_runtime_unregister_write_observer(bank,this);throw;}
    }
    ~MainReceiptAuthority(){close();source_runtime_unregister_topology_observer(bank,this);source_runtime_unregister_write_observer(bank,this);}
    bool anchors(std::uint32_t p,std::size_t n)const noexcept {
        if(!frame)return false;
        const auto overlaps=[&](std::uint32_t q){return std::uint64_t(p)<std::uint64_t(q)+4 && std::uint64_t(q)<std::uint64_t(p)+n;};
        return overlaps(0x4c6854u)||overlaps(0x51df54u)||overlaps(0x4ecc18u)||overlaps(frame->clock+8u);
    }
    void retire()noexcept{if(frame)frame->active=false;frame.reset();}
    void close()noexcept{retire();if(life)life->active=false;life.reset();}
    void begin() {
        retire();const auto clock=word(bank,0x51df54u),bsp=word(bank,0x4ecc18u);
        if(!clock || !bsp)throw FormatError("Actual render frame lacks source clock/BSP ownership");
        (void)bank.native_address(clock,12);(void)bank.native_address(bsp,4);
        frame=std::make_shared<MainReceiptFrame>(life,owner,word(bank,0x4c6854u),word(bank,clock+8u),bsp,clock);
    }
};
} //detail
bool SourceMainCameraRasterReceiptTu15::expired()const noexcept {
    const auto frame=frame_.lock();if(!frame||!frame->active)return true;
    const auto life=frame->life.lock();return !life||!life->active;
}
namespace detail {
void validate_main_camera_use(const std::shared_ptr<const MainReceiptFrame>& frame,
    const std::shared_ptr<const MainReadBorrow>& scope,unsigned view,unsigned completed_view) {
    // Executor identity is immutable. Reject foreign-thread use before any
    // mutable lifetime state or source-bank access; expired() is separately
    // safe across actual retirement through atomic active flags.
    if(!scope || std::this_thread::get_id()!=scope->executor)
        throw FormatError("Completed main checked use requires its actual retained executor");
    if(!frame||!frame->active.load()||!scope->active.load()||frame->owner!=scope->owner ||
       frame->life.owner_before(scope->life)||scope->life.owner_before(frame->life)||scope->frame.lock()!=frame ||
       view!=completed_view)
        throw FormatError("Completed main receipt lost its actual World/frame/view borrow");
    const auto life=frame->life.lock();
    if(!life||!life->active||!scope->memory || word(*scope->memory,0x4c6854u)!=frame->frame ||
       word(*scope->memory,0x51df54u)!=frame->clock || word(*scope->memory,frame->clock+8u)!=frame->tick ||
       word(*scope->memory,0x4ecc18u)!=frame->bsp)
        throw FormatError("Completed main receipt actual frame/tick/BSP ownership changed");
}
}
const SourceMainCameraOperandsTu15& SourceMainCameraRasterReceiptTu15::operands(const SourceRenderMemoryBorrowTu15& token,unsigned view)const {
    detail::validate_main_camera_use(frame_.lock(),token.scope_.lock(),view,data_.view);
    return data_;
}
SourceMainCameraRasterUseTu15 SourceMainCameraRasterReceiptTu15::bind(const SourceRenderMemoryBorrowTu15& token,unsigned view)const {
    detail::validate_main_camera_use(frame_.lock(),token.scope_.lock(),view,data_.view);
    return SourceMainCameraRasterUseTu15(frame_,token.scope_,data_);
}
const SourceMainCameraOperandsTu15& SourceMainCameraRasterUseTu15::operands()const & {
    detail::validate_main_camera_use(frame_.lock(),scope_.lock(),data_.view,data_.view);
    return data_;
}

struct PhysicsLiveWorldTu15::Impl {
    // Declared first so the table outlives every other member: translated
    // teardown dispatched while later members are destroyed still reads it.
    // Every cross-unit dispatch passes the services table. Its callbacks
    // capture only this owner and the table copies nothing that changes
    // except the main-thread identity, so one built instance is reused and
    // rebuilt only when that identity differs. Readers take the published
    // pointer without locking; retired instances stay alive for them.
    struct BuiltServices{std::thread::id main_thread;SourcePhysicsWorldRuntimeServices services;};
    std::atomic<const BuiltServices*> built_services{nullptr};
    std::mutex built_services_mutex;std::vector<std::unique_ptr<BuiltServices>> built_services_retained;
    const SourcePhysicsWorldRuntimeServices &services() {
        if(const auto *built=built_services.load(std::memory_order_acquire);built && built->main_thread==main_thread)
            return built->services;
        std::lock_guard guard(built_services_mutex);
        if(const auto *built=built_services.load(std::memory_order_acquire);built && built->main_thread==main_thread)
            return built->services;
        auto fresh=std::make_unique<BuiltServices>(BuiltServices{main_thread,build_services()});
        const auto *published=fresh.get();built_services_retained.push_back(std::move(fresh));
        built_services.store(published,std::memory_order_release);
        return published->services;
    }
    // Synchronous read-only observation of actual1A6C0B event payloads. No
    // event or source result is supplied; this borrow cannot survive a call.
    std::vector<std::array<std::byte,36>> *teleporter_event_capture{};
    std::vector<std::uint32_t> *teleporter_blocked_capture{};
    std::vector<SourceHudMessageEvent> *teleporter_notice_capture{};
    std::optional<std::uint32_t> teleporter_notice_recipient;
    std::function<void(const SourceTeleporterEventsTu15 &)> multiplayer_teleporter_observer;
    std::optional<std::pair<std::uint32_t,std::uint32_t>> nested_caller_frame;
    struct CallerFrame {
        Impl& owner;
        const decltype(nested_caller_frame) previous;
        CallerFrame(Impl& owner,std::uint32_t parent):owner(owner),previous(owner.nested_caller_frame) {
            if(!parent)return;
            // Helpers grow down from an independent caller frame. Their
            // argument area must stay below the still-active parent's ESP.
            if(parent<0x60000100u || parent>stack)
                throw FormatError("Nested native output caller escapes the original main stack");
            owner.nested_caller_frame=std::pair{(parent-0x100u)&~15u,parent};
        }
        ~CallerFrame(){owner.nested_caller_frame=previous;}
    };
    const Cache&cache_owner;
    const std::shared_ptr<PhysicsLiveClockSource> clocks;
    // Local original constructor output, deliberately absent from the raw
    // world/authority checkpoint and its prepared imported field bank.
    SourceOriginalCameraSeed original_camera_seed;
    std::optional<MccRuntimeCacheDelivery> canonical_cache;
    std::optional<NativeMccScriptBuiltinProfile> mcc_script_profile;
    SourceRuntimeResources resources;
    std::uint32_t tick_adjustment{};
    std::uint64_t cycle_adjustment{};
    std::uint64_t utc_adjustment{};
    mutable std::recursive_mutex call_gate;
    std::uint64_t authority_backing_revision{};
    const std::shared_ptr<const void> authority_backing_owner=std::make_shared<const unsigned char>(0);
    std::atomic<bool> publication_held{false};
    const void *publication_scope{};
    std::size_t public_calls{},execution_depth{};
    bool staging{},continuation_fault{};
    std::size_t render_read_borrows{};
    SourceCompletedMainCameraObserverTu15 main_camera_observer;
    std::weak_ptr<detail::MainCameraObserverBinding> scoped_main_camera_observer;
    struct PublicCall {
        Impl &owner;
        int exceptions=std::uncaught_exceptions();
        bool mutating;
        std::uint64_t writes{};
        explicit PublicCall(Impl &owner,bool mutating=true):owner(owner),mutating(mutating) {
            if(owner.publication_held.load(std::memory_order_acquire))
                throw FormatError("TU1.5 world call during native publication");
            std::lock_guard gate(owner.call_gate);
            if(owner.publication_held.load(std::memory_order_relaxed) || owner.staging ||
                (mutating && (owner.render_read_borrows || std::this_thread::get_id()!=owner.main_thread)))
                throw FormatError("TU1.5 world mutation requires its available retained executor");
            if(mutating)source_execution_untracked(owner.bank);
            writes=owner.mutations.serial.load();
            ++owner.public_calls;
        }
        ~PublicCall(){std::lock_guard gate(owner.call_gate);if(mutating && std::uncaught_exceptions()>exceptions &&
            owner.mutations.serial.load()!=writes)owner.continuation_fault=true;--owner.public_calls;}
    };
    void require_executor() const {
        if(std::this_thread::get_id()!=main_thread || publication_held.load(std::memory_order_acquire) || staging)
            throw FormatError("TU1.5 world requires its available retained executor");
    }
    struct Execution {
        Impl &owner;
        int exceptions=std::uncaught_exceptions();
        std::optional<SourceRuntimeDspSection::CallScope> dsp_scope;
        std::optional<SourceRuntimeSoundSdkResources::CallScope> sdk_scope;
        std::optional<SourceRuntimeSoundDmaResources::CallScope> sound_dma_scope;
        std::optional<SourceRuntimeSoundResetResources::CallScope> sound_reset_scope;
        std::optional<NativeMccScriptBuiltinBinding> script_scope;
        explicit Execution(Impl &owner):owner(owner){
            owner.require_executor();dsp_scope.emplace(owner.dsp_section.get());sdk_scope.emplace(owner.sound_sdk_resources.get());
            sound_dma_scope.emplace(owner.sound_dma.get());sound_reset_scope.emplace(owner.sound_reset.get());
            if(owner.mcc_script_profile && owner.mcc_script_profile->calls())
                script_scope.emplace(owner.bank,*owner.mcc_script_profile);
            ++owner.execution_depth;
        }
        ~Execution(){if(std::uncaught_exceptions()>exceptions)owner.continuation_fault=true;--owner.execution_depth;}
    };
    std::size_t vehicle_input_call_depth{};
    std::vector<SourceVehicleInputResetTu15> vehicle_input_resets;
    struct VehicleInputCall {
        Impl &owner;
        int exceptions=std::uncaught_exceptions();
        explicit VehicleInputCall(Impl &owner):owner(owner){++owner.vehicle_input_call_depth;}
        ~VehicleInputCall() noexcept(false) {
            if(--owner.vehicle_input_call_depth)return;
            if(exceptions!=std::uncaught_exceptions()){owner.vehicle_input_resets.clear();return;}
            owner.flush_vehicle_input_resets();
        }
    };
    std::shared_ptr<SourceRuntimeMemory> bank_storage;
    SourceRuntimeMemory &bank;
    std::unique_ptr<NativeTu15InputDevices> native_input_devices;
    bool native_input_initialized{};
    // Declared before every native resource owner: unwind/teardown joins the
    // arena and executes native exits before unbinding, then releases the bank.
    std::shared_ptr<SourceExecutionInputs> execution_inputs;
    std::unique_ptr<SourceExecutionInputBinding> execution_input_binding;
    std::unique_ptr<SourceEffectCompiledSymbolsTu15> effect_compiled_symbols;
    std::unique_ptr<SourceVehicleCompiledSymbolsTu15> vehicle_compiled_symbols;
    std::unique_ptr<SourceWeaponAnimationQuerySymbolsTu15> weapon_animation_query_symbols;
    std::unique_ptr<private_cpu_bitmap::WorldProjection> cpu_bitmap_projection;
    std::unique_ptr<SourceRuntimeDspSection> dsp_section;
    std::unique_ptr<SourceRuntimeSoundSdkResources> sound_sdk_resources;
    std::unique_ptr<NativeTu15AudioPreference> sound_preference;
    std::unique_ptr<SourceRuntimeSoundDmaResources> sound_dma;
    std::unique_ptr<SourceRuntimeSoundResetResources> sound_reset;
    void require_sound_quiescent() const {
        sound_sdk_resources->require_quiescent();
        if(sound_preference||sound_dma||sound_reset)
            throw FormatError("Bound native sound resources cannot cross a world authority checkpoint");
    }
    std::unique_ptr<NativeTu15RendererPalette> renderer_palette;
    WorldMutationObserver mutations{bank};
    detail::MainReceiptAuthority main_receipts{bank,this};
    SourceRuntimeHostVirtualMemory virtual_memory;
    SourcePhysicsPoolHostStorage pool_storage;
    PhysicsPlatformLocks locks;
    NativeSynchronization events{{0xf6000001u,0xf6010000ull}};
    NativeSynchronizationServices event_services=events.services();
    std::map<std::uint32_t,std::uint32_t> lock_events;
    bool body_pose_publication_active{};
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
    std::function<void(const SourceMultiplayerRoundEndTu15 &)> multiplayer_round_end_observer;
    std::function<void(const SourceGameplayFadeFrameTu15&)> gameplay_fade_observer;
    std::function<void(const SourceGameplayFadeActivationTu15&)> gameplay_fade_activation_observer;
    std::function<void(const SourcePlayerStatusPhaseTu15&)> player_status_observer;
    std::function<void(const SourcePlayerScoreboardEventTu15&)> player_scoreboard_observer;
    std::function<void(const SourceMultiplayerEventTu15 &)> multiplayer_event_observer;
    bool multiplayer_event_dispatch_active=false;
    std::uint32_t delivered_players{};
    std::map<std::pair<unsigned,std::uint32_t>,SourceObjectDrawCollector> model_collectors;
    std::map<std::uint32_t,SourceObjectAttachmentTu15> model_attachments;
    std::map<std::uint32_t,std::vector<Transform>> rendered_model_nodes;
    std::uint64_t next_model_collector=0x62000000u;
    static constexpr std::uint32_t model_scratch=0x60400000u;
    bool model_scratch_mounted{};
    std::vector<SourceProjectileDetonationTu15> projectile_detonations;
    struct ProjectileEffectCapture {std::uint32_t request;SourceProjectileDetonationTu15* event;};
    std::vector<ProjectileEffectCapture> projectile_effect_captures;
    std::function<void(const SourceBodyImpulseTu15 &)> body_impulse_observer;
    std::function<void(const SourceBodyImpulseTu15 &)> body_impulse_publisher;
    SourcePhantomPhaseObserverTu15 phantom_phase_observer;
    SourcePhysicsContactRecordObserver physics_store_observer;
    bool physics_store_observer_read_only{};
    NativeLiftScalarActivityTu15 native_lift_activity;
    NativeVehicleRateActivityTu15 native_vehicle_rate_activity;
    SourceBodyVelocityInputObserverTu15 body_velocity_input_observer;
    std::function<void(const SourceWeaponEffectEventTu15 &)> weapon_effect_observer;
    struct WeaponEffectCapture {
        SourceWeaponEffectEventTu15 *event;
        std::optional<std::uint32_t> request;
        std::optional<SourceWeaponEffectPresentation> birth;
    };
    std::vector<WeaponEffectCapture> weapon_effect_captures;
    SourceFirstPersonPhaseObserverTu15 weapon_effect_phase_observer;
    bool weapon_effect_phase_active{};
    std::vector<std::uint32_t> weapon_effect_phase_trace;
    std::vector<SourceFirstPersonCpuInputTu15> weapon_effect_phase_cpu;
    SourceEffectDamageObserverTu15 effect_damage_observer;
    // Native lag-compensation owner (borrowed) and the instantaneous
    // projectile whose FF15C -> FDEB0 movement is currently running.
    SourceHitscanRewindOwnerTu15 *hitscan_owner{};
    std::uint32_t hitscan_projectile=0xffffffffu;
    SourceFirstPersonPhaseObserverTu15 first_person_phase_observer;
    SourceSoundSubmitObserverTu15 sound_submit_observer;
    SourceSoundRequestObserverTu15 sound_request_observer;
    SourceLoopSoundRequestObserverTu15 loop_sound_request_observer;
    SourceEffectLightBirthObserverTu15 effect_light_birth_observer;
    bool first_person_phase_active{};
    std::vector<std::uint32_t> first_person_phase_trace;
    std::vector<SourceFirstPersonCpuInputTu15> first_person_phase_cpu;
    std::vector<UnitDamageEffectEvent> unit_damage_effects;
    std::vector<MeleeImpact> melee_impacts;
    std::optional<MccSoundRuntimePlan> sound_runtime_plan;
    std::optional<MccSoundTimingPlan> sound_timing_plan;
    std::shared_ptr<MccSoundTimingMemoryOwner> sound_timing_memory;
    std::optional<MccSoundTimingBoundPlan> sound_timing_bound;
    std::optional<MccSoundTimingResourceProof> sound_timing_resource;
    std::optional<std::tuple<std::uint32_t,unsigned,unsigned>> sound_timing_resource_key;
    bool sound_timing_active{},sound_timing_guard_registered{},sound_timing_observer_registered{};
    bool sound_timing_protected(std::uint32_t at,std::size_t bytes)const noexcept {
        if(!at && !bytes)return true;
        const auto overlaps=[&](std::uint32_t base,std::size_t n){return std::uint64_t(at)<std::uint64_t(base)+n && std::uint64_t(base)<std::uint64_t(at)+bytes;};
        if(canonical_cache && overlaps(canonical_cache->index().region.address,canonical_cache->reservation_bytes()))return true;
        for(const auto [base,n]:std::array<std::pair<std::uint32_t,std::size_t>,7>{{
            {0x60000000u,0x100000u},{0x556d90u,4},{0x4f0414u,4},{0x52d664u,4},
            {0x455aacu,44},{0x46a1dcu,4},{0x46a540u,4}}})if(overlaps(base,n))return true;
        return false;
    }
    void retire_sound_timing()noexcept {
        if(sound_timing_memory)sound_timing_memory->invalidate();
        sound_timing_bound.reset();sound_timing_resource.reset();sound_timing_resource_key.reset();
    }
    struct SoundTimingLifetime {
        Impl &owner;
        ~SoundTimingLifetime() {
            owner.retire_sound_timing();
            if(owner.sound_timing_observer_registered)source_runtime_unregister_topology_observer(owner.bank,this);
            if(owner.sound_timing_guard_registered)source_runtime_unregister_topology_guard(owner.bank,this);
        }
    } sound_timing_lifetime{*this};
    void install_sound_timing() {
        if(!sound_timing_plan)return;
        sound_timing_memory=std::make_shared<MccSoundTimingMemoryOwner>(bank_storage);
        source_runtime_register_topology_guard(bank,&sound_timing_lifetime,[this](auto at,auto bytes) {
            if(!sound_timing_protected(at,bytes))return;
            if(std::this_thread::get_id()!=main_thread)throw FormatError("MCC sound timing topology requires its retained executor");
            if(sound_timing_active)throw FormatError("MCC sound timing topology changed during execution");
        });sound_timing_guard_registered=true;
        source_runtime_register_topology_observer(bank,&sound_timing_lifetime,[this](auto at,auto bytes) {
            if(sound_timing_protected(at,bytes))retire_sound_timing();
        });sound_timing_observer_registered=true;
    }
    void run_sound_duration(const SourcePhysicsContactRuntimeCall &call,
        SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
        require_executor();
        if(!sound_timing_plan || !sound_timing_memory || sound_timing_active ||
            call.target!=0x21f590u || call.instruction!=0x21f6f3u || call.tail)
            throw FormatError("MCC sound duration requires the authenticated original caller");
        const auto sp=abi.gp[4];
        if(sp<0x60000004u || sp>0x600ffff4u)throw FormatError("MCC sound duration escapes its designated main stack");
        bool exact_stack=false;
        bank.visit_committed([&](auto address,auto bytes){if(address==0x60000000u && bytes.size()==0x100000u)exact_stack=true;});
        if(!exact_stack)throw FormatError("MCC sound duration lost its complete main stack owner");
        bank.validate_mutable_view(sp-4u,16);
        MccSoundTimingEntry entry{0x21f590u,0x21f6f8u,abi.gp[0],abi.gp[3],abi.gp[1],abi.gp[2],abi.gp[6],abi.gp[7],sp,
            {word(bank,sp),word(bank,sp+4u),word(bank,sp+8u)}};
        if(!sound_timing_bound)sound_timing_bound.emplace(bind_mcc_sound_timing_memory(*sound_timing_plan,sound_timing_memory));
        const MccSoundTimingBinding *binding=nullptr;
        const auto permutation=std::uint64_t(entry.edx)+entry.ecx;
        for(const auto &candidate:sound_timing_plan->bindings())if(candidate.native_root==entry.esi && candidate.native_range==entry.edi && candidate.native_permutation==permutation) {
            if(binding)throw FormatError("MCC sound duration has ambiguous actual metadata ownership");
            binding=&candidate;
        }
        if(!binding)throw FormatError("MCC sound duration has no authenticated original timing key");
        const auto key=std::tuple{binding->sound.value,binding->relative_pitch,binding->relative_permutation};
        if(sound_timing_resource_key!=key) {
            auto encoded=std::make_shared<const EncodedSound>(cache_owner.load_sound(binding->sound.value,binding->relative_pitch,binding->relative_permutation));
            auto proof=authenticate_mcc_sound_timing_resource(*sound_timing_bound,sound_timing_memory,binding->sound,binding->relative_pitch,binding->relative_permutation,std::move(encoded));
            sound_timing_resource.emplace(std::move(proof));sound_timing_resource_key=key;
        }
        struct Active {bool &value;explicit Active(bool &value):value(value){value=true;}~Active(){value=false;}} active(sound_timing_active);
        const auto status=with_mcc_sound_original_timing(*sound_timing_bound,sound_timing_memory,entry,*sound_timing_resource,
            [&](const auto &original){
                observe_source_sound_timing_input_tu15(bank,entry,original,*binding);
                run_mcc_sound_duration_read_projection(bank,entry,original,abi,fp,services(),observe);
            });
        if(status!=MccSoundTimingLookupStatus::Reference)throw FormatError("MCC sound duration rejected stale or changed actual ownership");
    }
    struct NativeExitRegistrations {
        Impl &owner;
        std::vector<std::uint32_t> callbacks;
        ~NativeExitRegistrations() noexcept {
            for(auto i=callbacks.rbegin();i!=callbacks.rend();++i) {
                try {owner.call_actual(*i);}
                catch(const std::exception &failure) {
                    owner.continuation_fault=true;
                    if(owner.execution_inputs)try{owner.execution_inputs->invalidate_recording();}catch(...){}
                    std::fprintf(stderr,"TU1.5 native exit callback %08x failed: %s\n",*i,failure.what());
                }catch(...) {
                    owner.continuation_fault=true;
                    if(owner.execution_inputs)try{owner.execution_inputs->invalidate_recording();}catch(...){}
                    std::fprintf(stderr,"TU1.5 native exit callback %08x failed with an unknown exception\n",*i);
                }
            }
        }
    } native_exits{*this,{}};
    // Destroy and join before destroying callbacks, locks or retained memory.
    std::unique_ptr<SourcePhysicsGameArenaNative> arena;

    Impl(const Cache&cache,const std::filesystem::path&xbe,const std::filesystem::path&files,std::size_t selected,
         std::shared_ptr<PhysicsLiveClockSource> clocks,std::shared_ptr<SourceExecutionInputs> inputs={})
        :cache_owner(cache),clocks(std::move(clocks)),resources(cache,[this](auto descriptor,auto bytes) {
            return canonical_cache?canonical_cache->source_address(descriptor,bytes):descriptor;
        }),bank_storage(std::make_shared<SourceRuntimeMemory>(xbe.empty()?
            native_tu15_world_memory(SourceRuntimeKernelTickCount{
                native_tick_count,{},"native monotonic millisecond tick-count data import"}):
            load_source_runtime_image(xbe,image,SourceRuntimeKernelTickCount{
                native_tick_count,{},"native monotonic millisecond tick-count data import"}))),bank(*bank_storage),
         execution_inputs(std::move(inputs)),execution_input_binding([&] {
             if(!execution_inputs)return std::unique_ptr<SourceExecutionInputBinding>{};
             execution_inputs->require_executor();
             if(execution_inputs->tracks_calls())
                 throw FormatError("Native startup accepts an operand tape, not a complete-call journal");
             return std::make_unique<SourceExecutionInputBinding>(bank,*execution_inputs);
         }()),virtual_memory(bank,0x08000000u),
         pool_storage(bank,0x10000000u),bsp(cache.is_mcc()?SourceRuntimeCacheBspImage{}:
             cache.load_source_runtime_cache_bsp_image(selected)) {
        native_constraint_schema_emitter_activity_reset_tu15(bank);
        if(!this->clocks)throw FormatError("TU1.5 world requires local native clock inputs");
        effect_compiled_symbols=std::make_unique<SourceEffectCompiledSymbolsTu15>(
            bank,cache.is_mcc(),[&cache](auto identifier){return cache.string_id(identifier);});
        vehicle_compiled_symbols=std::make_unique<SourceVehicleCompiledSymbolsTu15>(
            bank,cache.is_mcc(),[&cache](auto identifier){return cache.string_id(identifier);});
        weapon_animation_query_symbols=std::make_unique<SourceWeaponAnimationQuerySymbolsTu15>(
            bank,cache.is_mcc(),[&cache](auto identifier){return cache.string_id(identifier);});
        dsp_section=std::make_unique<SourceRuntimeDspSection>(bank,xbe,
            SourceRuntimeDspSection::LoadPolicy::on_demand);
        release_unused_loader_memory();
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
        // Actual32CBF8 calls this complete FP formatter registration before
        // any original printf consumer. Cold pointers otherwise still select
        //3334E7's genuine "floating point not loaded" fatal CRT owner.
        call_actual(0x32cbc0u);
        // Actual CRT row46DA08 constructs the static Havok scratch owner.
        // Its real native exit registration is retained through bank lifetime.
        // Original CRT rows46D7A0/A4/A8 construct constraint prototypes.
        //31F4E0 compares the live base prototype before installing31F770's
        //moving-surface solver. Unrun prototypes leave that comparison false.
        for(auto entry:{0x383420u,0x383440u,0x383460u})call_actual(entry);
        call_actual(0x3835f0u); // Actual CRT row46D7C8 Havok rigid-body deactivator.
        call_actual(0x381370u);
        call_actual(0x380fa0u); // Actual CRT row46D990 owns portal/object empty bounds.
        call_actual(0x380e70u); // Actual CRT row46D978 owns1024C0's fallback effect records.
        // Actual SDK accounting CRT rows46D7E4..46D7F0; source writes its
        // statistics pointers, not native-assigned pool/capability totals.
        for(auto entry:{0x387baau,0x387bb5u,0x38a988u,0x38a993u})call_actual(entry);
        // CRT rows46DC90/94 own245400's packet limits. Unrun BSS limits
        // clamp every normal camera packet to zero before observer repair.
        for(auto entry:{0x382360u,0x382380u})call_actual(entry);
        call_actual(0x3825d0u); // Actual CRT game-engine class registration.
        // Original forward _cinit rows46D944/46D948 initialize angular wrap.
        call_actual(0x380d50u); // Two-pi source operand into actual5564C4.
        call_actual(0x380d30u); // Pi source operand into actual5564C8.
        call_actual(0x381010u); // Actual _cinit46D98C directional math scalar.
        call_actual(0x3811b0u); // Actual _cinit46D9C4 environment spread scalar.
        call_actual(0x3817f0u); // Actual _cinit46DA84 ratio threshold.
        // Original _cinit row46DC64 owns the cosine threshold read by
        //2296D5 during the ordinary player's first spatial update. Preserve
        //its actual FLD/FCOS/FSTP computation before any gameplay consumer.
        call_actual(0x382230u);
        initialize_game_storage();
        call_actual(0x148d40u); // Actual startup row3 constructs both RNG streams.
        // Actual row4's68BE0 ->680C0 defaults also feed local B2C60 pickup
        // notifications. Execute the complete pure leaf, without starting
        // transport/session services or installing constructed field values.
        call_actual(0x680c0u);
        call_actual(0x1a5410u); // Actual row13 constructs both level-handle pools before their new-map callbacks.
        call_actual(0x149050u); // Actual source clock storage owner, row17.
        call_actual(0x1e7710u); // Actual row18 constructs team relationship state.
        call_actual(0x1f3830u); // Actual source gravity/physics constants, row25.
        call_actual(0x1f4700u); // Actual44B308 scenario kill-volume state owner.
        call_actual(0x219080u); // Actual44B7AC BSP collision-instance mask allocation.
        call(0x133760); // actual134B00: full renderer raw/descriptor construction.
        call(0x1c5f00); // actual1CB130: real pools/main heap/renderer overflow join.
        call_actual(0xb87f0); // Actual source object header/body storage constructors.
        call_actual(0x10e3b0u); // Actual object activation bookkeeping owner, row28.
        call_actual(0x10fa30u); // Literal44B2E4 row29 owns the shader-function arena bank.
        call_actual(0x1789f0u); // Actual body/component datum owners, row40.
        call_actual(0x14dfe0u); // Actual source multiplayer player storage, row19.
        call_actual(0x159f80u); // Actual source game-engine storage, row22.
        // Literal44AED0 row23 publishes the original simulation/world
        // bindings used by pickup and weapon-action notifications. Leaving
        // this constructor unrun makes actualB8180 dereference NULL+8.
        call_actual(0x69220u);
        // Original12560 owns these CPU cold stages once, after the earlier
        // raw draw/object storage owners. The complete GPU/device chain is
        // separate; do not publish its4922A8 successful-initialization flag.
        if(word(bank,0x4922a8u,1) || word(bank,0x515ec0u))
            throw FormatError("TU1.5 renderer CPU cold state was already constructed");
        call_actual(0x169d0u); // Actual12575: authored near/far/config defaults.
        call_actual(0x125c90u); // Actual12054 resource-cache construction.
        call_actual(0x1fb90u); // Actual12813: real arena/CRC, timed-effect/clip state.
        const auto render_effect_state=word(bank,0x515ec0u);
        if(!render_effect_state || bank.native_address(render_effect_state,0x3fcu).value<=0xffffffffu)
            throw FormatError("TU1.5 renderer CPU state lacks its original native arena owner");
        call_actual(0x1303c0u); // Actual12818 model-resource construction.
        call_actual(0x12ebe0u); // Actual1281D texture-cache construction.
        source_renderer_cpu_interface_initialize_tu15(bank,[&](auto entry,auto eax) {
            auto input=frame({});if(eax)input.gp[0]=*eax;
            run_actual(entry,input);return input.gp[0];
        }); // Actual12822: query/interface storage follows model/texture storage.
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
        // Actual1410A7 first constructs HUD storage through1976CF, before
        //1410AC constructs the first-person bank. Drop notifications reach
        //197F2C/25AA57 and require that original50EECC allocation.
        call_actual(0x1976cfu);
        call_actual(0x168ba3u); // Actual1410AC first-person view storage owner.
        call_actual(0x168bc5u); // Actual1410C2 resets its real four-view bank.
        call_actual(0x1888a0u); // Original local input/camera storage constructor.
        call_actual(0x1eecf0u); // Literal44B1C4 player/view action-cache constructor.
        call_actual(0x2293e0u); // Literal44B590 per-view feedback request storage.
        call_actual(0x1563a0u); // Literal44B5B4 local player effect/impulse storage.
        call_actual(0x157e10u); // Literal44B3BC original director storage.
        call_actual(0x171e10u); // Actual row36 owns51DF70's arena allocation and CRC update.
        call_actual(0x228260u); // Actual row43 owns50EBF0's cleared feedback storage.
        call_actual(0x380dd0u); // Original_cinit speech-weight normalization constant.
        call_actual(0x380df0u); // Original_cinit facial-weight normalization constant.
        for(auto entry:{0x381710u,0x381730u,0x381750u,0x381770u})call_actual(entry);
        // Literal _cinit table46D768 constructs the static contact agent.
        call_actual(0x383130u);
        // Original _cinit2DBBAD walks this published table in order. These
        // complete callbacks populate71 AI handler masks before AI storage
        // and behavior selection consume them.
        for(std::uint32_t slot=0x46dad8u;slot<0x46dbf4u;slot+=4u)
            call_actual(word(bank,slot));
        // Original _cinit scalar callbacks precede29BE40's AI geometry tables.
        for(std::uint32_t slot=0x46dd00u;slot<0x46dd0cu;slot+=4u)
            call_actual(word(bank,slot));
        call_actual(0x13e9e0u); // Actual44B644 row53 owns cinematic interface storage.
        call_actual(0x2129d0u); // Actual row54 constructs HS syntax, object lists, threads and globals.
        call_actual(0x2035b0u); // Literal44B68C row55 constructs recorded animations.
        call_actual(0x1cfa40u); // Actual44B6F8 complete AI storage initializer.
        release_unused_loader_memory();
        detail::release_native_startup_code_pages();
        call_actual(0x137870u); // Actual44B7D0 row64 environment mixer storage constructor.
        {
            if(cache.is_mcc()) {
                // Converted metadata has its own real native allocation.
                // The original physical arena and its resource-cache requests
                // retain their existing owners and exact source procedures.
                // Serialized pointers remain logical32 tokens; the bank owns
                // the host64 backing and includes it in normal checkpoints.
                constexpr std::uint32_t native_base=0x90000000u,native_budget=0x60000000u;
                if(auto prepared=take_prepared_mcc_world_metadata(cache,native_base,native_budget)) {
                    canonical_cache.emplace(std::move(prepared->delivery));
                    sound_runtime_plan.emplace(std::move(prepared->sound));
                    sound_timing_plan.emplace(std::move(prepared->timing));
                } else {
                  canonical_cache.emplace(MccRuntimeCacheDelivery::plan(cache,native_base,native_budget,
                    [this](MccSoundRuntimePlan &&plan){
                        if(sound_runtime_plan)throw FormatError("MCC native delivery produced duplicate sound plans");
                        sound_runtime_plan.emplace(std::move(plan));
                    }));
                if(!sound_runtime_plan)throw FormatError("MCC native delivery lost its sound ownership plan");
                sound_timing_plan.emplace(build_mcc_sound_timing_reference(cache,*sound_runtime_plan,
                    [this](auto address,auto bytes){return canonical_cache->runtime_address(address,bytes);}));
                }
                // Strict complete-schema publication precedes every source
                // tag consumer. The unchanged vanilla index/binders own the
                // converted definitions after this native I/O boundary.
                auto tags=canonical_cache->tag_image();
                const auto* scenario=cache.find_tag(cache.scenario_tag(),tag_groups::scenario);
                if(!scenario)throw FormatError("Canonical MCC script conversion lost its scenario");
                const Bytes root(cache.read_tag_bytes(scenario->address,0x240));
                const auto script_block=[&](unsigned at,std::size_t stride) {
                    const auto count=root.u32(at);
                    if(!count)return std::span<const std::byte>{};
                    if(count>0x10000u)throw FormatError("MCC script bank exceeds its serialized datum space");
                    return cache.read_tag_bytes(root.u32(at+4u),std::size_t(count)*stride);
                };
                // Strings have byte offsets, independently of syntax's
                // serialized16-bit datum indices.
                const auto strings=root.u32(0x1b0)?cache.read_tag_bytes(root.u32(0x1b4),root.u32(0x1b0)):
                    std::span<const std::byte>{};
                const auto syntax=project_mcc_script_layout(cache,script_block(0x238,20),
                    strings,script_block(0x1b8,0x28),{true});
                mcc_script_profile.emplace(syntax,strings);
                MccScriptLayout unowned;
                for(const auto& requirement:syntax.requirements)
                    if(!mcc_script_profile->owns(requirement))unowned.requirements.push_back(requirement);
                unowned.require_native_ready();
                if(!syntax.nodes.empty()) {
                    const auto target=canonical_cache->runtime_address(root.u32(0x23c),syntax.nodes.size());
                    const auto region=std::ranges::find_if(tags.regions,[&](const auto& allocation) {
                        return target>=allocation.address &&
                            std::uint64_t(target)-allocation.address+syntax.nodes.size()<=allocation.bytes.size();
                    });
                    if(region==tags.regions.end())throw FormatError("Converted MCC syntax has no owned native backing");
                    std::copy(syntax.nodes.begin(),syntax.nodes.end(),
                        region->bytes.begin()+std::ptrdiff_t(target-region->address));
                }
                bsp=canonical_cache->bsp_image(selected);
                // Original11EF70/11F170 protects individual converted tag
                // ranges through MmSetAddressProtect. This native allocation
                // must belong to the same real page/protection/checkpoint
                // owner; a plain mounted byte vector cannot honor that ABI.
                const auto pages=(std::uint64_t(tags.reservation_bytes)+0xfffu)&~std::uint64_t(0xfffu);
                const auto physical_base=native_base&0x7fffffffu;
                if(!pages || physical_base+pages>0x80000000ull || !arena ||
                    arena->allocate_native_pages({tags.reservation_bytes,physical_base,
                        std::uint32_t(physical_base+pages-1u),0x1000u,4u})!=native_base)
                    throw FormatError("Converted MCC metadata requires its exact native page allocation");
                source_bind_runtime_cache_tags_tu15(bank,tags,native_cache_range());
                put(bank,0x55658cu,tags.index); // Native cache reservation input, not a source completion flag.
            } else {
                const auto tags=cache.view_source_runtime_cache_tags();
                reserve_map_pages(tags);
                source_bind_runtime_cache_tags_tu15(bank,tags);
                detail::release_delivered_cache_source_pages(tags);
            }
        } // The independent source bank owns its bytes before definition fixups.
        source_bind_runtime_cache_sound_gestalt_tu15(bank);
        // Original1918D9 prepares every resident collision, biped, creature,
        // physical model and vehicle definition. In particular11F02A passes
        // zero to1F2C70, resolving serialized shape references before birth.
        call_actual(0x11ef70u);
        if(canonical_cache) {
            // Native English is language0. Actual191A50 selects this matg
            // row and calls whole1A9DF0 before gameplay. Its original physical
            // allocations, two actual file reads and final readiness store
            // own publication; serialized offsets are not runtime pointers.
            const auto locale=word(bank,0x4ecc1cu)+0x188u;
            if(std::to_integer<unsigned>(bank.view(locale+0x18u,1)[0]))
                throw FormatError("Native MCC locale was already marked loaded before its resource owner");
            a=frame({});a.gp[7]=locale;run_actual(0x1a9df0u,a);
        }
        for(const auto&tag:cache.select_tags({.group=tag_groups::biped}))bipeds.emplace(tag.datum,runtime_tag_address(tag));
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
        // Complete renderer new-map callback publishes4922A0 from the
        // resident matg Rasterizer Data block.4BCC0's camouflage admission
        // consumes that actual pointer; cold loader backing is not publication.
        call_actual(0x22570u);
        call_actual(0x1f4760u); // Actual44B310 resets scenario kill-volume state.
        call_actual(0x118430u); // Actual473A04 activates/resets antenna storage.
        call_actual(0x180170u); // Actual44B40C activates/resets the decal pool and per-cluster links.
        call_actual(0x17e700u); // Actual44B454 effect-instance/location new-map reset.
        call_actual(0x186d30u); // Actual44B3A0 activates breakable-surface effect storage.
        call_actual(0x2b520u); // Actual44B358 activates/resets cached render states.
        call_actual(0x1976feu); // Actual1410B8 resets HUD storage and selects the resident authored HUD globals.
        install_sound_timing();
        call_actual(0x1378c0u); // Actual44B7D8 row64 new-map environment mixer reset.
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
    std::uint32_t runtime_tag_address(const Tag& tag) const {
        return canonical_cache?canonical_cache->runtime_address(tag.address,tag.size):tag.address;
    }
    SourceRuntimeCacheBspImage cache_bsp_image(std::size_t selected) const {
        return canonical_cache?canonical_cache->bsp_image(selected):cache_owner.load_source_runtime_cache_bsp_image(selected);
    }
    SourceRuntimeNativeCacheRange native_cache_range() const {
        if(!canonical_cache)throw FormatError("Native cache allocation requires a canonical delivery owner");
        const auto bytes=canonical_cache->reservation_bytes();
        return {bank.native_address(canonical_cache->index().region.address,bytes),bytes};
    }
    template<class Image>void reserve_map_pages(const Image &tags) {
        // Original125354..1253A2 reserves the owning delivery's extent from
        // the selected physical lower bound before tag publication. Xbox
        // delivery supplies header+1C. Native MCC metadata uses its separate
        // owned allocation instead of entering this physical-only path.
        // Omitting this reservation lets12ECF0 give map pages to textures.
        const auto requested=tags.reservation_bytes,selected=word(bank,0x4f2cf0u);
        if(!requested || requested>0xfffff000u || selected>=5)
            throw FormatError("Native physical map reservation requires a valid owned extent");
        const auto slot=0x4f2cfcu+selected*4u;
        const auto lower=word(bank,slot),upper=word(bank,0x4f2d10u+selected*4u);
        const auto bytes=(requested+0xfffu)&0xfffff000u;
        if(!lower || lower>=0x80000000u || bytes>0x7fffffffu-lower || lower+bytes>upper ||
            tags.index!=(lower|0x80000000u))
            throw FormatError("Native map does not fit its original selected physical reservation");
        for(const auto &region:tags.regions)
            if(region.address<tags.index || std::uint64_t(region.address)+region.bytes.size()>
                std::uint64_t(tags.index)+bytes)
                throw FormatError("Native map payload escapes its owned physical reservation");
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
            if(body_pose_publication_active)throw FormatError("Body pose completion cannot create a synchronization owner");
            const auto created=event_services.create_semaphore(0,1);
            if(created.error)throw std::system_error(created.error);
            lock_events.emplace(p,created.handle);return created.handle;
        };
        return {
            [this] {
                std::lock_guard guard(mutex);const auto id=std::this_thread::get_id();
                const auto at=thread_ids.find(id);if(at!=thread_ids.end())return at->second;
                if(body_pose_publication_active)throw FormatError("Body pose completion requires its existing native executor identity");
                if(next_thread_token>UINT32_MAX)throw FormatError("TU1.5 native thread-token namespace exhausted");
                const auto token=std::uint32_t(next_thread_token);thread_ids.emplace(id,token);++next_thread_token;return token;
            },
            [this,event](auto p) {
                if(body_pose_publication_active)throw FormatError("Body pose completion cannot wait on a synchronization owner");
                acquire_native_synchronization(event_services,event(p),0xffffffffu,{},5);},
            [this,event](auto p) {
                if(body_pose_publication_active)throw FormatError("Body pose completion cannot signal a synchronization owner");
                const auto r=event_services.release_semaphore(event(p),1);
                if(r.error)throw std::system_error(r.error);
            }
        };
    }
    SourcePhysicsWorldRuntimeServices build_services() {
        SourcePhysicsWorldRuntimeServices s;
        static constexpr std::array<std::uint32_t,3> instruction_service_sites{
            0x38846eu,0x21f590u,0x33b595u};
        s.instruction=[this](auto p) {
            //38846E publishes work for the actual GP and then polls its
            // request word. This graph has no native GP request owner yet;
            // stop before publishing work which no processor can consume.
            if(p==0x38846eu)throw FormatError("Original DSP image publication requires the native GP scratch/DMA execution owner");
            if(canonical_cache && p==0x21f590u && !sound_timing_active)throw FormatError("MCC duration requires authenticated resource timing admission");
            if(std::this_thread::get_id()==main_thread) {
                last_instruction=p;
                if(first_person_phase_active)first_person_phase_trace.push_back(p);
                if(weapon_effect_phase_active)weapon_effect_phase_trace.push_back(p);
            }

            if(p==0x33b595u) {
                if(execution_inputs)execution_inputs->require_executor();
                put(bank,native_tick_count,std::uint32_t(source_execution_input(bank,SourceExecutionInputKind::Milliseconds,0,
                    [this]{return std::uint32_t(clocks->milliseconds()+tick_adjustment);} )));
            }
        };
        // The environment is fixed for the process; read these switches once.
        static const bool full_instruction_callbacks=[]{
            const auto *p=std::getenv("HALO2_REFERENCE_INSTRUCTION_CALLBACKS");return p && *p;}();
        static const bool generic_instruction_search=[]{
            const auto *p=std::getenv("HALO2_REFERENCE_LITERAL_INSTRUCTIONS");return p && *p;}();
        if(!full_instruction_callbacks)
            s.instruction_cursor={&last_instruction,&first_person_phase_active,
                &weapon_effect_phase_active,instruction_service_sites,main_thread,&s.instruction.target_type()};
        s.instruction_cursor.literal_service_sites=!generic_instruction_search;
#if defined(__i386__) || defined(__x86_64__)
        s.timestamp_counter=[this] {
            if(execution_inputs)execution_inputs->require_executor();
            const auto value=source_execution_input(bank,SourceExecutionInputKind::TimestampCounter,0,
                [this]{return clocks->cycles()+cycle_adjustment;});
            if(first_person_phase_active)first_person_phase_cpu.push_back({1,0,value});
            if(weapon_effect_phase_active)weapon_effect_phase_cpu.push_back({1,0,value});
            return value;
        };
        // Explicit host CPU estimate policy; Xbox estimate bits remain a
        // separate hardware comparison. The original Newton steps still run.
        s.reciprocal_sqrt=[this](std::uint32_t bits) {
            if(execution_inputs)execution_inputs->require_executor();
            const auto value=std::uint32_t(source_execution_input(bank,SourceExecutionInputKind::ReciprocalSquareRoot,bits,
                [bits]{return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(
                    _mm_rsqrt_ss(_mm_set_ss(std::bit_cast<float>(bits)))));}));
            if(first_person_phase_active)first_person_phase_cpu.push_back({2,bits,value});
            if(weapon_effect_phase_active)weapon_effect_phase_cpu.push_back({2,bits,value});
            return value;
        };
#endif
        s.required_owner=[this](auto&m,const auto&c,auto&a,const auto&o){required(m,c,a,o);};
        return s;
    }
    std::uint32_t retain_resource_read(NativeResourceRead read) {
        if(body_pose_publication_active)throw FormatError("Body pose completion cannot retain a resource job");
        if(next_resource_read>UINT32_MAX)throw FormatError("TU1.5 native resource-read namespace exhausted");
        const auto job=std::uint32_t(next_resource_read);
        if(!resource_reads.emplace(job,read).second)throw FormatError("TU1.5 native resource-read alias reused");
        ++next_resource_read;return job;
    }
    void capture_projectile_effect(SourceProjectileDetonationTu15& event,std::uint32_t instance) {
        const auto effect=record(word(bank,0x4f720cu),instance);
        if(!effect||word(bank,effect+4u)!=event.effect)
            throw FormatError("TU1.5 explosion creation lost its actual full effect identity");
        SourceExecutionObserverScope read_only(bank);
        const auto scalar=[&](auto p){return std::bit_cast<float>(word(bank,p));};
        event.colors={word(bank,effect+0x104u),word(bank,effect+0x108u)};
        event.controls={scalar(effect+0x6cu),scalar(effect+0x70u)};
        event.replica_render=source_effect_replica_render_tu15(bank,instance,event.effect);
        event.effect_locations=source_effect_world_locations_tu15(bank,instance,event.effect);
        if(event.effect_locations.empty())throw FormatError("Actual explosion creation has no completed source location");
        event.effect_instance=instance;
    }
    template<class Run>
    SourceTeleporterEventsTu15 capture_teleporters(Run&& run,bool include_notices,bool rebase_input=false) {
        if(teleporter_event_capture)throw FormatError("Recursive original teleporter phase");
        std::vector<std::array<std::byte,36>> emitted;
        std::vector<std::uint32_t> blocked;
        std::vector<SourceHudMessageEvent> messages;
        struct Capture {
            Impl &owner;
            ~Capture(){owner.teleporter_event_capture=nullptr;owner.teleporter_blocked_capture=nullptr;
                owner.teleporter_notice_capture=nullptr;owner.teleporter_notice_recipient.reset();}
        } capture{*this};
        teleporter_event_capture=&emitted;
        teleporter_blocked_capture=include_notices?&blocked:nullptr;
        teleporter_notice_capture=include_notices?&messages:nullptr;
        const auto parent_stack=run();
        CallerFrame caller(*this,parent_stack);
        teleporter_event_capture=nullptr;
        teleporter_blocked_capture=nullptr;
        SourceTeleporterEventsTu15 result;
        if(include_notices)for(const auto player:blocked) {
            const auto row=record(word(bank,0x4f54f4u),player);
            if(Bytes(bank.view(row+0x28u,2)).u16(0)==0xffffu) {
                // The actual B2CD0 unit event is delivered to this native remote
                // recipient. Resolve its literal through whole1A6530 with the
                // real NONE local index.25AAF1 then leaves the host HUD bank
                // untouched; no player/view identity or lookup result is supplied.
                teleporter_notice_recipient=player;
                auto a=frame({});a.gp[0]=player;run_actual(0x1a6530u,a);
                teleporter_notice_recipient.reset();
            }
            const auto at=std::ranges::find(messages,player,&SourceHudMessageEvent::player);
            if(at==messages.end())throw FormatError("Original blocked event lost its actual localized message");
            result.notices.push_back(std::move(*at));messages.erase(at);
        }
        for(const auto &payload:emitted) {
            const Bytes event(payload);const auto player=event.u32(8);
            // These are the actual1A6BDE..1A6C07 stores, observed before the
            // unchanged1A87A0 call. No replayed event is injected into gameplay.
            if(event.u32(0)!=0 || event.u32(4)!=22 || event.u32(12)!=player ||
               event.u32(16)!=0xffffffffu || event.u32(20)!=0xffffffffu || event.u32(24)!=0xffffffffu)
                throw FormatError("Original teleporter activation payload differs");
            std::uint32_t selected{};
            //1A79A0 tests this event's cause-player role0, then fallback role4;
            // all other player/team comparisons above are NONE. Whole1A7A30
            // retains the actual authored recipient/admission predicates.
            for(const auto role:{0u,4u}) {
                std::array<std::uint32_t,10> arguments;arguments[0]=role;
                for(unsigned n=0;n<9;++n)arguments[n+1]=event.u32(n*4u);
                auto a=frame(arguments);a.gp[0]=player;a.gp[7]=a.gp[4]+8u;
                run_actual(0x1a7a30u,a);selected=a.gp[0];if(selected)break;
            }
            auto sound=0xffffffffu;
            if(selected) {
                // Teleporter event rows in the prepared retail maps have a
                // single immediate sound. Admit that proved native output shape
                // explicitly; alternate/queued choices keep their source owner.
                if(std::bit_cast<std::int32_t>(word(bank,selected+0xa0u))>0 ||
                   std::bit_cast<float>(word(bank,selected+0x3cu))!=0.f ||
                   (word(bank,selected+0x40u)&1u))
                    throw FormatError("Authored teleporter audio needs its alternate/delay/announcer owner");
                sound=word(bank,selected+0x48u);
                if(sound!=0xffffffffu && !cache_owner.find_tag(TagHandle{sound},TagGroup::from_string("snd!")))
                    throw FormatError("Original teleporter activation sound has no actual cache owner");
            }
            result.activations.push_back({player,sound});
        }
        if(rebase_input)for(const auto& event:result.activations) {
            const auto player=record(word(bank,0x4f54f4u),event.player);
            if(!player)throw FormatError("Completed original teleport lost its retained player");
            const auto unit=word(bank,player+0x2cu);
            if(unit==0xffffffffu)throw FormatError("Completed original teleport lost its current unit");
            reorient_player_look(event.player,unit);
        }
        return result;
    }
    // The lifted TU1.5 routine owning a sound entry: the reference the
    // native kernels are checked against (HALO2_SOUND_MANAGER_ORACLE).
    static void run_lifted_sound(SourcePhysicsContactMemory &m,std::uint32_t t,SourcePhysicsWorldRuntimeAbi &a,
        const SourcePhysicsWorldRuntimeServices &services,const SourcePhysicsContactRecordObserver &o) {
        using Run=void(*)(SourcePhysicsContactMemory&,std::uint32_t,SourcePhysicsWorldRuntimeAbi&,
            SourcePhysicsFactoryFloatingPoint,const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver&);
        const std::pair<bool(*)(std::uint32_t),Run> owners[]{
            {source_sound_random_short_tu15_owns_entry,source_run_sound_random_short_tu15},
            {source_sound_random_scalar_tu15_owns_entry,source_run_sound_random_scalar_tu15},
            {source_sound_scalar_tu15_owns_entry,source_run_sound_scalar_tu15},
            {source_sound_range_selection_tu15_owns_entry,source_run_sound_range_selection_tu15},
            {source_sound_permutation_tu15_owns_entry,source_run_sound_permutation_tu15},
            {source_sound_gain_tu15_owns_entry,source_run_sound_gain_tu15},
            {source_sound_properties_tu15_owns_entry,source_run_sound_properties_tu15},
            {source_sound_listener_selection_tu15_owns_entry,source_run_sound_listener_selection_tu15},
            {source_sound_binding_tu15_owns_entry,source_run_sound_binding_tu15},
            {source_sound_cache_query_tu15_owns_entry,source_run_sound_cache_query_tu15},
            {source_sound_chunk_readiness_tu15_owns_entry,source_run_sound_chunk_readiness_tu15},
            {source_sound_startup_tu15_owns_entry,source_run_sound_startup_tu15},
            {source_sound_sdk_tu15_owns_entry,source_run_sound_sdk_tu15},
        };
        for(const auto &[owns,run]:owners)if(owns(t)){run(m,t,a,fp,services,o);return;}
        throw FormatError("Native sound kernel has no lifted reference owner");
    }
    void required(SourcePhysicsContactMemory&m,const SourcePhysicsContactRuntimeCall&c,
        SourcePhysicsWorldRuntimeAbi&a,const SourcePhysicsContactRecordObserver&o) {
        if(native_input_devices && native_input_devices->try_call(m,c,a))return;
        if(body_pose_publication_active && c.target==0x1a3b70u)
            throw FormatError("Body pose completion requires already admitted animation resources");
        if(multiplayer_event_observer && !multiplayer_event_dispatch_active &&
            (c.target==0x1a8740u || c.target==0x1a87a0u)) {
            const auto options=word(m,0x4f3218u),mode=word(m,options+0xcu,1);
            if(c.target==0x1a87a0u && mode==4) {required_entry(m,c,a,o);return;}
            SourceMultiplayerEventTu15 event;
            const auto payload=c.target==0x1a8740u?a.gp[7]:a.gp[0];
            for(unsigned i=0;i<8;++i)event.words[i]=word(m,payload+i*4u);
            event.data=std::uint16_t(word(m,payload+32u,2));event.mode=std::uint8_t(mode);
            event.tick=word(m,word(m,0x51df54u)+8u);
            event.random_seed=word(m,word(m,0x4f3cd8u)+4u);
            event.game_over=word(m,options+0x1128u,1)!=0;
            const auto pool=word(m,0x4f54f4u),rows=word(m,pool+0x44u),bits=word(m,word(m,pool+0x48u));
            if(word(m,pool+0x20u)!=16 || word(m,pool+0x24u)!=0x21c)
                throw FormatError("Original event dispatch player pool schema differs");
            for(unsigned i=0;i<16;++i)if(bits&(1u<<i)) {
                const auto row=rows+i*0x21cu;auto& player=event.players[i];
                player.datum=(word(m,row,2)<<16)|i;player.flags=std::uint16_t(word(m,row+2u,2));
                player.membership=std::bit_cast<std::int8_t>(std::uint8_t(word(m,row+0xc0u,1)));
            }
            multiplayer_event_dispatch_active=true;
            try {required_entry(m,c,a,o);}catch(...) {multiplayer_event_dispatch_active=false;throw;}
            multiplayer_event_dispatch_active=false;
            SourceExecutionObserverScope read_only(bank);multiplayer_event_observer(event);return;
        }
        if(c.instruction==0x15ebeeu && c.target==0x1a6830u && !c.tail && multiplayer_teleporter_observer) {
            const auto events=capture_teleporters([&]{required_entry(m,c,a,o);return a.gp[4];},true,true);
            SourceExecutionObserverScope read_only(bank);
            multiplayer_teleporter_observer(events);return;
        }
        if(c.target!=0x15ddc0u || !multiplayer_round_end_observer) {
            // Nested-owner census (HALO2_NESTED_OWNER_CENSUS=<hex target>):
            // store set and host time of one dispatched callee, so a tick
            // owner's per-type sub-owners can be costed without generated edits.
            if(c.target==nested_owner_census_target && !nested_census_active) {
                nested_census_active=true;
                const auto begin=std::chrono::steady_clock::now();
                try{tick_owner_census([&]{required_entry(m,c,a,o);});}
                catch(...){nested_census_active=false;throw;}
                nested_census_active=false;
                nested_census_ns+=std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-begin).count();
                ++nested_census_calls;return;
            }
            required_entry(m,c,a,o);return;
        }
        const auto engine=word(m,0x4f63b8u),options=word(m,0x4f3218u);
        SourceMultiplayerRoundEndTu15 event{a.gp[3],word(m,word(m,0x51df54u)+8u),
            std::bit_cast<std::int16_t>(std::uint16_t(word(m,engine+0x6eu,2))),
            (word(m,options+0x184u,1)&1u)!=0,false};
        required_entry(m,c,a,o);
        event.game_over=word(m,options+0x1128u,1)!=0;
        SourceExecutionObserverScope read_only(bank);
        multiplayer_round_end_observer(event);
    }
    // Snapshot oracle for native sound code that calls into the rest of the
    // live world (so it cannot be predicted over a copy-on-write view): run
    // the lifted reference (every sound entry lifted) under a write journal,
    // roll it back, run the native code, and compare every byte either left
    // below the dead stack scratch. Live callee hooks run twice. Only the
    // outermost call is checked; nested native calls just run.
    unsigned snapshot_depth{};
    void snapshot_oracle(SourcePhysicsWorldRuntimeAbi &abi,
        const std::function<void(SourcePhysicsWorldRuntimeAbi &)> &lifted,
        const std::function<void(SourcePhysicsWorldRuntimeAbi &)> &native,const char *what) {
        if(snapshot_depth){native(abi);return;}
        struct Depth{unsigned &d;explicit Depth(unsigned &x):d(x){++d;}~Depth(){--d;}} depth(snapshot_depth);
        struct Journal {
            SourceRuntimeMemory &bank;std::vector<std::pair<std::uint32_t,std::vector<std::byte>>> undo;
            bool recording=true;
            explicit Journal(SourceRuntimeMemory &b):bank(b) {
                source_runtime_register_write_observer(bank,this,[this](std::uint32_t at,std::size_t bytes) {
                    const auto before=bank.view(at,bytes);undo.emplace_back(at,std::vector<std::byte>(before.begin(),before.end()));
                });
            }
            void stop(){if(recording)source_runtime_unregister_write_observer(bank,this);recording=false;}
            ~Journal(){stop();}
        };
        const auto entry_esp=abi.gp[4];
        const auto dead=[&](std::uint32_t at){return at<entry_esp && at>=entry_esp-0x10000u;};
        std::map<std::uint32_t,std::byte> lifted_bytes;
        auto reference=abi;
        {
            Journal journal(bank);
            {NativeTu15SoundReferenceScope lifted_only;lifted(reference);}
            for(const auto &[at,before]:journal.undo)for(std::size_t i=0;i<before.size();++i)
                lifted_bytes[std::uint32_t(at+i)]=bank.view(std::uint32_t(at+i),1)[0];
            journal.stop();
            for(auto it=journal.undo.rbegin();it!=journal.undo.rend();++it)bank.write(it->first,it->second);
        }
        auto native_abi=abi;
        {
            Journal journal(bank);
            native(native_abi);
            journal.stop();
            for(const auto &[at,before]:journal.undo)for(std::size_t i=0;i<before.size();++i)
                lifted_bytes.try_emplace(std::uint32_t(at+i),before[i]);
        }
        for(const auto &[at,value]:lifted_bytes)if(!dead(at) && bank.view(at,1)[0]!=value) {
            std::ostringstream why;why<<"Native "<<what<<" differs from the lifted original at "<<std::hex<<at
                <<": native="<<std::to_integer<unsigned>(bank.view(at,1)[0])<<" lifted="<<std::to_integer<unsigned>(value);
            throw FormatError(why.str());
        }
        if(native_abi.gp[4]!=reference.gp[4])throw FormatError(std::string("Native ")+what+" returned another stack pointer");
        abi=native_abi;
    }
    // Lag-compensation boundary (SourceHitscanQueryTu15). Without an armed
    // owner both targets take the unchanged path below.
    void required_entry(SourcePhysicsContactMemory&m,const SourcePhysicsContactRuntimeCall&c,
        SourcePhysicsWorldRuntimeAbi&a,const SourcePhysicsContactRecordObserver&o) {
        if(hitscan_owner && !c.tail && (c.target==0xfdeb0u || c.target==0x16c620u) &&
            hitscan_query(m,c,a,o))return;
        required_entry_original(m,c,a,o);
    }
    bool hitscan_query(SourcePhysicsContactMemory&m,const SourcePhysicsContactRuntimeCall&c,
        SourcePhysicsWorldRuntimeAbi&a,const SourcePhysicsContactRecordObserver&o) {
        if(&m!=&bank)throw std::runtime_error("TU1.5 live world bank owner differs");
        if(c.target==0xfdeb0u) {
            // FF100 reaches FF15C only for proj+BC bit5: the projectile's
            // complete first (and only) movement, in this same call.
            if(c.instruction!=0xff15cu || !hitscan_owner->armed())return false;
            if(word(m,a.gp[4])!=0xff161u)throw FormatError("TU1.5 instantaneous projectile move lost its original caller");
            const auto previous=hitscan_projectile;hitscan_projectile=word(m,a.gp[4]+4u);
            try{required_entry_original(m,c,a,o);}catch(...){hitscan_projectile=previous;throw;}
            hitscan_projectile=previous;return true;
        }
        if(hitscan_projectile==0xffffffffu ||
            (c.instruction!=0x1010abu && c.instruction!=0x1012b0u && c.instruction!=0x10130cu))return false;
        const auto sp=a.gp[4];
        if(word(m,sp)!=c.instruction+5u)throw FormatError("TU1.5 projectile path query lost its original caller");
        const auto scalar=[&](std::uint32_t p){return std::bit_cast<float>(word(m,p));};
        const auto vector=[&](std::uint32_t p){return Vec3{scalar(p),scalar(p+4u),scalar(p+8u)};};
        SourceHitscanQueryTu15 q;q.projectile=hitscan_projectile;q.site=c.instruction;q.stack=sp;
        q.flags=word(m,sp+4u);q.origin=vector(word(m,sp+8u));q.delta=vector(word(m,sp+12u));
        q.ignore_object=word(m,sp+16u);q.ignore_unit=word(m,sp+20u);q.result=word(m,sp+24u);
        const auto body=object_body(hitscan_projectile);
        q.definition=word(m,body);q.owner_player=word(m,body+0xc4u);
        const auto entry=a;const auto projectile=hitscan_projectile;
        // Calls nested inside 16C620 are not this projectile's path test.
        hitscan_projectile=0xffffffffu;
        try{hitscan_owner->query(q,bank,[&]{a=entry;required_entry_original(m,c,a,o);});}
        catch(...){hitscan_projectile=projectile;throw;}
        hitscan_projectile=projectile;return true;
    }
    void required_entry_original(SourcePhysicsContactMemory&m,const SourcePhysicsContactRuntimeCall&c,
        SourcePhysicsWorldRuntimeAbi&a,const SourcePhysicsContactRecordObserver&o) {
        if(&m!=&bank)throw std::runtime_error("TU1.5 live world bank owner differs");
        if(native_campaign_camera_startup_tu15_owns_entry(c.target)) {
            native_campaign_camera_startup_tu15_run(m,c.target,a,o);return;
        }
        if(native_campaign_camera_reset_tu15_owns_entry(c.target)) {
            native_campaign_camera_reset_tu15_run(m,c.target,a,o);return;
        }
        if(native_tu15_render_camera_packet_owns_entry(c.target)) {
            native_tu15_render_camera_packet_run(m,c.target,c.instruction,a,o);return;
        }
        if(native_campaign_camera_control_tu15_owns_entry(c.target)) {
            native_campaign_camera_control_tu15_run(m,c.target,a,o);return;
        }
        if(native_campaign_camera_commands_tu15_owns_entry(c.target)) {
            native_campaign_camera_commands_tu15_run(m,c.target,a,o);return;
        }
        if(source_texture_camera_frame_call_tu15(m,c,a))return;
        if(native_tu15_sound_census_enabled()) {
            const auto t=c.target;
            const bool native=native_tu15_sound_manager_owns_entry(t) &&
                native_tu15_sound_manager_mode()!=NativeTu15SoundManagerMode::reference;
            if(native || source_sound_dsp_image_tu15_owns_entry(t) || source_object_loop_audio_tu15_owns_entry(t) ||
                source_sound_startup_tu15_owns_entry(t) || source_sound_dma_storage_tu15_owns_entry(t) ||
                source_sound_hardware_storage_tu15_owns_entry(t) || source_sound_cache_query_tu15_owns_entry(t) ||
                source_sound_scalar_tu15_owns_entry(t) || source_sound_listener_selection_tu15_owns_entry(t) ||
                source_sound_gain_tu15_owns_entry(t) || source_sound_random_scalar_tu15_owns_entry(t) ||
                source_sound_properties_tu15_owns_entry(t) || source_sound_random_short_tu15_owns_entry(t) ||
                source_sound_range_selection_tu15_owns_entry(t) || source_sound_permutation_tu15_owns_entry(t) ||
                source_sound_binding_tu15_owns_entry(t) || source_sound_chunk_readiness_tu15_owns_entry(t) ||
                source_sound_sdk_tu15_owns_entry(t) || (t>=0x383d00u && t<0x38f600u))
                native_tu15_sound_census(t,native);
        }
        if(c.instruction==0x1a65dbu && c.target==0x25aaf1u && !c.tail && teleporter_notice_capture) {
            if(word(m,a.gp[4])!=0x1a65e0u)throw FormatError("Original blocked message caller differs");
            const auto view=a.gp[0];
            const auto player=teleporter_notice_recipient?*teleporter_notice_recipient:
                view<4?word(m,word(m,0x4f54f0u)+0xcu+view*4u):0xffffffffu;
            if(player==0xffffffffu || !record(word(m,0x4f54f4u),player))
                throw FormatError("Original blocked message lost its actual player recipient");
            SourceHudMessageEvent event{player,word(m,word(m,0x51df54u)+8u),{}};
            const auto text=word(m,a.gp[4]+4u);
            for(unsigned n=0;n<256;++n) {
                const auto character=word(m,text+n*2u,2);
                if(!character)break;
                if(n==255)throw FormatError("Original blocked message is not terminated");
                event.text+=char16_t(character);
            }
            teleporter_notice_capture->push_back(std::move(event));
        }
        if(c.instruction==0xb2d2du && c.target==0xb8180u && !c.tail && teleporter_blocked_capture) {
            const auto sp=a.gp[4],event=word(m,sp+0x14u);
            if(word(m,sp)!=0xb2d32u || word(m,sp+4u)!=9 || word(m,sp+8u)!=1 ||
               word(m,sp+0x10u)!=12 || word(m,event,2)!=4 ||
               word(m,event+4u)!=0xffffffffu || word(m,event+8u)!=0)
                throw FormatError("Original blocked unit event caller/payload differs");
            const auto unit=word(m,word(m,sp+0xcu)),player=word(m,object_body(unit)+0x13cu);
            if(player==0xffffffffu || !record(word(m,0x4f54f4u),player))
                throw FormatError("Original blocked unit event lost its actual player");
            teleporter_blocked_capture->push_back(player);
        }
        if(c.instruction==0x1a6c0bu && c.target==0x1a87a0u && !c.tail && teleporter_event_capture) {
            if(word(m,a.gp[4])!=0x1a6c10u)throw FormatError("Original teleporter event caller differs");
            std::array<std::byte,36> payload;m.read(a.gp[0],payload);
            teleporter_event_capture->push_back(payload);
        }
        if(c.instruction==0x179063u&&c.target==0x17c6e0u&&!c.tail) {
            //178E30 has completed locations/colors before this first update.
            // A finite one-shot can retire inside17C6E0 and return NONE even
            // though its real creation completed (actual lifecycle v983).
            if(word(bank,a.gp[4])!=0x179068u||a.gp[0]!=a.gp[5])
                throw FormatError("TU1.5 effect first-update caller ABI differs");
            if(!projectile_effect_captures.empty()) {
                const auto& capture=projectile_effect_captures.back();
                if(capture.request==a.gp[7]) {
                    if(capture.event->effect_instance!=0xffffffffu)
                        throw FormatError("TU1.5 explosion repeats its initial effect publication");
                    capture_projectile_effect(*capture.event,a.gp[0]);
                }
            }
            if(!weapon_effect_captures.empty()) {
                auto& capture=weapon_effect_captures.back();
                if(capture.request==a.gp[7]) {
                    auto& event=*capture.event;
                    const auto effect=record(word(bank,0x4f720cu),a.gp[0]);
                    if(capture.birth || !effect ||
                        word(bank,effect+4u)!=event.input.effect)
                        throw FormatError("TU1.5 weapon first update lost its actual birth allocation");
                    SourceExecutionObserverScope read_only(bank);
                    capture.birth=SourceWeaponEffectPresentation{a.gp[0],
                        {word(bank,effect+0x104u),word(bank,effect+0x108u)},true};
                    event.birth_locations=source_effect_world_locations_tu15(bank,a.gp[0],event.input.effect);
                }
            }
            source_run_death_cycle_runtime_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(c.instruction==0x1796e5u&&c.target==0x178e30u&&!c.tail&&!weapon_effect_captures.empty()) {
            auto& capture=weapon_effect_captures.back();
            if(!capture.request) {
                const auto request=word(bank,a.gp[4]+4u);
                if(word(bank,a.gp[4])!=0x1796eau || word(bank,request+4u)!=capture.event->input.effect)
                    throw FormatError("TU1.5 scheduled weapon factory lost its exact request owner");
                capture.request=request;
            }
        }
        if(canonical_cache&&c.instruction==0x1714acu&&c.target==0x3bca0u&&!c.tail) {
            //171440 has its own inline bitmap index*116 lookup. Qualify the
            // real type0/full-handle/frame record before crossing this native
            // resource boundary; do not interpret MCC texture offsets as live
            // original cache datums (actual frame32 frontier v925).
            if(word(bank,a.gp[4])!=0x1714b1u||(a.gp[7]&65535u)!=2u)
                throw FormatError("MCC inline bitmap resource caller ABI differs");
            const auto count=word(bank,a.gp[6]),records=word(bank,a.gp[6]+4u),index=a.gp[5];
            if(count>65536u||index>=count||std::uint64_t(records)+std::uint64_t(index)*8u+8u>0x100000000ull)
                throw FormatError("MCC inline bitmap resource record escapes its original array");
            const auto record=records+index*8u;
            const Bytes input(bank.view(record,8));
            const auto frame=std::bit_cast<std::int16_t>(input.u16(2));
            if(input.u16(0)!=0||frame<0||
                std::uint64_t(word(bank,a.gp[1]+0x48u))+unsigned(frame)*116u!=a.gp[0])
                throw FormatError("MCC inline bitmap resource lost its actual typed frame flow");
            if(!cpu_bitmap_projection)cpu_bitmap_projection=std::make_unique<private_cpu_bitmap::WorldProjection>(cache_owner,bank,*canonical_cache);
            a.gp[0]=cpu_bitmap_projection->row_for_canonical_frame(TagHandle{input.u32(4)},a.gp[1],unsigned(frame));
            source_run_physics_live_support_tu15(bank,0x3bca0u,a,fp,services(),o);return;
        }
        if(canonical_cache && (c.instruction==0xd5506u || c.instruction==0x1185f3u) && c.target==0x13a030u && !c.tail) {
            // Both callers use the complete original lookup ABI.118480
            // consumes only width (11860A); D5506 additionally needs the
            // existing owned pixel/header projection for resource access.
            const auto returned=c.instruction==0xd5506u?0xd550bu:0x1185f8u;
            if(word(bank,a.gp[4])!=returned)
                throw FormatError("MCC CPU bitmap lookup lost its exact original caller");
            if(!cpu_bitmap_projection)cpu_bitmap_projection=std::make_unique<private_cpu_bitmap::WorldProjection>(cache_owner,bank,*canonical_cache);
            if(c.instruction==0x1185f3u)cpu_bitmap_projection->lookup_dimensions(a,fp,services(),o);
            else cpu_bitmap_projection->lookup(a,fp,services(),o);
            return;
        }
        if(c.target==0x21f590u && canonical_cache){run_sound_duration(c,a,o);return;}
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
                struct CaptureLifetime {
                    Impl& owner;
                    ~CaptureLifetime(){
                        auto& capture=owner.weapon_effect_captures.back();
                        // Nested initial-update effects can write their own
                        // colors after this birth. Retain this full generation's
                        // immutable snapshot, including when its return is NONE.
                        if(capture.birth)capture.event->presentation=*capture.birth;
                        owner.weapon_effect_captures.pop_back();
                    }
                };
                try {
                    weapon_effect_captures.push_back({&event,{},{}});
                    CaptureLifetime lifetime{*this};
                    required(m,{c.target,c.target,false},a,observed);
                }
                catch(...) {if(capture)weapon_effect_phase_active=false;throw;}
                if(capture) {
                    weapon_effect_phase_active=false;
                    weapon_effect_phase_observer(false,a,bank,weapon_effect_phase_trace,weapon_effect_phase_cpu);
                }
                if(a.gp[0]!=0xffffffffu) {
                    const auto effect=record(word(m,0x4f720cu),a.gp[0]);
                    if(!effect || word(m,effect+4u)!=input.effect)
                        throw FormatError("Scheduled TU1.5 weapon effect lost its completed allocation");
                    if(event.presentation.instance!=0xffffffffu && event.presentation.instance!=a.gp[0])
                        throw FormatError("Scheduled TU1.5 weapon effect changed its completed birth generation");
                    event.presentation.instance=a.gp[0];
                    event.presentation.colors={word(m,effect+0x104u),word(m,effect+0x108u)};
                    event.presentation.colors_observed=true;
                }
                if(weapon_effect_observer)weapon_effect_observer(event);return;
            }
        }
        if(c.instruction==0x1d94ceu && c.target==0x324f80u && !c.tail && (body_impulse_observer || body_impulse_publisher)) {
            // The diagnostic observer requires the exact caller/ownership
            // shape. Presentation publication alone leaves an unqualified
            // shape to the ordinary dispatch below; both are read-only.
            const bool strict=bool(body_impulse_observer);
            const auto sp=a.gp[4],list=a.gp[6],index=a.gp[7],motion=a.gp[1];
            std::optional<SourceBodyImpulseTu15> event;
            try {
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
            event.emplace(SourceBodyImpulseTu15{object,word(m,word(m,0x51df54u)+8u),index,{},{},{},{}});
            m.read(motion,event->before);m.read(word(m,sp+4u),event->impulse);m.read(word(m,sp+8u),event->point);
            }catch(const std::exception &) {
                if(strict)throw;
                event.reset();
            }
            if(event) {
                required(m,{c.target,c.target,false},a,o);
                m.read(motion,event->after);
                if(body_impulse_observer)body_impulse_observer(*event);
                if(body_impulse_publisher){SourceExecutionObserverScope read_only(bank);body_impulse_publisher(*event);}
                return;
            }
        }

        if(c.target==0x220c60u && !c.tail && loop_sound_request_observer &&
            (c.instruction==0x18d7fau || c.instruction==0x18d93bu || c.instruction==0x224342u)) {
            const auto sp=a.gp[4];
            if(word(m,sp)!=c.instruction+5u)throw FormatError("Original loop sound caller differs");
            SourceLoopSoundRequestTu15 request;
            for(unsigned i=0;i<request.arguments.size();++i)request.arguments[i]=word(m,sp+4u+i*4u);
            m.read(request.arguments[4],request.location);
            if(c.instruction==0x18d7fau || c.instruction==0x18d93bu) {
                const auto loop=record(word(m,0x4f9b5cu),request.arguments[2]);
                if(loop && (word(m,loop+3u,1)&15u)==1u) {
                    if(word(m,loop+0xcu)!=request.arguments[0])
                        throw FormatError("Original attached loop lost its definition owner");
                    request.emitter_owner=word(m,loop+0x10u);
                }
            }
            required(m,{c.target,c.target,false},a,o);
            SourceExecutionObserverScope read_only(bank);
            loop_sound_request_observer(request);return;
        }
        if(c.instruction==0x18cefcu && c.target==0x223b20u && !c.tail && (sound_submit_observer || sound_request_observer)) {
            const auto sp=a.gp[4],request=a.gp[0],sound=word(m,sp+4u);
            if(word(m,sp)!=0x18cf01u)throw FormatError("Original sound submit caller differs");
            const auto raw=bank.view(request,160);
            std::array<std::byte,160> caller;
            if(sound_request_observer)std::copy(raw.begin(),raw.end(),caller.begin());
            const bool was_active=first_person_phase_active;
            if(!was_active){first_person_phase_trace.clear();first_person_phase_cpu.clear();}
            const auto trace_begin=first_person_phase_trace.size(),cpu_begin=first_person_phase_cpu.size();
            if(sound_submit_observer)sound_submit_observer(true,sound,request,a,bank,{},{});
            // Forward the full unchanged request/ABI into the complete source
            // admission owner. Its real result, including NONE, remains final.
            if(sound_submit_observer)first_person_phase_active=true;
            try {required(m,{c.target,c.target,false},a,o);}catch(...) {first_person_phase_active=was_active;throw;}
            first_person_phase_active=was_active;
            if(sound_submit_observer)sound_submit_observer(false,sound,request,a,bank,
                std::span(first_person_phase_trace).subspan(trace_begin),
                std::span(first_person_phase_cpu).subspan(cpu_begin));
            if(sound_request_observer) {
                SourceExecutionObserverScope read_only(bank);
                sound_request_observer(sound,caller);
            }
            return;
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
        if((c.instruction==0x17db3bu || c.instruction==0x17db6du) && c.target==0xc2380u && !c.tail) {
            // The literal part birth still executes completeC2380. Observing
            // its real return is the identity producer; no typed-pool remap.
            std::optional<SourceEffectLightBirthTu15> input;
            if(effect_light_birth_observer) {
                input=source_effect_light_birth_before_tu15(bank,c.instruction,a);
                SourceExecutionObserverScope read_only(bank);
                effect_light_birth_observer(true,*input,a,bank);
            }
            source_run_death_cycle_runtime_tu15(m,c.target,a,fp,services(),o);
            if(input) {
                const auto output=source_effect_light_birth_after_tu15(bank,*input,a);
                SourceExecutionObserverScope read_only(bank);
                effect_light_birth_observer(false,output,a,bank);
            }
            return;
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
            projectile_effect_captures.push_back({request,&event});
            {
                struct Scope {
                    std::vector<ProjectileEffectCapture>& captures;
                    ~Scope(){captures.pop_back();}
                } scope{projectile_effect_captures};
                required(m,{c.target,c.target,false},a,o);
            }
            if(a.gp[0]!=0xffffffffu) {
                if(event.effect_instance!=0xffffffffu&&event.effect_instance!=a.gp[0])
                    throw FormatError("TU1.5 explosion changed its constructed effect identity");
                capture_projectile_effect(event,a.gp[0]);
            } else if(event.effect_instance!=0xffffffffu&&record(word(m,0x4f720cu),event.effect_instance)) {
                throw FormatError("TU1.5 effect returned NONE without its observed original retirement");
            }
            projectile_detonations.push_back(std::move(event));return;
        }
        if(source_try_native_process_heap_memory_call(bank,c,a,image,virtual_memory))return;
        // The complete Process memmove owns this shared entry. Other generated
        // closures contain private copies with incomplete indirect branches;
        // they must not preempt callers outside those closures.
        if(c.target==0x32a990u) {
            source_run_process_heap_with_native_platform(bank,0x326e20u,a,image,fp,
                services(),virtual_memory,locks,lock_services(),o);return;
        }
        if(source_try_native_physics_lock_call(m,c,a,image,locks,lock_services()))return;
        // Actual TU1.5 RtlFreeHeap callers use this world's same native
        // critical-section owner as RtlAllocateHeap. These PCs are already
        // TU1.5 addresses and must not be remapped as canonical TU1.1 PCs.
        constexpr std::array free_heap_locks{
            std::array{0x2deeeeu,0x80000115u,0x2deef4u},
            std::array{0x2df027u,0x80000126u,0x2df02du},
            std::array{0x2df076u,0x80000126u,0x2df07cu}};
        for(const auto &owner:free_heap_locks)if(c.instruction==owner[0] && c.target==owner[1]) {
            const auto sp=a.gp[4],section=word(m,sp+4u);
            if(c.tail || word(m,sp)!=owner[2] || section!=word(m,word(m,0x56ee28u)+0x580u))
                throw FormatError("TU1.5 native heap-free critical-section frame differs");
            if(c.target==0x80000115u)locks.enter(m,section,lock_services());
            else locks.leave(m,section,lock_services());
            a.gp[4]+=8u;return;
        }
        if(pool_storage.try_call(m,c,a))return;
        if(renderer_palette && renderer_palette->try_call(m,c,a))return;
        if(c.instruction==0x21f4bdu) {
            require_executor();
            const SourceSoundCacheIoTu15Services io{
                [this](auto encoded,auto bytes){return resources.load(encoded,bytes);},
                [this](auto descriptor,auto destination,auto bytes){return retain_resource_read({descriptor,destination,bytes});}};
            if(source_try_sound_cache_io_tu15(m,c,a,io))return;
        }
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
        if(sound_reset && sound_reset->try_call(m,c,a,fp,services(),o))return;
        if(sound_dma && sound_dma->dispatch(m,c,a))return;
        if(sound_sdk_resources && sound_sdk_resources->dispatch(m,c,a,o))return;
        if(sound_dma && source_sound_hardware_storage_tu15_owns_entry(c.target)) {
            source_run_sound_hardware_storage_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(sound_dma && source_sound_dma_storage_tu15_owns_entry(c.target)) {
            source_run_sound_dma_storage_tu15(m,c.target,a,fp,services(),o);return;
        }
        // These complete cold CPU owners require no SDK device. Keep their
        // admission narrow: the unrun12560 GPU initializer is not exposed.
        if(c.target==0x169d0u) {
            source_run_renderer_request_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(c.target==0x1fb90u || c.target==0x20e30u || c.target==0x20e90u) {
            source_run_renderer_tail_storage_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_sound_sdk_tu15_owns_entry(c.target)) {
            source_run_sound_sdk_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(native_tu15_sound_manager_owns_entry(c.target) && !native_tu15_sound_reference_active() &&
            native_tu15_sound_manager_mode()!=NativeTu15SoundManagerMode::reference) {
            // Native sound manager kernels. HALO2_SOUND_MANAGER_ORACLE checks
            // every call against the lifted routine on the live game state.
            if(native_tu15_sound_manager_mode()==NativeTu15SoundManagerMode::oracle &&
                native_tu15_sound_manager_calls_out(c.target)) {
                snapshot_oracle(a,
                    [&](SourcePhysicsWorldRuntimeAbi &abi){required_entry(m,c,abi,o);},
                    [&](SourcePhysicsWorldRuntimeAbi &abi){native_tu15_sound_manager_run(m,c.target,abi,fp,o,services());},
                    "sound kernel");
            } else if(native_tu15_sound_manager_mode()==NativeTu15SoundManagerMode::oracle &&
                !native_tu15_sound_manager_calls_out(c.target)) {
                const auto why=native_tu15_sound_manager_compare(m,c.target,a,fp,o,
                    [&](auto &memory,auto &abi,const auto &observer) {
                        run_lifted_sound(memory,c.target,abi,services(),observer);
                    },services());
                if(!why.empty())throw FormatError(why);
            } else native_tu15_sound_manager_run(m,c.target,a,fp,o,services());
            return;
        }
        if(source_sound_cache_query_tu15_owns_entry(c.target)) {
            source_run_sound_cache_query_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(c.target==0x127b20u) {
            source_run_sound_scalar_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(c.target==0x12a810u || c.target==0x12a930u) {
            source_run_sound_listener_selection_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(c.target==0x127cf0u) {
            source_run_sound_gain_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(c.target==0x127d70u) {
            source_run_sound_random_scalar_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(c.target==0x2290c0u) {
            source_run_sound_properties_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(c.target==0x127e00u) {
            source_run_sound_random_short_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(c.target==0x21f960u) {
            source_run_sound_range_selection_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(c.target==0x21fa80u) {
            source_run_sound_permutation_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(c.target==0x12b1c0u) {
            source_run_sound_binding_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(c.target==0x128970u || c.target==0x1293f0u || c.target==0x21f3b0u) {
            source_run_sound_chunk_readiness_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_sound_startup_tu15_owns_entry(c.target)) {
            source_run_sound_startup_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_static_registration_tu15_owns_entry(c.target)) {
            source_run_static_registration_tu15(m,c.target,a,fp,services(),o);return;
        }
        // These camera owners have the current exact FP spill lowering. The
        // older sandbox closure also contains18A300; do not shadow this owner.
        if(source_campaign_environment_tu15_owns_entry(c.target)) {
            source_run_campaign_environment_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_vehicle_look_tu15_owns_entry(c.target)) {
            source_run_vehicle_look_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_dialogue_admission_tu15_owns_entry(c.target)) {
            SourcePhysicsEntryObservation diagnostic(m,c.target,[&]{return a;});
            source_run_dialogue_admission_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_targeting_input_tu15_owns_entry(c.target)) {
            source_run_targeting_input_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_input_collection_tu15_owns_entry(c.target)) {
            source_run_input_collection_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_profile_binding_tu15_owns_entry(c.target)) {
            source_run_profile_binding_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(c.target==0x21ee30u || c.target==0x21e5d0u || c.target==0x1273c0u ||
           c.target==0x127460u || c.target==0x127430u) {
            source_run_input_startup_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_vehicle_director_tu15_owns_entry(c.target)) {
            source_run_vehicle_director_tu15(m,c.target,a,fp,services(),o);return;
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
        if(pstencil_native_light_portal_tu15_owns_entry(c.target)) {
            pstencil_run_native_light_portal_tu15(m,c.target,a,fp,services(),o);return;
        }
        // Complete per-view math has no query/cache publication. Keep the
        // existing16EA60 route; this module owns it only as a private callee.
        if(pstencil_native_light_constants_tu15_owns_entry(c.target)) {
            pstencil_run_native_light_constants_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(pstencil_native_scenario_lights_tu15_owns_entry(c.target)) {
            pstencil_run_native_scenario_lights_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(pstencil_native_light_shape_tu15_owns_entry(c.target)) {
            pstencil_run_native_light_shape_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(pstencil_native_light_update_tu15_owns_entry(c.target)) {
            pstencil_run_native_light_update_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(c.target==0xc2a80u || c.target==0xc3300u || c.target==0xc35d0u) {
            source_run_effect_light_view_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_receiver_request_tu15_owns_entry(c.target)) {
            source_run_receiver_request_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_death_cycle_runtime_tu15_owns_entry(c.target)) {
            source_run_death_cycle_runtime_tu15(m,c.target,a,fp,services(),o);return;
        }
        // Reuse the already-verified compiled CRT leaves used by native startup.
        if(c.target==0x3817f0u) {
            source_run_ratio_threshold_crt_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(c.target==0x380d50u || c.target==0x380d30u ||
           c.target==0x381010u || c.target==0x3811b0u) {
            source_run_environment_crt_pair_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_map_world_support_tu15_owns_entry(c.target)) {
            source_run_map_world_support_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_ui_script_startup_tu15_owns_entry(c.target)) {
            source_run_ui_script_startup_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(c.target==0x680c0u) {
            source_run_network_session_startup_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_scene_device_constructor_tu15_owns_entry(c.target)) {
            if(c.target==0xf0e70u) {
                SourcePhysicsEntryObservation diagnostic(m,c.target,[&]{return a;});
                source_run_scene_device_constructor_tu15(m,c.target,a,fp,services(),o);return;
            }
            source_run_scene_device_constructor_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_first_person_light_pose_tu15_owns_entry(c.target)) {
            source_run_first_person_light_pose_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_model_render_tu15_owns_entry(c.target)) {
            if(canonical_cache && c.target==0x3daf0u) {
                if(!cpu_bitmap_projection)cpu_bitmap_projection=std::make_unique<private_cpu_bitmap::WorldProjection>(cache_owner,bank,*canonical_cache);
                private_cpu_bitmap::run_model_3daf0_with_inline_cpu_bitmaps(bank,a,fp,services(),o,
                    [this](TagHandle requested,unsigned root){return cpu_bitmap_projection->first_row_for_canonical_root(requested,root);});
                return;
            }
            source_run_model_render_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_collision_instance_storage_tu15_owns_entry(c.target)) {
            source_run_collision_instance_storage_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_vehicle_occupancy_tu15_owns_entry(c.target)) {
            source_run_vehicle_occupancy_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_conveyor_tu15_owns_entry(c.target)) {
            source_run_conveyor_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_collision_query_tu15_owns_entry(c.target)) {
            source_run_collision_query_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(c.target==0xdc540u) {
            source_run_damage_model_children_tu15(m,a,fp,services(),o);return;
        }
        if(source_damage_receiver_tu15_owns_entry(c.target)) {
            source_run_damage_receiver_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_vehicle_auxiliary_tu15_owns_entry(c.target)) {
            source_run_vehicle_auxiliary_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_vehicle_evasion_tu15_owns_entry(c.target)) {
            source_run_vehicle_evasion_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_constraint_instance_tu15_owns_entry(c.target)) {
            source_run_constraint_instance_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_body_impulse_tu15_owns_entry(c.target)) {
            source_run_body_impulse_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_cloth_tu15_owns_entry(c.target)) {
            source_run_cloth_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_physics_deactivation_tu15_owns_entry(c.target)) {
            source_run_physics_deactivation_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_sound_dsp_image_tu15_owns_entry(c.target)) {
            source_run_sound_dsp_image_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_campaign_audio_tu15_owns_entry(c.target)) {
            source_run_campaign_audio_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_object_sound_tu15_owns_entry(c.target)) {
            source_run_object_sound_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_object_loop_audio_tu15_owns_entry(c.target)) {
            source_run_object_loop_audio_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_weapon_hud_tu15_owns_entry(c.target)) {
            source_run_weapon_hud_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_crt_float_tu15_owns_entry(c.target)) {
            source_run_crt_float_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_cinematic_shadow_control_tu15_owns_entry(c.target)) {
            source_run_cinematic_shadow_control_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_shadow_frame_tu15_owns_entry(c.target)) {
            source_run_shadow_frame_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_shadow_array_swap_tu15_owns_entry(c.target)) {
            source_run_shadow_array_swap_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_transparent_group_tu15_owns_entry(c.target)) {
            source_run_transparent_group_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_transparent_collection_tu15_owns_entry(c.target)) {
            source_run_transparent_collection_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_havok_static_lifetime_tu15_owns_entry(c.target)) {
            source_run_havok_static_lifetime_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_recorded_animation_tu15_owns_entry(c.target)) {
            std::optional<SourcePhysicsEntryObservation> observation;
            if(c.instruction!=c.target)observation.emplace(m,c.target,[&]{return a;});
            source_run_recorded_animation_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_damage_feedback_tu15_owns_entry(c.target)) {
            std::optional<SourcePhysicsEntryObservation> nested;
            if(c.instruction!=c.target)nested.emplace(m,c.target,[&]{return a;});
            source_run_damage_feedback_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_object_material_tu15_owns_entry(c.target)) {
            SourcePhysicsEntryObservation diagnostic(m,c.target,[&]{return a;});
            source_run_object_material_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_level_handles_new_map_tu15_owns_entry(c.target)) {
            source_run_level_handles_new_map_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(c.target==0x171e10u || c.target==0x228260u) {
            source_run_render_object_startup_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_geometry_cache_frame_tu15_owns_entry(c.target)) {
            source_run_geometry_cache_frame_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_campaign_activation_tu15_owns_entry(c.target)) {
            source_run_campaign_activation_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_result_recording_support_tu15_owns_entry(c.target)) {
            source_run_result_recording_support_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_unit_view_reset_tu15_owns_entry(c.target)) {
            source_run_unit_view_reset_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_unit_animation_cancel_tu15_owns_entry(c.target)) {
            source_run_unit_animation_cancel_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_physics_live_support_tu15_owns_entry(c.target)) {
            source_run_physics_live_support_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_vehicle_bv_agent_tu15_owns_entry(c.target)) {
            source_run_vehicle_bv_agent_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_particle_swarm_tu15_owns_entry(c.target)) {
            source_run_particle_swarm_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(c.target==0x2aa360u && source_script_effect_tu15_owns_entry(c.target)) {
            source_run_script_effect_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_campaign_cinematic_tu15_owns_entry(c.target)) {
            source_run_campaign_cinematic_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_campaign_completion_tu15_owns_entry(c.target)) {
            source_run_campaign_completion_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_texture_camera_producer_tu15_owns_entry(c.target)) {
            source_run_texture_camera_producer_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_campaign_camera_tu15_owns_entry(c.target)) {
            source_run_campaign_camera_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(native_campaign_screen_lifecycle_tu15_owns_entry(c.target)) {
            native_campaign_screen_lifecycle_tu15_run(m,c.target,a,o);return;
        }
        if(native_campaign_screen_timed_tu15_owns_entry(c.target)) {
            native_campaign_screen_timed_tu15_run(m,c.target,a,o);return;
        }
        if(source_animation_composition_tu15_owns_entry(c.target)) {
            source_run_animation_composition_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_object_functions_tu15_owns_entry(c.target)) {
            source_run_object_functions_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_unit_cinematic_tu15_owns_entry(c.target)) {
            source_run_unit_cinematic_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_unit_speech_tu15_owns_entry(c.target)) {
            source_run_unit_speech_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_resource_cache_pressure_tu15_owns_entry(c.target)) {
            source_run_resource_cache_pressure_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_lens_flare_tu15_owns_entry(c.target)) {
            source_run_lens_flare_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_lens_query_tu15_owns_entry(c.target)) {
            source_run_lens_query_tu15(m,c.target,a,fp,services(),o);return;
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
        if(native_campaign_ai_queries_tu15_owns_entry(c.target)) {
            native_campaign_ai_queries_tu15_run(m,c.target,a,services(),o);return;
        }
        if(native_campaign_ai_behavior_reset_tu15_owns_entry(c.target)) {
            native_campaign_ai_behavior_reset_tu15_run(m,c.target,a,o);return;
        }
        if(native_campaign_ai_area_tu15_owns_entry(c.target)) {
            native_campaign_ai_area_tu15_run(m,c.target,a,o);return;
        }
        if(native_campaign_ai_clump_retirement_tu15_owns_entry(c.target)) {
            native_campaign_ai_clump_retirement_tu15_run(m,c.target,a,services());return;
        }
        if(native_campaign_ai_combat_reset_tu15_owns_entry(c.target)) {
            native_campaign_ai_combat_reset_tu15_run(m,c.target,a,services());return;
        }
        if(native_campaign_unit_action_support_tu15_owns_call(c.target,a)) {
            native_campaign_unit_action_support_tu15_run(m,c.target,a,services());return;
        }
        if(native_campaign_unit_animation_reset_tu15_owns_entry(c.target)) {
            native_campaign_unit_animation_reset_tu15_run(m,c.target,a,services());return;
        }
        if(source_campaign_ai_conditions_tu15_owns_entry(c.target)) {
            source_run_campaign_ai_conditions_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_campaign_ai_scalar_defaults_tu15_owns_entry(c.target)) {
            source_run_campaign_ai_scalar_defaults_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_campaign_ai_object_decisions_tu15_owns_entry(c.target)) {
            source_run_campaign_ai_object_decisions_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_campaign_ai_object_callbacks_tu15_owns_entry(c.target)) {
            source_run_campaign_ai_object_callbacks_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_campaign_birth_query_variants_tu15_owns_entry(c.target)) {
            source_run_campaign_birth_query_variants_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_campaign_ai_actor_behavior_tu15_owns_entry(c.target)) {
            source_run_campaign_ai_actor_behavior_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_campaign_ai_placement_settings_tu15_owns_entry(c.target)) {
            source_run_campaign_ai_placement_settings_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_campaign_runtime_tu15_owns_entry(c.target)) {
            source_run_campaign_runtime_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_ctf_activation_tu15_owns_entry(c.target)) {
            source_run_ctf_activation_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_ctf_configuration_tu15_owns_entry(c.target)) {
            source_run_ctf_configuration_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_ctf_player_iterator_tu15_owns_entry(c.target)) {
            source_run_ctf_player_iterator_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_ctf_activation_spawn_tu15_owns_entry(c.target)) {
            source_run_ctf_activation_spawn_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_ctf_flag_position_tu15_owns_entry(c.target)) {
            source_run_ctf_flag_position_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_multiplayer_engines_tu15_owns_entry(c.target)) {
            source_run_multiplayer_engines_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_teleporter_tu15_owns_entry(c.target)) {
            source_run_teleporter_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_ai_actor_contact_selector_tu15_owns_entry(c.target)) {
            source_run_ai_actor_contact_selector_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(source_ai_actor_tu15_owns_entry(c.target)) {
            source_run_ai_actor_tu15(m,c.target,a,fp,services(),o);return;
        }
        // Complete custom-controller/query bodies already belong to this
        // module. Native campaign callers also enter them directly, outside
        // its usual AI-frame root. Its dispatcher retains their full ABI.
        if(source_level_handles_startup_tu15_owns_entry(c.target)) {
            source_run_level_handles_startup_tu15(m,c.target,a,fp,services(),o);return;
        }
        if(c.target==0x11dc00u || c.target==0x11dd90u || source_ai_frame_tu15_owns_entry(c.target)) {
            source_run_ai_frame_tu15(m,c.target,a,fp,services(),o);return;
        }
        // Reuse the complete physical wrappers and their retained page owner.
        // These actual TU1.5 entries correspond to the existing internal IDs.
        if(c.target==0x2db45au || c.target==0x2db4cau) {
            if(!arena)throw FormatError("Native physical wrapper requires its retained arena owner");
            arena->run(c.target==0x2db45au?0x2d79aau:0x2d7a1au,a);return;
        }
        const auto sp=a.gp[4];
        if(c.instruction==0x33a4d3u && c.target==0x80000080u && c.tail) {
            // NtQuerySystemTime receives the actual32BBAE local output cell.
            // Native UTC is converted to the original 100ns epoch; the real
            // CRT time conversion, rand and stream seed instructions execute.
            if(std::this_thread::get_id()!=main_thread || word(m,sp)!=0x32bbbcu ||
                word(m,sp+4u)!=a.gp[5]-8u)
                throw FormatError("TU1.5 native system-time caller differs");
            const auto ticks=source_execution_input(bank,SourceExecutionInputKind::UtcFiletime,0,
                [this]{return clocks->utc_filetime()+utc_adjustment;});
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
        constexpr std::array native_exit_frames{
            std::pair{0x38313fu,0x383b30u},std::pair{0x381389u,0x383970u},std::pair{0x3835ffu,0x383c90u},
            std::pair{0x38342fu,0x383c10u},std::pair{0x38344fu,0x383c00u},
            std::pair{0x38346fu,0x383bf0u}};
        if(c.target==0x32b0b4u && !c.tail)for(const auto [site,callback]:native_exit_frames) {
            if(c.instruction!=site)continue;
            if(word(m,sp)!=site+5u || word(m,sp+4)!=callback)
                throw FormatError("TU1.5 Havok native exit registration frame differs");
            native_exits.callbacks.push_back(callback);
            // Native C runtime registration retains the actual destructor
            // through this source bank's lifetime and continuation.
            a.gp[0]=0;a.gp[4]+=4;return;
        }
        if((c.instruction==0x1a9e5fu || c.instruction==0x1a9ecdu) && c.target==0x125870u && !c.tail) {
            const auto locale=word(bank,0x4ecc1cu)+0x188u;
            const bool indices=c.instruction==0x1a9e5fu;
            const auto offset=word(m,locale+(indices?0x10u:0x14u));
            const auto logical=indices?std::uint64_t(word(m,locale+8u))*8u:word(m,locale+0xcu);
            if(std::this_thread::get_id()!=main_thread || a.gp[7]!=locale ||
               word(m,sp)!=c.instruction+5u || word(m,sp+4u)!=0xffffffffu ||
               word(m,sp+8u)!=offset || a.gp[indices?0u:1u]!=offset ||
               logical>UINT32_MAX-511u || a.gp[6]!=((logical+511u)&~511ull) ||
               a.gp[2]!=word(m,locale+(indices?0u:4u)))
                throw FormatError("Original locale read differs from its actual MCC resource extent");
            const auto data=cache_owner.load_source_runtime_resource(offset,a.gp[6]);
            if(data.size()!=a.gp[6])throw FormatError("Native locale resource read was incomplete");
            bank.write(a.gp[2],data);
            a.gp[0]=(a.gp[0]&0xffffff00u)|1u;a.gp[4]+=12u;return;
        }
        if(c.instruction==0x1255e7u && c.target==0x125870u && !c.tail) {
            if(std::this_thread::get_id()!=main_thread || word(m,sp)!=0x1255ecu ||
                word(m,sp+4)!=0xffffffffu || word(m,sp+8)!=bsp.file_offset ||
                a.gp[0]!=bsp.file_offset || a.gp[2]!=bsp.region.address ||
                a.gp[6]!=((bsp.region.bytes.size()+511u)&~511u))
                throw FormatError("TU1.5 BSP I/O frame differs from the actual cache resource");
            if(canonical_cache)source_deliver_runtime_cache_bsp_tu15(bank,bsp,native_cache_range());
            else source_deliver_runtime_cache_bsp_tu15(bank,bsp);
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
            case Module::Damage:source_run_damage_delivery_runtime_selected_observations(m,e.canonical,a,image,fp,services(),o);return;
            case Module::World:source_run_physics_world_runtime_selected_observations(m,e.canonical,a,image,fp,services(),o);return;
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
        if(c.instruction==0x2deed4u && c.target==allocation_callback && !c.tail) {
            // Original RtlFreeHeap reports the payload before changing its
            // free lists. This native observer owns only its allocation log;
            // the complete original allocator still owns the free result.
            const auto p=word(m,sp+8u);
            if(std::this_thread::get_id()!=main_thread || word(m,sp)!=0x2deed7u ||
                word(m,sp+4u)!=11u || p!=a.gp[5]-0x6cu || word(m,p+4u) || word(m,p+8u))
                throw FormatError("TU1.5 native free notification frame differs");
            const auto payload=word(m,p);
            std::erase_if(allocations,[&](const auto &row){return row.payload==payload;});
            a.gp[4]+=12u;return;
        }
        std::ostringstream message;message<<"TU1.5 live world owner missing at "<<std::hex
            <<c.instruction<<" -> "<<c.target<<" return "<<word(m,sp);
        throw std::runtime_error(message.str());
    }
    SourcePhysicsWorldRuntimeAbi caller_abi() const {
        const auto caller=nested_caller_frame?nested_caller_frame->first:stack;
        SourcePhysicsWorldRuntimeAbi a{};a.eflags=2;a.fs_base=tls;a.gp[4]=caller;
        a.floating_point=fp;return a;
    }
    SourcePhysicsWorldRuntimeAbi frame(std::span<const std::uint32_t>args) {
        require_executor();
        if(std::this_thread::get_id()!=main_thread)
            throw std::runtime_error("TU1.5 live world caller thread differs");
        auto a=caller_abi();const auto caller=a.gp[4];
        if(args.size()>0x1000)throw std::runtime_error("TU1.5 live call arguments exceed native frame");
        if(nested_caller_frame && std::uint64_t(caller)+4u+args.size()*4u>nested_caller_frame->second)
            throw FormatError("Nested native output arguments overlap the active original caller");
        put(bank,caller,0x60300000u);
        for(unsigned i=0;i<args.size();++i)put(bank,caller+4+i*4,args[i]);return a;
    }
    void capture_gameplay_fade(unsigned phase,const SourcePhysicsWorldRuntimeAbi& parent) {
        if(phase) {
            if(!gameplay_fade_activation_observer)return;
            //15A7C9 has EAX=the actual initialized player row;14F755
            // has ESI=the actual successfully bound player row. Neither
            // changes source local-view or the separate respawn owner.
            const auto row=phase==1?parent.gp[0]:parent.gp[6];
            const auto pool=word(bank,0x4f54f4u),rows=word(bank,pool+0x44u);
            if(row<rows || (row-rows)%0x21cu || (row-rows)/0x21cu>=16u)
                throw FormatError("Original gameplay fade activation lost its actual player row");
            const auto player=(word(bank,row,2)<<16u)|((row-rows)/0x21cu);
            if(record(pool,player)!=row)throw FormatError("Original gameplay fade activation lost its salted player");
            if(!parent.floating_point)throw FormatError("Original fade activation omitted its active FP context");
            const auto engine=word(bank,0x4f63b8u),clock=word(bank,0x51df54u),kind=word(bank,engine+0xc14u);
            const SourceGameplayFadeActivationTu15 event{player,kind<=10u && word(bank,0x56dfa0u+kind*4u)!=0,
                std::bit_cast<std::int16_t>(std::uint16_t(word(bank,clock+2u,2))),parent.floating_point->x87_control_word,parent.floating_point->mxcsr};
            SourceExecutionObserverScope read_only(bank);gameplay_fade_activation_observer(event);return;
        }
        if(!gameplay_fade_observer)return;
        const auto engine=word(bank,0x4f63b8u),game=word(bank,0x4f3218u),clock=word(bank,0x51df54u);
        SourceGameplayFadeFrameTu15 out;
        const auto kind=word(bank,engine+0xc14u);out.engine_present=kind<=10u && word(bank,0x56dfa0u+kind*4u)!=0;
        out.tick=word(bank,clock+8u);out.round_start_tick=word(bank,engine+0x70u);
        out.raw_mode=std::uint8_t(word(bank,game+0xcu,1));
        out.phase=std::bit_cast<std::int16_t>(std::uint16_t(word(bank,engine+0x6cu,2)));
        out.rate=std::bit_cast<std::int16_t>(std::uint16_t(word(bank,clock+2u,2)));
        out.phase_counter_before=std::bit_cast<std::int16_t>(std::uint16_t(word(bank,engine+0xfau,2)));
        out.pending_phase=std::bit_cast<std::int32_t>(word(bank,engine+0xc04u));
        out.life_limit=std::bit_cast<std::int32_t>(word(bank,game+0x1b8u));
        if(!parent.floating_point)throw FormatError("Original gameplay fade observation omitted its active FP context");
        out.x87_control_word=parent.floating_point->x87_control_word;out.mxcsr=parent.floating_point->mxcsr;
        const auto pool=word(bank,0x4f54f4u),rows=word(bank,pool+0x44u);
        if(word(bank,pool+0x20u)!=16u || word(bank,pool+0x24u)!=0x21cu)
            throw FormatError("Original gameplay fade player pool schema differs");
        const auto occupied=word(bank,word(bank,pool+0x48u));
        for(unsigned slot=0;slot<16;++slot)if(occupied&(1u<<slot)) {
            const auto row=rows+slot*0x21cu;out.identities[slot].player=(word(bank,row,2)<<16u)|slot;
            auto &p=out.players[slot];p.present=true;p.current_unit=word(bank,row+0x2cu);
            p.flags=std::uint16_t(word(bank,row+2u,2));p.membership=std::bit_cast<std::int8_t>(std::uint8_t(word(bank,row+0xc0u,1)));
            p.lives=std::bit_cast<std::int16_t>(std::uint16_t(word(bank,row+0x1acu,2)));
            p.respawn_seconds=std::bit_cast<std::int32_t>(word(bank,row+0x170u));
        }
        SourceExecutionObserverScope read_only(bank);gameplay_fade_observer(out);
    }
    void capture_player_scoreboard_gate(const SourcePhysicsWorldRuntimeAbi& parent) {
        if(!player_scoreboard_observer)return;
        const auto pool=word(bank,0x4f54f4u),player=word(bank,parent.gp[4]+0x1cu);
        const auto row=record(pool,player);
        if(!row || row!=parent.gp[6])
            throw FormatError("Original15E6BC scoreboard frame lost its full player owner");
        const auto engine=word(bank,0x4f63b8u),game=word(bank,0x4f3218u),clock=word(bank,0x51df54u);
        SourcePlayerScoreboardGateTu15 gate;
        gate.tick=std::bit_cast<std::int32_t>(word(bank,clock+8u));
        gate.respawn_seconds=std::bit_cast<std::int32_t>(word(bank,row+0x170u));
        gate.life_limit=std::bit_cast<std::int32_t>(word(bank,game+0x1b8u));
        gate.current_unit=word(bank,row+0x2cu);gate.player_flags=std::uint16_t(word(bank,row+2u,2));
        gate.lives_remaining=std::bit_cast<std::int16_t>(std::uint16_t(word(bank,row+0x1acu,2)));
        gate.lifecycle=std::bit_cast<std::int16_t>(std::uint16_t(word(bank,engine+0x6cu,2)));
        gate.options_state=std::uint8_t(word(bank,game+0xcu,1));
        gate.engine_status=std::bit_cast<std::int32_t>(word(bank,engine+0xc04u));
        const auto watched=word(bank,row+0x1b0u);
        if(gate.current_unit==0xffffffffu && watched!=0xffffffffu) {
            const auto target=record(pool,watched);
            // Exact1656D0 exclusion predicate. Its source-owned flag/timer
            // writes still execute in the untouched original caller below.
            if(target && word(bank,target+0xc4u,1) && word(bank,target+0x28u,2)==0xffffu &&
                !(word(bank,target+2u,2)&2u)) {
                const auto elapsed=(gate.player_flags&0x1000u)?
                    std::bit_cast<std::int32_t>(std::uint32_t(gate.tick)-word(bank,row+0x190u)):0;
                const auto rate=std::bit_cast<std::int16_t>(std::uint16_t(word(bank,clock+2u,2)));
                gate.death_observer_excluded=elapsed<std::int32_t(rate)*10;
            }
        }
        SourceExecutionObserverScope read_only(bank);player_scoreboard_observer({player,gate});
    }
    void capture_player_status_phase(const SourcePhysicsWorldRuntimeAbi& parent) {
        if(!player_status_observer)return;
        const auto engine=word(bank,0x4f63b8u),game=word(bank,0x4f3218u);
        const auto kind=word(bank,engine+0xc14u);
        if(kind>10u || !word(bank,0x56dfa0u+kind*4u))return;
        const auto pool=word(bank,0x4f54f4u),rows=word(bank,pool+0x44u);
        if(word(bank,pool+0x20u)!=16u || word(bank,pool+0x24u)!=0x21cu)
            throw FormatError("Original status phase player pool schema differs");
        const auto occupied=word(bank,word(bank,pool+0x48u));
        const auto users=word(bank,0x4f54f0u);
        const auto lifecycle=std::bit_cast<std::int16_t>(std::uint16_t(word(bank,engine+0x6cu,2)));
        const bool special=word(bank,game+0x1128u,1)!=0;
        const auto options_state=std::uint8_t(word(bank,game+0xcu,1));
        const auto engine_status=std::bit_cast<std::int32_t>(word(bank,engine+0xc04u));
        SourcePlayerStatusPhaseTu15 event{word(bank,word(bank,0x51df54u)+8u),{}};
        CallerFrame caller(*this,parent.gp[4]);
        for(unsigned i=0;i<16;++i)if(occupied&(1u<<i)) {
            const auto row=rows+i*0x21cu;
            const auto player=(word(bank,row,2)<<16u)|i;
            const auto view=std::bit_cast<std::int16_t>(std::uint16_t(word(bank,row+0x28u,2)));
            std::int32_t status;
            if(view>=0 && view<4 && word(bank,users+0xcu+unsigned(view)*4u)==player) {
                // The complete original161610 just published this local
                // descriptor. Preserve its phase rather than querying again.
                status=std::bit_cast<std::int32_t>(word(bank,0x4f63d8u+unsigned(view)*12u));
            } else {
                // Host-remote participants have no host local-view cache.
                // Execute the same complete selector against this exact phase
                // and actual mode/ranking owners, below the active caller ESP.
                auto query=frame(std::array{player});run_actual(0x1618b0u,query);
                status=std::bit_cast<std::int32_t>(query.gp[0]);
            }
            event.players.push_back({player,{status,lifecycle,special,options_state,engine_status}});
        }
        SourceExecutionObserverScope read_only(bank);player_status_observer(event);
    }
    void run(std::uint32_t entry,SourcePhysicsWorldRuntimeAbi&a,
        const SourcePhysicsContactRecordObserver&o={}) {
        source_execution_untracked(bank,entry);Execution execution(*this);
        SourceObjectAttachmentBindingTu15 attachment_events(bank,[this](const auto &event){model_attachments[event.child]=event;});
        VehicleInputCall input_call(*this);
        SourceVehicleInputResetBindingTu15 input_events(bank,[this](const auto &event){vehicle_input_resets.push_back(event);});
        SourceGameplayFadePhaseBindingTu15 fade_phase(bank,[this](unsigned phase,const auto& parent){capture_gameplay_fade(phase,parent);});
        SourcePlayerStatusPhaseBindingTu15 status_phase(bank,[this](const auto& parent){capture_player_status_phase(parent);},
            [this](const auto& parent){capture_player_scoreboard_gate(parent);});
        try {
            if(entry==0x133760u || entry==0x1360f0u || entry==0x165740u)
                source_run_physics_renderer_storage_with_native_pages(bank,entry,a,fp,services(),pool_storage,o);
            else if(entry==0x1c5f00u || entry==0x1f15b0u)
                source_run_physics_early_initializer_with_native_storage(bank,entry,a,fp,services(),pool_storage,o);
            else source_run_physics_world_runtime_selected_observations(bank,entry,a,image,fp,services(),o);
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
    NativeTu15PhysicsFrameExecutionStats native_physics_frames;
    NativeTu15WorldIntegrationStats native_physics_integrations;
    std::uint64_t native_world_residual_calls{};
    void step_native_physics_frame() {
        // The world phase is a genuine standalone host call: no caller reads
        // the old root's volatile registers. Keep the true fresh FP/live-value
        // boundary and marshal only the residual owners' proved arguments.
        auto state=frame({});
        const auto continuation=word(bank,state.gp[4]);
        NativeTu15PhysicsFrameFp context{state.floating_point->x87_control_word,
            state.x87_status,state.floating_point->mxcsr};
        source_execution_untracked(bank,0x1cc8d0u);Execution execution(*this);
        NativeTu15PhysicsFrameObservation diagnostic(bank,context,state);
        // The semantic kernel labels each completed store itself. Publish
        // that diagnostic site before delivering the registered collector;
        // the last residual instruction is not the native write's source PC.
        SourcePhysicsContactRecordObserver observed;
        if(physics_store_observer)observed=[this,collector=physics_store_observer](const auto &store) {
            last_instruction=store.instruction;collector(store);
        };
        native_tu15_step_physics_frame_with_residuals(bank,context,state,
            {stack-0x1000u,0x1000u},
            [this](std::uint32_t entry,auto &actual,const auto &) {
                // run_actual composes the registered collector exactly once.
                // These named residual owners remain explicit migration work.
                run_actual(entry,actual);
            },observed,&native_physics_frames,
            [this,&state,continuation,&observed](auto world,const auto &step,auto &context) {
                // Keep the real collision dependency explicit, then execute
                // native world/island/force algorithms. No old world-step
                // root or manufactured volatile-register result is involved.
                native_tu15_step_physics_world_with_residuals(bank,world,step,context,state,
                    continuation,{stack-0x11000u,0x10000u},
                    [this](std::uint32_t entry,auto &actual,const auto &) {run_actual(entry,actual);},
                    observed,&native_physics_integrations,&native_world_residual_calls,
                    services().timestamp_counter,[this]{
                        return source_execution_input(bank,SourceExecutionInputKind::UtcFiletime,0,
                            [this]{return clocks->utc_filetime()+utc_adjustment;});
                    });
            });
    }
    void run_actual(std::uint32_t entry,SourcePhysicsWorldRuntimeAbi&a,
                    const SourcePhysicsContactRecordObserver&o={}) {
        source_execution_untracked(bank,entry);Execution execution(*this);
        SourceObjectAttachmentBindingTu15 attachment_events(bank,[this](const auto &event){model_attachments[event.child]=event;});
        VehicleInputCall input_call(*this);
        SourceVehicleInputResetBindingTu15 input_events(bank,[this](const auto &event){vehicle_input_resets.push_back(event);});
        SourceGameplayFadePhaseBindingTu15 fade_phase(bank,[this](unsigned phase,const auto& parent){capture_gameplay_fade(phase,parent);});
        SourcePlayerStatusPhaseBindingTu15 status_phase(bank,[this](const auto& parent){capture_player_status_phase(parent);},
            [this](const auto& parent){capture_player_scoreboard_gate(parent);});
        if(std::this_thread::get_id()!=main_thread)
            throw FormatError("TU1.5 live entry requires its retained executor thread");
        NativeLiftScalarConsumerScopeTu15 lift_consumer(bank,{0x60000000u,0x60100000u,
            [this](auto pc){last_instruction=pc;},&first_person_phase_active,&weapon_effect_phase_active,
            !o&&(!physics_store_observer||physics_store_observer_read_only),&native_lift_activity});
        NativeVehicleRateConsumerScopeTu15 rate_consumer(bank,{0x60000000u,0x60100000u,
            [this](auto pc){last_instruction=pc;},&first_person_phase_active,&weapon_effect_phase_active,
            !o&&(!physics_store_observer||physics_store_observer_read_only),&native_vehicle_rate_activity});
        NativeConstraintSchemaEmitterConsumerScopeTu15 schema_consumer(bank,{0x60000000u,0x60100000u,
            [this]{return last_instruction;},[this](auto pc){last_instruction=pc;},
            &first_person_phase_active,&weapon_effect_phase_active,
            !o&&(!physics_store_observer||physics_store_observer_read_only),nullptr});
        SourcePhysicsEntryObservation diagnostic(bank,entry,[&]{return a;});
        try {
            const auto observed=source_physics_store_observers_tu15(o,physics_store_observer);
            required(bank,{entry,entry,false},a,observed);
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
    std::uint32_t record_player_for_look(std::uint32_t player) const {
        if(player==0xffffffffu || (player&0xffffu)>=16u)return 0;
        return record(word(bank,0x4f54f4u),player);
    }
    DeathCameraPacket player_camera_packet(std::uint32_t player,float seconds,
        SourceCameraControllerState &controller,SourceObserverState &observer,std::uint32_t &camera_random,bool cycle_pressed,std::uint8_t *camera_mode) {
        if(!std::isfinite(seconds) || seconds<0)
            throw FormatError("TU1.5 camera scheduler requires finite elapsed time");
        const auto owner=record_player_for_look(player);
        if(!owner)throw FormatError("TU1.5 camera scheduler requires an actual full player identity");
        const auto unit=word(bank,owner+0x2cu);
        // Bind one private native view to the original globals for the duration
        // of its complete call. Peer views retain their own controller/observer;
        // no player/object result or source routine is substituted.
        struct Context {
            SourceRuntimeMemory &bank;
            std::vector<std::pair<std::uint32_t,std::vector<std::byte>>> saved;
            void save(std::uint32_t p,std::size_t n) {
                std::vector<std::byte> bytes(n);bank.read(p,bytes);
                saved.emplace_back(p,std::move(bytes));
            }
            ~Context(){for(auto i=saved.rbegin();i!=saved.rend();++i)bank.write(i->first,i->second);}
        } context{bank,{}};
        constexpr auto source_controller=0x4f5514u,source_observer=0x4f64a4u;
        const auto users=word(bank,0x4f54f0u)+0xcu;
        const auto input=word(bank,0x4f9b54u)+0x14u;
        context.save(users,16);context.save(input,0x94);context.save(0x4f5508u,12);
        context.save(source_controller,sizeof(controller));context.save(source_observer,sizeof(observer));
        const auto auxiliary=word(bank,0x4f3cd8u)+4u;
        context.save(auxiliary,4);put(bank,auxiliary,camera_random);
        // Native keyboard/controller edges are external inputs. Bind their
        // private slot to the exact247140 controller-handle/present/button
        // fields, then restore the shared platform and roster associations.
        // The original owner still decides when cycling is allowed.
        context.save(owner+0x24u,4);context.save(0x4f2a9cu,4);
        context.save(0x4f2a89u,1);context.save(0x4f2aacu,0x48);
        put(bank,owner+0x24u,0);put(bank,0x4f2a9cu,0x4f2aacu);
        const std::array normal_input{std::byte{0}};bank.write(0x4f2a89u,normal_input);
        const std::array button{std::byte(cycle_pressed?1:0)};bank.write(0x4f2aacu+0x10u,button);
        if(!(controller[0]&255u))bank.read(source_controller,std::as_writable_bytes(std::span(controller)));
        bank.write(source_controller,std::as_bytes(std::span(controller)));
        bank.write(source_observer,std::as_bytes(std::span(observer)));
        put(bank,users,player);for(unsigned i=1;i<4;++i)put(bank,users+4*i,0xffffffffu);
        put(bank,0x4f550cu,0);put(bank,0x4f5510u,0);
        const auto retained=0x60070000u+(player&0xffffu)*0xa0u;
        if(word(bank,retained+0x94u)==player && word(bank,retained)==unit)
            bank.write(input,bank.view(retained,0x94));
        else {
            put(bank,input,0xffffffffu);
            auto a=frame({});a.gp[0]=0;a.gp[1]=unit;run_actual(0x18a0c0u,a);
        }
        call_actual(0x157fe0u,std::array{std::bit_cast<std::uint32_t>(seconds)});
        camera_random=word(bank,auxiliary);
        bank.read(source_controller,std::as_writable_bytes(std::span(controller)));
        bank.read(source_observer,std::as_writable_bytes(std::span(observer)));
        DeathCameraPacket packet{};
        bank.read(source_controller+0x5cu,std::as_writable_bytes(std::span(packet)));
        auto mode_abi=frame({});mode_abi.gp[0]=0;run_actual(0x158290u,mode_abi);
        if(camera_mode)*camera_mode=std::uint8_t(mode_abi.gp[0]);
        return packet;
    }
    DeathCameraPacket unit_camera_packet(std::uint32_t unit,float seconds,
        SourceCameraControllerState &controller,SourceObserverState &observer,std::uint8_t *camera_mode) {
        const auto player=word(bank,object_body(unit)+0x13cu);
        const auto owner=record_player_for_look(player);
        if(!owner || word(bank,owner+0x2cu)!=unit)
            throw FormatError("TU1.5 unit camera requires the actual current player unit");
        auto random=word(bank,word(bank,0x4f3cd8u)+4u);
        return player_camera_packet(player,seconds,controller,observer,random,false,camera_mode);
    }
    void prepare_player_look_births() {
        const auto pool=word(bank,0x4f54f4u),rows=word(bank,pool+0x44u);
        if(word(bank,pool+0x20u)!=16u || word(bank,pool+0x24u)!=0x21cu)
            throw FormatError("TU1.5 look birth player pool schema differs");
        const auto occupied=word(bank,word(bank,pool+0x48u));
        for(unsigned slot=0;slot<16u;++slot)if(occupied&(1u<<slot)) {
            const auto row=rows+slot*0x21cu;
            if(word(bank,row+2u,2)&2u)continue;
            const auto player=(word(bank,row,2)<<16u)|slot,unit=word(bank,row+0x2cu);
            if(unit==0xffffffffu)continue;
            object_body(unit);
            const auto retained=0x60070000u+slot*0xa0u;
            const auto baseline=word(bank,retained+0x94u);
            if(word(bank,retained)==unit && (baseline==player || baseline==0xffffffffu))continue;
            const auto input_user=std::bit_cast<std::int16_t>(std::uint16_t(word(bank,row+0x28u,2)));
            if(input_user>=0 && input_user<4) {
                const auto local=word(bank,0x4f9b54u)+0x14u+std::uint32_t(input_user)*0x94u;
                //14F769 already owns the real local birth input, before
                //later object/inventory phases. Retain those exact bytes.
                if(word(bank,local)==unit) {
                    bank.write(retained,bank.view(local,0x94));
                    put(bank,retained+0x94u,0xffffffffu);continue;
                }
            }
            //14F75C publishes a newly bound unit before14F769 constructs its
            // local18A0C0 input. A remote player has no hardware user, but its
            // private FP owner needs the same original birth record before
            // this tick's phase. Borrow logical0 without changing any actual
            // local input, and retain no raw command baseline until delivery.
            const auto input=word(bank,0x4f9b54u)+0x14u;
            struct Restore {
                SourceRuntimeMemory &bank;std::uint32_t address;std::array<std::byte,0x94> bytes;
                ~Restore(){bank.write(address,bytes);}
            } restore{bank,input,{}};
            bank.read(input,restore.bytes);put(bank,input,0xffffffffu);
            auto a=frame({});a.gp[0]=0;a.gp[1]=unit;run_actual(0x18a0c0u,a);
            bank.write(retained,bank.view(input,0x94));put(bank,retained+0x94u,0xffffffffu);
        }
    }
    void prepare_player_look(std::uint32_t player,std::uint32_t unit,
        std::span<std::byte,0x5c> command) {
        const auto owner=record_player_for_look(player);
        if(!owner || word(bank,owner+0x2cu)!=unit)
            throw FormatError("TU1.5 look requires the player's actual current unit");
        object_body(unit);
        const Bytes raw(command);const auto yaw=raw.f32(4),pitch=raw.f32(8);
        if(!std::isfinite(yaw) || !std::isfinite(pitch))
            throw FormatError("TU1.5 look requires finite native input");
        // Native caller ABI storage is part of the retained/checkpointed bank:
        // original94-byte input, full player identity, previous raw yaw/pitch.
        // Private player records also cover remote users beyond four local views.
        const auto retained=0x60070000u+(player&0xffffu)*0xa0u;
        const auto input=word(bank,0x4f9b54u)+0x14u;
        const bool existing=word(bank,retained+0x94u)==player && word(bank,retained)==unit;
        // A teleport can construct/reorient the source input before the first
        // native packet. NONE at+94 means its raw yaw baseline is still unset.
        const bool initialized=word(bank,retained)==unit &&
            (existing || word(bank,retained+0x94u)==0xffffffffu);
        // The native accumulator wraps at +/-pi. Deliver the continuous delta
        // across that boundary; a spurious full turn can hit seat yaw limits
        // before18A300's final normalization.
        const auto dy=existing?std::remainder(yaw-std::bit_cast<float>(word(bank,retained+0x98u)),
            std::bit_cast<float>(word(bank,0x44923cu))):0.f;
        const auto dp=existing?pitch-std::bit_cast<float>(word(bank,retained+0x9cu)):0.f;
        struct Restore {
            SourceRuntimeMemory &bank;std::uint32_t address;std::array<std::byte,0x94> bytes;
            ~Restore(){bank.write(address,bytes);}
        } restore{bank,input,{}};
        bank.read(input,restore.bytes);
        if(initialized)bank.write(input,bank.view(retained,0x94));
        else put(bank,input,0xffffffffu); // Original constructor owns new-unit initialization.
        // Native delivery owns an active input context. The source world's
        // hardware input-manager flag is inactive for this PC adapter; calling
        //18A0C0 again under that flag would discard the retained limits/look.
        // Construct only at the actual new-unit boundary.
        if(!initialized) {
            auto a=frame({});a.gp[0]=0;a.gp[1]=unit;run_actual(0x18a0c0u,a);
        }
        // Native zoom selection is the same signed input field published by
        //1898A0 at189E34. Preserve it in the retained camera input record.
        bank.write(input+0x2eu,command.subspan(0x24u,2));
        const std::array args{std::bit_cast<std::uint32_t>(dy),std::bit_cast<std::uint32_t>(dp)};
        auto a=frame(args);a.gp[0]=0;run_actual(0x18a300u,a);
        bank.write(retained,bank.view(input,0x94));
        put(bank,retained+0x94u,player);put(bank,retained+0x98u,std::bit_cast<std::uint32_t>(yaw));
        put(bank,retained+0x9cu,std::bit_cast<std::uint32_t>(pitch));
        for(unsigned at:{4u,8u}) {
            const auto value=word(bank,input+0xcu+at);
            for(unsigned n=0;n<4;++n)command[at+n]=std::byte(value>>(n*8));
        }
    }
    void reorient_retained_input(std::uint32_t player,std::uint32_t unit,
        std::span<const std::byte,12> completed_forward,bool construct_input) {
        const auto owner=record_player_for_look(player);
        if(!owner || word(bank,owner+0x2cu)!=unit)return;
        const auto retained=0x60070000u+(player&0xffffu)*0xa0u;
        const bool existing=word(bank,retained+0x94u)==player && word(bank,retained)==unit;
        const bool initialized=word(bank,retained)==unit &&
            (existing || word(bank,retained+0x94u)==0xffffffffu);
        if(!initialized && !construct_input)return;
        const auto input=word(bank,0x4f9b54u)+0x14u;
        struct Restore {
            SourceRuntimeMemory &bank;std::uint32_t address;std::array<std::byte,0x94> bytes;
            ~Restore(){bank.write(address,bytes);}
        } restore{bank,input,{}};
        bank.read(input,restore.bytes);
        if(initialized)bank.write(input,bank.view(retained,0x94));
        else {
            put(bank,input,0xffffffffu);
            auto construct=frame({});construct.gp[0]=0;construct.gp[1]=unit;
            run_actual(0x18a0c0u,construct);
        }
        constexpr std::uint32_t forward=0x6005e000u;
        bank.write(forward,completed_forward);
        auto a=frame({});a.gp[0]=forward;a.gp[1]=0;run_actual(0x18a2a0u,a);
        bank.write(retained,bank.view(input,0x94));
        if(!existing)put(bank,retained+0x94u,0xffffffffu);
    }
    void flush_vehicle_input_resets() {
        auto events=std::move(vehicle_input_resets);vehicle_input_resets.clear();
        for(const auto &event:events)
            reorient_retained_input(event.player,event.unit,event.forward,false);
    }
    void reorient_player_look(std::uint32_t player,std::uint32_t unit) {
        std::array<std::byte,12> forward;
        bank.read(object_body(unit)+0x70u,forward);
        // Both original successful traversal and driver entry publish the
        // completed forward through18A2A0. Reuse the native retained-input
        // adapter; it preserves the independent raw native yaw baseline.
        reorient_retained_input(player,unit,forward,true);
    }
    std::optional<SourcePlayerActionTarget> player_action_target(std::uint32_t player,std::uint32_t unit) const {
        const auto owner=record_player_for_look(player);
        if(!owner || unit==0xffffffffu || word(bank,owner+0x2cu)!=unit)return {};
        const auto retained=0x60070000u+(player&0xffffu)*0xa0u;
        if(word(bank,retained+0x94u)!=player || word(bank,retained)!=unit)return {};
        const Bytes bytes(bank.view(retained+0x6cu,8));
        return SourcePlayerActionTarget{std::bit_cast<std::int16_t>(bytes.u16(0)),
            std::bit_cast<std::int16_t>(bytes.u16(2)),bytes.u32(4)};
    }
    std::optional<Vec2> player_look(std::uint32_t player,std::uint32_t unit) const {
        const auto owner=record_player_for_look(player);
        if(!owner || unit==0xffffffffu || word(bank,owner+0x2cu)!=unit)return {};
        const auto retained=0x60070000u+(player&0xffffu)*0xa0u;
        if(word(bank,retained+0x94u)!=player || word(bank,retained)!=unit)return {};
        return Vec2{std::bit_cast<float>(word(bank,retained+0x10u)),
            std::bit_cast<float>(word(bank,retained+0x14u))};
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
    SourcePlayerConfiguration update_player_configuration(std::uint32_t player,const SourcePlayerConfiguration &configuration) {
        const auto pool=word(bank,0x4f54f4u),owner=record(pool,player);
        if(!session_configured || !owner)throw FormatError("Team update requires its live original player");
        auto call=frame(std::array{player});const auto input=call.gp[4]+8u;
        bank.write(input,std::as_bytes(std::span(configuration)));call.gp[0]=input;
        run_actual(0x14e9e0u,call);
        SourcePlayerConfiguration accepted{};bank.read(owner+0x44u,std::as_writable_bytes(std::span(accepted)));
        return accepted;
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
        // Creation+8 is the simulation machine key; creation+E is the
        // separately produced player identity. Original13B120 constructs
        // the local roster's key from options+2CD, while14DF40 derives its
        // player ID from the platform address. The constructed local world
        // publishes the matching key at+0D, consumed by whole69AD0. Using
        // the hardware MAC for both fields makes14FDC0 reject every local
        // player in the original standalone simulation world.
        const auto simulation=word(bank,0x4dc04cu);
        if(!simulation || !word(bank,simulation+8u))
            throw FormatError("TU1.5 local player creation requires its constructed simulation machine");
        const auto simulation_machine=bank.view(simulation+0xdu,machine.size());
        std::copy(simulation_machine.begin(),simulation_machine.end(),input.begin()+8);
        for(unsigned i=0;i<identity.size();++i)input[0xeu+i]=std::byte(identity[i]);
        for(unsigned i=0;i<configuration.size();++i)input[0x1cu+i]=std::byte(configuration[i]);
        bank.write(creation,input);

        // DatumPool has already performed the genuine allocation. Resume
        // whole14E730 at14E73C, immediately after its16E7F0 call, retaining
        //14E736's saved ESI above the caller return address. The original
        // continuation owns fresh-record mode, census, AI notification and
        //15A660's initial lives.14EAD0 is the distinct rejoin continuation.
        auto a=frame({});a.gp[4]-=4u;put(bank,a.gp[4],a.gp[6]);
        a.gp[0]=player;a.gp[3]=creation;run_actual(0x14e73cu,a);
        if(a.gp[0]!=player)
            throw FormatError("Original fresh-player continuation lost its allocated datum");
    }
    bool uses_scenario_player_spawns(std::span<const Spawn> starts) const {
        const auto& authored=cache_owner.spawns();
        return starts.size()==authored.size() && std::equal(starts.begin(),starts.end(),authored.begin(),
            [](const auto&a,const auto&b) {
                return std::bit_cast<std::uint32_t>(a.position.x)==std::bit_cast<std::uint32_t>(b.position.x) &&
                    std::bit_cast<std::uint32_t>(a.position.y)==std::bit_cast<std::uint32_t>(b.position.y) &&
                    std::bit_cast<std::uint32_t>(a.position.z)==std::bit_cast<std::uint32_t>(b.position.z) &&
                    std::bit_cast<std::uint32_t>(a.facing)==std::bit_cast<std::uint32_t>(b.facing) &&
                    a.bsp==b.bsp && a.team==b.team && a.player_type==b.player_type &&
                    a.game_types==b.game_types && a.spawn_types==b.spawn_types;
            });
    }
    std::uint32_t birth_player(std::uint32_t player) {
        const auto row=record(word(bank,0x4f54f4u),player);
        if(!session_configured || !word(bank,0x52be48u) || !row ||
            word(bank,word(bank,0x4f3218u)+8u)!=2u || word(bank,row+0x2cu)!=0xffffffffu)
            throw FormatError("TU1.5 player factory requires an allocated absent multiplayer unit");
        if(!(word(bank,row+2u,2)&1u))call_actual(0x14fdc0u);
        if((word(bank,row+2u,2)&3u)!=1u)return 0xffffffffu;
        const auto created=call_actual(0x151da0u,std::array{player});
        const auto unit=word(bank,row+0x2cu);
        if(!(created&255u))return 0xffffffffu;
        if(unit==0xffffffffu || !object_body(unit))
            throw FormatError("Original player factory completed without its genuine current unit");
        return unit;
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
    void set_player_input_user(std::uint32_t player,std::int16_t user) {
        const auto row=record(word(bank,0x4f54f4u),player);
        if(!session_configured || user<-1 || user>=4 || !row)
            throw FormatError("TU1.5 input user requires a live full player and logical index0..3 or NONE");
        const auto locals=word(bank,0x4f54f0u);
        const auto prior=std::bit_cast<std::int32_t>(word(bank,row+0x24u));
        if(!locals || prior < -1 || prior>=4 ||
            (prior>=0 && word(bank,locals+0x1cu+std::uint32_t(prior)*4u)!=player))
            throw FormatError("TU1.5 player input-user reverse ownership differs");
        if(user>=0) {
            const auto occupied=word(bank,locals+0x1cu+unsigned(user)*4u);
            if(occupied!=0xffffffffu && (occupied!=player || prior!=user))
                throw FormatError("TU1.5 logical input user belongs to another player");
        }
        call_actual(0x151d30u,std::array{player,std::uint32_t(std::int32_t(user))});
        if(word(bank,row+0x24u)!=std::uint32_t(std::int32_t(user)) ||
            (user>=0 && word(bank,locals+0x1cu+unsigned(user)*4u)!=player) ||
            (prior>=0 && prior!=user && word(bank,locals+0x1cu+std::uint32_t(prior)*4u)!=0xffffffffu))
            throw FormatError("Original151D30 did not publish the requested logical input-user binding");
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
        // Original1898A0 calls153BA0 directly into its command at+28.
        // Retain all12 bytes: action/index,full interaction object and the
        // independent melee target. Canonical items have no legacy pickup row.
        constexpr std::uint32_t prompt=0x60041000u,parameters_=0x60041100u,
            eye=0x60041200u,velocity=0x60041220u,look=0x60041240u,result=0x60041300u;
        call_actual(0x153ba0u,std::array{player,prompt});
        //1898D4 selects the94-byte per-user input record;189CF5 passes its
        //+6C directly to153BA0. Keep the same completed result in the native
        //producer's retained input,already bound by prepare_player_look.
        const auto retained=0x60070000u+(player&0xffffu)*0xa0u;
        if(word(bank,retained+0x94u)==player && word(bank,retained)==object)
            bank.write(retained+0x6cu,bank.view(prompt,12));
        const auto put_command=[&](unsigned at,std::uint32_t value) {
            for(unsigned n=0;n<4;++n)command[at+n]=std::byte(value>>(8*n));
        };
        put_command(0x28u,word(bank,prompt));
        put_command(0x2cu,word(bank,prompt+4u));
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
    void populate_weapon_selection(std::uint32_t unit,std::span<std::byte,0x5c> command,bool switch_weapon) {
        const auto body=object_body(unit);
        // Literal1899A1..1899F5 refreshes the local sequence/request record
        // from actual unit+214, then calls complete D0390 with mode NONE.
        const auto selection=word(bank,body+0x214u);
        for(unsigned n=0;n<4;++n)command[0x1eu+n]=std::byte(selection>>(n*8));
        auto primary=std::bit_cast<std::int8_t>(std::uint8_t(selection>>16));
        if(primary==-1)primary=std::bit_cast<std::int8_t>(std::uint8_t(word(bank,body+0x212u,1)));
        const bool cycle=switch_weapon && word(bank,body+0x213u,1)==255u;
        if(cycle || primary==-1) {
            const auto selected=call_actual(0xd0390u,std::array{unit,0xffffffffu,
                std::uint32_t(std::int32_t(primary)),unsigned(cycle)});
            command[0x20u]=std::byte(selected);
        }
        auto forced=frame({});forced.gp[1]=unit;run_actual(0xd2330u,forced);
        if((forced.gp[0]&65535u)!=65535u && word(bank,body+0x212u,1)!=(forced.gp[0]&255u)) {
            command[0x20u]=std::byte(forced.gp[0]);command[0x21u]=std::byte{0xff};
        }
        //189A26..189A32 removes an identical other-hand request.
        if(command[0x20u]==command[0x21u] && command[0x20u]!=std::byte{0xff})command[0x21u]=std::byte{0xff};
    }
    VehicleWeaponHudStatus vehicle_weapon_hud(std::uint32_t unit) {
        (void)object_body(unit);
        VehicleWeaponHudStatus result;
        result.ownership=weapon_hud_ownership(bank,unit,SourcePhysicsContactImage::TitleUpdate15);
        if(result.ownership.vehicle_weapon==0xffffffffu)return result;
        result.definition=word(bank,object_body(result.ownership.vehicle_weapon));
        result.weapon=weapon_hud_state(result.ownership.vehicle_weapon);
        return result;
    }
    SourceWeaponHudState weapon_hud_state(std::uint32_t weapon) {
        (void)object_body(weapon);
        constexpr std::uint32_t output=0x60058000u,tag=output+0x38u;
        std::array<std::byte,0x3c> clear{};bank.write(output,clear);
        auto a=frame(std::array{tag});a.gp[0]=output;a.gp[1]=weapon;
        run_actual(0x13da89u,a);
        SourceWeaponHudState result;
        bank.read(output,std::as_writable_bytes(std::span(&result,1)));
        return result;
    }
    WeaponHudTarget weapon_hud_target(std::uint32_t weapon) {
        const auto body=object_body(weapon);WeaponHudTarget result;
        const auto definition=cache_owner.query_tag({.handle=TagHandle{word(bank,body)},.require_address=true});
        if(!definition)throw FormatError("HUD weapon lost its resident definition");
        const auto definition_address=runtime_tag_address(*definition);
        if(!word(bank,definition_address+0x2c8u) ||
            word(bank,word(bank,definition_address+0x2ccu)+6u,2)!=5u ||
            std::bit_cast<std::int16_t>(std::uint16_t(word(bank,body+0x22cu,2)))<=0)return result;
        const auto mode=word(bank,body+0x20cu,1),target=word(bank,body+0x194u);
        if(target==0xffffffffu)return result;
        std::uint32_t other{};
        try {other=object_body(target);}catch(const FormatError &) {return result;}
        if(mode==0u) {
            //Complete10A410: recent retained target, valid full datum. Tick
            //subtraction and comparison are signed source DWORD operations.
            const auto clock=word(bank,0x51df54u);
            const auto elapsed=std::bit_cast<std::int32_t>(word(bank,clock+8u)-word(bank,body+0x24cu));
            const auto rate=std::bit_cast<std::int16_t>(std::uint16_t(word(bank,clock+2u,2)));
            result.acquired=elapsed<rate;
        } else if(mode==5u || mode==6u) {
            const auto region=std::bit_cast<std::int8_t>(std::uint8_t(word(bank,body+0x177u,1)));
            if(region<0)return result;
            const auto tag=cache_owner.query_tag({.handle=TagHandle{word(bank,other)},.require_address=true});
            if(!tag)throw FormatError("HUD lock target lost its definition");
            const auto model=word(bank,runtime_tag_address(*tag)+0x38u);
            if(model==0xffffffffu)return result;
            const auto model_tag=cache_owner.query_tag({.handle=TagHandle{model},.require_address=true});
            if(!model_tag)throw FormatError("HUD lock target lost its model");
            const auto model_address=runtime_tag_address(*model_tag);
            if(region>=std::bit_cast<std::int32_t>(word(bank,model_address+0x68u)))return result;
            const auto marker=word(bank,word(bank,model_address+0x6cu)+std::uint32_t(region)*0x1cu);
            constexpr std::uint32_t output=0x60057000u;
            auto a=frame(std::array{target,marker,1u});a.gp[1]=0;a.gp[3]=output;
            Execution execution(*this);
        SourceObjectAttachmentBindingTu15 attachment_events(bank,[this](const auto &event){model_attachments[event.child]=event;});
            source_run_damage_delivery_runtime(bank,0xb9c20u,a,image,fp,services());
            if((a.gp[0]&65535u)==1u)result.position=Vec3{
                std::bit_cast<float>(word(bank,output+0x60u)),std::bit_cast<float>(word(bank,output+0x64u)),
                std::bit_cast<float>(word(bank,output+0x68u))};
        }
        return result;
    }
    std::uint32_t weapon_hud_target_flags(std::uint32_t unit) {
        const auto body=object_body(unit);
        constexpr std::uint32_t output=0x6005a000u;
        std::array<std::byte,0xf8> clear{};bank.write(output,clear);
        //The actual primary/secondary HUD status layout consumed by complete
        //13DC44.13DA89 owns charging/target status; no trigger-mode substitute
        //or native hit/region classification reaches the original mask owner.
        for(unsigned hand=0;hand<2;++hand) {
            const auto slot=std::bit_cast<std::int8_t>(std::uint8_t(word(bank,body+0x212u+hand,1)));
            if(slot< -1 || slot>=4)throw FormatError("HUD target hand escapes source inventory");
            const auto weapon=slot==-1?0xffffffffu:word(bank,body+0x218u+unsigned(slot)*4u);
            put(bank,output+0xcu+hand*4u,weapon);
            if(weapon!=0xffffffffu) {
                const auto state=weapon_hud_state(weapon);
                bank.write(output+0x88u+hand*0x38u,std::as_bytes(std::span(&state,1)));
            }
        }
        auto a=frame({});a.gp[0]=unit;a.gp[3]=output;run_actual(0x13dc44u,a);
        return word(bank,output+0x4cu);
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
    const std::uint32_t tick_owner_census_entry=[]{
        const auto* p=std::getenv("HALO2_TICK_OWNER_CENSUS");return p && *p?std::uint32_t(std::strtoul(p,nullptr,16)):0u;}();
    struct TickOwnerCensus {
        std::map<std::uint32_t,std::pair<std::uint64_t,std::uint32_t>> stores;std::uint64_t ticks{},total{};
    } tick_census;
    const std::uint32_t nested_owner_census_target=[]{
        const auto* p=std::getenv("HALO2_NESTED_OWNER_CENSUS");return p && *p?std::uint32_t(std::strtoul(p,nullptr,16)):0u;}();
    bool nested_census_active{};std::uint64_t nested_census_calls{},nested_census_ticks{};double nested_census_ns{};
    void tick_owner_census(const std::function<void()>& translated) {
        struct Observer {
            SourceRuntimeMemory& bank;TickOwnerCensus& census;
            Observer(SourceRuntimeMemory& b,TickOwnerCensus& c):bank(b),census(c) {
                source_runtime_register_write_observer(bank,this,[this](std::uint32_t address,std::size_t bytes) {
                    auto& e=census.stores[address];++e.first;e.second=std::max(e.second,std::uint32_t(bytes));++census.total;
                });
            }
            ~Observer(){source_runtime_unregister_write_observer(bank,this);}
        } observer{bank,tick_census};
        translated();
        static const unsigned every=[]{const auto* p=std::getenv("HALO2_CALL_CENSUS_EVERY");return p && *p?unsigned(std::strtoul(p,nullptr,10)):240u;}();
        if(++tick_census.ticks%every==0 && !nested_owner_census_target)report_tick_census();
    }
    void report_tick_census() {
        std::cerr<<"Tick owner census "<<std::hex<<(tick_owner_census_entry?tick_owner_census_entry:nested_owner_census_target)<<std::dec<<": calls="<<tick_census.ticks<<" stores="<<tick_census.total<<" ranges:";
        std::uint64_t first=0,end=0,hits=0;unsigned ranges=0;
        const auto flush=[&]{if(end){std::cerr<<" ["<<std::hex<<first<<","<<end<<")x"<<std::dec<<hits;++ranges;}};
        for(const auto& [address,e]:tick_census.stores) {
            if(end && address<=end){end=std::max(end,std::uint64_t(address)+e.second);hits+=e.first;continue;}
            flush();first=address;end=std::uint64_t(address)+e.second;hits=e.first;
        }
        flush();std::cerr<<" ("<<ranges<<" ranges)"<<'\n';
    }
    // Per-tick state digest of one translated tick owner's observable
    // write set (HALO2_TICK_OWNER_DIGEST=<hex entry>): the observer learns
    // every non-stack source range the owner stores to; after each call the
    // current bytes of all learned ranges are FNV-hashed. A native replica of
    // that owner must reproduce the identical per-tick digest sequence.
    // Physical-namespace stores are digested by their 4KiB page set.
    const std::uint32_t tick_owner_digest_entry=[]{
        const auto* p=std::getenv("HALO2_TICK_OWNER_DIGEST");return p && *p?std::uint32_t(std::strtoul(p,nullptr,16)):0u;}();
    struct TickOwnerDigest {
        std::map<std::uint32_t,std::uint32_t> ranges; // start → end (non-stack, non-physical)
        std::set<std::uint32_t> physical_pages;
        std::vector<std::uint64_t> digests;
        std::uint64_t ticks{};
    } tick_digest;
    void tick_owner_digest(const std::function<void()>& translated) {
        struct Observer {
            SourceRuntimeMemory& bank;TickOwnerDigest& d;
            Observer(SourceRuntimeMemory& b,TickOwnerDigest& x):bank(b),d(x) {
                source_runtime_register_write_observer(bank,this,[this](std::uint32_t address,std::size_t bytes) {
                    if(address>=0x600e0000u && address<0x60100000u)return;
                    if(address>=0x80000000u){d.physical_pages.insert(address&~4095u);return;}
                    const auto end=std::uint32_t(address+bytes);
                    auto it=d.ranges.lower_bound(address);
                    if(it!=d.ranges.begin() && std::prev(it)->second>=address)it=std::prev(it);
                    std::uint32_t first=address,last=end;
                    while(it!=d.ranges.end() && it->first<=last){first=std::min(first,it->first);last=std::max(last,it->second);it=d.ranges.erase(it);}
                    d.ranges.emplace(first,last);
                });
            }
            ~Observer(){source_runtime_unregister_write_observer(bank,this);}
        } observer{bank,tick_digest};
        translated();
        std::uint64_t h=1469598103934665603ull;
        const auto mix=[&](std::uint32_t first,std::size_t size) {
            for(auto byte:bank.view(first,size))h=(h^std::to_integer<std::uint8_t>(byte))*1099511628211ull;
        };
        for(const auto& [first,last]:tick_digest.ranges)mix(first,last-first);
        for(const auto page:tick_digest.physical_pages)
            try{mix(page,4096);}catch(const std::exception&){h=(h^0xdeadu)*1099511628211ull;}
        tick_digest.digests.push_back(h);
        if(++tick_digest.ticks%240)return;
        std::uint64_t sequence=1469598103934665603ull;
        for(auto x:tick_digest.digests)for(unsigned b=0;b<8;++b)sequence=(sequence^((x>>(b*8))&255u))*1099511628211ull;
        std::cerr<<"Tick owner digest "<<std::hex<<tick_owner_digest_entry<<": ticks="<<std::dec<<tick_digest.ticks<<
            " ranges="<<tick_digest.ranges.size()<<" physical_pages="<<tick_digest.physical_pages.size()<<
            " sequence="<<std::hex<<sequence<<" last="<<tick_digest.digests.back()<<std::dec<<'\n';
    }
    void step_objects(SourceEngineFrameOwnerTu15 engine_owner) {
        if(engine_owner!=SourceEngineFrameOwnerTu15::World && engine_owner!=SourceEngineFrameOwnerTu15::MatchAdapter)
            throw FormatError("TU1.5 world tick requires one engine frame owner");
        main_receipts.retire();
        if(!session_configured || !word(bank,0x52be48u))
            throw FormatError("TU1.5 object tick requires the configured source world");
        // Native source consumers request textures during game ticks. Run the
        // complete original cache lock/load/scale owner at this boundary;
        // its ten-request counter must not accumulate across all game ticks.
        // This is native scheduling, not the complete Xbox render scheduler.
        call_actual(0x12f110u);
        // Original13AAFF applies the complete player/input/action scheduler
        // before PVS and the object/world phases. Its1500C0..1501A8 path
        // produces the player clusters consumed by13B6F0 itself.
        const std::array commands{0x60028000u};call_actual(0x14fec0u,commands);
        delivered_players=0;put(bank,0x60028008u,0);
        // Literal13AB0D owns AI decisions and the actual NPC unit-control
        // producer before PVS/object physics. Its own source AI-enabled byte
        // admits campaign work and returns immediately for an inactive bank.
        call_actual(0x1d0670u);
        // Literal13AB1C runs the selected engine before PVS/object updates.
        // Existing match adapters explicitly retain this interval until their
        // score/lifecycle/current-unit consumers use the source world bank.
        if(engine_owner==SourceEngineFrameOwnerTu15::World)call_actual(0x15eba0u);
        for(auto entry:{0x13b6f0u,0xb8f40u,0x1cc8d0u,0xb9090u,0xb9180u}) {
            // Store census of one translated tick owner (HALO2_TICK_OWNER_CENSUS=
            // <hex entry>): the write set a native kernel must reproduce.
            const auto advance=[&] {
                if(entry==0x1cc8d0u)step_native_physics_frame();
                else call_actual(entry);
            };
            if(tick_owner_census_entry==entry)tick_owner_census(advance);
            else if(tick_owner_digest_entry==entry)tick_owner_digest(advance);
            else advance();
        }
        if(nested_owner_census_target && ++nested_census_ticks%240==0 && nested_census_calls) {
            std::cerr<<"Nested owner census "<<std::hex<<nested_owner_census_target<<std::dec<<": ticks="<<nested_census_ticks<<
                " calls="<<nested_census_calls<<" mean_us="<<nested_census_ns/nested_census_calls/1000.0<<
                " per_tick_us="<<nested_census_ns/nested_census_ticks/1000.0<<'\n';
            report_tick_census();
        }
        // Literal13AB49 advances source effect owners after object retirement.
        // Native presentation/audio remain separate consumers of completed
        // source requests; source instance lifetime and RNG cannot be skipped.
        call_actual(0x179cd0u);
        // Original13AB4E updates every native light, including sleeping
        // owners, after effects and before later player/render phases.
        call_actual(0xc3430u);
        // Literal13AB53/13AB58 finalize engine state, then advance later
        // player actions after effects/lights and before first-person work.
        if(engine_owner==SourceEngineFrameOwnerTu15::World) {
            call_actual(0x15f450u);call_actual(0x1503d0u);
            // Literal13AB67 owns the game-over countdown and fade request.
            call_actual(0x13b990u);
        }
        // Engine birth follows this tick's command delivery. Construct only
        // missing current-unit inputs before private FP publication; do not
        // invent a command, a look delta or another gameplay/animation tick.
        if(SourceFirstPersonActionObserverTu15::phase_bound(bank))prepare_player_look_births();
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
        if(SourceFirstPersonActionObserverTu15::phase_bound(bank)) {
            const auto producer_caller=frame({});
            SourceFirstPersonActionObserverTu15::phase(bank,producer_caller,services());
        }
        // Literal13AB94..13ABBB follows the FP/feedback phases and precedes
        //13ABD3's admitted game-clock increment. Keep the director timer and
        //FOV elapsed input on this same source timeline.
        source_campaign_camera_game_tick_tu15(bank);
        const auto clock=word(bank,0x51df54u);
        if(!clock)throw FormatError("TU1.5 frame has no source clock owner");
        // Literal13ABD3..13ABD8 follows these phases in the original frame.
        put(bank,clock+8u,word(bank,clock+8u)+1u);
    }
    void initialize_map() {
        if(word(bank,0x52be48u))throw FormatError("TU1.5 physics world is already constructed");
        detail::release_native_startup_code_pages();
        call_actual(0x1e7760u); // Actual row18 new-map team relationship reset.
        for(auto entry:{0x37f110u,0x37f130u,0x37f150u,0x37f1e0u,0x37f240u,
            0x37fc70u,0x37fc90u,0x37fcb0u})call(entry);
        call_actual(0xb8a00u);
        call(0x1c5ff0u);
        if(!word(bank,0x52be48u))throw FormatError("TU1.5 new-map world was not published");
        call_actual(0xb8be0u);
        call_actual(0x10fa90u); // Literal44B2EC row29 activates/resets all32 shader-function slots.
        call_actual(0x1eed30u); // Literal44B1CC resets that cache for the new map.
        call_actual(0x229420u); // Literal44B598 resets feedback slots and original gains.
        call_actual(0x1563e0u); // Literal44B5BC new-map local player effect reset.
        call_actual(0x157eb0u); // Literal44B3C4 original director new-map reset.
        //157EB0 initializes the real four140-byte director records. Preserve
        // the first default for locally owned numeric camera initialization;
        // a later received bank must not provide a new dispatcher selector.
        bank.read(0x4f5514u,std::as_writable_bytes(std::span(original_camera_seed.controller_)));
        original_camera_seed.available_=(original_camera_seed.controller_[0]&255u)==1u;
        call_actual(0x22c480u); // Literal44B574 resets that frame state through its real owner.
        detail::release_native_startup_code_pages();
    }
    void switch_campaign_bsp(std::size_t selected) {
        const auto game=word(bank,0x4f3218u);
        if(!session_configured || word(bank,game+8u)!=1u)
            throw FormatError("Source BSP transition requires its actual campaign session");
        if(selected==bsp.index)return;
        auto next=cache_bsp_image(selected);
        //13A886 unloads in descending subsystem order;13A860 loads in
        //ascending order. These are the original callbacks of this native
        //world's constructed gameplay systems. Rendering has its own owner.
        for(auto at=campaign_bsp_callbacks_tu15.rbegin();at!=campaign_bsp_callbacks_tu15.rend();++at)
            if(at->unload && at->unload!=0x31d400u)call_actual(at->unload);
        bsp=std::move(next);
        auto a=frame({});a.gp[0]=bsp.mapping;run_actual(0x1255c0u,a);
        if(!(a.gp[0]&255u))throw FormatError("Original campaign BSP resource publication failed");
        a=frame({});a.gp[0]=bsp.bsp;run_actual(0x171120u,a);
        source_bind_runtime_cache_bsp_tu15(bank,bsp);
        const std::array ordinal{std::byte(selected),std::byte(selected>>8)};
        bank.write(game+2u,ordinal); // Literal13A851..13A857 native selected-BSP input.
        for(const auto &entry:campaign_bsp_callbacks_tu15)
            if(entry.load && entry.load!=0x31d400u)call_actual(entry.load);
        rendered_model_nodes.clear();
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
    std::uint32_t equip_weapon(std::uint32_t unit,std::uint32_t definition) {
        (void)object_body(unit);
        const auto tag=cache_owner.query_tag({.handle=TagHandle{definition},.group=tag_groups::weapon,.require_address=true});
        if(!tag)throw FormatError("Source equipment input requires a resident weapon definition");
        call_actual(0xcf4d0u,std::array{unit});
        constexpr std::uint32_t request=0x60010000u;
        auto a=frame(std::array{definition,0xffffffffu,0u});a.gp[6]=request;
        run_actual(0xb9960u,a);
        // Literal1A8CAB sets request+C to zero before original object birth.
        put(bank,request+0xcu,0u);
        const auto weapon=call_actual(0xb9b70u,std::array{request});
        if(weapon==0xffffffffu)return weapon;
        call_actual(0xcf5a0u,std::array{unit,weapon,1u});
        const auto inventory=player_equipment(unit);
        return std::find(inventory.weapons.begin(),inventory.weapons.end(),weapon)!=inventory.weapons.end()?weapon:0xffffffffu;
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
    std::map<std::uint32_t,SourceObjectAttachmentTu15> current_model_attachments() const {
        std::map<std::uint32_t,SourceObjectAttachmentTu15> result;
        const auto pool=word(bank,0x4ecbd0u);
        for(const auto &[child,event]:model_attachments) {
            if(event.caller_return!=0xc08a6u || !record(pool,child) || !record(pool,event.parent))continue;
            const auto body=object_body(child);
            if(word(bank,body+0x14u)==event.parent && (word(bank,body+4u)&0x04000000u))result.emplace(child,event);
        }
        return result;
    }
    std::uint32_t model_child_object(std::uint32_t parent,std::uint32_t definition,
        std::uint32_t parent_marker,std::uint32_t child_marker) const {
        const auto body=object_body(parent);std::uint32_t result=0xffffffffu;unsigned count=0;
        for(auto child=word(bank,body+0x10u);child!=0xffffffffu;) {
            if(++count>0x800u)throw FormatError("TU1.5 source child chain is cyclic");
            const auto child_body=object_body(child);
            if(word(bank,child_body+0x14u)!=parent)throw FormatError("TU1.5 source child has another salted parent");
            if(word(bank,child_body)==definition) {
                const auto at=model_attachments.find(child);
                if(at==model_attachments.end())throw FormatError("TU1.5 model child requires its completed original attachment owner");
                const auto &event=at->second;
                if(event.parent==parent && event.parent_marker==parent_marker && event.child_marker==child_marker &&
                    event.caller_return==0xc08a6u && (word(bank,child_body+4u)&0x04000000u)) {
                    if(result!=0xffffffffu)throw FormatError("TU1.5 authored model child marker identity is duplicated");
                    result=child;
                }
            }
            child=word(bank,child_body+0xcu);
        }
        return result;
    }
    std::uint32_t create_scenario_object(const ScenarioObject &placement,SourceScenarioBirth birth) {
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
        if(birth!=SourceScenarioBirth::automatic && birth!=SourceScenarioBirth::runtime)
            throw FormatError("TU1.5 scenario birth requires its actual caller context");
        // D7AA0/D7B80 automatic calls use1/1. B15B0 runtime creation uses0/0.
        // D75A0 then honors the authored at-rest flag rather than forcing
        // creation flag8 for runtime bipeds/vehicles/crates/creatures.
        const auto automatic=birth==SourceScenarioBirth::automatic?1u:0u;
        const std::array args{std::uint32_t(placement.type),std::uint32_t(placement.placement),palette,automatic,automatic};
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
        // Original render scheduling advances the physical geometry cache
        // once before this frame's requests. Without130890 all earlier cuts'
        // blocks remain current and cannot be evicted under cache pressure.
        call_actual(0x130890u);
        rendered_model_nodes.clear();
        auto frame=word(bank,0x4c6854u),cursor=word(bank,0x494338u);
        source_render_frame_begin(frame,cursor);
        put(bank,0x4c6854u,frame);put(bank,0x494338u,cursor);
        // Literal2B630..2B63A clears the512 cluster union once per frame.
        std::ranges::fill(bank.mutable_view(0x4d1ee0u,64),std::byte{});
        const auto scoped=scoped_main_camera_observer.lock();
        if(main_camera_observer || scoped) {
            main_receipts.begin();if(scoped)scoped->frame=main_receipts.frame;
        } else main_receipts.retire();
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
        m.cache_acquired=[this,frame=m.render_frame](auto owner,auto){
            source_validate_presented_model_frame_tu15(bank,owner,frame);};
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
        w.nodes.presented_world_nodes=[this](auto owner,auto model,auto nodes,unsigned count)->std::shared_ptr<void> {
            if(!source_has_presented_model_pose_tu15(bank))return {};
            auto use=std::make_shared<SourcePresentedModelNodeUseTu15>(bank);
            use->publish(owner,model,nodes,count);return use;
        };
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
    std::vector<Transform> completed_body_world_nodes(std::uint32_t object) {
        require_executor();
        if(render_read_borrows || execution_depth || nested_caller_frame)
            throw FormatError("Native body publication requires a completed source phase without a render borrow");
        const auto body=object_body(object);
        const auto short_at=[&](auto at){return std::bit_cast<std::int16_t>(std::uint16_t(word(bank,at,2)));};
        const auto bytes=short_at(body+0x114u),offset=short_at(body+0x116u);
        if(bytes<=0 || bytes%sizeof(Transform) || offset<0 || bytes/sizeof(Transform)>256)
            throw FormatError("Native body publication has no complete WORLD bank");
        const auto nodes=body+std::uint32_t(offset);
        std::vector<Transform> result(std::size_t(bytes)/sizeof(Transform));
        const auto copy=[&](auto at){std::memcpy(result.data(),bank.view(at,std::size_t(bytes)).data(),std::size_t(bytes));};
        // The exact BD610 gates precede BF640. Other object types retain
        // their normal simulation publication until a body owner requests it.
        if(word(bank,body+0xaau,1)!=0 || short_at(body+0x112u)==-1 || word(bank,body+0xb3u,1)>0) {
            copy(nodes);return result;
        }
        const auto tag_table=word(bank,0x4f0414u);
        const auto metadata=[&](std::uint32_t tag){
            if(tag==0xffffffffu)throw FormatError("Native body publication lacks authored model metadata");
            const auto at=word(bank,tag_table+(tag&65535u)*16u+8u);
            if(!at)throw FormatError("Native body publication has an unresolved authored model");
            return at;
        };
        const auto definition=metadata(word(bank,body));
        const auto model=metadata(word(bank,metadata(word(bank,definition+0x38u))+4u));
        if(word(bank,model+0x48u)!=result.size())
            throw FormatError("Native body publication WORLD count differs from its authored model");
        const auto count=word(bank,model+0x1cu);
        const auto region_bytes=short_at(body+0x118u),region_offset=short_at(body+0x11au);
        if(count>16 || region_bytes<0 || region_bytes%10 || std::uint32_t(region_bytes/10)!=count || region_offset<0)
            throw FormatError("Native body publication regions differ from the authored render model");
        // Use the source's normal complete ordinary detail (2C490: one view
        // caps at4). Selection is the same authored permutation/LOD lookup
        // used by4C2B0; no resource readiness or allocation result is supplied.
        // BF640 consumes these section indices only for its deferred node mask.
        const auto lod=std::uint32_t(source_geometry_lod_policy(1).ordinary_maximum);
        std::array<std::byte,16> selected;selected.fill(std::byte{255});
        for(std::uint32_t n=0;n<count;++n) {
            const auto permutation=word(bank,body+std::uint32_t(region_offset)+n,1);
            if(permutation==255)continue;
            const auto region=word(bank,model+0x20u)+n*16u;
            if(permutation>=word(bank,region+8u))throw FormatError("Native body publication permutation escapes its authored region");
            const auto section=word(bank,word(bank,region+12u)+permutation*16u+4u+lod*2u,2);
            if(section!=65535u && section>=word(bank,model+0x24u))throw FormatError("Native body publication section escapes its authored model");
            selected[n]=std::byte(std::uint8_t(section));
        }
        // Only bytes are journaled. The native kernel owns no new collector,
        // resource, backing or C++ owner; topology changes are rejected before
        // publication. Restore in reverse store order, including stack/cache
        // scratch, on successful completion and exceptions alike.
        const auto* reference_journal=std::getenv("HALO2_REFERENCE_BODY_POSE_JOURNAL");
        SourceRuntimeUndoJournal journal(bank,reference_journal && *reference_journal?
            SourceRuntimeUndoJournal::Storage::store_vectors:SourceRuntimeUndoJournal::Storage::packed,
            [](auto,auto){throw FormatError("Native body publication cannot change memory ownership");});
        constexpr auto selected_at=stack+0x100u;
        bank.write(selected_at,selected);
        const std::array args{object,selected_at,nodes};auto abi=frame(args);
        // This is the complete statically compiled TU1.5 entry. Avoid the
        // public untracked-call adapter: MatchState owns its execution tape.
        struct PublicationScope {
            bool& active;
            explicit PublicationScope(bool& active):active(active){active=true;}
            ~PublicationScope(){active=false;}
        } publication(body_pose_publication_active);
        Execution execution(*this);
        source_run_model_render_tu15(bank,0xbf640u,abi,fp,services());
        copy(abi.gp[0]);return result;
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
            // Secondary40ED0 resets only banks1/2. Preserve the completed
            // ordinary bank0 input and any actual41C70 callback writes.
            if(n==0)continue;
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
        if(kind==1u) {
            // Native campaign admission supplies the actual cache scenario
            // path in game-options+1C (game+24). Native1A5B60 resolves its
            // campaign/map key from the retained matg+170 records, including
            // its exact case-sensitive basename comparison and32C0A0 search.
            // Zero-filled IDs are a different campaign and must not survive
            // delivery. NONE remains explicit when metadata has no match.
            const auto &path=cache_owner.scenario();
            if(path.size()>=0x104u)throw FormatError("Admitted campaign scenario path exceeds original game options");
            std::array<std::byte,0x104> scenario_path{};
            std::memcpy(scenario_path.data(),path.data(),path.size());
            bank.write(game+0x24u,scenario_path);
            put(bank,game+0x1cu,0xffffffffu);put(bank,game+0x20u,0xffffffffu);
            const auto record=native_campaign_scenario_record_tu15(bank,game+0x24u);
            if(record) {
                put(bank,game+0x1cu,word(bank,record));
                put(bank,game+0x20u,word(bank,record+4u));
            }
        }
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
        // Complete original row23 owns world69FF0, then the86BD0/86C30
        //player/machine context and retained-result19A430 startup. Deliver
        //the actual session inputs once, without skipping those later owners.
        if(kind==2u)call_actual(0x692d0u);
        else {
            // Campaign keeps its separately staged startup sequence.
            auto simulation_world=frame(std::array{word(bank,0x4dc054u)});
            simulation_world.gp[0]=word(bank,0x4dc050u);
            simulation_world.gp[6]=word(bank,0x4dc04cu);
            simulation_world.gp[1]=(mode>=4 && mode<=5)?word(bank,0x4e5448u):0u;
            run_actual(0x69ff0u,simulation_world);
        }
        // Original13A8B0 admits the gameplay seed before13A780 walks the
        // new-map rows. Noise consumes that stream; constructing it before
        // session delivery and reseeding afterward loses those source draws.
        call_actual(0x11e550u); // Actual44B238 row24: complete219F90 noise/RNG owner.
        call_actual(0x1f38d0u); // Actual44B25C row25: reset the constructed physics scalars.
        call_actual(0x219300u); // Actual44B4C0 row42: derive contrail scalars from that state.
        // Row53 follows those original producers and consumes the admitted
        // campaign/title options before HS and AI new-map initialization.
        call_actual(0x13ea40u);
        call_actual(0x212a20u); // Actual row54 resets HS threads/globals before AI new-map initialization.
        call_actual(0x2035f0u); // Literal44B694 row55 activates and clears recorded animations.
        call_actual(0x1cfc90u); // Actual44B700 AI reset consumes delivered game options.
        session_configured=true;
        detail::release_native_startup_code_pages();
    }
    std::uint32_t ai_object(std::uint32_t reference) {
        if(reference==null_datum)return null_datum;
        if(!session_configured)
            throw FormatError("AI object query requires its configured source world");
        // Original 2AEB30 passes ECX=AI and ESI=a caller's 0x30-byte iterator.
        // The no-argument frame puts scratch above ESP; callees push below it.
        auto a=frame({});
        const auto caller_stack=a.gp[4];
        if(std::uint64_t(caller_stack)+4u+0x30u>0x100000000ull)
            throw FormatError("AI object iterator frame exceeds the source address space");
        const auto iterator=caller_stack+4u;
        bank.view(iterator,0x30u);
        const auto return_address=word(bank,caller_stack);
        a.gp[1]=reference;a.gp[6]=iterator;
        run_actual(0x27caa0u,a);
        // Retain the original register/flag state between the two calls;
        // reinstall only the native caller's return frame consumed by RET.
        a.gp[4]=caller_stack;put(bank,caller_stack,return_address);
        run_actual(0x27cb30u,a);
        if(!a.gp[0])return null_datum;
        if(a.gp[0]>0xffffffffu-0x1bu)
            throw FormatError("AI object unit field exceeds the source address space");
        return word(bank,a.gp[0]+0x18u);
    }
    void place_ai(std::uint32_t reference,std::optional<std::int16_t> count) {
        if(!session_configured)
            throw FormatError("AI placement requires its configured source world");
        SourceAiPlacementOwners owners;
        owners.place_reference=[&](std::uint32_t value) {
            auto a=frame({});a.gp[7]=value;run_actual(0x27d190u,a);
        };
        owners.place_squad=[&](std::int16_t squad,std::int16_t requested) {
            const std::array args{std::uint32_t(std::int32_t(squad)),0xffffffffu,0u,1u};
            auto a=frame(args);a.gp[0]=std::uint32_t(std::int32_t(requested));
            run_actual(0x2097e0u,a);
        };
        owners.remember_reference=[&](std::uint32_t value) {
            const auto pvs=word(bank,0x5020a8u);
            if(!pvs)throw FormatError("AI placement requires its actual PVS state");
            put(bank,pvs+0x364u,value);
        };
        source_place_ai(reference,count,owners);
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
    bool trigger_contains(std::size_t trigger,Vec3 point) {
        const auto scenario=word(bank,0x4ecc20u);
        if(!session_configured || !scenario || trigger>=word(bank,scenario+0x108u))
            throw FormatError("Trigger containment requires its admitted source scenario row");
        auto a=frame({});const auto input=a.gp[4]+4u;
        put(bank,input,std::bit_cast<std::uint32_t>(point.x));
        put(bank,input+4u,std::bit_cast<std::uint32_t>(point.y));
        put(bank,input+8u,std::bit_cast<std::uint32_t>(point.z));
        a.gp[0]=std::uint32_t(trigger);a.gp[7]=input;run_actual(0x11eba0u,a);
        return (a.gp[0]&255u)!=0;
    }
    bool object_in_kill_volume(std::uint32_t object) {
        if(!session_configured || !word(bank,0x52be48u) || !word(bank,0x52be6cu))
            throw FormatError("Kill-volume containment requires its actual configured source world");
        if(object!=0xffffffffu)object_body(object);
        auto a=frame({});a.gp[7]=object;run_actual(0x1f47e0u,a);
        return (a.gp[0]&255u)!=0;
    }
};
struct PhysicsLiveWorldTu15::NativePublicationHold::Impl {
    PhysicsLiveWorldTu15 &world;
    PhysicsLiveWorldTu15::Impl &owner;
    const std::thread::id executor=std::this_thread::get_id();
    std::unique_lock<std::recursive_mutex> gate;
    bool write_guard{},topology_guard{};
    std::unique_ptr<SourceRuntimePolicyHold> policies;
    explicit Impl(PhysicsLiveWorldTu15 &world)
        :world(world),owner(*world.impl_),gate(owner.call_gate,std::try_to_lock) {
        owner.require_executor();
        if(!gate.owns_lock() || owner.public_calls || owner.execution_depth || owner.render_read_borrows ||
            owner.continuation_fault || owner.publication_scope || source_execution_inputs(owner.bank) ||
            !owner.session_configured)
            throw FormatError("Native publication requires a completed unborrowed TU1.5 World");
        // Full native/DSP/sound eligibility was checked by the enclosing
        // ObjectRuntime before acquiring either retained gate. Register the
        // whole-bank guards before timing/SDL and before publishing the flag.
        SourceRuntimePolicyChange admission(owner.bank);
        source_runtime_register_write_guard(owner.bank,this,[](auto,auto){
            throw FormatError("Native publication cannot write source memory");
        });write_guard=true;
        try {
            source_runtime_register_topology_guard(owner.bank,this,[](auto,auto){
                throw FormatError("Native publication cannot replace source backing");
            });topology_guard=true;
            policies=std::make_unique<SourceRuntimePolicyHold>(admission);
        }catch(...){
            if(topology_guard)source_runtime_unregister_topology_guard(owner.bank,this);
            source_runtime_unregister_write_guard(owner.bank,this);throw;
        }
        owner.publication_scope=this;
        owner.publication_held.store(true,std::memory_order_release);
    }
    ~Impl() {
        if(std::this_thread::get_id()!=executor)std::terminate();
        SourceRuntimePolicyChange releasing(*policies);
        policies.reset();
        if(topology_guard)source_runtime_unregister_topology_guard(owner.bank,this);
        if(write_guard)source_runtime_unregister_write_guard(owner.bank,this);
        owner.publication_scope=nullptr;
        owner.publication_held.store(false,std::memory_order_release);
        gate.unlock();
    }
};
PhysicsLiveWorldTu15::NativePublicationHold::NativePublicationHold(PhysicsLiveWorldTu15 &world)
    :impl_(std::make_unique<Impl>(world)){}
PhysicsLiveWorldTu15::NativePublicationHold::~NativePublicationHold(){
    if(std::this_thread::get_id()!=executor_)std::terminate();
}
bool PhysicsLiveWorldTu15::NativePublicationHold::matches(const PhysicsLiveWorldTu15 &world)const noexcept {
    if(std::this_thread::get_id()!=executor_)return false;
    const auto *p=impl_.get();
    if(!p)return false;
    return &world==&p->world && world.impl_.get()==&p->owner && p->gate.owns_lock() &&
        p->owner.publication_scope==p && p->owner.publication_held.load(std::memory_order_acquire);
}
std::unique_ptr<PhysicsLiveWorldTu15::NativePublicationHold> PhysicsLiveWorldTu15::hold_native_publication() {
    // This private borrowed helper is called only by the ObjectRuntime that
    // already performed full completed native preflight before its own gate.
    if(std::this_thread::get_id()!=impl_->main_thread)
        throw FormatError("Native publication requires its retained World executor");
    return std::unique_ptr<NativePublicationHold>(new NativePublicationHold(*this));
}
PhysicsLiveWorldTu15::PhysicsLiveWorldTu15(const Cache&cache,const std::filesystem::path&files,std::size_t bsp)
    :PhysicsLiveWorldTu15(cache,{},files,bsp) {}
PhysicsLiveWorldTu15::PhysicsLiveWorldTu15(const Cache&cache,const std::filesystem::path&files,std::size_t bsp,
    std::shared_ptr<PhysicsLiveClockSource> clocks):PhysicsLiveWorldTu15(cache,{},files,bsp,std::move(clocks)) {}
PhysicsLiveWorldTu15::PhysicsLiveWorldTu15(const Cache&cache,const std::filesystem::path&xbe,const std::filesystem::path&files,std::size_t bsp)
    :PhysicsLiveWorldTu15(cache,xbe,files,bsp,std::make_shared<NativeWorldClock>()) {}
PhysicsLiveWorldTu15::PhysicsLiveWorldTu15(const Cache&cache,const std::filesystem::path&xbe,const std::filesystem::path&files,std::size_t bsp,
    std::shared_ptr<PhysicsLiveClockSource> clocks):impl_(std::make_unique<Impl>(cache,xbe,files,bsp,std::move(clocks))) {}
PhysicsLiveWorldTu15::PhysicsLiveWorldTu15(const Cache&cache,const std::filesystem::path&xbe,const std::filesystem::path&files,std::size_t bsp,
    std::shared_ptr<PhysicsLiveClockSource> clocks,std::shared_ptr<SourceExecutionInputs> inputs)
    :impl_(std::make_unique<Impl>(cache,xbe,files,bsp,clocks?std::move(clocks):std::make_shared<NativeWorldClock>(),std::move(inputs))) {}
PhysicsLiveWorldTu15::~PhysicsLiveWorldTu15(){
    if(impl_ && impl_->publication_held.load(std::memory_order_acquire))std::terminate();
    if(impl_){
        native_lift_scalar_activity_reset_tu15(impl_->bank);
        native_vehicle_rate_activity_reset_tu15(impl_->bank);
        native_constraint_schema_emitter_activity_reset_tu15(impl_->bank);
        impl_->main_receipts.close();
    }
}
SourceRuntimeMemory&PhysicsLiveWorldTu15::memory(){Impl::PublicCall active(*impl_,false);return impl_->bank;}
const SourceRuntimeResources &PhysicsLiveWorldTu15::runtime_resources()const {
    Impl::PublicCall active(*impl_,false);impl_->require_executor();return impl_->resources;
}
NativeTu15PhysicsFrameExecutionStats PhysicsLiveWorldTu15::native_physics_frame_activity() const {
    Impl::PublicCall active(*impl_,false);impl_->require_executor();auto result=impl_->native_physics_frames;
    result.world_integrations=impl_->native_physics_integrations.world_integrations;
    result.island_integrations=impl_->native_physics_integrations.island.islands;
    result.native_force_calls=impl_->native_physics_integrations.island.native_force_calls;
    result.native_pose_calls=impl_->native_physics_integrations.native_pose_calls;
    result.native_deactivation_calls=impl_->native_physics_integrations.native_deactivation_calls;
    result.native_kinematic_pose_calls=impl_->native_physics_integrations.native_kinematic_pose_calls;
    result.native_fixed_pose_calls=impl_->native_physics_integrations.native_fixed_pose_calls;
    result.native_prescribed_force_calls=impl_->native_physics_integrations.native_prescribed_force_calls;
    result.native_body_policy_calls=impl_->native_physics_integrations.native_body_policy_calls;
    result.native_island_transition_calls=impl_->native_physics_integrations.native_island_transition_calls;
    result.native_array_growth_calls=impl_->native_physics_integrations.native_array_growth_calls;
    result.native_array_reserve_calls=impl_->native_physics_integrations.native_array_reserve_calls;
    result.native_array_pool_allocations=impl_->native_physics_integrations.native_array_pool_allocations;
    result.native_array_pool_releases=impl_->native_physics_integrations.native_array_pool_releases;
    result.native_world_collision_calls=impl_->native_physics_integrations.native_world_collision_calls;
    result.native_world_collision_listener_calls=impl_->native_physics_integrations.native_world_collision_listener_calls;
    result.native_island_collision_listener_calls=impl_->native_physics_integrations.native_island_collision_listener_calls;
    result.native_constraint_calls=impl_->native_physics_integrations.native_constraint_calls;
    result.native_dynamic_accumulator_calls=impl_->native_physics_integrations.native_dynamic_accumulator_calls;
    result.native_constraint_setup_calls=impl_->native_physics_integrations.native_constraint_setup_calls;
    result.native_island_collision_calls=impl_->native_physics_integrations.native_island_collision_calls;
    const auto&agents=impl_->native_physics_integrations.island_agents;
    result.native_island_agent_flag_clear_calls=agents.flag_clear_calls;
    result.native_island_agent_flag_set_calls=agents.flag_set_calls;
    result.native_island_agent_process_calls=agents.process_calls;
    result.native_island_agent_finish_calls=agents.finish_calls;
    result.native_island_contact_minimum_calls=agents.native_minimum_calls;
    result.native_island_contact_time_calls=agents.advance_time_calls;
    result.native_island_contact_buffers_constructed=agents.constructed_buffers;
    result.native_island_contact_buffers_destroyed=agents.destroyed_buffers;
    const auto&advance=impl_->native_physics_integrations.contact_advance;
    result.native_contact_advance_calls=advance.calls;
    result.native_contact_advance_motion_calls=advance.motion_calls;
    result.native_contact_advance_process_calls=advance.process_calls;
    result.native_contact_advance_single_iterations=advance.single_motion_iterations;
    result.native_contact_advance_paired_iterations=advance.paired_motion_iterations;
    result.native_contact_advance_negative_single_restores=advance.negative_single_restores;
    result.native_contact_advance_negative_paired_restores=advance.negative_paired_restores;
    result.native_motion_commit_calls=impl_->native_physics_integrations.native_motion_commit_calls;
    result.native_motion_commit_kinds=impl_->native_physics_integrations.native_motion_commit_kinds;
    result.native_broad_phase_calls=impl_->native_physics_integrations.native_broad_phase_calls;
    const auto&bounds=impl_->native_physics_integrations.shape_bounds;
    result.native_shape_bounds_box_calls=bounds.box_calls;
    result.native_shape_bounds_convex_calls=bounds.convex_calls;
    result.native_shape_bounds_transformed_calls=bounds.transformed_calls;
    result.native_shape_bounds_forward_calls=bounds.forward_calls;
    result.native_shape_bounds_motion_calls=bounds.motion_calls;
    result.native_shape_bounds_centered_box_calls=impl_->native_physics_integrations.centered_shape_bounds.centered_box_calls;
    result.native_shape_bounds_centered_radius_calls=impl_->native_physics_integrations.centered_shape_bounds.radius_calls;
    result.shape_bounds_residual_calls=impl_->native_physics_integrations.shape_bounds_residual_calls;
    result.shape_bounds_residual_owners=impl_->native_physics_integrations.shape_bounds_residual_owners;
    result.residual_owner_calls=impl_->native_physics_integrations.residual_owner_calls;
    const auto&compound=impl_->native_physics_integrations.compound_shape_bounds;
    result.native_shape_bounds_endpoint_calls=compound.endpoint_calls;
    result.native_shape_bounds_compound_calls=compound.compound_calls;
    result.native_shape_bounds_compound_child_calls=compound.child_calls;
    result.native_solver_calls=impl_->native_physics_integrations.native_solver_calls;
    result.native_solver_initializations=impl_->native_physics_integrations.native_solver_initializations;
    result.native_solver_iterations=impl_->native_physics_integrations.native_solver_iterations;
    result.native_solver_primary_rows=impl_->native_physics_integrations.native_solver_primary_rows;
    result.native_solver_secondary_rows=impl_->native_physics_integrations.native_solver_secondary_rows;
    result.native_solver_exported_rows=impl_->native_physics_integrations.native_solver_exported_rows;
    result.native_solver_custom_refusals=impl_->native_physics_integrations.native_solver_custom_refusals;
    result.native_solver_clock_calls=impl_->native_physics_integrations.native_solver_clock_calls;
    result.native_solver_custom_calls=impl_->native_physics_integrations.native_solver_custom_calls;
    result.native_solver_custom_kinds=impl_->native_physics_integrations.native_solver_custom_kinds;
    result.native_constraint_producer_axis_calls=impl_->native_physics_integrations.native_constraint_producer_axis_calls;
    result.native_constraint_producer_scaled_calls=impl_->native_physics_integrations.native_constraint_producer_scaled_calls;
    result.native_constraint_producer_base_calls=impl_->native_physics_integrations.native_constraint_producer_base_calls;
    result.native_constraint_producer_base_rows=impl_->native_physics_integrations.native_constraint_producer_base_rows;
    result.native_constraint_producer_property_lookups=impl_->native_physics_integrations.native_constraint_producer_property_lookups;
    result.native_constraint_producer_emissions=impl_->native_physics_integrations.native_constraint_producer_emissions;
    result.native_constraint_producer_completed_emissions=impl_->native_physics_integrations.native_constraint_producer_completed_emissions;
    result.native_lift_scalar_calls=impl_->native_lift_activity.caller_calls;
    result.native_lift_power_calls=impl_->native_lift_activity.power_calls;
    result.native_lift_errno_writes=impl_->native_lift_activity.errno_writes;
    result.native_lift_observation_refusals=impl_->native_lift_activity.observation_refusals;
    result.native_vehicle_rate_acceleration_calls=impl_->native_vehicle_rate_activity.acceleration_calls;
    result.native_vehicle_rate_approach_calls=impl_->native_vehicle_rate_activity.approach_calls;
    result.native_vehicle_rate_cursor_calls=impl_->native_vehicle_rate_activity.live_cursor_calls;
    result.native_vehicle_rate_refusals=impl_->native_vehicle_rate_activity.observation_refusals;
    const auto schema=native_constraint_schema_emitter_activity_tu15(impl_->bank);
    result.native_schema_emitter_calls=schema.calls;
    result.native_schema_emitter_completed_calls=schema.completed_calls;
    result.native_schema_emitter_cursor_calls=schema.live_cursor_calls;
    result.native_schema_emitter_refusals=schema.refusals;
    result.native_schema_emitter_caller_kinds=schema.caller_kinds;
    result.native_schema_emitter_refusal_kinds={schema.observation_refusals,schema.caller_refusals,
        schema.stack_refusals,schema.executor_refusals,schema.image_refusals};
    result.world_residual_calls=impl_->native_world_residual_calls;return result;
}
NativeRuntimeAddress PhysicsLiveWorldTu15::resident_cache_address(const Cache& expected_owner,
    std::uint32_t cache_address,std::size_t bytes) const {
    Impl::PublicCall active(*impl_,false);impl_->require_executor();
    if(&expected_owner!=&impl_->cache_owner || !bytes)
        throw FormatError("Resident cache address requires this world's complete Cache owner");
    const auto logical=impl_->canonical_cache?
        impl_->canonical_cache->runtime_address(cache_address,bytes):cache_address;
    if(impl_->canonical_cache) {
        const auto range=impl_->native_cache_range();
        const auto base=impl_->bank.source_address(range.allocation,range.bytes);
        if(logical<base || std::uint64_t(logical)-base>range.bytes ||
           bytes>range.bytes-(std::uint64_t(logical)-base))
            throw FormatError("Resident cache address escapes its canonical native allocation");
    }else (void)impl_->cache_owner.read_tag_bytes(cache_address,bytes);
    return impl_->bank.native_address(logical,bytes);
}
MccScriptSoundWorldAccess PhysicsLiveWorldTu15::script_sound_access(const Cache& expected_owner) {
    require_execution_boundary();
    Impl::PublicCall active(*impl_,false);impl_->require_executor();
    if(!impl_->canonical_cache||!impl_->sound_runtime_plan||!impl_->sound_timing_plan||!impl_->sound_timing_memory)
        throw FormatError("Script sound World access requires complete converted sound ownership");
    auto provider=detail::make_mcc_sound_variation_provider(impl_->cache_owner,expected_owner,
        *impl_->canonical_cache,*impl_->sound_runtime_plan,impl_->sound_timing_memory);
    return detail::make_mcc_script_sound_world_access(*this,impl_->cache_owner,expected_owner,
        std::move(provider),*impl_->sound_runtime_plan,*impl_->sound_timing_plan,impl_->sound_timing_memory,
        SoundDriverFloatingPointContext{fp.x87_control_word,fp.mxcsr});
}
MccSoundVariationProvider PhysicsLiveWorldTu15::sound_variation_provider(const Cache& expected_owner) const {
    Impl::PublicCall active(*impl_,false);impl_->require_executor();
    if(impl_->execution_depth||impl_->render_read_borrows||impl_->continuation_fault)
        throw FormatError("Native sound variation provider requires a completed available World boundary");
    if(!impl_->canonical_cache||!impl_->sound_runtime_plan||!impl_->sound_timing_memory)
        throw FormatError("Native sound variation provider requires complete converted sound ownership");
    return detail::make_mcc_sound_variation_provider(impl_->cache_owner,expected_owner,
        *impl_->canonical_cache,*impl_->sound_runtime_plan,impl_->sound_timing_memory);
}
void PhysicsLiveWorldTu15::require_publication_provider(const MccSoundVariationProvider &provider,
    const Cache &cache)const {
    // Called only by the exact factory-minted owning scope. The actual native
    // gates are already held; all timing admission still precedes timing/SDL.
    if(std::this_thread::get_id()!=impl_->main_thread)
        throw FormatError("Native metadata publication requires its World executor");
    if(!impl_->publication_held.load(std::memory_order_acquire) || !impl_->publication_scope ||
        &cache!=&impl_->cache_owner || !impl_->sound_timing_memory)
        throw FormatError("Native metadata publication lost its retained World owner");
    detail::require_mcc_sound_variation_owner(provider,cache,impl_->sound_timing_memory);
}
void PhysicsLiveWorldTu15::write_source_call_input(std::uint32_t address,std::span<const std::byte> bytes){
    SourceExecutionCallScope journal(&impl_->bank,SourceExecutionCallKind::WriteSourceCallInput,[&]{return source_execution_arguments(address,bytes);});
    Impl::PublicCall active(*impl_);impl_->bank.write(address,bytes);
}
void PhysicsLiveWorldTu15::write_script_sound_inputs(
    std::span<const std::pair<std::uint32_t,std::span<const std::byte>>> writes){
    // The closed sound adapter already needs a completed, unbound producer
    // boundary. An outer PublicCall extends the existing serial-based fault
    // policy across all writes, including a later guard rejecting before its
    // own write begins. Individual writes retain their ordinary journals.
    require_execution_boundary();
    Impl::PublicCall active(*impl_);
    for(const auto& [address,bytes]:writes)write_source_call_input(address,bytes);
}
void PhysicsLiveWorldTu15::run(std::uint32_t p,SourcePhysicsWorldRuntimeAbi&a,
    const SourcePhysicsContactRecordObserver&o){
    SourceExecutionCallScope journal(&impl_->bank,SourceExecutionCallKind::RunSourceCall,[&]{return source_execution_arguments(p,a);});
    Impl::PublicCall active(*impl_);impl_->run(p,a,o);journal.complete([&]{return source_execution_result(a);});
}
SourcePhysicsWorldRuntimeAbi PhysicsLiveWorldTu15::prepare_source_call(std::span<const std::uint32_t> args){
    SourceExecutionCallScope journal(&impl_->bank,SourceExecutionCallKind::PrepareSourceCall,[&]{return source_execution_arguments(args);});
    Impl::PublicCall active(*impl_);auto a=impl_->frame(args);journal.complete([&]{return source_execution_result(a);});return a;
}

void PhysicsLiveWorldTu15::stop_campaign_screen() {
    SourceExecutionCallScope journal(&impl_->bank,SourceExecutionCallKind::StopCampaignScreen,
        []{return std::vector<std::byte>{};});
    Impl::PublicCall active(*impl_);impl_->require_executor();
    native_campaign_screen_stop_tu15(impl_->bank);
}
void PhysicsLiveWorldTu15::acknowledge_campaign_screen_capture() {
    SourceExecutionCallScope journal(&impl_->bank,SourceExecutionCallKind::AcknowledgeCampaignScreenCapture,
        []{return std::vector<std::byte>{};});
    Impl::PublicCall active(*impl_);impl_->require_executor();
    native_campaign_screen_capture_complete_tu15(impl_->bank);
}

void PhysicsLiveWorldTu15::with_texture_camera_scene_prefix_tu15(
    std::span<const std::byte,0x118> view,bool texture_camera,unsigned local_players,
    const std::function<void(const SourceTextureCameraScenePrefixTu15 &)> &consume) {
    if(!consume || local_players!=1 || Bytes(view).u32(0)!=0 || Bytes(view).u32(4)!=0)
        throw FormatError("Native texture scene prefix requires its actual one-view campaign caller");
    const auto user=std::bit_cast<std::int32_t>(Bytes(view).u32(8));
    if(user< -1 || user>=4 || (texture_camera && user!=-1))
        throw FormatError("Native texture scene prefix lost its actual local-user owner");
    Impl::PublicCall active(*impl_);
    // Literal2B609 actual caller input. The existing cold field remains owned
    // by native submitted-view storage; no second count owner is mounted.
    put(impl_->bank,0x4c686cu,local_players);
    const auto input=impl_->frame({}).gp[4]+0x1000u;
    impl_->bank.write(input,view);
    auto abi=impl_->frame(std::array{input,std::uint32_t(texture_camera)});
    SourceTextureCameraScenePrefixTu15 prefix;
    {
        // This is a separate typed CPU-prefix operation. It does NOT call
        // run_actual(2B770) or journal a suspended scene as a completed entry.
        Impl::Execution execution(*impl_);
        prefix=source_capture_texture_camera_scene_prefix_tu15(impl_->bank,abi,
            *abi.floating_point,impl_->services());
        if(prefix.backend_requested)source_publish_texture_camera_scene_prefix_tu15(impl_->bank,prefix);
    }
    if(!prefix.backend_requested) {
        active.writes=impl_->mutations.serial.load();consume(prefix);return;
    }
    const auto finish=[&] {
        auto cleanup=impl_->frame({});
        Impl::Execution execution(*impl_);
        source_finish_texture_camera_scene_prefix_tu15(impl_->bank,prefix,cleanup,
            *cleanup.floating_point,impl_->services());
        active.writes=impl_->mutations.serial.load();
    };
    try {consume(prefix);} catch(...) {
        // Whole source13C00 completes native CPU stack/context cleanup before
        // a downstream consumer error leaves this typed scope. An actual
        // cleanup fault still propagates and poisons the interrupted World.
        finish();throw;
    }
    finish();
}

void PhysicsLiveWorldTu15::run_actual(std::uint32_t p,SourcePhysicsWorldRuntimeAbi&a,
    const SourcePhysicsContactRecordObserver&o){
    SourceExecutionCallScope journal(&impl_->bank,SourceExecutionCallKind::RunActualSourceCall,[&]{return source_execution_arguments(p,a);});
    Impl::PublicCall active(*impl_);
    const auto scoped_observer=impl_->scoped_main_camera_observer.lock();
    const auto& observer=scoped_observer?scoped_observer->observer:impl_->main_camera_observer;
    const bool main=bool(observer) && p==0x40e80u && a.gp[0]==0 && word(impl_->bank,0x4cd3a0u)==0x556e18u &&
        !word(impl_->bank,0x556e18u+0x2a78u) && !word(impl_->bank,0x556e18u+0x2a7cu);
    impl_->run_actual(p,a,o);
    journal.complete([&]{return source_execution_result(a);});
    // The actual native call returned completely. A later read-only consumer
    // exception is outside its mutation transaction and must not poison it.
    active.writes=impl_->mutations.serial.load();
    if(main && observer) {
        const auto frame=impl_->main_receipts.frame;
        if(!frame||!frame->active||!impl_->session_configured)
            throw FormatError("Completed main camera observation lacks an actual prepared World frame");
        const auto& memory=impl_->bank;SourceMainCameraOperandsTu15 data;
        data.frame=frame->frame;data.tick=frame->tick;data.bsp=frame->bsp;data.clock=frame->clock;
        data.view=word(memory,0x4c66f8u);if(data.view>=4)throw FormatError("Completed main source view lacks its real local user");
        const auto read=[&](auto address,auto& value){memory.read(address,std::as_writable_bytes(std::span(&value,1)));};
        read(0x4c65c0u,data.raw_camera_words);read(0x4c6634u,data.projection_words);read(0x491e68u,data.viewport);
        read(0x4117e0u,data.source_aa);read(0x4922f4u,data.near_far_words);
        read(0x4c686cu,data.local_players);read(0x491e22u,data.submitting_mode);read(0x491e27u,data.submitting_scaled);
        read(0x515ec0u,data.timed_effect_source);read(0x4f2950u,data.game_arena_source);read(0x4f2954u,data.game_arena_used);
        if(!data.timed_effect_source || data.timed_effect_source==0xffffffffu || !data.game_arena_source ||
           data.timed_effect_source<data.game_arena_source || std::uint64_t(data.timed_effect_source)+0x3fcu>
           std::uint64_t(data.game_arena_source)+data.game_arena_used)
            throw FormatError("Completed main timed-effect owner is outside actual allocated game arena");
        data.timed_effect_backing=memory.native_address(data.timed_effect_source,0x3fcu);
        if(data.timed_effect_backing.is_null() || memory.source_address(data.timed_effect_backing,0x3fcu)!=data.timed_effect_source)
            throw FormatError("Completed main timed-effect owner lacks complete actual native backing");
        constexpr std::array effect_offsets{0xacu,0x3cu,0x40u,0x190u,0x194u};
        for(unsigned n=0;n<effect_offsets.size();++n)read(data.timed_effect_source+effect_offsets[n],data.timed_effect_overrides[n]);
        if(data.local_players<0 || data.local_players>1 || std::ranges::any_of(data.timed_effect_overrides,[](auto word){return word!=0;}))
            throw FormatError("Completed main split or timed-boundary overrides lack an owned ordinary depth route");
        const Bytes global(memory.view(0x4c65c0u,0x74)),submitting(memory.view(0x491e38u,0x74));
        if(data.submitting_mode||data.submitting_scaled||data.viewport[0] || data.viewport[1] ||
           data.viewport[2]<=0 || data.viewport[3]<=0 ||
           !std::ranges::equal(global.slice(0,12),submitting.slice(0,12))||!std::ranges::equal(global.slice(0x30,8),submitting.slice(0x30,8))||
           !std::ranges::equal(global.slice(0x40,8),submitting.slice(0x40,8))||data.source_aa[0]||data.source_aa[1] ||
           !std::ranges::equal(memory.view(0x4922f4u,8),submitting.slice(0x40,8)))
            throw FormatError("Completed main receipt lacks its actual ordinary camera/viewport/AA aliases");
        for(auto word:data.projection_words)if(!std::isfinite(std::bit_cast<float>(word)))
            throw FormatError("Completed main source projection contains a nonfinite word");
        SourceViewProjectionBank bank;std::memcpy(&bank,data.projection_words.data(),sizeof(bank));
        const Vec3 eye{global.f32(0),global.f32(4),global.f32(8)};
        const float near=std::bit_cast<float>(data.near_far_words[0]),far=std::bit_cast<float>(data.near_far_words[1]);
        if(!std::isfinite(near)||!std::isfinite(far)||near<=0||far<=near||std::memcmp(&bank.camera_to_world.position,&eye,sizeof(eye)))
            throw FormatError("Completed main receipt projection/eye/planes differ");
        //226F0's640x480 test only selects the timed split. With the actual
        //override bank zero the ordinary13430 W-depth calculation and its
        //camera upload use this completed source viewport at every size.
        const auto camera=source_view_shader_bank(bank,far,eye,
            RenderViewport{0,0,float(data.viewport[3]),float(data.viewport[2]),1,false,false,0},false);
        for(const auto& v:camera)for(float f:v)if(!std::isfinite(f))throw FormatError("Completed main camera contains nonfinite source operands");
        std::memcpy(data.camera_words.data(),camera.data(),sizeof(data.camera_words));
        const float reciprocal=1.f/far,maximum=16777215.f;const std::array<float,4> clip{0,maximum,std::clamp((reciprocal*near)*maximum,0.f,maximum),std::clamp((reciprocal*far)*maximum,0.f,maximum)};
        std::memcpy(data.clip_words.data(),clip.data(),sizeof(clip));
        const SourceMainCameraRasterReceiptTu15 receipt(frame,data);
        struct ReadOnly {
            Impl& owner;ReadOnly(Impl& o):owner(o){
                source_runtime_register_write_guard(owner.bank,this,[](auto,auto){throw FormatError("Completed main callback cannot write source memory");});
                try{source_runtime_register_topology_guard(owner.bank,this,[](auto,auto){throw FormatError("Completed main callback cannot replace source backing");});}
                catch(...){source_runtime_unregister_write_guard(owner.bank,this);throw;}
                ++owner.render_read_borrows;
            }
            ~ReadOnly(){source_runtime_unregister_topology_guard(owner.bank,this);source_runtime_unregister_write_guard(owner.bank,this);--owner.render_read_borrows;}
        } read_only(*impl_);
        SourceExecutionObserverScope observation(memory);
        observer(receipt,memory);
    }
}
void PhysicsLiveWorldTu15::completed_main_camera_observer(SourceCompletedMainCameraObserverTu15 observer){
    Impl::PublicCall active(*impl_,false);impl_->require_executor();if(impl_->execution_depth||impl_->render_read_borrows)throw FormatError("Main observer requires a completed source boundary");
    if(!impl_->scoped_main_camera_observer.expired())throw FormatError("Main camera observer slot has an active exclusive scoped owner");
    if(!observer)impl_->main_receipts.retire();impl_->main_camera_observer=std::move(observer);
}
SourceCompletedMainCameraObserverScopeTu15 PhysicsLiveWorldTu15::scoped_completed_main_camera_observer(SourceCompletedMainCameraObserverTu15 observer){
    Impl::PublicCall active(*impl_,false);impl_->require_executor();
    if(impl_->execution_depth || impl_->render_read_borrows || !observer)
        throw FormatError("Scoped main observer requires a completed source boundary and callback");
    if(impl_->main_camera_observer || !impl_->scoped_main_camera_observer.expired())
        throw FormatError("Main camera observer slot already has an exclusive owner");
    auto state=std::make_shared<detail::MainCameraObserverBinding>(std::move(observer));
    impl_->main_receipts.retire();impl_->scoped_main_camera_observer=state;
    return SourceCompletedMainCameraObserverScopeTu15(std::move(state));
}
void PhysicsLiveWorldTu15::inspect_render_memory(const SourceRenderMemoryInspectorTu15& inspect)const {
    std::unique_lock gate(impl_->call_gate,std::try_to_lock);impl_->require_executor();
    if(!gate.owns_lock()||!inspect||impl_->public_calls||impl_->execution_depth||impl_->render_read_borrows||impl_->continuation_fault)
        throw FormatError("Read-only render memory requires a completed actual World boundary");
    Impl::PublicCall active(*impl_,false);
    const auto state=std::make_shared<detail::MainReadBorrow>(impl_->main_receipts.life,impl_->main_receipts.frame,impl_.get(),&impl_->bank,impl_->main_thread);
    const SourceRenderMemoryBorrowTu15 token(state);
    struct Scope {
        Impl& owner;std::shared_ptr<detail::MainReadBorrow> state;
        Scope(Impl& o,std::shared_ptr<detail::MainReadBorrow> s):owner(o),state(std::move(s)){
            source_runtime_register_write_guard(owner.bank,this,[](auto,auto){throw FormatError("Render borrow cannot write source memory");});
            try{source_runtime_register_topology_guard(owner.bank,this,[](auto,auto){throw FormatError("Render borrow cannot replace source backing");});}
            catch(...){source_runtime_unregister_write_guard(owner.bank,this);throw;}
            ++owner.render_read_borrows;
        }
        ~Scope(){state->active=false;source_runtime_unregister_topology_guard(owner.bank,this);source_runtime_unregister_write_guard(owner.bank,this);--owner.render_read_borrows;}
    } scope(*impl_,state);
    SourceExecutionObserverScope observation(impl_->bank);
    inspect(token,impl_->bank);
}

std::uint32_t PhysicsLiveWorldTu15::call(std::uint32_t p,std::span<const std::uint32_t>a){
    SourceExecutionCallScope journal(&impl_->bank,SourceExecutionCallKind::CallSource,[&]{return source_execution_arguments(p,a);});
    Impl::PublicCall active(*impl_);const auto value=impl_->call(p,a);journal.complete([&]{return source_execution_result(value);});return value;
}
std::uint32_t PhysicsLiveWorldTu15::call_actual(std::uint32_t p,std::span<const std::uint32_t>a){
    // Store census of a native-called whole-engine entry (HALO2_TICK_OWNER_CENSUS=<hex>).
    if(p && impl_->tick_owner_census_entry==p) {
        std::uint32_t result=0;impl_->tick_owner_census([&]{result=impl_->call_actual(p,a);});return result;
    }
    SourceExecutionCallScope journal(&impl_->bank,SourceExecutionCallKind::CallActualSource,[&]{return source_execution_arguments(p,a);});
    Impl::PublicCall active(*impl_);const auto value=impl_->call_actual(p,a);journal.complete([&]{return source_execution_result(value);});return value;
}
void PhysicsLiveWorldTu15::append_render_attachments(SourceBspPartCommandState& state,
    const SourceObjectAdmission& queue,std::uint32_t mode) {
    SourceExecutionCallScope journal(&impl_->bank,SourceExecutionCallKind::RenderAttachments,[&]{return source_execution_arguments(state,queue,mode);});
    Impl::PublicCall active(*impl_);impl_->append_render_attachments(state,queue,mode);
    journal.complete([&]{return source_execution_attachment_result(state);});
}
void PhysicsLiveWorldTu15::preload_render_model(std::uint32_t model){Impl::PublicCall active(*impl_);impl_->preload_render_model(model);}
void PhysicsLiveWorldTu15::begin_model_frame(){
    SourceExecutionCallScope journal(&impl_->bank,SourceExecutionCallKind::BeginModelFrame,[]{return std::vector<std::byte>{};});
    Impl::PublicCall active(*impl_);impl_->begin_model_frame();
}
void PhysicsLiveWorldTu15::publish_render_visibility(const SourcePortalVisibilitySet& visibility){
    SourceExecutionCallScope journal(&impl_->bank,SourceExecutionCallKind::RenderVisibility,[&]{return source_execution_arguments(visibility);});
    Impl::PublicCall active(*impl_);impl_->publish_render_visibility(visibility);
}
void PhysicsLiveWorldTu15::begin_model_view(){
    SourceExecutionCallScope journal(&impl_->bank,SourceExecutionCallKind::BeginModelView,[]{return std::vector<std::byte>{};});
    Impl::PublicCall active(*impl_);impl_->call_actual(0x22830u);
}
std::optional<std::vector<Transform>> PhysicsLiveWorldTu15::rendered_world_nodes(std::uint32_t object)const {
    Impl::PublicCall active(*impl_,false);
    const auto found=impl_->rendered_model_nodes.find(object);
    return found==impl_->rendered_model_nodes.end()?std::nullopt:std::optional(found->second);
}
std::vector<Transform> PhysicsLiveWorldTu15::completed_body_world_nodes(std::uint32_t object) {
    Impl::PublicCall active(*impl_,false);
    return impl_->completed_body_world_nodes(object);
}
bool PhysicsLiveWorldTu15::request_render_geometry(std::uint32_t flags,std::uint32_t header) {
    SourceExecutionCallScope journal(&impl_->bank,SourceExecutionCallKind::RenderGeometry,[&]{return source_execution_arguments(flags,header);});
    Impl::PublicCall active(*impl_);
    auto a=impl_->frame({});a.gp[0]=flags;a.gp[1]=header;
    impl_->run_actual(0x130980u,a);const bool result=(a.gp[0]&255u)!=0;
    journal.complete([&]{return source_execution_result(std::uint32_t(result));});return result;
}
SourceObjectAdmission PhysicsLiveWorldTu15::admit_render_objects(const SourcePortalVisibilitySet& visibility,
    const SourceObjectAdmissionInput& input,std::span<const std::array<std::uint32_t,4>> prior)const {Impl::PublicCall active(*impl_,false);
    const auto& b=impl_->bank;
    if(!impl_->session_configured)throw FormatError("TU1.5 object admission requires the current session");
    return source_object_admission_memory(visibility,{word(b,0x4ecbd0u),word(b,0x4eabb0u),
        word(b,0x4eabb4u),word(b,0x4eaba4u),word(b,0x4eaba8u),
        [&](auto address,auto size){return b.view(address,size);}},input,prior);
}
SourceObjectClusterMemberships PhysicsLiveWorldTu15::render_cluster_memberships()const {
    Impl::PublicCall active(*impl_,false);const auto& b=impl_->bank;
    if(!impl_->session_configured)throw FormatError("Source cluster census requires the current session");
    const auto count=impl_->cache_owner.load_source_bsp_admission(impl_->bsp.index).clusters.size();
    return source_object_cluster_memberships({word(b,0x4ecbd0u),word(b,0x4eabb0u),
        word(b,0x4eabb4u),word(b,0x4eaba4u),word(b,0x4eaba8u),
        [&](auto address,auto size){return b.view(address,size);}},count);
}
SourceObjectDrawCollector PhysicsLiveWorldTu15::build_world_model(std::uint32_t object,unsigned view,
    unsigned players,bool modifier,float metric,float lighting_metric){
    SourceExecutionCallScope journal(&impl_->bank,SourceExecutionCallKind::BuildWorldModel,[&]{return source_execution_arguments(object,view,players,modifier,metric,lighting_metric);});
    Impl::PublicCall active(*impl_);
    return impl_->build_world_model(object,view,players,modifier,metric,lighting_metric);
}
void PhysicsLiveWorldTu15::build_render_model(SourceObjectDrawCollector& collector,const SourceObjectBuildRequest& request,
    unsigned view,unsigned players,bool modifier) {Impl::PublicCall active(*impl_);
    impl_->build_render_model(collector,request,view,players,modifier);
}
SourceWeaponEffectPresentation PhysicsLiveWorldTu15::request_weapon_effect(const WeaponEffectInput&i){Impl::PublicCall active(*impl_);return impl_->request_weapon_effect(i);}
void PhysicsLiveWorldTu15::populate_weapon_targeting(std::uint32_t p,std::uint32_t o,std::span<std::byte,0x5c> c){Impl::PublicCall active(*impl_);impl_->populate_weapon_targeting(p,o,c);}
void PhysicsLiveWorldTu15::populate_weapon_selection(std::uint32_t u,std::span<std::byte,0x5c> c,bool s){Impl::PublicCall active(*impl_);impl_->populate_weapon_selection(u,c,s);}
std::uint32_t PhysicsLiveWorldTu15::weapon_hud_target_flags(std::uint32_t unit){Impl::PublicCall active(*impl_);return impl_->weapon_hud_target_flags(unit);}
WeaponHudTarget PhysicsLiveWorldTu15::weapon_hud_target(std::uint32_t weapon){Impl::PublicCall active(*impl_);return impl_->weapon_hud_target(weapon);}
SourceWeaponHudState PhysicsLiveWorldTu15::weapon_hud_state(std::uint32_t weapon){Impl::PublicCall active(*impl_);return impl_->weapon_hud_state(weapon);}
VehicleWeaponHudStatus PhysicsLiveWorldTu15::vehicle_weapon_hud(std::uint32_t unit){Impl::PublicCall active(*impl_);return impl_->vehicle_weapon_hud(unit);}
std::optional<ParticleCollisionHit> PhysicsLiveWorldTu15::particle_collision(std::uint32_t f,Vec3 p,Vec3 d){
    SourceExecutionCallScope journal(&impl_->bank,SourceExecutionCallKind::ParticleCollision,[&]{return source_execution_arguments(f,p,d);});
    Impl::PublicCall active(*impl_);auto result=impl_->particle_collision(f,p,d);
    journal.complete([&]{return source_execution_collision_result(result);});return result;
}
bool PhysicsLiveWorldTu15::objective_waypoint_occluded(Vec3 p,Vec3 e,std::uint32_t flags,std::uint32_t viewer,std::uint32_t target) {
    constexpr std::uint32_t start=0x60041400u,end=0x60041420u,result=0x60041500u;
    write_source_call_input(start,std::as_bytes(std::span(&p,1)));
    write_source_call_input(end,std::as_bytes(std::span(&e,1)));
    auto abi=prepare_source_call(std::array{flags,viewer,result});
    abi.gp[0]=start;abi.gp[1]=end;abi.gp[2]=target;
    run_actual(0x16cea0u,abi);return (abi.gp[0]&255u)!=0;
}
std::uint32_t PhysicsLiveWorldTu15::source_world()const{Impl::PublicCall active(*impl_,false);return word(impl_->bank,0x52be48);}
NativeRuntimeAddress PhysicsLiveWorldTu15::world()const {Impl::PublicCall active(*impl_,false);
    const auto source=source_world();
    return source?impl_->bank.native_address(source,4):NativeRuntimeAddress{};
}
void PhysicsLiveWorldTu15::initialize_map(){Impl::PublicCall active(*impl_);impl_->initialize_map();}
SourceOriginalCameraSeed PhysicsLiveWorldTu15::original_camera_seed()const {
    impl_->require_executor();Impl::PublicCall active(*impl_,false);
    if(!impl_->original_camera_seed.available())
        throw FormatError("Locally initialized original camera seed is unavailable");
    return impl_->original_camera_seed;
}
std::array<std::byte,0x5c> PhysicsLiveWorldTu15::collect_native_input(std::uint32_t player,
    unsigned view,float frame_seconds,float game_seconds,SourceCameraControllerState&controller,
    SourceObserverState&observer) {
    Impl::PublicCall active(*impl_);
    if(!impl_->native_input_devices || !impl_->native_input_initialized ||
        source_execution_inputs(impl_->bank) || view>=4 ||
        !std::isfinite(frame_seconds) || frame_seconds<0 ||
        !std::isfinite(game_seconds) || game_seconds<0)
        throw FormatError("Complete native collection requires its local device/sample owner and finite elapsed times");
    auto&bank=impl_->bank;
    const auto owner=impl_->record_player_for_look(player);
    if(!owner || word(bank,owner+0x28u,2)!=view)
        throw FormatError("Native collection requires the actual full player and logical view");
    const auto device=word(bank,owner+0x24u),locals=word(bank,0x4f54f0u);
    if(device>=4 || word(bank,locals+0xcu+view*4u)!=player ||
        word(bank,locals+0x1cu+device*4u)!=player)
        throw FormatError("Native collection lost its physical-device/player inverse association");
    const auto unit=word(bank,owner+0x2cu);
    const auto input=word(bank,0x4f9b54u)+0x14u+view*0x94u;
    const auto retained=0x60070000u+(player&0xffffu)*0xa0u;
    const auto source_controller=0x4f5514u+view*0x140u;
    const auto source_observer=0x4f64a4u+view*0x358u;
    struct Context {
        SourceRuntimeMemory&bank;
        std::vector<std::pair<std::uint32_t,std::vector<std::byte>>> saved;
        void save(std::uint32_t at,std::size_t count) {
            std::vector<std::byte> bytes(count);bank.read(at,bytes);
            saved.emplace_back(at,std::move(bytes));
        }
        ~Context(){for(auto i=saved.rbegin();i!=saved.rend();++i)bank.write(i->first,i->second);}
    } context{bank,{}};
    context.save(input,0x94);context.save(source_controller,sizeof(controller));
    context.save(source_observer,sizeof(observer));
    if(word(bank,retained)==unit &&
        (word(bank,retained+0x94u)==player || word(bank,retained+0x94u)==0xffffffffu))
        bank.write(input,bank.view(retained,0x94));
    else if(word(bank,input)!=unit) {
        auto a=impl_->frame({});a.gp[0]=view;a.gp[1]=unit;
        impl_->run_actual(0x18a0c0u,a);
    }
    bank.write(source_controller,std::as_bytes(std::span(controller)));
    bank.write(source_observer,std::as_bytes(std::span(observer)));
    constexpr auto output=0x6005c200u;
    std::array<std::byte,0x5c> command;initialize_source_neutral_command(command);
    bank.write(output,command);
    impl_->call_actual(0x1898a0u,std::array{player,std::bit_cast<std::uint32_t>(frame_seconds),
        std::bit_cast<std::uint32_t>(game_seconds),output});
    bank.read(output,command);
    if(!source_player_command_valid(command))
        throw FormatError("Original1898A0 produced an invalid native command");
    bank.write(retained,bank.view(input,0x94));put(bank,retained+0x94u,player);
    bank.read(source_controller,std::as_writable_bytes(std::span(controller)));
    bank.read(source_observer,std::as_writable_bytes(std::span(observer)));
    return command;
}
void PhysicsLiveWorldTu15::initialize_native_input(NativeTu15InputSelection selection) {
    Impl::PublicCall active(*impl_);
    if(source_execution_inputs(impl_->bank))
        throw FormatError("Native physical input construction requires its independent local input world");
    if(impl_->native_input_devices)
        throw FormatError("Native physical input already has its retained owner");
    for(unsigned device=0;device<4;++device)
        if(word(impl_->bank,0x4f2a9cu+device*4u))
            throw FormatError("Native input construction cannot adopt existing or received device handles");
    mount_native_tu15_input_collection_data(impl_->bank);
    if(!impl_->native_input_initialized) {
        for(unsigned device=0;device<4;++device)
            if(word(impl_->bank,0x55d770u+device*0xc74u)&0x10u)
                throw FormatError("Native input construction cannot adopt an incoming selected profile");
        impl_->call_actual(0x192380u);
    }
    impl_->native_input_devices=std::make_unique<NativeTu15InputDevices>(impl_->bank,std::move(selection));
    try {
        impl_->call_actual(0x1273c0u);
        impl_->call_actual(0x21e5d0u);
        impl_->native_input_initialized=true;
    }catch(...) {
        try{impl_->call_actual(0x127430u);}catch(...){}
        impl_->native_input_devices.reset();throw;
    }
}
void PhysicsLiveWorldTu15::publish_native_input_profile(unsigned device,const SourceFullPlayerProfile&profile,
    const SourceMachineIdentity&machine) {
    Impl::PublicCall active(*impl_);
    if(!impl_->native_input_devices || device>=4 || source_execution_inputs(impl_->bank))
        throw FormatError("Native selected profile requires its actual local device/input owner");
    constexpr auto argument=0x6005c000u;
    impl_->bank.write(0x4dc42cu,std::as_bytes(std::span(machine)));
    impl_->bank.write(argument,std::as_bytes(std::span(profile)));
    impl_->call_actual(0x192931u,std::array{device,argument,0xffffffffu});
}
void PhysicsLiveWorldTu15::sample_native_input() {
    Impl::PublicCall active(*impl_);
    if(!impl_->native_input_devices)
        throw FormatError("Native physical sampling requires its original device owner");
    impl_->native_input_devices->clear_calls();
    impl_->call_actual(0x127460u);
    impl_->call_actual(0x21e650u);
}
void PhysicsLiveWorldTu15::close_native_input() {
    Impl::PublicCall active(*impl_);
    if(!impl_->native_input_devices)return;
    impl_->native_input_devices->clear_calls();
    impl_->call_actual(0x127430u);
    impl_->native_input_devices.reset();
}
void PhysicsLiveWorldTu15::admit_renderer_palette(RendererPaletteFactory factory) {
    Impl::PublicCall active(*impl_);
    if(impl_->session_configured || impl_->renderer_palette)
        throw FormatError("Native source palette admission requires an unconfigured world without a prior graphics owner");
    impl_->renderer_palette=std::make_unique<NativeTu15RendererPalette>(impl_->bank,std::move(factory));
}
std::size_t PhysicsLiveWorldTu15::source_bsp_index()const{Impl::PublicCall active(*impl_,false);return impl_->bsp.index;}
void PhysicsLiveWorldTu15::switch_campaign_bsp(std::size_t b){Impl::PublicCall active(*impl_);impl_->switch_campaign_bsp(b);}
std::uint32_t PhysicsLiveWorldTu15::source_object_pool()const{Impl::PublicCall active(*impl_,false);return word(impl_->bank,0x4ecbd0u);}
std::uint32_t PhysicsLiveWorldTu15::source_player_pool()const{Impl::PublicCall active(*impl_,false);return word(impl_->bank,0x4f54f4u);}
NativeRuntimeAddress PhysicsLiveWorldTu15::object_pool()const{Impl::PublicCall active(*impl_,false);return impl_->bank.native_address(source_object_pool(),0x4c);}
NativeRuntimeAddress PhysicsLiveWorldTu15::player_pool()const{Impl::PublicCall active(*impl_,false);return impl_->bank.native_address(source_player_pool(),0x4c);}
void PhysicsLiveWorldTu15::construct_player(std::uint32_t p,unsigned u,
    const SourcePlayerConfiguration&c,const SourceMachineIdentity&m){Impl::PublicCall active(*impl_);impl_->construct_player(p,u,c,m);}
bool PhysicsLiveWorldTu15::uses_scenario_player_spawns(std::span<const Spawn> starts)const{
    Impl::PublicCall active(*impl_);return impl_->uses_scenario_player_spawns(starts);}
std::uint32_t PhysicsLiveWorldTu15::birth_player(std::uint32_t player){
    Impl::PublicCall active(*impl_);return impl_->birth_player(player);}
SourcePlayerConfiguration PhysicsLiveWorldTu15::update_player_configuration(std::uint32_t p,const SourcePlayerConfiguration &c) {
    Impl::PublicCall active(*impl_);return impl_->update_player_configuration(p,c);
}
SourceMultiplayerEngineStateTu15 PhysicsLiveWorldTu15::multiplayer_engine_state()const {
    Impl::PublicCall active(*impl_,false);
    auto& memory=impl_->bank;const auto engine=word(memory,0x4f63b8u),options=word(memory,0x4f3218u);
    if(!impl_->session_configured || !engine || !options || word(memory,options+8u)!=2u)
        throw FormatError("TU1.5 engine observation requires its configured multiplayer world");
    SourceMultiplayerEngineStateTu15 result;
    auto& round=result.round;
    const auto short_at=[&](auto address){return std::bit_cast<std::int16_t>(std::uint16_t(word(memory,address,2)));};
    round.flags=word(memory,engine);round.phase=short_at(engine+0x6cu);round.index=short_at(engine+0x6eu);
    round.start_tick=word(memory,engine+0x70u);
    for(unsigned i=0;i<round.ranking.size();++i)round.ranking[i]=word(memory,engine+0x74u+i*4u);
    round.clock_seconds=short_at(engine+0xe0u);
    round.countdown=std::bit_cast<std::int32_t>(word(memory,engine+0xc00u));
    round.status=std::bit_cast<std::int32_t>(word(memory,engine+0xc04u));
    memory.read(engine+0x304u,std::as_writable_bytes(std::span(&result.statistics,1)));
    result.recording.enabled=word(memory,0x51dfa0u,1)!=0;
    result.recording.frozen=word(memory,0x51dfa1u,1)!=0;
    result.game_over={word(memory,options+0x1128u,1)!=0,word(memory,options+0x1129u,1)!=0,
        std::bit_cast<std::int32_t>(word(memory,options+0x112cu))};
    result.retained_teams=std::uint16_t(word(memory,engine+0xeu,2));
    result.available_teams=std::uint16_t(word(memory,engine+4u,2));
    result.finalized=word(memory,engine+0xc08u,1)!=0;return result;
}
SourceMultiplayerEngineEntityTu15 PhysicsLiveWorldTu15::multiplayer_engine_entity()const {
    Impl::PublicCall active(*impl_,false);
    if(!impl_->session_configured)
        throw FormatError("TU1.5 engine entity requires its configured multiplayer world");
    return source_multiplayer_engine_entity_tu15(impl_->bank,fp);
}
void PhysicsLiveWorldTu15::observe_multiplayer_round_end(std::function<void(const SourceMultiplayerRoundEndTu15 &)> observer) {
    Impl::PublicCall active(*impl_);impl_->multiplayer_round_end_observer=std::move(observer);
}
void PhysicsLiveWorldTu15::observe_multiplayer_events(std::function<void(const SourceMultiplayerEventTu15 &)> observer) {
    Impl::PublicCall active(*impl_);impl_->multiplayer_event_observer=std::move(observer);
}
void PhysicsLiveWorldTu15::set_local_player(std::uint32_t p,std::int16_t v){Impl::PublicCall active(*impl_);impl_->set_local_player(p,v);}
void PhysicsLiveWorldTu15::set_player_input_user(std::uint32_t p,std::int16_t u){Impl::PublicCall active(*impl_);impl_->set_player_input_user(p,u);}
void PhysicsLiveWorldTu15::publish_native_observer_position(unsigned view,Vec3 position) {
    SourceExecutionCallScope journal(&impl_->bank,SourceExecutionCallKind::ObserverPosition,[&]{return source_execution_arguments(view,position);});
    Impl::PublicCall active(*impl_);
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
    Vec3 p,Vec3 f,Vec3 u,const SourceFirstPersonPhaseObserverTu15 &observer) {
    SourceExecutionCallScope journal(&impl_->bank,SourceExecutionCallKind::FirstPersonModels,[&]{return source_execution_arguments(view,p,f,u);});
    Impl::PublicCall active(*impl_);
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
void PhysicsLiveWorldTu15::prepare_player_look(std::uint32_t p,std::uint32_t o,
    std::span<std::byte,0x5c>c){Impl::PublicCall active(*impl_);impl_->prepare_player_look(p,o,c);}
std::optional<SourcePlayerActionTarget> PhysicsLiveWorldTu15::player_action_target(std::uint32_t p,std::uint32_t o) const {
    Impl::PublicCall active(*impl_);return impl_->player_action_target(p,o);
}
std::optional<Vec2> PhysicsLiveWorldTu15::player_look(std::uint32_t p,std::uint32_t o) const {
    Impl::PublicCall active(*impl_,false);return impl_->player_look(p,o);
}
void PhysicsLiveWorldTu15::bind_sound_user_settings(const std::filesystem::path &path) {Impl::PublicCall active(*impl_);
    impl_->sound_sdk_resources->bind_user_settings(SourceTu15UserSettings::load(path));
}
void PhysicsLiveWorldTu15::bind_native_sound_output(std::shared_ptr<SdlSoundPlatform> output,const std::filesystem::path &configuration) {
    Impl::PublicCall active(*impl_);impl_->require_sound_quiescent();
    if(word(impl_->bank,0x52d674u))throw FormatError("Native sound binding requires an unconstructed source driver");
    auto preference=std::make_unique<NativeTu15AudioPreference>(impl_->bank,output,native_tu15_load_audio_preference(configuration));
    auto dma=std::make_unique<SourceRuntimeSoundDmaResources>(impl_->bank,output);
    auto reset=std::make_unique<SourceRuntimeSoundResetResources>(impl_->bank,std::move(output),tls);
    mount_native_tu15_sound_dma_imports(impl_->bank);
    preference->publish();
    impl_->sound_preference=std::move(preference);impl_->sound_dma=std::move(dma);impl_->sound_reset=std::move(reset);
}
void PhysicsLiveWorldTu15::bind_sound_boot_smc_input(const std::filesystem::path &path) {Impl::PublicCall active(*impl_);
    impl_->require_sound_quiescent();
    source_mount_tu15_boot_smc_video_mode_input(impl_->bank,SourceBootSmcVideoModeInput::load(path));
}
void PhysicsLiveWorldTu15::observe_effect_light_births(SourceEffectLightBirthObserverTu15 observer) {Impl::PublicCall active(*impl_);
    impl_->effect_light_birth_observer=std::move(observer);
}
void PhysicsLiveWorldTu15::observe_sound_submissions(SourceSoundSubmitObserverTu15 observer) {Impl::PublicCall active(*impl_);
    impl_->sound_submit_observer=std::move(observer);
}
void PhysicsLiveWorldTu15::observe_sound_requests(SourceSoundRequestObserverTu15 observer) {Impl::PublicCall active(*impl_);
    impl_->sound_request_observer=std::move(observer);
}
void PhysicsLiveWorldTu15::observe_loop_sound_requests(SourceLoopSoundRequestObserverTu15 observer) {Impl::PublicCall active(*impl_);
    impl_->loop_sound_request_observer=std::move(observer);
}
std::vector<SourceObjectLoopEmitterTu15> PhysicsLiveWorldTu15::object_loop_emitters() {
    SourceExecutionCallScope journal(&impl_->bank,SourceExecutionCallKind::ObjectLoopEmitters,
        []{return std::vector<std::byte>{};});
    Impl::PublicCall active(*impl_);
    auto& memory=impl_->bank;const auto pool=word(memory,0x4f9b5cu);
    if(!pool)throw FormatError("Native object-loop publication lost its constructed source pool");
    const auto capacity=word(memory,pool+0x20u),stride=word(memory,pool+0x24u),high=word(memory,pool+0x38u);
    if(capacity!=source_object_loop_emitter_capacity_tu15 || stride!=24 || high>capacity)
        throw FormatError("Native object-loop publication differs from original18B5E9 storage");
    const auto rows=word(memory,pool+0x44u),bits=word(memory,pool+0x48u);
    std::vector<SourceObjectLoopEmitterTu15> bank;
    for(unsigned index=0;index<high;++index) {
        if(!(word(memory,bits+(index/32u)*4u)&(1u<<(index%32u))))continue;
        const auto record=rows+index*stride;
        const auto flags=word(memory,record+4u,2);
        // Detached loops, player/weapon/vehicle listeners and nonordinary
        // definitions retain their real source owners. This publication is
        // the bounded plain attached-static route, including the grav lift.
        if(word(memory,record+3u,1)!=1 || (flags&1u))continue;
        const auto object=word(memory,record+0x10u),loop=(word(memory,record,2)<<16)|index;
        if(impl_->record(pool,loop)!=record)throw FormatError("Native loop publication lost its salted generation");
        const auto header=impl_->record(word(memory,0x4ecbd0u),object);
        if(!header || !word(memory,header+8u))continue;
        const auto body=word(memory,header+8u);
        if(word(memory,body+0x14u)!=0xffffffffu)continue;
        const auto owner=impl_->cache_owner.query_tag({.handle=TagHandle{word(memory,body)},.require_address=true});
        if(!owner || (owner->group!="scen" && owner->group!="bloc" && owner->group!="ssce"))continue;
        const auto definition=word(memory,record+0xcu);
        const auto tag=impl_->cache_owner.query_tag({.handle=TagHandle{definition},.group=TagGroup::from_string("lsnd"),.require_address=true});
        if(!tag)throw FormatError("Native static-loop publication lost its authored lsnd definition");
        const auto root=impl_->runtime_tag_address(*tag);
        if(word(memory,root)&255u)continue;
        auto abi=impl_->frame({});abi.gp[0]=object;impl_->run_actual(0xbbd50u,abi);
        if(!(abi.gp[0]&255u))continue;
        // The native caller owns this zeroed output storage. Complete source
        // routines own every function, marker, velocity and location value.
        // These calls and their actual writes remain inside this journal.
        abi=impl_->frame({});const auto output=abi.gp[4]+0x100u;
        std::array<std::byte,68> initial{};memory.write(output,initial);
        put(memory,output+4u,word(memory,record+8u));
        abi=impl_->frame(std::array{object,word(memory,record+0x14u),output+4u});
        impl_->run_actual(0xbcb70u,abi);
        const bool function_active=(abi.gp[0]&255u)!=0;
        abi=impl_->frame({});abi.gp[0]=loop;abi.gp[1]=output;
        impl_->run_actual(0x18f9b0u,abi);
        SourceObjectLoopEmitterTu15 emitter;
        emitter.loop=loop;emitter.object=object;emitter.definition=definition;
        // Source playback progress (bit1 and byte5bit0) belongs solely to
        // the authority's local presentation; never export it as client state.
        emitter.source_flags=std::uint16_t(flags&~0x102u);emitter.function_active=function_active;
        memory.read(output,emitter.location);
        const Bytes location(emitter.location);
        emitter.cluster=std::bit_cast<std::int16_t>(location.u16(0x2cu));
        emitter.maximum_distance=(std::to_integer<unsigned>(emitter.location[0])&8u)?location.f32(0x38u):std::bit_cast<float>(word(memory,root+0x10u));
        if(flags&4u) {
            abi=impl_->frame({});impl_->run_actual(0x1904d0u,abi);
            emitter.stop_duration=std::bit_cast<float>(abi.xmm[0][0]);
        } else emitter.stop_duration=(flags&8u)?.01f:0.f;
        bank.push_back(std::move(emitter));
    }
    validate_source_object_loop_emitters_tu15(bank);
    journal.complete([&]{return source_execution_loop_emitters_result(bank);});return bank;
}
void PhysicsLiveWorldTu15::step_native_object_audio(const SourceObjectAudioViewsTu15& views,float elapsed) {
    Impl::PublicCall active(*impl_);
    if(!std::isfinite(elapsed) || elapsed<0)
        throw FormatError("Native object audio requires a finite elapsed camera interval");
    const auto locals=word(impl_->bank,0x4f54f0u);
    const auto bsp=word(impl_->bank,0x4ecc18u),clusters=word(impl_->bank,bsp+0x9cu);
    for(unsigned slot=0;slot<views.size();++slot)if(const auto &view=views[slot]) {
        const auto p=view->position;
        if(!locals || word(impl_->bank,locals+0xcu+slot*4u)==0xffffffffu ||
            !std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z) ||
            view->cluster< -1 || (view->cluster>=0 && unsigned(view->cluster)>=clusters))
            throw FormatError("Native object audio lost its actual local view or BSP location");
    }
    auto abi=impl_->frame(std::array{std::bit_cast<std::uint32_t>(elapsed)});
    // The oracle also checks a partially converted group (unconverted
    // routines run their lifted originals on both sides).
    const auto mode=native_tu15_sound_manager_mode();
    const bool native=mode==NativeTu15SoundManagerMode::oracle ||
        (native_tu15_sound_manager_owns_entry(0x18ee50u) && mode!=NativeTu15SoundManagerMode::reference);
    if(native_tu15_sound_census_enabled())native_tu15_sound_census(0x18ee50u,native);
    if(mode==NativeTu15SoundManagerMode::oracle) {
        impl_->snapshot_oracle(abi,
            [&](SourcePhysicsWorldRuntimeAbi &reference) {
                source_run_object_loop_audio_tu15(impl_->bank,0x18ee50u,reference,fp,impl_->services(),{},&views);
            },
            [&](SourcePhysicsWorldRuntimeAbi &native_abi) {
                native_tu15_object_audio_frame(impl_->bank,native_abi,fp,impl_->services(),&views);
            },"object audio frame 18ee50");
    } else if(native)native_tu15_object_audio_frame(impl_->bank,abi,fp,impl_->services(),&views);
    else source_run_object_loop_audio_tu15(impl_->bank,0x18ee50u,abi,fp,
        impl_->services(),{},&views);
}
void PhysicsLiveWorldTu15::observe_first_person_phase(SourceFirstPersonPhaseObserverTu15 observer) {Impl::PublicCall active(*impl_);
    impl_->first_person_phase_observer=std::move(observer);
}
void PhysicsLiveWorldTu15::set_unit_controller_active(std::uint32_t unit,bool requested){Impl::PublicCall active(*impl_);
    const auto body=impl_->object_body(unit);
    if((word(impl_->bank,body+0xaau)&255u)>1u)throw FormatError("TU1.5 controller admission requires a live unit body");
    const std::array args{unit,requested?1u:0u};impl_->call_actual(0xce440u,args);
}
void PhysicsLiveWorldTu15::deliver_unit_input(std::uint32_t unit,Vec3 throttle,Vec3 facing){Impl::PublicCall active(*impl_);
    const auto body=impl_->object_body(unit);
    if((word(impl_->bank,body+0xaau)&255u)>1u)throw FormatError("TU1.5 unit input requires a live unit body");
    // Caller-owned synchronous input storage in the existing native ABI bank.
    constexpr std::uint32_t packet=0x6005b100u;
    auto a=impl_->frame({});a.gp[2]=packet;impl_->run_actual(0xc93d0u,a);
    impl_->bank.write(packet+0x14u,std::as_bytes(std::span(&throttle,1)));
    for(auto offset:{0x28u,0x34u,0x40u})impl_->bank.write(packet+offset,std::as_bytes(std::span(&facing,1)));
    a=impl_->frame({});a.gp[0]=unit;a.gp[1]=packet;impl_->run_actual(0xc92c0u,a);
}
void PhysicsLiveWorldTu15::step_objects(SourceEngineFrameOwnerTu15 owner){Impl::PublicCall active(*impl_);impl_->step_objects(owner);}
std::vector<SourceTeleporterActivationTu15> PhysicsLiveWorldTu15::step_teleporters(
    std::vector<SourceHudMessageEvent> *notices){
    Impl::PublicCall active(*impl_);
    if(notices)notices->clear();
    auto events=impl_->capture_teleporters([&] {
        const auto engine=word(impl_->bank,0x4f63b8u),game=word(impl_->bank,0x4f3218u);
        if(word(impl_->bank,0x56dfa0u+word(impl_->bank,engine+0xc14u)*4u) &&
           std::to_integer<unsigned>(impl_->bank.view(game+0xcu,1)[0])!=4)
            impl_->call_actual(0x1a6830u);
        return 0u;
    },notices!=nullptr);
    if(notices)*notices=std::move(events.notices);
    return std::move(events.activations);
}
void PhysicsLiveWorldTu15::observe_multiplayer_teleporters(
    std::function<void(const SourceTeleporterEventsTu15 &)> observer) {
    Impl::PublicCall active(*impl_);impl_->multiplayer_teleporter_observer=std::move(observer);
}
void PhysicsLiveWorldTu15::reorient_player_look(std::uint32_t player,std::uint32_t unit){
    Impl::PublicCall active(*impl_);impl_->reorient_player_look(player,unit);
}
std::array<float,2> PhysicsLiveWorldTu15::hud_message_timing()const {
    Impl::PublicCall active(*impl_);
    const auto hud=word(impl_->bank,0x51df94u);
    if(!hud)throw FormatError("Native HUD messages require the original selected HUD globals");
    const std::array result{std::bit_cast<float>(word(impl_->bank,hud+0x58u)),
        std::bit_cast<float>(word(impl_->bank,hud+0x5cu))};
    for(auto value:result)if(!std::isfinite(value))throw FormatError("Nonfinite authored HUD message timing");
    return result;
}
void PhysicsLiveWorldTu15::observe_gameplay_fade(std::function<void(const SourceGameplayFadeFrameTu15&)> phase,
    std::function<void(const SourceGameplayFadeActivationTu15&)> activation) {
    Impl::PublicCall active(*impl_);impl_->gameplay_fade_observer=std::move(phase);
    impl_->gameplay_fade_activation_observer=std::move(activation);
}
void PhysicsLiveWorldTu15::observe_player_status(std::function<void(const SourcePlayerStatusPhaseTu15&)> observer) {
    Impl::PublicCall active(*impl_);impl_->player_status_observer=std::move(observer);
}
void PhysicsLiveWorldTu15::observe_player_scoreboard(std::function<void(const SourcePlayerScoreboardEventTu15&)> observer) {
    Impl::PublicCall active(*impl_);impl_->player_scoreboard_observer=std::move(observer);
}
void PhysicsLiveWorldTu15::advance_effect_particles(float seconds,const SourceFirstPersonPhaseObserverTu15& observer){
    SourceExecutionCallScope journal(&impl_->bank,SourceExecutionCallKind::EffectParticles,[&]{return source_execution_arguments(seconds);});
    Impl::PublicCall active(*impl_);impl_->advance_effect_particles(seconds,observer);
}
std::vector<SourceProjectileDetonationTu15> PhysicsLiveWorldTu15::take_projectile_detonations() {Impl::PublicCall active(*impl_);
    auto result=std::move(impl_->projectile_detonations);impl_->projectile_detonations.clear();return result;
}
void PhysicsLiveWorldTu15::observe_physics_stores(SourcePhysicsStoreObserverTu15 observer){Impl::PublicCall active(*impl_);
  impl_->physics_store_observer_read_only=false;
  if(!observer) {impl_->physics_store_observer={};return;}
  impl_->physics_store_observer=[owner=impl_.get(),observer=std::move(observer)](const auto &store){observer(owner->last_instruction,store);};
}
void PhysicsLiveWorldTu15::observe_physics_stores_read_only(SourcePhysicsStoreObserverTu15 observer){Impl::PublicCall active(*impl_);
  impl_->physics_store_observer_read_only=true;
  if(!observer){impl_->physics_store_observer={};return;}
  impl_->physics_store_observer=[owner=impl_.get(),observer=std::move(observer)](const auto &store){observer(owner->last_instruction,store);};
}
void PhysicsLiveWorldTu15::observe_body_velocity_inputs(SourceBodyVelocityInputObserverTu15 observer){Impl::PublicCall active(*impl_);impl_->body_velocity_input_observer=std::move(observer);}
void PhysicsLiveWorldTu15::observe_phantom_phase(SourcePhantomPhaseObserverTu15 observer){Impl::PublicCall active(*impl_);impl_->phantom_phase_observer=std::move(observer);}
void PhysicsLiveWorldTu15::observe_body_impulses(std::function<void(const SourceBodyImpulseTu15 &)> observer) {Impl::PublicCall active(*impl_);
    impl_->body_impulse_observer=std::move(observer);
}
void PhysicsLiveWorldTu15::publish_body_impulses(std::function<void(const SourceBodyImpulseTu15 &)> publisher) {Impl::PublicCall active(*impl_);
    impl_->body_impulse_publisher=std::move(publisher);
}
void PhysicsLiveWorldTu15::observe_effect_damage(SourceEffectDamageObserverTu15 observer) {Impl::PublicCall active(*impl_);
    impl_->effect_damage_observer=std::move(observer);
}
void PhysicsLiveWorldTu15::set_hitscan_rewind_owner(SourceHitscanRewindOwnerTu15 *owner) {
    // Changes no bank byte, so it does not revoke an execution transcript.
    Impl::PublicCall active(*impl_,false);impl_->require_executor();
    if(impl_->hitscan_projectile!=0xffffffffu)throw FormatError("TU1.5 hitscan owner changed inside an instantaneous projectile move");
    impl_->hitscan_owner=owner;
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
DeathCameraPacket PhysicsLiveWorldTu15::player_camera_packet(std::uint32_t player,float seconds,
    SourceCameraControllerState &controller,SourceObserverState &observer,std::uint32_t &random,
    bool cycle,std::uint8_t *mode) {
    SourceExecutionCallScope journal(&impl_->bank,SourceExecutionCallKind::PlayerCamera,[&]{
        return source_execution_arguments(SourcePlayerCameraCall{player,seconds,controller,observer,random,cycle,mode!=nullptr});});
    Impl::PublicCall active(*impl_);
    auto packet=impl_->player_camera_packet(player,seconds,controller,observer,random,cycle,mode);
    journal.complete([&]{return source_execution_camera_result(packet,controller,observer,random,
        mode?std::optional<std::uint8_t>{*mode}:std::nullopt);});return packet;
}
DeathCameraPacket PhysicsLiveWorldTu15::unit_camera_packet(std::uint32_t unit,float seconds,
    SourceCameraControllerState &controller,SourceObserverState &observer,std::uint8_t *camera_mode) {Impl::PublicCall active(*impl_);
    return impl_->unit_camera_packet(unit,seconds,controller,observer,camera_mode);
}
std::uint32_t PhysicsLiveWorldTu15::source_object_body(std::uint32_t d)const{Impl::PublicCall active(*impl_,false);return impl_->object_body(d);}
NativeRuntimeAddress PhysicsLiveWorldTu15::object_body(std::uint32_t d)const{Impl::PublicCall active(*impl_,false);return impl_->bank.native_address(source_object_body(d),4);}
std::uint32_t PhysicsLiveWorldTu15::create_biped(std::uint32_t d,const Vec3&p,const Vec3&f,std::uint32_t player){Impl::PublicCall active(*impl_);return impl_->create_biped(d,p,f,player);}
std::uint32_t PhysicsLiveWorldTu15::equip_weapon(std::uint32_t u,std::uint32_t d){Impl::PublicCall active(*impl_);return impl_->equip_weapon(u,d);}
std::uint32_t PhysicsLiveWorldTu15::create_scenario_object(const ScenarioObject&p,SourceScenarioBirth birth){Impl::PublicCall active(*impl_);return impl_->create_scenario_object(p,birth);}
std::uint32_t PhysicsLiveWorldTu15::create_netgame_object(unsigned r){Impl::PublicCall active(*impl_);return impl_->create_netgame_object(r);}
std::uint32_t PhysicsLiveWorldTu15::netgame_object(unsigned r)const{Impl::PublicCall active(*impl_,false);return impl_->netgame_object(r);}
std::uint32_t PhysicsLiveWorldTu15::child_object(std::uint32_t p,std::uint32_t d)const{Impl::PublicCall active(*impl_,false);return impl_->child_object(p,d);}
std::uint32_t PhysicsLiveWorldTu15::model_child_object(std::uint32_t p,std::uint32_t d,std::uint32_t pm,std::uint32_t cm)const{Impl::PublicCall active(*impl_,false);return impl_->model_child_object(p,d,pm,cm);}
void PhysicsLiveWorldTu15::destroy_object(std::uint32_t object){Impl::PublicCall active(*impl_);impl_->destroy_object(object);}
void PhysicsLiveWorldTu15::configure_session(const SourceGameVariant&v,std::uint16_t m,std::int16_t r,std::uint32_t s){Impl::PublicCall active(*impl_);impl_->configure_session(v,m,r,s);}
void PhysicsLiveWorldTu15::configure_campaign_session(std::int16_t d,std::uint16_t m,std::int16_t r,std::uint32_t s){Impl::PublicCall active(*impl_);impl_->configure_session(source_game_variant_default(0),m,r,s,1u,d);}
std::optional<std::string> PhysicsLiveWorldTu15::next_campaign_scenario()const {
    Impl::PublicCall active(*impl_,false);
    const auto game=word(impl_->bank,0x4f3218u);
    if(!game || word(impl_->bank,game+8u)!=1u)return {};
    const auto campaign=std::bit_cast<std::int32_t>(word(impl_->bank,game+0x1cu));
    const auto current=std::bit_cast<std::int32_t>(word(impl_->bank,game+0x20u));
    if(campaign==-1 || current==-1)return {};
    const auto next=native_campaign_next_level_key_tu15(impl_->bank,campaign,current);
    if(next==-1)return {};
    const auto record=native_campaign_level_path_record_tu15(impl_->bank,campaign,next);
    if(!record)return {};
    std::string path;
    for(unsigned n=0;n<256u;++n){
        std::array<std::byte,1>value{};impl_->bank.read(record+8u+n,value);
        const auto character=std::to_integer<unsigned char>(value[0]);
        if(!character)return path;
        path.push_back(char(character));
    }
    throw FormatError("Native next campaign scenario has no bounded path terminator");
}
void PhysicsLiveWorldTu15::place_ai(std::uint32_t r,std::optional<std::int16_t> n){Impl::PublicCall active(*impl_);impl_->place_ai(r,n);}
std::uint32_t PhysicsLiveWorldTu15::ai_object(std::uint32_t r){Impl::PublicCall active(*impl_);return impl_->ai_object(r);}
void PhysicsLiveWorldTu15::set_object_position(std::uint32_t o,Vec3 p){Impl::PublicCall active(*impl_);impl_->set_object_position(o,p);}
void PhysicsLiveWorldTu15::set_kill_volume_enabled(std::int16_t t,bool e){Impl::PublicCall active(*impl_);impl_->set_kill_volume_enabled(t,e);}
bool PhysicsLiveWorldTu15::object_in_kill_volume(std::uint32_t o){Impl::PublicCall active(*impl_);return impl_->object_in_kill_volume(o);}
bool PhysicsLiveWorldTu15::trigger_contains(std::size_t t,Vec3 p){Impl::PublicCall active(*impl_);return impl_->trigger_contains(t,p);}
SourcePhysicsWorldRuntimeAbi PhysicsLiveWorldTu15::step_physics(const SourcePhysicsContactRecordObserver&o) {Impl::PublicCall active(*impl_);
    if(!impl_->session_configured || world().is_null())
        throw FormatError("TU1.5 physics frame requires its constructed world and admitted session");
    auto a=impl_->frame({});
    impl_->run(0x1c7660u,a,o);
    return a;
}
PhysicsLiveWorldTu15::AuthorityCheckpoint PhysicsLiveWorldTu15::authority_checkpoint() const {
    std::unique_lock gate(impl_->call_gate,std::try_to_lock);impl_->require_executor();
    if(!gate.owns_lock() || impl_->public_calls || impl_->execution_depth || impl_->continuation_fault || source_execution_inputs(impl_->bank) || !impl_->session_configured ||
        !word(impl_->bank,0x52be48u))throw FormatError("TU1.5 authority export requires a completed configured world");
    if(impl_->native_input_initialized)
        throw FormatError("A local physical-input world cannot export hosting authority");
    impl_->dsp_section->require_quiescent();
    impl_->require_sound_quiescent();
    AuthorityCheckpoint saved;
    saved.resources=impl_->resources.identity();saved.bsp=impl_->bsp.index;
    saved.arena=impl_->arena->ownership_checkpoint();saved.locks=impl_->locks.ownership_checkpoint(impl_->bank);
    saved.virtual_memory=impl_->virtual_memory.ownership_checkpoint();saved.pool_storage=impl_->pool_storage.ownership_checkpoint();
    saved.events=impl_->events.checkpoint();
    if(impl_->cpu_bitmap_projection && impl_->render_read_borrows)
        throw FormatError("MCC CPU bitmap export requires an unborrowed completed world");
    saved.bank=impl_->cpu_bitmap_projection?impl_->cpu_bitmap_projection->checkpoint_without_resources():impl_->bank.checkpoint();
    {
        std::lock_guard lock(impl_->mutex);
        const auto main=impl_->thread_ids.find(impl_->main_thread);
        if(main==impl_->thread_ids.end())throw FormatError("TU1.5 authority lacks its retained executor token");
        saved.main_thread_token=main->second;saved.next_thread_token=impl_->next_thread_token;
        saved.lock_events=impl_->lock_events;
    }
    saved.last_instruction=impl_->last_instruction;saved.delivered_players=impl_->delivered_players;
    saved.model_attachments=impl_->current_model_attachments();
    saved.allocations=impl_->allocations;saved.resource_reads=impl_->resource_reads;
    saved.next_resource_read=impl_->next_resource_read;saved.model_collectors=impl_->model_collectors;
    saved.next_model_collector=impl_->next_model_collector;saved.model_scratch_mounted=impl_->model_scratch_mounted;
    saved.session_configured=impl_->session_configured;saved.exit_callbacks=impl_->native_exits.callbacks;
    saved.projectile_detonations=impl_->projectile_detonations;saved.unit_damage_effects=impl_->unit_damage_effects;
    saved.melee_impacts=impl_->melee_impacts;
    saved.logical_milliseconds=impl_->clocks->milliseconds()+impl_->tick_adjustment;
    saved.logical_cycles=impl_->clocks->cycles()+impl_->cycle_adjustment;
    saved.logical_utc_filetime=impl_->clocks->utc_filetime()+impl_->utc_adjustment;
    return saved;
}
void PhysicsLiveWorldTu15::require_execution_boundary() const {
    std::unique_lock gate(impl_->call_gate,std::try_to_lock);impl_->require_executor();
    if(!gate.owns_lock() || impl_->public_calls || impl_->execution_depth || impl_->continuation_fault ||
        source_execution_inputs(impl_->bank) || !impl_->session_configured || !word(impl_->bank,0x52be48u))
        throw FormatError("TU1.5 execution clocks require a completed unbound world");
    impl_->dsp_section->require_quiescent();impl_->require_sound_quiescent();
}
SourceExecutionClocks PhysicsLiveWorldTu15::execution_clocks() const {
    std::unique_lock gate(impl_->call_gate,std::try_to_lock);
    if(!gate.owns_lock())throw FormatError("TU1.5 execution clock owner is busy");
    require_execution_boundary();
    return {std::uint32_t(impl_->clocks->milliseconds()+impl_->tick_adjustment),
        impl_->clocks->cycles()+impl_->cycle_adjustment,impl_->clocks->utc_filetime()+impl_->utc_adjustment};
}
void PhysicsLiveWorldTu15::anchor_execution_clocks(const SourceExecutionClocks &clocks) {
    std::unique_lock gate(impl_->call_gate,std::try_to_lock);
    if(!gate.owns_lock())throw FormatError("TU1.5 clock anchor owner is busy");
    require_execution_boundary();
    const auto milliseconds=clocks.milliseconds-impl_->clocks->milliseconds();
    const auto cycles=clocks.cycles-impl_->clocks->cycles();
    const auto utc=clocks.utc_filetime-impl_->clocks->utc_filetime();
    impl_->tick_adjustment=milliseconds;impl_->cycle_adjustment=cycles;impl_->utc_adjustment=utc;
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
    std::shared_ptr<MccSoundTimingMemoryOwner> prepared_sound_timing;
    std::optional<SourceRuntimeCacheBspImage> campaign_bsp;
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
        if(owner.cpu_bitmap_projection && owner.cpu_bitmap_projection->has_resources() &&
            owner.render_read_borrows)
            throw FormatError("MCC CPU bitmap restore requires an unborrowed source call");
        if(owner.sound_timing_plan)prepared_sound_timing=std::make_shared<MccSoundTimingMemoryOwner>(owner.bank_storage);
        if(!gate.owns_lock() || owner.public_calls || owner.execution_depth || saved.version!=3 ||
            !saved.session_configured || saved.resources!=owner.resources.identity())
            throw FormatError("TU1.5 authority image/resource/BSP or barrier mismatch");
        if(owner.native_input_initialized)
            throw FormatError("A physical-input world cannot replace its local device/profile history with a hosting snapshot");
        if(saved.bsp!=owner.bsp.index) {
            if(word(owner.bank,word(owner.bank,0x4f3218u)+8u)!=1u)
                throw FormatError("Multiplayer authority cannot change its admitted BSP");
            campaign_bsp=owner.cache_bsp_image(saved.bsp);
        }
        // Require every separately declared writable environment owner before
        // restoring bytes. Old/partial captures must fail here rather than
        // losing required cold storage and faulting in a later producer.
        // Offline-XBE worlds retain their existing whole-section topology.
        bool native_environment_owner{};
        owner.bank.visit_committed([&](auto address,auto bytes) {
            if(address==0x4c6710u && bytes.size()==0x120u)native_environment_owner=true;
        });
        if(native_environment_owner) {
            struct Required {std::uint32_t address;std::size_t bytes;const char *name;};
            for(const auto &required:std::array{
                Required{0x4f2d88u,0x10u,"native TU1.5 cold environment flags and current view position"},
                Required{0x4f2d98u,0x4u,"native TU1.5 cold environment membership control"},
                Required{0x4f3008u,0x8u,"native TU1.5 cold environment interpolation scalars"},
                Required{0x556554u,0x4u,"native TU1.5 cold CRT directional math scalar destination"},
                Required{0x556e10u,0x4u,"native TU1.5 cold CRT fog spread scalar destination"},
                Required{0x4c6710u,0x120u,"native TU1.5 cold complete main environment publication destination"},
            }) {
                const auto incoming=std::ranges::find(saved.bank.regions,required.address,
                    &SourceRuntimeMemory::CheckpointRegion::address);
                if(incoming==saved.bank.regions.end() || incoming->bytes.size()!=required.bytes ||
                    incoming->access!=SourceRuntimeMemory::Access::ReadWrite ||
                    incoming->owner!=required.name || incoming->page_bytes || !incoming->committed.empty())
                    throw FormatError("TU1.5 authority lacks its required complete environment owner");
            }
        }
        owner.dsp_section->require_quiescent();
        owner.require_sound_quiescent();
        barrier.emplace(owner);
        if(owner.canonical_cache) {
            // The cache is an exact separate logical allocation backed by
            // the physical arena's real pages. Validate its topology here;
            // arena preparation below also admits its page protections.
            const auto base=owner.canonical_cache->index().region.address;
            const auto native=std::ranges::find(saved.bank.regions,base,
                &SourceRuntimeMemory::CheckpointRegion::address);
            if(native==saved.bank.regions.end() ||
                native->bytes.size()!=owner.canonical_cache->reservation_bytes() ||
                native->owner!="actual host physical allocation pages" ||
                native->access!=SourceRuntimeMemory::Access::ReadWrite ||
                native->page_bytes || !native->committed.empty())
                throw FormatError("MCC authority changed its admitted native cache allocation");
        }
        native_tu15_validate_type_runtime_restore(owner.bank,saved.bank);
        replacement.restore(saved.bank);
        if(owner.canonical_cache) {
            const auto& canonical=*owner.canonical_cache;
            const auto& index=canonical.index();
            const auto& selected=campaign_bsp?*campaign_bsp:owner.bsp;
            const auto prefix=std::size_t(index.table-index.region.address);
            if(!std::ranges::equal(replacement.view(index.region.address,prefix),
                std::span(index.region.bytes).first(prefix)))
                throw FormatError("MCC authority changed its vanilla index or group directory");
            const Bytes admitted(index.region.bytes);
            const auto selected_tag=[&](std::uint32_t datum) {
                return admitted.u32(prefix+std::size_t(datum&65535u)*16u+8u);
            };
            for(const auto& [address,expected]:std::array{
                std::pair{0x55658cu,index.region.address},std::pair{0x556d90u,index.region.address},
                std::pair{0x4f0414u,index.table},std::pair{0x474c20u,index.scenario},
                std::pair{0x4ecc20u,selected_tag(index.scenario)},
                std::pair{0x4ecc1cu,selected_tag(index.globals)}})
                if(word(replacement,address)!=expected)
                    throw FormatError("MCC authority changed its vanilla tag publication bindings");
            for(std::size_t n=0;n<index.count;++n) {
                const auto at=prefix+n*16u;
                const auto row=index.table+std::uint32_t(n)*16u;
                if(word(replacement,row)!=admitted.u32(at) ||
                    word(replacement,row+4u)!=admitted.u32(at+4u) ||
                    word(replacement,row+12u)!=admitted.u32(at+12u))
                    throw FormatError("MCC authority changed an admitted tag identity or extent");
                const auto address=word(replacement,row+8u),group=admitted.u32(at);
                if(group==tag_groups::scenario_structure_bsp.value ||
                    group==tag_groups::scenario_structure_lightmap.value) {
                    const auto* tag=owner.cache_owner.find_tag({admitted.u32(at+4u)});
                    if(!tag || (address && address!=canonical.runtime_address(tag->address,tag->size)))
                        throw FormatError("MCC authority changed a BSP tag publication address");
                } else if(address!=admitted.u32(at+8u))
                    throw FormatError("MCC authority changed an admitted tag payload address");
            }
            //1255C0 legitimately publishes the selected BSP/lightmap row's
            // address while leaving its zero directory extent unchanged.
            // Validate those publications separately from mutable payloads.
            const Bytes header(selected.region.bytes);
            const auto bsp_row=index.table+(selected.bsp&65535u)*16u;
            if(word(replacement,0x556d94u)!=selected.region.address ||
                word(replacement,0x474c24u,2)!=saved.bsp ||
                word(replacement,bsp_row+8u)!=header.u32(4) ||
                word(replacement,0x4ecc18u)!=header.u32(4) ||
                word(replacement,0x4ecc14u)!=(selected.lightmap==0xffffffffu?0u:header.u32(8)) ||
                (selected.lightmap!=0xffffffffu &&
                    word(replacement,index.table+(selected.lightmap&65535u)*16u+8u)!=header.u32(8)) ||
                !std::ranges::equal(replacement.view(selected.region.address,16),
                    std::span(selected.region.bytes).first(16)) ||
                !std::ranges::equal(replacement.view(selected.mapping,0x44),selected.mapping_bytes))
                throw FormatError("MCC authority changed its admitted selected BSP bindings");
        }
        if(campaign_bsp && (word(replacement,word(replacement,0x4f3218u)+8u)!=1u ||
            word(replacement,0x474c24u,2)!=saved.bsp ||
            !std::ranges::equal(replacement.view(campaign_bsp->mapping,0x44),campaign_bsp->mapping_bytes)))
            throw FormatError("Campaign checkpoint BSP binding differs from its admitted cache");
        // Immutable declared operands (or the explicit offline reference
        // image) must remain exactly those of the locally admitted owner.
        if(owner.cpu_bitmap_projection?
            !owner.cpu_bitmap_projection->matches_read_only_checkpoint_for_restore(saved.bank):
            !owner.bank.matches_read_only_checkpoint(saved.bank))
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
        for(auto callback:saved.exit_callbacks)if(callback!=0x383970u && callback!=0x383c90u &&
            callback!=0x383bf0u && callback!=0x383c00u && callback!=0x383c10u && callback!=0x383b30u)
            throw FormatError("TU1.5 authority exit callback is not an admitted source owner");
        // Original _cinit slot 46D768 calls 383130/30BAE0 once. Its static
        // contact agent and registered destructor must survive world handoff.
        if(std::ranges::count(saved.exit_callbacks,0x383b30u)!=1 ||
            word(replacement,0x48cdc4u)!=0x41d638u)
            throw FormatError("TU1.5 authority static contact-agent initializer binding differs");
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
        virtual_memory.emplace(owner.virtual_memory.prepare_ownership(saved.virtual_memory,saved.bank,replacement));
        pool_storage.emplace(owner.pool_storage.prepare_ownership(saved.pool_storage,saved.bank));
        arena.emplace(owner.arena->prepare_ownership(saved.arena,saved.bank,replacement));
        locks.emplace(owner.locks.prepare_ownership(owner.bank,saved.locks,saved.bank));
        events.emplace(owner.events.prepare_restore(saved.events));
        thread_ids.emplace(owner.main_thread,saved.main_thread_token);
        fields.last_instruction=saved.last_instruction;fields.delivered_players=saved.delivered_players;
        fields.main_thread_token=saved.main_thread_token;fields.next_thread_token=saved.next_thread_token;
        fields.logical_milliseconds=saved.logical_milliseconds;fields.logical_cycles=saved.logical_cycles;
        fields.logical_utc_filetime=saved.logical_utc_filetime;
        fields.lock_events=saved.lock_events;fields.allocations=saved.allocations;fields.resource_reads=saved.resource_reads;
        fields.next_resource_read=saved.next_resource_read;fields.next_model_collector=saved.next_model_collector;
        const auto object_pool=word(replacement,0x4ecbd0u);
        const SourceRawDatumPoolServices attachment_storage{.read=[&](auto at,auto bytes){return replacement.view(at,bytes);}};
        if(saved.model_attachments.size()>word(replacement,object_pool+0x20u))
            throw FormatError("TU1.5 model attachment owners exceed the original object pool");
        for(const auto &[child,event]:saved.model_attachments) {
            const auto header=source_raw_datum_lookup(object_pool,child,attachment_storage);
            if(child!=event.child || event.caller_return!=0xc08a6u || !header ||
                !source_raw_datum_lookup(object_pool,event.parent,attachment_storage))
                throw FormatError("TU1.5 model attachment owner lost its original full generation/caller");
            const auto body=word(replacement,header+8u);
            if(!body || word(replacement,body+0x14u)!=event.parent || !(word(replacement,body+4u)&0x04000000u))
                throw FormatError("TU1.5 model attachment owner differs from its actual source child link");
        }
        fields.model_attachments=saved.model_attachments;
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
SourcePhysicsWorldRuntimeAbi PhysicsLiveWorldTu15::PreparedAuthority::replacement_caller() const {
    if(!impl_ || !impl_->gate.owns_lock() || !impl_->barrier ||
        std::this_thread::get_id()!=impl_->owner.main_thread || !impl_->owner.staging ||
        impl_->owner.public_calls || impl_->owner.execution_depth || impl_->owner.nested_caller_frame)
        throw FormatError("TU1.5 staged caller requires its held quiescent native executor");
    // The same executor-owned ABI as frame(), with no return/argument stores,
    // execution, or source address taken from the incoming archive.
    return impl_->owner.caller_abi();
}
void PhysicsLiveWorldTu15::PreparedAuthority::retain_live_campaign_lens() {
    if(!impl_ || !impl_->gate.owns_lock() || !impl_->barrier ||
        std::this_thread::get_id()!=impl_->owner.main_thread || !impl_->owner.staging ||
        impl_->owner.public_calls || impl_->owner.execution_depth || impl_->owner.continuation_fault)
        throw FormatError("Live campaign lens adoption requires its held authority preparation barrier");
    const auto campaign=[](const SourceRuntimeMemory &memory) {
        const auto game=word(memory,0x4f3218u);return game && word(memory,game+8u)==1u;
    };
    if(!campaign(impl_->owner.bank) || !campaign(impl_->replacement))
        throw FormatError("Live lens renderer adoption cannot alter a multiplayer authority import");
    source_stage_live_campaign_lens_tu15(impl_->owner.bank,impl_->replacement);
}
PhysicsLiveWorldTu15::PreparedAuthority PhysicsLiveWorldTu15::prepare_authority(const AuthorityCheckpoint &saved) {
    PreparedAuthority prepared;prepared.impl_=std::make_unique<PreparedAuthority::Impl>(*impl_,saved);return prepared;
}
void PhysicsLiveWorldTu15::PreparedAuthority::retain_live_multiplayer_render_history() {
    if(!impl_ || !impl_->gate.owns_lock() || !impl_->barrier ||
       std::this_thread::get_id()!=impl_->owner.main_thread || !impl_->owner.staging ||
       impl_->owner.public_calls || impl_->owner.execution_depth || impl_->owner.continuation_fault)
        throw FormatError("Native multiplayer render retention requires its quiescent preparation barrier");
    source_stage_multiplayer_render_history_tu15(impl_->owner.bank,impl_->replacement);
}
void PhysicsLiveWorldTu15::PreparedAuthority::retain_live_multiplayer_render_history_from(
    const PhysicsLiveWorldTu15 &source) {
    if(!impl_ || !impl_->gate.owns_lock() || !impl_->barrier ||
       &impl_->owner==source.impl_.get() || std::this_thread::get_id()!=impl_->owner.main_thread ||
       !impl_->owner.staging || impl_->owner.public_calls || impl_->owner.execution_depth ||
       impl_->owner.continuation_fault)
        throw FormatError("Native presentation history needs its held distinct preparation owner");
    std::unique_lock gate(source.impl_->call_gate,std::try_to_lock);
    source.require_presentation_boundary();
    if(!gate.owns_lock() || source.impl_->resources.identity()!=impl_->owner.resources.identity() ||
       source.impl_->bsp.index!=impl_->owner.bsp.index)
        throw FormatError("Native presentation history changed its admitted resource or BSP owner");
    source_stage_multiplayer_render_history_tu15(source.impl_->bank,impl_->replacement);
}
void PhysicsLiveWorldTu15::require_presentation_boundary() const {
    std::unique_lock gate(impl_->call_gate,std::try_to_lock);impl_->require_executor();
    if(!gate.owns_lock() || impl_->public_calls || impl_->execution_depth || impl_->render_read_borrows ||
       impl_->nested_caller_frame || impl_->staging || impl_->continuation_fault ||
       impl_->publication_held.load(std::memory_order_acquire))
        throw FormatError("Native presented inputs require a completed foreground render boundary");
}
std::uint64_t PhysicsLiveWorldTu15::authority_backing_revision() const {
    std::lock_guard gate(impl_->call_gate);impl_->require_executor();
    return impl_->authority_backing_revision;
}
std::weak_ptr<const void> PhysicsLiveWorldTu15::authority_backing_owner() const {
    std::lock_guard gate(impl_->call_gate);impl_->require_executor();
    return impl_->authority_backing_owner;
}
bool PhysicsLiveWorldTu15::commit_authority(PreparedAuthority &&prepared) noexcept {
    if(!prepared.for_owner(*this) || std::this_thread::get_id()!=impl_->main_thread)return false;
    if(impl_->publication_held.load(std::memory_order_acquire) ||
       impl_->authority_backing_revision==std::numeric_limits<std::uint64_t>::max())return false;
    if(impl_->dsp_section->active_references() || !impl_->sound_sdk_resources->quiescent() ||
       impl_->sound_preference || impl_->sound_dma || impl_->sound_reset)return false;
    auto &state=*prepared.impl_;auto &fields=state.fields;
    if(!state.virtual_memory->matches_memory(state.replacement) ||
       !state.arena->matches_memory(state.replacement))return false;
    // Native clock domains are local inputs. Preserve their observed logical
    // values at the handoff; source game clocks/RNG remain in imported bytes.
    const auto tick_adjustment=fields.logical_milliseconds-impl_->clocks->milliseconds();
    const auto cycle_adjustment=fields.logical_cycles-impl_->clocks->cycles();
    const auto utc_adjustment=fields.logical_utc_filetime-impl_->clocks->utc_filetime();
    // Denied topology admission must leave both current owners and receipts intact.
    // Checked swap has no throwing operations after both permission gates pass.
    try{impl_->bank.swap_backing(state.replacement);}catch(...){return false;}
    ++impl_->authority_backing_revision;
    // A successful real bank swap stales only the outgoing derived owner.
    // Keep it through every failed/cancelled prepare and denied swap.
    impl_->cpu_bitmap_projection.reset();
    if(impl_->sound_timing_plan)impl_->sound_timing_memory.swap(state.prepared_sound_timing);
    impl_->main_receipts.retire();
    if(state.campaign_bsp)std::swap(impl_->bsp,*state.campaign_bsp);
    impl_->virtual_memory.commit_ownership(std::move(*state.virtual_memory));
    impl_->pool_storage.commit_ownership(std::move(*state.pool_storage));
    impl_->arena->commit_ownership(std::move(*state.arena));
    impl_->locks.commit_ownership(std::move(*state.locks));impl_->events.commit_restore(std::move(*state.events));
    impl_->thread_ids.swap(state.thread_ids);impl_->next_thread_token=fields.next_thread_token;
    impl_->lock_events.swap(fields.lock_events);impl_->allocations.swap(fields.allocations);
    impl_->resource_reads.swap(fields.resource_reads);impl_->next_resource_read=fields.next_resource_read;
    impl_->model_attachments.swap(fields.model_attachments);
    impl_->model_collectors.swap(fields.model_collectors);impl_->next_model_collector=fields.next_model_collector;
    impl_->rendered_model_nodes.clear();
    impl_->model_scratch_mounted=fields.model_scratch_mounted;impl_->session_configured=fields.session_configured;
    impl_->native_exits.callbacks.swap(fields.exit_callbacks);
    impl_->projectile_detonations.swap(fields.projectile_detonations);impl_->unit_damage_effects.swap(fields.unit_damage_effects);
    impl_->melee_impacts.swap(fields.melee_impacts);impl_->last_instruction=fields.last_instruction;
    impl_->delivered_players=fields.delivered_players;impl_->tick_adjustment=tick_adjustment;impl_->cycle_adjustment=cycle_adjustment;impl_->utc_adjustment=utc_adjustment;
    impl_->continuation_fault=false;
    prepared.impl_.reset();return true;
}
} // namespace halo2
