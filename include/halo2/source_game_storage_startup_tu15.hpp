#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"

namespace halo2 {
// Actual TU1.5 13A700, including all 68 original subsystem dispatches. Every
// callee remains a required owner with its actual live caller ABI. A missing
// subsystem stops before executing it and never completes this initializer.
void source_run_game_storage_startup_tu15(SourcePhysicsContactMemory&,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,
    const SourcePhysicsContactRecordObserver& = {});
// Only the cold game-options binding and the 68 consumed initializer slots.
// Other columns of the original lifecycle table are separate data owners.
void mount_native_tu15_game_startup_data(SourceRuntimeMemory&);
}
