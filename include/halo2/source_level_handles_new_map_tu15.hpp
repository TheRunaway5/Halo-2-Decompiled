#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete original registered row13 new-map callback and its private closure.
bool source_level_handles_new_map_tu15_owns_entry(std::uint32_t);
void source_run_level_handles_new_map_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
}
