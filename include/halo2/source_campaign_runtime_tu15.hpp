#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Whole original campaign squad actions, command and HS storage owners.
// They require the actual initialized AI, object, tag and navigation owners.
// Missing reached owners fail explicitly; no actor body or result is supplied.
bool source_campaign_runtime_tu15_owns_entry(std::uint32_t);
void source_run_campaign_runtime_tu15(
    SourcePhysicsContactMemory &, std::uint32_t, SourcePhysicsWorldRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
