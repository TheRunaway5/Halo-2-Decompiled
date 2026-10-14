#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool source_collision_instance_storage_tu15_owns_entry(std::uint32_t);
// Complete original44B7AC startup callback and its real allocator boundary.
void source_run_collision_instance_storage_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
