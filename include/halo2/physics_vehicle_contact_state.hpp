#pragma once
#include "halo2/physics_vehicle_motion_math.hpp"
namespace halo2 {
// TU1.5-only composition. Entry IDs retain the parent's internal identifiers;
// instruction/store IDs for the new bodies are TU1.5 virtual addresses.
void source_run_physics_vehicle_contact_state_runtime(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry,SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint &,const SourcePhysicsContactRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
