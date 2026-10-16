#pragma once
#include "halo2/source_runtime_memory.hpp"
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Whole3889F1. Executes CMPXCHG on the actual aligned native backing. Caller
// must hold its actual interrupt/event domain; no snapshot or pending-bit
// replacement is supplied. Every opaque source status bit is retained.
void source_run_sound_pending_aggregate_tu15(SourceRuntimeMemory&,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
}
