#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool source_dialogue_admission_tu15_owns_entry(std::uint32_t);
// Complete217470/217310 campaign dialogue weight and team classifier.
void source_run_dialogue_admission_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
}
