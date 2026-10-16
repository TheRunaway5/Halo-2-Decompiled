#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Original startup row55 constructs recorded-animation storage, activates and
// clears it on map load, and deactivates it on map disposal. Original2039B0
// queries the active records for the supplied full object identity.
bool source_recorded_animation_tu15_owns_entry(std::uint32_t);
void source_run_recorded_animation_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
