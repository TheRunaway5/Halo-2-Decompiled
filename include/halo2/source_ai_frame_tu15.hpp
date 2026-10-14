#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Whole1D0670 AI frame and its recovered original call closure. Actual actor,
// team, object, tag and clock banks are required. Unprovided owners fail.
// Only the frame entry is exported; helpers retain their existing world owners.
bool source_ai_frame_tu15_owns_entry(std::uint32_t);
void source_run_ai_frame_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
