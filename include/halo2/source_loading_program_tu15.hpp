#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Directly compiled TU1.5 loading compositor/quad/ring/ticker bodies. Entries
// and reported sites are actual authenticated Xbox addresses. GPU, texture,
// viewport and math service ownership remains explicit through the sourceABI;
// this is not a standalone readiness/clock or a production opcode interpreter.
bool source_loading_program_tu15_owns_entry(std::uint32_t);
void source_run_loading_program_tu15(SourcePhysicsContactMemory&,
    std::uint32_t actual_entry,SourcePhysicsWorldRuntimeAbi&,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsWorldRuntimeServices&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
