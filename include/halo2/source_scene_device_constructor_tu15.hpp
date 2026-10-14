#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete TU1.5 scene-device birth/post-birth/update callbacks and reached
// support bodies. Animation/physics indirect calls retain strict real owners.
void source_run_scene_device_constructor_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
bool source_scene_device_constructor_tu15_owns_entry(std::uint32_t);
}
