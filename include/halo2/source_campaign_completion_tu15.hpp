#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Whole original completion/progress/options/transition-request bodies. Profile,
// metadata, statistics and scenario-loader calls retain their complete ABI;
// missing owners must stop instead of reporting a successful level handoff.
bool source_campaign_completion_tu15_owns_entry(std::uint32_t);
void source_run_campaign_completion_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
