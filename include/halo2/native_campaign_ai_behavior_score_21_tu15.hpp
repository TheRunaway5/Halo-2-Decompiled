#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Owns the TU1.5 descriptor-21 indirect score callback at 1B0195. An
// unowned frame is left untouched. Admitted faults retain the executed prefix.
bool native_campaign_ai_behavior_score_21_tu15_try_run(SourcePhysicsContactMemory &,
    SourcePhysicsContactImage, const SourcePhysicsContactRuntimeCall &, SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsWorldRuntimeServices &, const SourcePhysicsContactRecordObserver & = {});
}
