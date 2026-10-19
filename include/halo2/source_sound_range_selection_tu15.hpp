#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete original21F960 preferred and nearest interval selection.
// Uses the actual caller banks and state; supplies no replacement input records.
bool source_sound_range_selection_tu15_owns_entry(std::uint32_t);
void source_run_sound_range_selection_tu15(SourcePhysicsContactMemory &, std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &, const SourcePhysicsContactRecordObserver & = {});
}
