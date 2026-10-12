#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Complete static source paths for the fresh physics pool producers and raw
// pool lifecycle. Canonical TU1.1 entries select independently authenticated
// TU1.5 instructions/operands. Caller ABI and backing remain explicit inputs.
// 2284C0 creates both impact pools; 1D2620 creates physical components;
// 228540 disposes both impact pools. Pool validity belongs to the later world
// initializer; construction alone does not mark a pool valid or create bodies.
void source_run_physics_pool_startup_runtime(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry, SourcePhysicsWorldRuntimeAbi &,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
