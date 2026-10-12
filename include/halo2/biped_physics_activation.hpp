#pragma once
#include "halo2/physics_contact_record.hpp"

namespace halo2 {
struct SourceBipedPhysicsActivationServices {
  // Actual27F9F0 zero-reference admission and the body's deleting virtual.
  // Required only when312A90 removes the last retained reference.
  std::function<bool(std::uint32_t)> deferred_delete;
  std::function<void(std::uint32_t)> destroy_body;
  // Actual3131C0 world/island activation andB6760 attached-parent marking.
  std::function<void(std::uint32_t world, std::uint32_t island)> activate_island;
  std::function<void(std::uint32_t object, std::uint32_t flags)> mark_parent;
};
// Complete312A90 removal from the world's deferred body bank. Swap deletion,
// the original forward traversal and source reference ownership are retained.
void source_remove_biped_deferred_body_1_1(
    SourcePhysicsContactMemory &, std::uint32_t world, std::uint32_t body,
    const SourceBipedPhysicsActivationServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete3121C0 routing, with3131C0 required for an inactive live island.
void source_activate_biped_body_1_1(
    SourcePhysicsContactMemory &, std::uint32_t body,
    const SourceBipedPhysicsActivationServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete1D5650; reads the actual committed component body bank/count.
void source_activate_biped_component_1_1(
    SourcePhysicsContactMemory &, std::uint32_t component,
    const SourceBipedPhysicsActivationServices &,
    const SourcePhysicsContactRecordObserver & = {});
// CompleteBAA80 and raw-memoryB8250 update propagation. A false request wakes
// registered bodies before clearing objectC1bit0; true preserves that bit on
// the source registered-body branch. Parent marking remains mandatory.
void source_biped_physics_activation_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object, bool request,
    const SourceBipedPhysicsActivationServices &,
    const SourcePhysicsContactRecordObserver & = {});
// CompleteB9730 object wrapper around the required1D4C50 removal owner.
void source_remove_biped_object_physics_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object,
    const std::function<void(std::uint32_t component)> &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
