#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete original127E00 signed-short interpolation with the caller's RNG.
// Returns the original live x87 result; keeps its extended value and controls.
bool source_sound_random_short_tu15_owns_entry(std::uint32_t);
void source_run_sound_random_short_tu15(SourcePhysicsContactMemory &, std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &, const SourcePhysicsContactRecordObserver & = {});
}
