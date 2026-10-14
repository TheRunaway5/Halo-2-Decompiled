#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool source_physics_deactivation_tu15_owns_entry(std::uint32_t);
// Complete TU1.5 gameplay admission and rigid-body deactivation queries over
// the retained source world. Missing owners never become supplied outcomes.
void source_run_physics_deactivation_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
