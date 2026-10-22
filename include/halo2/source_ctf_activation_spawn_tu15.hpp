#pragma once

#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Complete original TU1.5 callbacks249600 and24A6F0 only. Player/team,
// objective and caller-stack state remain source-owned. All four actual
// external callee identities remain required through the existing services.
bool source_ctf_activation_spawn_tu15_owns_entry(std::uint32_t actual_entry);
void source_run_ctf_activation_spawn_tu15(
    SourcePhysicsContactMemory &memory, std::uint32_t actual_entry,
    SourcePhysicsWorldRuntimeAbi &abi, SourcePhysicsFactoryFloatingPoint floating_point,
    const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observer = {});
} // namespace halo2
