#pragma once
#include "halo2/physics_vehicle_runtime.hpp"

namespace halo2 {
// Connect the original controller to its authenticated object, velocity,
// animation-state and vehicle-update owners. All other callees retain their
// reached ABI and require actual caller-provided owners.
void source_run_physics_vehicle_update_runtime(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry, SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
