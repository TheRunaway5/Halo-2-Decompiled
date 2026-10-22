#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Standalone test composition of existing complete production services.
// All unknown callees retain their original full ABI and required-owner stop.
bool source_multiplayer_engine_services_tu15_owns_entry(std::uint32_t);
void source_run_multiplayer_engine_services_tu15(SourcePhysicsContactMemory&,
    std::uint32_t,SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,
    const SourcePhysicsContactRecordObserver& = {});
}
