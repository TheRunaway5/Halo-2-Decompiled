#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Whole original166C30: signed word indices, four parallel arrays, and the
// original pointer reload/store order. The caller supplies actual owned memory.
bool source_shadow_array_swap_tu15_owns_entry(std::uint32_t);
void source_run_shadow_array_swap_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
}
