#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool source_conveyor_tu15_owns_entry(std::uint32_t);
// Complete original shader-map derivative used by184C50's physical surface
// velocity. Shared function evaluation remains in its existing original owner.
void source_run_conveyor_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
