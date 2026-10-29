#pragma once
#include "halo2/physics_contact_record.hpp"

namespace halo2 {
// Complete TU1.5 cinematic interface reset (1FCB0) and completed capture
// acknowledgement (1FCE0). These operate on real retained data only. No CPU
// state, executable image, instruction dispatcher or renderer result is used.
// Reset publishes 255 ordered words before inspecting target invalidation;
// an absent interface leaves even the target state untouched.
void native_campaign_screen_stop_tu15(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRecordObserver & = {});
void native_campaign_screen_capture_complete_tu15(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
