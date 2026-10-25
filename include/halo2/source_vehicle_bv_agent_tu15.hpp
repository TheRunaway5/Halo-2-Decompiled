#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool source_vehicle_bv_agent_tu15_owns_entry(std::uint32_t);
// Complete statically compiled TU1.5 BV-agent and compound-shape query owners.
// External shape/collector owners retain their real call ABI and dispatch.
void source_run_vehicle_bv_agent_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
