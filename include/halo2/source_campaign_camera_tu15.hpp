#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/math.hpp"
#include <optional>
namespace halo2 {
bool source_campaign_camera_tu15_owns_entry(std::uint32_t);
void source_run_campaign_camera_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
class PhysicsLiveWorldTu15;
void source_campaign_camera_control_tu15(PhysicsLiveWorldTu15 &,bool active);
// Actual16F150: DX point index; stack signed-short ticks and objectNONE.
// Full source owner also performs the immediate director/observer flush.
void source_campaign_camera_point_tu15(PhysicsLiveWorldTu15 &,std::int16_t point,std::int16_t ticks);
void source_campaign_camera_animation_tu15(PhysicsLiveWorldTu15 &,
    std::uint32_t graph,std::uint32_t name,std::uint32_t object,std::int16_t flag);
void source_campaign_camera_field_of_view_tu15(PhysicsLiveWorldTu15 &,float degrees,std::int16_t ticks);
// Original ordinary frame composition, once after accepted effects and before
// camera projection. seconds is the caller's incoming frame delta, not the
// admitted game-tick count or presentation render fraction.
void source_campaign_camera_frame_tu15(PhysicsLiveWorldTu15 &,float seconds);
struct SourceCampaignCameraViewTu15 {
    Vec3 position,forward,up;
    float horizontal_field_of_view,aspect,aim_displacement;
};
// Read the completed1720E0 observer output for a real registered local user.
// No camera, interpolation, source mutation or frame update occurs here.
std::optional<SourceCampaignCameraViewTu15> source_campaign_camera_view_tu15(
    const SourceRuntimeMemory &,unsigned local_view);
std::int16_t source_campaign_camera_time_tu15(PhysicsLiveWorldTu15 &);
// Native composition of original13AB94..13ABBB, after the object/FP phases
// and before the existing source game-clock increment. No caller ABI or
// skipped surrounding game-frame owners are claimed by this value seam.
void source_campaign_camera_game_tick_tu15(SourceRuntimeMemory &);
std::int16_t source_campaign_camera_time_value_tu15(std::int32_t ticks,float tick_seconds);
}
