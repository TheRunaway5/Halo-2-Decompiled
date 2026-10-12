#pragma once
#include "halo2/physics_vehicle_constraint_cleanup.hpp"
namespace halo2 {
// TU1.5-only environment state, environment queries, pose composition and force-retirement composition.
// Retained parent IDs and new TU1.5 entry IDs select authenticated native bodies.
void source_run_physics_vehicle_environment_state_runtime(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry,SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint &,const SourcePhysicsContactRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
