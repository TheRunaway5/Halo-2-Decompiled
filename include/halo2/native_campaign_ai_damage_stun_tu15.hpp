#pragma once

#include "halo2/physics_world_runtime.hpp"

namespace halo2 {

// TU1.5 AI damage-stun notification 1D20C0 -> tail 1EAB60 (called from damage delivery CCA54/CB8D4).
bool native_campaign_ai_damage_stun_tu15_try_run(SourcePhysicsContactMemory &memory,
    SourcePhysicsContactImage image, const SourcePhysicsContactRuntimeCall &call,
    SourcePhysicsWorldRuntimeAbi &abi, SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe = {});

}
