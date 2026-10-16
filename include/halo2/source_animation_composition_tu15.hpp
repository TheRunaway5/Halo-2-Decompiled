#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool source_animation_composition_tu15_owns_entry(std::uint32_t);
void source_run_animation_composition_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
}
