#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete original2290C0 sound properties and its four closed CPU callees.
// Consumes actual tag, extra-info and context banks; creates no fallback records.
bool source_sound_properties_tu15_owns_entry(std::uint32_t);
void source_run_sound_properties_tu15(SourcePhysicsContactMemory &, std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &, const SourcePhysicsContactRecordObserver & = {});
}
