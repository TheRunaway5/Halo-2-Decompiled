#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Original tag-block cache retry/eviction owners. The existing source LRU,
// resource headers, cancellation, worker and release services remain required.
bool source_resource_cache_pressure_tu15_owns_entry(std::uint32_t);
void source_run_resource_cache_pressure_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
}
