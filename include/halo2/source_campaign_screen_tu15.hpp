#pragma once
#include "halo2/math.hpp"
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/cinematic_crossfade.hpp"
namespace halo2 {
// Compatibility entry names dispatch the four semantic native screen kernels.
// The production graph contains no generated/lifted screen implementation.
bool source_campaign_screen_tu15_owns_entry(std::uint32_t);
void source_run_campaign_screen_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
class PhysicsLiveWorldTu15;
// Original2B5150 consumes a parsed REAL word. A null interface is a
// completed no-op; the checked writer retains the exact one-word store.
void source_campaign_far_clip_tu15(const SourceRuntimeMemory &,
    const std::function<void(std::uint32_t,std::span<const std::byte>)> &,float);
void source_campaign_far_clip_tu15(PhysicsLiveWorldTu15 &,float);
// Actual12B0B..12B46: positive interface overrides replace the configured
// defaults. The world overload publishes4922F4/F8 before environment camera
// clipping. Configured W distance remains distinct from the clamped camera.
Vec2 source_campaign_clip_planes_tu15(const SourceRuntimeMemory &);
Vec2 source_campaign_clip_planes_tu15(PhysicsLiveWorldTu15 &);

// Complete original1FC10, called by2B4F60 with the first REAL in XMM0
// and six literal words on the retained source stack. No host float cast or
// duration sanitation changes the original signed-zero/NaN/infinity stores.
void source_campaign_depth_of_field_tu15(PhysicsLiveWorldTu15 &,
    std::span<const std::uint32_t,7> arguments);
struct SourceCampaignDepthOfFieldFrameTu15 {
    bool enabled{};
    float focal_distance{},far_blur{},near_blur{};
};
// Completed1FD00 output plus actual13660 admission. target_ready represents
// the real native equivalent of1DE00(target18), never an invented success.
SourceCampaignDepthOfFieldFrameTu15 source_campaign_depth_of_field_frame_tu15(
    const SourceRuntimeMemory &,bool view_modifier,bool target_ready);
struct SourceCampaignDepthOfFieldSceneTu15 {
    std::uint32_t mode{},target{};
    float minimum_w{},maximum_w{};
};
// Ordered13430 modes1/2. The second pass clears/selects target18 in the real
// renderer; this pure recipe does not allocate, clear or draw that target.
std::array<SourceCampaignDepthOfFieldSceneTu15,2> source_campaign_depth_of_field_scenes_tu15(
    float configured_near,float configured_far,float focal_distance,std::uint32_t current_target);

// Native argument/result composition of the original HS command stores.
// These value seams do not execute the HS parser or complete an HS thread.
void source_campaign_screen_start_tu15(SourceRuntimeMemory &,bool reset);
void source_campaign_crossfade_set_tu15(SourceRuntimeMemory &,float seconds,float strength=1.f);
// Native renderer clock publication. Whole renderer/frame/SDK execution is
// separate; accepted elapsed has already passed13AA50's native value adapter.
void source_campaign_screen_clock_tu15(SourceRuntimeMemory &,double clock);
void source_campaign_screen_update_tu15(PhysicsLiveWorldTu15 &);
void source_campaign_screen_stop_tu15(PhysicsLiveWorldTu15 &);
void source_campaign_screen_capture_complete_tu15(PhysicsLiveWorldTu15 &);
CinematicCrossfadeState source_campaign_crossfade_state_tu15(const SourceRuntimeMemory &);
CinematicCrossfadeFrame source_campaign_crossfade_frame_tu15(const SourceRuntimeMemory &);
}
