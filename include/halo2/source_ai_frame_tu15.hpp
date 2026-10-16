#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Whole1D0670 AI frame and its recovered original call closure. Actual actor,
// team, object, tag and clock banks are required. Unprovided owners fail.
// The original 47B478 descriptor row130 supplies the complete1C9C90
// actor-state callback reached by 1B0020; no host result is substituted.
// Shared placement helpers, actual204553/204589 post-object notifications
// and the actualCC9CD damage notification reuse these AI bodies and owners.
bool source_ai_frame_tu15_owns_entry(std::uint32_t);
void source_run_ai_frame_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
