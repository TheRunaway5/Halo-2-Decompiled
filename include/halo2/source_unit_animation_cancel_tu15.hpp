#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete116790: cancel the matching primary/secondary overlay or active
// native unit animation request. Object state and all callee results remain
// owned by the actual source unit and native live-support routines.
bool source_unit_animation_cancel_tu15_owns_entry(std::uint32_t);
void source_run_unit_animation_cancel_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
}
