#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
struct NativeTu15UnitMaximumVitalityInput {
    // Ordinary scalar input DATA. A null address selects the real model's
    // authored default; these are never unit/body AFTER images.
    std::uint32_t health{},shield{};
};
struct NativeTu15UnitMaximumVitalityStats {
    std::uint64_t calls{},definition_queries{},null_models{},no_damage_information{},
        default_vitality{},explicit_health{},explicit_shield{},stores{},
        live_requests{},none_units{},dead_units{},applied{};
};
// CompleteD81E0 with the actualD80A0 model/damage-information lookup.
void native_tu15_set_unit_maximum_vitality(SourcePhysicsContactMemory&,std::uint32_t unit,
    NativeTu15UnitMaximumVitalityInput,NativeTu15PhysicsFrameFp&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15UnitMaximumVitalityStats* = nullptr);
// The actual2ADCC4..2ADCFF HS admission after scalar argument marshalling:
// NONE/dead units are unchanged. No HS evaluator/frame state enters this API.
bool native_tu15_set_live_unit_maximum_vitality(SourcePhysicsContactMemory&,std::uint32_t unit,
    NativeTu15UnitMaximumVitalityInput,NativeTu15PhysicsFrameFp&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15UnitMaximumVitalityStats* = nullptr);
}
