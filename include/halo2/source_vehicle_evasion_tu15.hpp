#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete B3350, reached after the original flying owner admits a maneuver.
// Player admission and type20 simulation-event delivery retain their actual
// source owners; this producer supplies neither a maneuver nor a physics result.
bool source_vehicle_evasion_tu15_owns_entry(std::uint32_t);
void source_run_vehicle_evasion_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
}
