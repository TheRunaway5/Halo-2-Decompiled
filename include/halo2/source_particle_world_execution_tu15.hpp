#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Execute the existing original particle, collection and render owners over
// one memory/service context. Unknown platform/resource calls retain the
// caller's required-owner service; no return or graphics state is supplied.
bool source_particle_world_execution_tu15_owns_entry(std::uint32_t);
void source_run_particle_world_execution_tu15(SourcePhysicsContactMemory &,
    std::uint32_t,SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
