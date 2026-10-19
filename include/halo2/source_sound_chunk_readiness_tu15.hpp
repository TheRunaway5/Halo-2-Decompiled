#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Original128970 readiness,1293F0 source pin, and21F260 cache-state composition.
// Includes original21F3B0 cache-entry retention guard.
// Allocation21F420 and scheduling2DBF74 remain explicit required-owner calls.
bool source_sound_chunk_readiness_tu15_owns_entry(std::uint32_t);
void source_run_sound_chunk_readiness_tu15(SourcePhysicsContactMemory &, std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &, const SourcePhysicsContactRecordObserver & = {});
}
