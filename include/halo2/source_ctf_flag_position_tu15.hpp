#pragma once

#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Complete original TU1.5 FCE50. Actual BAD60/BBE00 callees and their source
// object, output and scratch ownership remain caller-provided native services.
bool source_ctf_flag_position_tu15_owns_entry(std::uint32_t actual_entry);
void source_run_ctf_flag_position_tu15(
    SourcePhysicsContactMemory &memory, std::uint32_t actual_entry,
    SourcePhysicsWorldRuntimeAbi &abi, SourcePhysicsFactoryFloatingPoint floating_point,
    const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observer = {});
} // namespace halo2
