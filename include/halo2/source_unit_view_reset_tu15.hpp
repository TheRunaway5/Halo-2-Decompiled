#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete English TU1.5 unit/local-view reset, director initialization and
// spectator selector bodies. All operands and RNG storage remain source-owned.
bool source_unit_view_reset_tu15_owns_entry(std::uint32_t);
void source_run_unit_view_reset_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
}
