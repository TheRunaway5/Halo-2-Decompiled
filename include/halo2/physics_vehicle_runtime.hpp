#pragma once
#include "halo2/physics_contact_runtime.hpp"

namespace halo2 {
// Complete original vehicle update/controllers and their rate callees retain
// their real GP/SSE/x87/stack state. Missing world, body, activation, animation
// and math owners stop at the reached call with all preceding state preserved.
// Production compiles static C++ paths; it never executes an original binary.
void source_run_physics_vehicle_runtime(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry, SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
