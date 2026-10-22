#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Whole original1B7240/1B7000 behavior,1EA360 scalar and missing nested Frame dependencies.
// Existing Frame owners share the same bank/ABI; physical services stay required.
bool source_campaign_ai_actor_behavior_tu15_owns_entry(std::uint32_t);
void source_run_campaign_ai_actor_behavior_tu15(SourcePhysicsContactMemory&,
    std::uint32_t,SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
}
