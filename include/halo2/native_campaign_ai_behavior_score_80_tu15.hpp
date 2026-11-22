#pragma once

#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Exact descriptor-80 score call at 1B0195 and its character-property leaf.
// Other callbacks retain their original required-owner dispatch.
bool native_campaign_ai_behavior_score_80_tu15_try_run(SourcePhysicsContactMemory &,
    SourcePhysicsContactImage, const SourcePhysicsContactRuntimeCall &, SourcePhysicsWorldRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint, const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
