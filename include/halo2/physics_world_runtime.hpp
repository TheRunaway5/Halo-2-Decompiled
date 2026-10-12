#pragma once
#include "halo2/physics_contact_runtime.hpp"

namespace halo2 {
using SourcePhysicsWorldRuntimeAbi = SourcePhysicsContactRuntimeAbi;
using SourcePhysicsWorldRuntimeServices = SourcePhysicsContactRuntimeServices;
// Complete source world, island, motion, solver and contact bodies share their
// real caller ABI. A semantic argument-only adapter must not invent this state.
// Unimplemented external owners stop with the actual register/stack state.
void source_run_physics_world_runtime(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry, SourcePhysicsWorldRuntimeAbi &,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
