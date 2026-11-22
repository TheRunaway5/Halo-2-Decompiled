#pragma once

#include "halo2/physics_world_runtime.hpp"

namespace halo2 {

// Descriptor 42's selection slot and its original AX/EAX return contract.
bool native_campaign_ai_behavior_select_42_tu15_try_run(SourcePhysicsContactMemory &memory,
    SourcePhysicsContactImage image, const SourcePhysicsContactRuntimeCall &call,
    SourcePhysicsWorldRuntimeAbi &abi, const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe = {});

// Descriptor 42's start slot at the actual actor behavior handoff.
bool native_campaign_ai_behavior_start_42_tu15_try_run(SourcePhysicsContactMemory &memory,
    SourcePhysicsContactImage image, const SourcePhysicsContactRuntimeCall &call,
    SourcePhysicsWorldRuntimeAbi &abi, const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe = {});

// Descriptor 42's update slot and its original behavior selection child.
bool native_campaign_ai_behavior_update_42_tu15_try_run(SourcePhysicsContactMemory &memory,
    SourcePhysicsContactImage image, const SourcePhysicsContactRuntimeCall &call,
    SourcePhysicsWorldRuntimeAbi &abi, const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe = {});

}
