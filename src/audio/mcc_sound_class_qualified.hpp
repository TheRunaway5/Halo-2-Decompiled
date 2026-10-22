#pragma once
// Internal native Cache authoring domain. Generic Source callers never invoke
// this seam or infer its authority from flags, bank shape or current bytes.
#include "audio/mcc_sound_class_apply.hpp"
#include "halo2/cache.hpp"
namespace halo2::mcc_sound_class_detail {
inline MccSoundRuntimePlan stage_qualified(const MccSoundRuntimeSource&actual,
                                         const MccSoundRuntimePlan&base){
    auto result=stage(actual,base);
    if(result.reference.status!=MccSoundClassReferenceStatus::Converted&&
       result.reference.status!=MccSoundClassReferenceStatus::AlreadyEquivalent)
        throw FormatError("Native MCC Legacy classes lack a complete authenticated current namespace");
    return std::move(result.native);
}
}
