#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool native_tu15_camera_frame_policy_owns_entry(std::uint32_t);
// 2C470: actual229B9E carries EAX=view count; its continuation kills volatile
// results. 25940: actual14D67/14D9A/198A8 consume full EAX only. The258F7
// synchronous Renderer capture service is deliberately outside this binding.
void native_tu15_camera_frame_policy_run(SourcePhysicsContactMemory &,std::uint32_t entry,
    std::uint32_t caller,SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver & = {});
std::uint64_t native_tu15_camera_frame_policy_calls();
}
