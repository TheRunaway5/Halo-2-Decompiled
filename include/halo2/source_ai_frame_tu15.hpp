#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Whole1D0670 AI frame and its recovered original call closure. Actual actor,
// team, object, tag and clock banks are required. Unprovided owners fail.
// The frame and three shared original helpers are exported. Campaign squad
// placement reuses these bodies rather than introducing another owner.
bool source_ai_frame_tu15_owns_entry(std::uint32_t);
void source_run_ai_frame_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
