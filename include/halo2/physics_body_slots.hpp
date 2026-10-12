#pragma once
#include "halo2/physics_body_factory.hpp"

namespace halo2 {
// The body and component are real retained identities. body_indices points at
// the actual 1D95E0-produced byte bank; it is not a definition or node bank.
struct SourcePhysicsBodySlotConstruction {
  std::uint32_t slot, body, body_indices, count, flag_word;
};
struct SourcePhysicsBodySlotCommit {
  std::uint32_t component, body, body_indices, count, flag_word;
};
// Complete measured 1DC0C0: initializes one retained0x60-byte slot, installs
// property2003, owns inline/external byte-bank allocation and copies indices.
std::uint32_t source_construct_physics_body_slot_1_1(
    SourcePhysicsContactMemory &, const SourcePhysicsBodySlotConstruction &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete actual 1D3A70 mode7/mode4 commit through actual body property banks
// and current motion state. No world registration/contact is admitted here.
// Unmeasured component flag40 position owner and other virtual motion classes
// fail at the owning boundary after preceding source stores.
std::uint32_t source_commit_physics_body_slot_1_1(
    SourcePhysicsContactMemory &, const SourcePhysicsBodySlotCommit &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete admitted 1D29A0/1D4260 copy from the actual current motion. It does
// not choose a pose or derive body position from the authored object root.
void source_refresh_physics_body_slot_1_1(
    SourcePhysicsContactMemory &, std::uint32_t component,
    std::uint32_t slot_index, const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
