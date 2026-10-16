#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete original registered static-geometry destructor callbacks and their
// shared conditional reference/array releases. External native resource owners
// remain required; no destructor, refcount or allocator result is supplied.
bool source_havok_static_lifetime_tu15_owns_entry(std::uint32_t);
void source_run_havok_static_lifetime_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
