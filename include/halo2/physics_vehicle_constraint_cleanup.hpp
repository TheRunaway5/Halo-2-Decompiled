#pragma once
#include "halo2/physics_vehicle_contact_state.hpp"
namespace halo2 {
// TU1.5-only constraint cleanup, lookup, state and force-delivery composition.
// Retained parent IDs and new TU1.5 entry IDs select authenticated native bodies.
void source_run_physics_vehicle_constraint_cleanup_runtime(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry,SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint &,const SourcePhysicsContactRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
