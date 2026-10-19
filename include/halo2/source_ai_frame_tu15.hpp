#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Whole1D0670 AI frame and its recovered original call closure. Actual actor,
// team, object, tag and clock banks are required. Unprovided owners fail.
// The original 47B478 descriptor row130 supplies the complete1C9C90
// actor-state callback reached by 1B0020; no host result is substituted.
// Kind0 row48 also supplies the original RET8 1BFE00 selector reached
// by 1B0128; its real dependencies reuse their existing engine owners.
// Kind0 row51 supplies the complete RET8 1BFCE0 selector through the
// same caller and existing tag/actor lookup owners.
// Shared placement helpers, actual204553/204589 post-object notifications
// and the actualCC9CD damage notification reuse these AI bodies and owners.
// Original actor iterator entries are also exported for campaign AI queries.
bool source_ai_frame_tu15_owns_entry(std::uint32_t);
void source_run_ai_frame_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
