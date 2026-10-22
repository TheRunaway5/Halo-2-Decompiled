#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete original324E60/324F80 virtual methods. The retained motion body,
// impulse and application point come from their actual source callers.
bool source_body_impulse_tu15_owns_entry(std::uint32_t);
void source_run_body_impulse_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
}
