#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool native_campaign_camera_commands_tu15_owns_entry(std::uint32_t);
// Ordinary void callers only: transport real arguments and RET word, preserve
// unrelated ABI packet fields, and publish only the owed MXCSR change.
void native_campaign_camera_commands_tu15_run(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,const SourcePhysicsContactRecordObserver & = {});
struct NativeCampaignCameraCommandActivityTu15 {std::uint64_t resets{},stops{},fovs{};};
NativeCampaignCameraCommandActivityTu15 native_campaign_camera_command_activity_tu15();
}
