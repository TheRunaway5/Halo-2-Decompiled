#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete authenticated TU1.5 birth/departure/retained-remap instructions. Native
// downstream owners remain required; no game/platform result is substituted.
bool source_player_departure_tu15_owns_entry(std::uint32_t);
void source_run_player_departure_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
