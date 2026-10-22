#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Whole26EFA0 decision admission,26EEF0 object-part query,265620 state
// publication,2753C0 tracking handler,1FB3C0 actor-state update and274E30
// object/property owner lookup, plus274450/274490 property creation using
// the original Frame classification/allocation/initialization owners.
// Original nested owners retain the
// same world/ABI; absent physics, script and platform services remain errors.
bool source_campaign_ai_object_decisions_tu15_owns_entry(std::uint32_t);
void source_run_campaign_ai_object_decisions_tu15(SourcePhysicsContactMemory&,
    std::uint32_t,SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
}
