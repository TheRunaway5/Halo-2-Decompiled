#pragma once

#include "halo2/physics_world_runtime.hpp"

namespace halo2 {

// The campaign's 11231B call to 1E2F60 yields a total and a marker prefix.
bool native_campaign_animation_distance_tu15_try_run(SourcePhysicsContactMemory &memory,
    SourcePhysicsContactImage image, const SourcePhysicsContactRuntimeCall &call,
    SourcePhysicsWorldRuntimeAbi &abi, SourcePhysicsFactoryFloatingPoint controls,
    const SourcePhysicsWorldRuntimeServices &services, const SourcePhysicsContactRecordObserver &observe = {});

}
