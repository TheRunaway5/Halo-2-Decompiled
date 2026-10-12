#pragma once
#include "halo2/unit_animation_selection_runtime.hpp"

namespace halo2 {
// Complete E4D30/E5700, support/point and request initialization/attribution,
// independently paired to their TU1.5 bodies. Reads current
// owned object/tag/options/globals banks and exposes the actual lower ABI.
// No native fall-distance formula or supplied support/damage outcome exists.
void source_run_biped_falling_damage_runtime(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry, SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Join existing TU1.1 velocity, buoyancy and animation owners on the same bank.
// Player notification and complete damage delivery remain
// required when reached. The full incoming/outgoing ABI remains observable.
void source_run_biped_falling_damage_1_1(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry, SourceUnitAnimationRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint, const SourceUnitAnimationRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_call_biped_falling_damage_1_1(SourcePhysicsContactMemory &,
    const SourceRuntimeCall &, SourcePhysicsFactoryFloatingPoint,
    const SourceUnitAnimationRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
