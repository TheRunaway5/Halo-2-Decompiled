#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool source_physics_live_support_tu15_owns_entry(std::uint32_t);
// Complete unchanged TU1.5 support bodies reached by fresh live construction.
// Entry IDs are actual TU1.5 addresses. All missing owners retain full ABI.
void source_run_physics_live_support_tu15(SourcePhysicsContactMemory &,
    std::uint32_t actual_entry,SourcePhysicsWorldRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
