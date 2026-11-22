#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Owns the TU1.5 descriptor-43 score callback at 1B0195.
bool native_campaign_ai_behavior_score_43_tu15_try_run(SourcePhysicsContactMemory &,
    SourcePhysicsContactImage, const SourcePhysicsContactRuntimeCall &, SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsWorldRuntimeServices &, const SourcePhysicsContactRecordObserver & = {});
}
