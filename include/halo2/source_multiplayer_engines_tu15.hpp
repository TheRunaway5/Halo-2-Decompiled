#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Recovered whole TU1.5 multiplayer callbacks and reached mode/shared helpers.
// Existing native owners remain external dependencies on the same world bank;
// missing services preserve their actual call ABI and fail in the dispatcher.
bool source_multiplayer_engines_tu15_owns_entry(std::uint32_t actual_entry);
void source_run_multiplayer_engines_tu15(SourcePhysicsContactMemory&,
    std::uint32_t actual_entry,SourcePhysicsWorldRuntimeAbi&,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsWorldRuntimeServices&,
    const SourcePhysicsContactRecordObserver& = {});
}
