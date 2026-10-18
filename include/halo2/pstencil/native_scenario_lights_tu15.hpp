#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Whole C29F0/C2550. Caller191E0B supplies actual game-options+8 in EAX;
// scenario placement/palette, allocation and spatial registration remain native.
bool pstencil_native_scenario_lights_tu15_owns_entry(std::uint32_t);
void pstencil_run_native_scenario_lights_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
