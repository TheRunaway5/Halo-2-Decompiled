#pragma once
#include "halo2/physics_vehicle_update.hpp"

namespace halo2 {
// Construct original vehicle state and dispatch authenticated motion owners.
// Other callees retain their reached ABI and require caller-provided owners.
void source_run_physics_vehicle_motion_runtime(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry, SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
