#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete original127D70 parameter interpolation and original RNG update.
// Reads and writes the actual retained RNG bank; supplies no replacement seed.
bool source_sound_random_scalar_tu15_owns_entry(std::uint32_t);
void source_run_sound_random_scalar_tu15(SourcePhysicsContactMemory &, std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &, const SourcePhysicsContactRecordObserver & = {});
}
