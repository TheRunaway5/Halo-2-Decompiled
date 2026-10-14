#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool source_collision_query_tu15_owns_entry(std::uint32_t);
// Complete inverted-contact query, transform and query delivery. Actual
// virtual owners remain required; this module supplies no contact results.
void source_run_collision_query_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
