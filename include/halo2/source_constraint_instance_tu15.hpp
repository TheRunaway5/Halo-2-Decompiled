#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete TU1.5 constraint instance construction owners.
// Original allocation, object-space transforms and math calls retain their ABI.
bool source_constraint_instance_tu15_owns_entry(std::uint32_t);
void source_run_constraint_instance_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
