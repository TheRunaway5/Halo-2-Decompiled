#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Whole original3D2B0 and31B30. The live world owns all camera, model,
// animation and downstream call inputs; no pose or admission is supplied.
bool source_first_person_light_pose_tu15_owns_entry(std::uint32_t);
void source_run_first_person_light_pose_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
