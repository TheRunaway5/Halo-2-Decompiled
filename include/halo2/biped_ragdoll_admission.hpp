#pragma once
#include "halo2/biped_death_runtime.hpp"
#include "halo2/biped_physics_rebuild.hpp"
#include "halo2/physics_body_matrix.hpp"

namespace halo2 {
// Source183650 maps every authored rigid node to the currently committed
// physical slot; the extra byte is the authored root marker, not node0.
void source_biped_rigid_node_slots_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object_datum,
    std::uint32_t output108, const SourcePhysicsContactRecordObserver & = {});
// DD9A0 obtains actual body slot identities from retained joint properties.
void source_biped_constraint_slots_1_1(
    SourcePhysicsContactMemory &, std::uint32_t component,
    std::uint32_t constraint_index, std::uint32_t first_output,
    std::uint32_t second_output,
    const SourcePhysicsContactRecordObserver & = {});
// Complete13A710 over the actual occupied/non-disposed observer pool and
// retained current ragdoll count. No native player count substitutes for it.
bool source_biped_ragdoll_budget_1_1(const SourcePhysicsContactMemory &, bool);
// Complete110EE0, including actual authored key-frame lookup. The graph
// resource owner1E13C0 is mandatory when1DE890 resolves a valid definition.
using SourceBipedAnimationResourceOwner =
    std::function<void(std::uint32_t graph, std::uint32_t packed)>;
// Connects the existing complete1E13C0 resource owner to retained physics
// memory. Runtime resource bytes518410/518411 remain actual owner inputs;
// admitted124E40 resource requests must be handled by the provided owner.
void source_biped_animation_resource_1_1(
    const SourcePhysicsContactMemory &, std::uint32_t graph,
    std::uint32_t packed,
    const std::function<void(std::uint32_t, std::uint8_t)> & = {});
std::uint8_t source_biped_animation_death_gate_1_1(
    const SourcePhysicsContactMemory &, std::uint32_t object_datum, bool,
    SourcePhysicsFactoryFloatingPoint,
    const SourceBipedAnimationResourceOwner &);
// Complete1F0E20 current source contact timestamp/body/normal delivery.
bool source_biped_recent_contact_1_1(
    SourcePhysicsContactMemory &, std::uint32_t contact,
    std::uint32_t body_output, std::uint32_t normal_output,
    SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
// CompleteBACC0: unparented root or the actual parent's current WORLD node.
void source_biped_world_position_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object_datum,
    std::uint32_t output, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});

struct SourceBipedPlacementRequest {
  std::uint32_t object_datum, anchor, height_word, slot, constraint,
      parent_slot;
  std::uint8_t inherited_valid;
  friend bool operator==(const SourceBipedPlacementRequest &,
                         const SourceBipedPlacementRequest &) = default;
};
struct SourceBipedPlacementServices {
  // Actual completeE3270 physical-body/collision placement owner. It must
  // consume the real body bank and collision services; no chosen success.
  std::function<std::uint8_t(const SourceBipedPlacementRequest &)> place_body;
  // Production composition also receives the actual E3270 entry frame.
  // Recursive children have distinct frames; no captured call address is used.
  std::function<std::uint8_t(const SourceBipedPlacementRequest &,
                              std::uint32_t entry_esp)> place_body_abi;
};
struct SourceBipedPlacementScratch {
  std::uint32_t node_slots, visited_mask, result_word;
  // DD9A0 output pair and recursive result packet. Calculations belong to
  // the caller; source recursion can reuse them after each child returns.
  std::uint32_t constraint_first, constraint_second, child_result;
  // Zero retains an explicitly owned diagnostic packet. Nonzero selects
  // complete original per-recursion frame derivation from E3190's entry.
  std::uint32_t entry_esp{};
};
SourceBipedPlacementScratch
source_biped_placement_scratch_1_1(std::uint32_t entry_esp);
// CompleteE3190 and recursiveE3A10: starts at the slot belonging to WORLD
// node0 and traverses current joints once per body. Defined result is low AL.
std::uint8_t source_place_biped_ragdoll_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object_datum,
    std::uint32_t anchor, std::uint32_t height_word,
    const SourceBipedPlacementScratch &, const SourceBipedPlacementServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Actual1D4C20: computes the current body's shape extent through184510.
