#pragma once
#include "halo2/physics_contact_record.hpp"

namespace halo2 {
// The complete 22836..2286C model-slot reset inside actual TU1.5 22830.
// These are the PRT cache's24 owner records at4C51D0 (stride0x68).
// Reset all24 slots, preserving each row's fourteen unspecified bytes.
// BSP rotation/interpolation after2286E remains a separate source owner.
void native_tu15_reset_model_view_slots(SourcePhysicsContactMemory&,
    const SourcePhysicsContactRecordObserver& = {});
}
