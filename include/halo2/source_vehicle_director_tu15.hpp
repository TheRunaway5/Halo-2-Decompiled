#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/observer_motion.hpp"
namespace halo2 {
using SourceCameraControllerState=std::array<std::uint32_t,0x140/4>;
bool source_vehicle_director_tu15_owns_entry(std::uint32_t);
// Complete157FE0 scheduler,158450/158900 mode/transition owners, ordinary
//245400/245060 producer,246CF0/247140 death/watch producers and171FF0
// registration. Actual nested owners run.
void source_run_vehicle_director_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
