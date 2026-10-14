#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete authenticated TU1.5 special-action owners on the retained world
// bank. External services preserve their full ABI and fail when unavailable.
bool source_sandbox_special_tu15_owns_entry(std::uint32_t);
void source_run_sandbox_special_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
