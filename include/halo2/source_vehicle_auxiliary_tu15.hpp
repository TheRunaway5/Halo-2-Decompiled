#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete TU1.5 component refresh and class3 vehicle surface-effect owners.
// All downstream physics, collision and effect calls retain their real ABI.
bool source_vehicle_auxiliary_tu15_owns_entry(std::uint32_t);
void source_run_vehicle_auxiliary_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
