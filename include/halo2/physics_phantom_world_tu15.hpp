#pragma once
#include "halo2/physics_phantom_force_tu15.hpp"
namespace halo2 {
// TU1.5-only composition. Entry and observation IDs are actual TU1.5 addresses.
// It accepts the complete retained phantom/vehicle/world graph and the TU1.5
// source-installed activation callbacks.
// All body/world/island/allocator state still requires its real producer.
void source_run_physics_phantom_world_tu15_runtime(SourcePhysicsContactMemory &,
    std::uint32_t tu15_entry,SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint &,const SourcePhysicsContactRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
