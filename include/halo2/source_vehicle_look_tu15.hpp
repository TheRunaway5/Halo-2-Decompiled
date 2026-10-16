#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool source_vehicle_look_tu15_owns_entry(std::uint32_t);
// Complete original18A300: seat-relative yaw, authored pitch constraints and
// velocity-dependent automatic pitch. The original nested owners keep results.
void source_run_vehicle_look_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
