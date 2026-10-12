#pragma once
#include "halo2/biped_postphysics_runtime.hpp"

namespace halo2 {
// Complete DEFC0 postphysics, actual DE6D0 packet producer and contact refresh
// execute on the same retained bank. Unknown collision/damage/animation
// owners retain their actual ABI and must execute when reached.
void source_run_biped_postphysics_phase_1_1(
    SourcePhysicsContactMemory &, std::uint32_t canonical_entry,
    SourceUnitAnimationRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourceUnitAnimationRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
void source_update_biped_postphysics_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint,
    const SourceUnitAnimationRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
