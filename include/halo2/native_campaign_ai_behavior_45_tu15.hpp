#pragma once

#include "halo2/physics_world_runtime.hpp"

namespace halo2 {

// Descriptor 45 (0x48a1f8) selection callback 1B1FA0 and its AX/EAX contract.
bool native_campaign_ai_behavior_select_45_tu15_try_run(SourcePhysicsContactMemory &memory,
    SourcePhysicsContactImage image, const SourcePhysicsContactRuntimeCall &call,
    SourcePhysicsWorldRuntimeAbi &abi, const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe = {});

// Descriptor 45 (0x48a1f8) perform callback 1B1FE0 at 1B0AD9.
bool native_campaign_ai_behavior_perform_45_tu15_try_run(SourcePhysicsContactMemory &memory,
    SourcePhysicsContactImage image, const SourcePhysicsContactRuntimeCall &call,
    SourcePhysicsWorldRuntimeAbi &abi, const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe = {});

// Descriptor 45 (0x48a1f8) posture/end callback 1B2090 at 1B0F56.
bool native_campaign_ai_behavior_end_45_tu15_try_run(SourcePhysicsContactMemory &memory,
    SourcePhysicsContactImage image, const SourcePhysicsContactRuntimeCall &call,
    SourcePhysicsWorldRuntimeAbi &abi, const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe = {});

}
