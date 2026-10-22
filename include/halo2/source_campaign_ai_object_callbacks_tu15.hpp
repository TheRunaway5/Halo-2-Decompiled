#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete missing callbacks in original456818/456878 and their absent
// direct dependency closure, plus the registered267A30/267A40 query pair.
// Existing Frame/physics owners retain the same
// source memory and ABI. No callback result or object state is supplied.
bool source_campaign_ai_object_callbacks_tu15_owns_entry(std::uint32_t);
void source_run_campaign_ai_object_callbacks_tu15(SourcePhysicsContactMemory&,
    std::uint32_t,SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
}
