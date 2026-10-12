#pragma once
#include "halo2/unit_animation_selection_runtime.hpp"

namespace halo2 {
bool source_damage_owns_entry_1_1(std::uint32_t);
// Native composition boundary for original damage and vitality owners. Retains
// the caller's existing memory, call frame, MMX alias and nested CPU mutations.
void source_run_damage_1_1(SourcePhysicsContactMemory &, std::uint32_t,
    SourceUnitAnimationRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourceUnitAnimationRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
