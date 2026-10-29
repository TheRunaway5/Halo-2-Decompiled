#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool native_tu15_render_camera_packet_owns_entry(std::uint32_t);
// Currently authenticated229A90 main/secondary view callers only. Retire the
// existing generated local dispatch to use its required-owner transport;
// this handler does not implement that remaining caller's CPU machinery.
void native_tu15_render_camera_packet_run(SourcePhysicsContactMemory &,std::uint32_t entry,
    std::uint32_t caller,SourcePhysicsWorldRuntimeAbi &,const SourcePhysicsContactRecordObserver & = {});
std::uint64_t native_tu15_render_camera_packet_calls();
}
