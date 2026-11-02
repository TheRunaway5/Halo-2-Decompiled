#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool native_tu15_camera_view_layout_owns_entry(std::uint32_t);
// Actual229B44/229B5F/229B7D only. All three are void and integer-only;
// transport preserves opaque GP/flags/FP and consumes the real return word.
void native_tu15_camera_view_layout_run(SourcePhysicsContactMemory &,std::uint32_t entry,
    std::uint32_t caller,SourcePhysicsWorldRuntimeAbi &,const SourcePhysicsContactRecordObserver & = {});
std::uint64_t native_tu15_camera_view_layout_calls();
}
