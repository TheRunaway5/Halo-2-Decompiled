#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Complete original1C7660 outer game physics frame (TU1.5:1CC8D0).
// The caller owns the actual clock/context, world, object pools, ABI frame and
// CPU/platform services. Missing ownership propagates at the original call;
// this entry never constructs a captured world or substitutes a callee result.
void source_run_game_physics_frame(
    SourcePhysicsContactMemory &, SourcePhysicsWorldRuntimeAbi &,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
