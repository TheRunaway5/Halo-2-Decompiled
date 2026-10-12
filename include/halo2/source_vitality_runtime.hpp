#pragma once
#include "halo2/physics_contact_runtime.hpp"

namespace halo2 {
// Complete TU1.1 D73E0 update, D7D90 health recovery, D80F0 overshield
// activation and their authenticated local tag, fade, attribution, player,
// difficulty, trait and HUD-history owners. TU1.5 is independently paired.
// The caller owns actual object/tag/actor/player/clock/region/replication banks.
// Unimplemented effect, section-response and replication callees expose their
// actual full ABI through required_owner; no success/result is synthesized.
void source_run_vitality_runtime(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry, SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
