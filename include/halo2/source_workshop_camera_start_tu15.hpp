#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete original 2B27B0 HS wrapper. Its parser, native camera effect and
// completion remain required owners; component proof alone does not admit MCC.
bool source_workshop_camera_start_tu15_owns_entry(std::uint32_t);
void source_run_workshop_camera_start_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
}
