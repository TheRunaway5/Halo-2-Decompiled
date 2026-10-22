#pragma once
#include "halo2/physics_contact_runtime.hpp"

namespace halo2 {
// Complete paired damage/vitality graph. The caller owns its original bank,
// request, stack and full CPU state. Unknown delivery, physics and effect
// services preserve their real pending frame and produced prefix.
bool source_damage_delivery_owns_entry(std::uint32_t canonical_entry);
void source_run_damage_delivery_runtime(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry, SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Live composition reports selected-image instruction/store PCs, preserving
// the legacy canonical entry and required-owner call-site ABI.
void source_run_damage_delivery_runtime_selected_observations(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry, SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
