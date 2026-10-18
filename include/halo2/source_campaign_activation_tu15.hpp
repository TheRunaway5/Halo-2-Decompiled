#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Original map/BSP activation and selected registered callback bodies. Readiness
// is produced only by the original path after its real owners complete.
bool source_campaign_activation_tu15_owns_entry(std::uint32_t);
void source_run_campaign_activation_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
