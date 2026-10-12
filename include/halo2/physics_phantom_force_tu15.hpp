#pragma once
#include "halo2/physics_vehicle_environment_state.hpp"
namespace halo2 {
// TU1.5-only complete phantom force and velocity-delivery composition.
void source_run_physics_phantom_force_tu15_runtime(SourcePhysicsContactMemory &,
    std::uint32_t entry,SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint &,const SourcePhysicsContactRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
