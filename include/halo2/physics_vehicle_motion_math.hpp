#pragma once
#include "halo2/physics_vehicle_motion.hpp"
namespace halo2 {
// Execute authenticated CRT remainder and vehicle solver dispatch owners.
// FP controls are in/out state, including at a reached required-owner boundary.
void source_run_physics_vehicle_motion_math_runtime(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry, SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint &,
    const SourcePhysicsContactRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
