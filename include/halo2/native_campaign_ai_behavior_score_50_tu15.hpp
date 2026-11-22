#pragma once

#include "halo2/physics_world_runtime.hpp"

namespace halo2 {

// Descriptor 50's indirect behavior selection at 1B0128.
bool native_campaign_ai_behavior_score_50_tu15_try_run(SourcePhysicsContactMemory &memory,
    SourcePhysicsContactImage image, const SourcePhysicsContactRuntimeCall &call,
    SourcePhysicsWorldRuntimeAbi &abi, SourcePhysicsFactoryFloatingPoint controls,
    const SourcePhysicsWorldRuntimeServices &services, const SourcePhysicsContactRecordObserver &observe = {});

}
