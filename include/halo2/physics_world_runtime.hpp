#pragma once
#include "halo2/physics_contact_runtime.hpp"

namespace halo2 {
using SourcePhysicsWorldRuntimeAbi = SourcePhysicsContactRuntimeAbi;
using SourcePhysicsWorldRuntimeServices = SourcePhysicsContactRuntimeServices;
// Complete source world, island, motion, solver and contact bodies share their
// real caller ABI. A semantic argument-only adapter must not invent this state.
// Unimplemented external owners stop with the actual register/stack state.
// Entry IDs, required-owner sites and legacy observer PCs are canonical.
void source_run_physics_world_runtime(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry, SourcePhysicsWorldRuntimeAbi &,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
// The live selected-image composition reports original instruction/store PCs
// while preserving the canonical entry and required-owner ABI above.
void source_run_physics_world_runtime_selected_observations(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry, SourcePhysicsWorldRuntimeAbi &,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
