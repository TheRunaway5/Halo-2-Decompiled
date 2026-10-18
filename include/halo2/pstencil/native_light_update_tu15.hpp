#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Whole native C3430, scheduled by original13AB4E after effects. Source
// allocations, attachment functions and retirement remain native owners.
bool pstencil_native_light_update_tu15_owns_entry(std::uint32_t);
void pstencil_run_native_light_update_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
