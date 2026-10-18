#pragma once
#include "halo2/mcc_sound_class_reference.hpp"
namespace halo2::mcc_sound_class_detail {
// PRIVATE staged prototype. Supply the actual current Cache namespace here;
// raw fingerprint source is passed separately to other builders.
// Owns complete plan/namespace before copying callbacks. No caller/source or
// runtime publication writes; all allocations/reads finish in owned staging.
struct Result {
    MccSoundRuntimePlan native;
    MccSoundClassReferenceProjection reference;
    unsigned diagnostic_ranges{}, diagnostic_users{};
};
Result stage(const MccSoundRuntimeSource&,const MccSoundRuntimePlan&);
} // namespace halo2::mcc_sound_class_detail
