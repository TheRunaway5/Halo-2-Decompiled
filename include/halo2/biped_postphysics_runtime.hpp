#pragma once
#include "halo2/unit_animation_selection_runtime.hpp"

namespace halo2 {
// Complete1E93F0 physical-state dispatch and1F0E90 retained contact refresh,
// including1F0D30 support release. Actual object/geometry/velocity services
// remain required on reached branches, with their full original ABI.
void source_run_biped_postphysics_runtime_1_1(
    SourcePhysicsContactMemory &, std::uint32_t canonical_entry,
    SourceUnitAnimationRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourceUnitAnimationRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Defined1E93F0 inputs: physical packet in EAX, output in ECX and retained
// state in EBX. No incidental incoming registers become dependencies.
void source_update_biped_physical_contacts_1_1(
    SourcePhysicsContactMemory &, std::uint32_t physical_state,
    std::uint32_t physical_packet, std::uint32_t output,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint,
    const SourceUnitAnimationRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
