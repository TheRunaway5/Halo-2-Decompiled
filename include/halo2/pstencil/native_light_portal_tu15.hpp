#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool pstencil_native_light_portal_tu15_owns_entry(std::uint32_t);
void pstencil_run_native_light_portal_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
}
