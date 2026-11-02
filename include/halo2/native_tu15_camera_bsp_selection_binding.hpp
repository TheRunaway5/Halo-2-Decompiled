#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool native_tu15_camera_bsp_selection_owns_entry(std::uint32_t);
// Actual2B770 call2B7E6 only: EDXpoint,EDIleaf,stackcluster; owedAL/RET4.
// Volatile registers, flags and private SIMD scratch are dead in its caller.
void native_tu15_camera_bsp_selection_run(SourcePhysicsContactMemory &,std::uint32_t entry,
    std::uint32_t caller,SourcePhysicsWorldRuntimeAbi &,const SourcePhysicsContactRecordObserver & = {});
std::uint64_t native_tu15_camera_bsp_selection_calls();
}
