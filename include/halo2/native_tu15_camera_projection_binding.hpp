#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool native_tu15_camera_projection_owns_entry(std::uint32_t);
// Production interception is limited to these authenticated public calls.
// The shared inverse-frame helper remains a typed internal dependency.
bool native_tu15_camera_projection_owns_call(std::uint32_t entry,std::uint32_t caller);
// Authenticated ordinary2FD79/2FFE9 and2FD70 callers13C9B/2BAB7/2BAFC.
// All are void, DF=0. Real hardware FP context is imported/exported once;
// an unrepresentable sparse result remains an explicit required ABI owner.
void native_tu15_camera_projection_run(SourcePhysicsContactMemory &,std::uint32_t entry,
    std::uint32_t caller,SourcePhysicsWorldRuntimeAbi &,const SourcePhysicsContactRecordObserver & = {});
std::uint64_t native_tu15_camera_projection_calls();
}
