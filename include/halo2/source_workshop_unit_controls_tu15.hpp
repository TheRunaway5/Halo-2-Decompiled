#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete original HS evaluators2AD5B0,2B2D80 and2B2DA0. The actual unit header
// table/player-action state and HS parser/completion remain caller owners.
// Component qualification does not admit MCC scripts or schedule their input.
bool source_workshop_unit_controls_tu15_owns_entry(std::uint32_t);
void source_run_workshop_unit_controls_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
}
