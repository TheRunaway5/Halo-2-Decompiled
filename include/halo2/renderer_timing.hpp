#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace halo2 {
// Numeric preparation observations from the caller of this draw, on its host
// steady clock. No source state, pose, observer or other borrowed payload.
// These cover the current frame before draw; interval_ms spans two draw starts.
struct RendererHostPreparationTiming {
    struct View {double camera_lighting_ms{},static_world_ms{},objects_ms{},body_ms{},first_person_ms{},hud_ms{};};
    // Nested returned-call observations: integer accounting while source/native
    // execution is active. Conversion to milliseconds belongs to the writer.
    struct ReturnedCall {std::uint64_t nanoseconds{};unsigned calls{};};
    struct NativeView {
        ReturnedCall model_collection,main_cull,main_commands,shadow_selection,shadow_roots;
    };
    unsigned frame{},views{};
    std::uint32_t tick_begin{},tick_end{};
    double input_ms{},common_ms{},pre_render_ms{};
    std::array<View,4> view;
    ReturnedCall match_step,state_projection,post_step;
    std::uint64_t shared_pre_view_ns{};
    std::array<NativeView,4> native_view;
};
// Host elapsed times around the existing submission path. These are not GPU
// timestamps: fence, acquire, submit and present may include driver waits.
struct RendererFrameTiming {
    bool measured{};
    double fence_ms{}, global_constants_ms{}, view_constants_ms{}, geometry_ms{},
        acquire_ms{}, commands_ms{}, submit_ms{}, present_ms{}, screenshot_ms{};
    std::size_t materials{}, views{}, dynamic_vertices{};
    unsigned width{},height{};
    double geometry_pack_ms{},geometry_upload_ms{},geometry_diagnostics_ms{};
    double geometry_plan_ms{},geometry_bind_ms{};
    // Actual immutable-prefix jobs; overlap is intersection with render-owner
    // draw-state preparation, measured on the same steady clock (not GPU time).
    std::size_t presentation_copy_jobs{},presentation_copy_bytes{};
    std::uint64_t presentation_copy_ns{},presentation_copy_overlap_ns{},presentation_copy_wait_ns{};
    bool presentation_copy_separate_thread{};
    std::size_t model_skin_gpu_jobs{},model_skin_gpu_vertices{},model_skin_gpu_deformed_vertices{},model_skin_gpu_verified_vertices{};
    std::size_t model_skin_gpu_post_identity_vertices{};
    // Actual retained skin-input operations, reset for each prepare attempt.
    std::uint64_t model_skin_input_allocations{},
        model_skin_input_maps{},
        model_skin_input_unmaps{},
        model_skin_record_pack_bytes{},
        model_skin_record_upload_bytes{},
        model_skin_palette_upload_bytes{},
        model_skin_descriptor_writes{},
        model_skin_record_cache_hits{};
    // Actual native-world submission calls. Prepare/upload calls count attempts;
    // completed/nonempty calls and draws count only successful returns. Records
    // count successful command recordings, not GPU executions or visible pixels.
    std::size_t world_native_prepare_calls{},world_native_prepare_completed_calls{},
        world_native_prepare_nonempty_calls{},world_native_prepared_draws{},
        world_native_record_calls{};
    std::size_t world_native_title_prepared_draws{},
        world_native_shadow_generate_prepared_draws{},
        world_native_shadow_apply_prepared_draws{},world_native_light_prepared_draws{};
    std::size_t world_native_model_depth_recorded_draws{},world_native_model_depth_recorded_vertices{};
    std::size_t world_storage_upload_calls{},world_storage_upload_completed_calls{},
        world_storage_uploaded_draws{};
    // Inclusive host elapsed time, including failed attempts. These overlap the
    // existing renderer phases and must not be added to those phase totals.
    double world_native_prepare_ms{},world_storage_upload_ms{};
    // Completed mapped CPU jobs after a typed GPU prepare resource refusal.
    // These never count as GPU dispatches or GPU readback verification.
    std::size_t model_skin_cpu_fallback_jobs{},model_skin_cpu_fallback_vertices{},
        model_skin_cpu_fallback_deformed_vertices{},model_skin_cpu_fallback_post_identity_vertices{};
    std::size_t model_skin_cpu_fallback_quota_jobs{},model_skin_cpu_fallback_allocation_jobs{},
        model_skin_cpu_fallback_mapping_jobs{};
    std::int32_t model_skin_cpu_fallback_vulkan_result{};
};
}
