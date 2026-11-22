#pragma once

#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Exact descriptor-17 indirect score at 1B0195. Admitted children retain the
// standard required-owner ABI, including any deeper unsupported boundaries.
bool native_campaign_ai_behavior_score_17_tu15_try_run(SourcePhysicsContactMemory &,
    SourcePhysicsContactImage, const SourcePhysicsContactRuntimeCall &, SourcePhysicsWorldRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint, const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
