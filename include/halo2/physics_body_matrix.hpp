#pragma once
#include "halo2/physics_contact_record.hpp"

namespace halo2 {
// Original2E0B60 conditional component correction. The callback must implement
// that actual owner over retained storage; absence is a hard boundary after
// the preceding original transform copies. It cannot supply a completed pose.
using SourcePhysicsBodyMatrixCorrection = std::function<void(
    SourcePhysicsContactMemory &, std::uint32_t transform40,
    std::uint32_t component_offset30,
    const SourcePhysicsContactRecordObserver &)>;

// Complete1D42D0 / authenticated1.5 1D8B90 current body-matrix consumer.
// Reads an actual committed slot/body/motion bank and emits the source0x34
// matrix. scratch40 is caller-owned,16-byte-aligned source stack storage.
// Real component bit6 requires the original2E0B60 correction owner. This does
// not construct bodies, admit death/ragdolls, insert a world or solve contacts.
void source_physics_body_matrix_1_1(SourcePhysicsContactMemory &,
    std::uint32_t component, std::uint32_t slot_index,
    std::uint32_t output34, std::uint32_t scratch40,
    const SourcePhysicsBodyMatrixCorrection & = {},
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
