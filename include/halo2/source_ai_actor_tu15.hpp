#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete TU1.5 AI storage/new-map, cold actor, binding and birth-state
// owners. They borrow the actual source bank, arena, tags and object owners.
// Unprovided squad, perception, lifecycle or frame owners fail explicitly.
// This module does not supply AI decisions or replace missing firing input.
bool source_ai_actor_tu15_owns_entry(std::uint32_t);
void source_run_ai_actor_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
