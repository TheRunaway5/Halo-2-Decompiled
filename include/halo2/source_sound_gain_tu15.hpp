#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete original127CF0 gain interpolation and its six closed CPU callees.
// Uses the caller's actual sound, class and transition banks.
bool source_sound_gain_tu15_owns_entry(std::uint32_t);
void source_run_sound_gain_tu15(SourcePhysicsContactMemory &, std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &, const SourcePhysicsContactRecordObserver & = {});
}
