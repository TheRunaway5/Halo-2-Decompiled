#pragma once
#include "halo2/mcc_sound_timing_reference.hpp"
#include "halo2/source_sound_duration_tu15.hpp"
namespace halo2 {
// Caller must hold with_mcc_sound_original_timing's actual owner/resource
// transaction and prove its designated writable stack before entry. This
// leaf projects reads only; it neither authenticates a resource nor binds one.
void run_mcc_sound_duration_read_projection(SourcePhysicsContactMemory &,
    const MccSoundTimingEntry &,const MccSoundOriginalTimingOperands &,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver &);
}