// The original entry ESP owns its temporary scalar and transform packets.
std::uint32_t source_biped_body_extent_1_1(
    SourcePhysicsContactMemory &, std::uint32_t component, std::uint32_t slot,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
// CompleteE30B0 periodic corpse placement. Only mode1, a valid root slot,
// and component flag800 admit traversal. Height comes from the actual
// current shape extent, and the anchor comes from the current motion COM.
void source_maintain_biped_ragdoll_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object_datum,
    std::uint32_t entry_esp, const SourceBipedPlacementScratch &,
    SourcePhysicsFactoryFloatingPoint, const SourceBipedPlacementServices &,
    const SourcePhysicsContactRecordObserver & = {});
void source_maintain_biped_ragdoll_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object_datum,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint,
    const SourceBipedPlacementServices &,
    const SourcePhysicsContactRecordObserver & = {});
struct SourceBipedRagdollRebuildServices {
  std::function<void(std::uint32_t)> remove_object;
  SourceObjectPhysicsRebuildServices physics;
  std::function<void(std::uint32_t)> insert_component;
  SourceBipedPlacementServices placement;
};
struct SourceBipedRagdollRebuildScratch {
  std::uint32_t anchor;
  SourceBipedPlacementScratch placement;
  std::uint32_t requirements;
};
SourceBipedRagdollRebuildScratch
source_biped_ragdoll_rebuild_scratch_1_1(std::uint32_t entry_esp);
// CompleteE2CE0 rebuild/admission, including authored capsule center/height,
// conditional root/node lift, prior world membership, placement and death
// clock.
std::uint8_t source_rebuild_biped_ragdoll_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object_datum,
    const SourceBipedRagdollRebuildScratch &, SourcePhysicsFactoryFloatingPoint,
    const SourceBipedRagdollRebuildServices &,
    const SourcePhysicsContactRecordObserver & = {});
std::uint8_t source_rebuild_biped_ragdoll_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object_datum,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint,
    const SourceBipedRagdollRebuildServices &,
    const SourcePhysicsContactRecordObserver & = {});

using SourceBipedBodyVelocityDelta =
    std::function<void(std::uint32_t, std::int32_t, std::uint32_t)>;
// Complete185700 retained rigid-node to committed physical-slot selection.
std::int32_t
source_biped_contact_body_slot_1_1(const SourcePhysicsContactMemory &,
                                   std::uint32_t object_datum,
                                   std::int32_t contact_node);
// Original positive-contact fragmentE27E1..E2BD2, including source RNG,
// integer mass weighting and every actual per-body1D4A00 velocity request.
// frame is E2520's current caller ESP: normal at+38, delta at+50.
// False follows the original root-body fallback; no contact is fabricated.
bool source_biped_contact_velocity_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object_datum,
    std::int32_t contact_node, std::uint32_t frame,
    SourcePhysicsFactoryFloatingPoint, const SourceBipedBodyVelocityDelta &,
    const SourcePhysicsContactRecordObserver & = {});
struct SourceBipedDeathAdmissionServices {
  SourceBipedAnimationResourceOwner animation_resource;
  std::function<void(std::uint32_t)> update_activation;
  std::function<std::uint8_t(std::uint32_t)> rebuild_ragdoll;
  std::function<void(std::uint32_t)> maintain_placement;
  std::function<void(std::uint32_t, std::uint32_t)> set_velocity;
  std::function<void(std::uint32_t)> rebuild_controller, recover_controller;
  // Original1D4A00 adds the supplied delta to current body velocity. The
  // historical callback name does not imply a mass-scaled physical impulse.
  SourceBipedBodyVelocityDelta impulse;
};
struct SourceBipedDeathAdmissionScratch {
  std::uint32_t requirements, contact_body, contact_normal, impulse;
};
SourceBipedDeathAdmissionScratch
source_biped_death_admission_scratch_1_1(std::uint32_t entry_esp);
// CompleteE2520 contact/animation/budget arbitration and retained state writes.
// Required nested owners must produce their real memory effects. Mode0 does
// not force a death clip; the ordinary source cleanup controls animation.
std::uint8_t source_admit_biped_death_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object_datum,
    const SourceBipedDeathAdmissionScratch &, SourcePhysicsFactoryFloatingPoint,
    const SourceBipedDeathAdmissionServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Production ABI overload derives all caller-local storage from original ESP.
std::uint8_t source_admit_biped_death_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object_datum,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint,
    const SourceBipedDeathAdmissionServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
