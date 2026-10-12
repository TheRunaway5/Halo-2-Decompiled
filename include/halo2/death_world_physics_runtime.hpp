#pragma once
#include "halo2/unit_animation_selection_runtime.hpp"

namespace halo2 {
// Complete original dynamically reached contact and solver bodies. Current
// body/constraint/contact banks and every reached lower owner are mandatory.
void source_run_death_world_physics_runtime_1_1(
    SourcePhysicsContactMemory &, std::uint32_t canonical_entry,
    SourceUnitAnimationRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourceUnitAnimationRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
