#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Complete original24B390 configuration predicate and249440 source-kind
// getter on the retained world bank. Preserve the full caller ABI, including
// partial EAX writes; no bank, callback result or game mode is supplied.
bool source_ctf_configuration_tu15_owns_entry(std::uint32_t actual_entry);
void source_run_ctf_configuration_tu15(SourcePhysicsContactMemory&,
    std::uint32_t actual_entry,SourcePhysicsWorldRuntimeAbi&,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsWorldRuntimeServices&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
