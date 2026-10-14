#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool source_vehicle_occupancy_tu15_owns_entry(std::uint32_t);
// Complete original ED500 occupant/team guard, B2DC0 event notifier and
// D3470 boarding ejection and154C70 player transition requests, reached by
// the original ED640 owner.
// Actual object, iterator, relationship and action owners retain their results.
void source_run_vehicle_occupancy_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
