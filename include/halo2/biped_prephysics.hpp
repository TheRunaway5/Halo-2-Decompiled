#pragma once
#include "halo2/biped_body_velocity.hpp"
#include "halo2/biped_ragdoll_admission.hpp"
#include "halo2/biped_ragdoll_runtime.hpp"
#include "halo2/source_runtime_call.hpp"
#include "halo2/unit_animation_selection_runtime.hpp"
#include <optional>

namespace halo2 {
// Literal ABI request from the source biped phase. Only defined input
// registers are present; incidental volatile registers are not dependencies.
using SourceBipedPhaseCall = SourceRuntimeCall;
struct SourceBipedPrePhysicsServices {
  // When connected, DE960 invokes native E2520 directly with derived ABI
  // storage. The generic E2520 boundary remains for isolated caller tests.
  std::optional<SourceBipedDeathAdmissionServices> death_admission;
  // Complete native DDBC0 input/root-motion producer. Reached cache-byte,
  // collision and other lower owners retain their actual raw call ABI.
  std::optional<SourceUnitAnimationRuntimeServices> physical_input;
  // E1800 action input, E4F40 recovery, DF7C0 object update, BC910 marking
  // and movement basis/root-motion execute on the same retained source bank.
  std::optional<SourceUnitAnimationRuntimeServices> phase_runtime;
  // Complete1C8430 and body velocity/activation on actual committed bodies.
  // Also connects E2520's BAA80/1D4A00 activation and velocity-delta owners
  // directly, using E2520's actual caller frame for temporary storage.
  std::optional<SourceBipedVelocityServices> object_velocity;
  // Native E2CE0/E30B0 composition uses the same world/component/body bank.
  // Actual collision and platform owners are still mandatory when reached.
  std::optional<SourceBipedRagdollRuntimeServices> ragdoll_runtime;
  // Must execute the actual animation, physical-state, movement or object
  // owner and mutate its retained memory. In particular E2520 must bind
  // source_admit_biped_death_1_1 with its current source frame and services.
  // Recorded callee effects/returns are test boundaries, never live providers.
  SourceRuntimeOwner required_owner;
};
// Complete1E8F60 physical-state step caller and1E94E0 output-mode selection.
// State-specific integrators and collision/shape owners execute through the
// required services over the same retained storage. No contact or solved
// position is inferred from a native Unit.
void source_step_biped_physical_1_1(
    SourcePhysicsContactMemory &, std::uint32_t state,
    std::uint32_t physical_packet, std::uint32_t output,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint,
    const SourceBipedPrePhysicsServices &,
    const SourcePhysicsContactRecordObserver & = {});
void source_select_biped_physical_movement_1_1(
    SourcePhysicsContactMemory &, std::uint32_t state,
    std::uint32_t physical_packet, std::uint32_t output,
    SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
// Complete1F0080 state3 movement, support tracking, damping and source
// controller admission. The source collision, point-motion, activation and
// component-rebuild owners remain required only on their reached branches.
void source_step_biped_dead_1_1(
    SourcePhysicsContactMemory &, std::uint32_t dead_packet,
    std::uint32_t physical_packet, std::uint32_t output,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint,
    const SourceBipedPrePhysicsServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete DF5E0 physical activation predicate and DEE00 buoyancy/gravity
// fraction. Both consume source state and component/actor banks directly.
bool source_biped_physical_activation_1_1(const SourcePhysicsContactMemory &,
                                          std::uint32_t object,
                                          SourcePhysicsFactoryFloatingPoint);
std::uint32_t source_biped_physical_fraction_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object, std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
// Complete E44C0 movement/action arbitration. The actual graph availability
// owner is required only for physical state2.
void source_biped_movement_action_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object,
    std::uint32_t mode_output, std::uint32_t action_output,
    std::uint32_t action_byte_output, std::uint32_t mode,
    std::uint8_t action_byte, std::uint32_t entry_esp,
    const SourceBipedPrePhysicsServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete E3B20 physical-step delivery, action arbitration, activation,
// root/velocity delivery and movement scalar. The actual state-specific
// 1E8F60 step, world relocation and animation owners remain mandatory when
// reached. This never derives physical contacts from native grounded flags.
std::uint8_t source_update_biped_movement_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object,
    std::uint32_t action_packet, std::uint32_t physical_packet,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint,
    const SourceBipedPrePhysicsServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete BBE30 walks parent links through objects with flag26 set.
std::uint32_t
source_biped_unmarked_parent_1_1(const SourcePhysicsContactMemory &,
                                 std::uint32_t parent);
// Complete DE900 action classification, not a native alive/grounded enum.
std::uint8_t source_biped_action_class_1_1(std::uint32_t action);
// Complete finite-input1F0940: current source physical packet orientation
// and forward/up basis delivery, including its x87 acos/sin/cos and SSE
// rotation. The original CRT domain-error owner remains required for nonfinite
// angles.
void source_update_biped_dead_outputs_1_1(
    SourcePhysicsContactMemory &, std::uint32_t physical_packet,
    std::uint32_t up_output, std::uint32_t forward_output,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
// Complete1E9480 state dispatch. States4..6 perform no output update;
// states1/2 require their actual1F4E20/1FE3E0 owners.
void source_update_biped_physical_outputs_1_1(
    SourcePhysicsContactMemory &, std::uint32_t physical_packet,
    std::uint32_t physical_state, std::uint32_t up_output,
    std::uint32_t forward_output, std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint, const SourceBipedPrePhysicsServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete E43B0 delayed recovery gate and E52C0 dead action selection.
// These consume actual animation-manager state; landing_dead is requested
// only by the original admitted branch, never by a native dead enum.
std::uint8_t source_update_biped_recovery_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object, std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint, const SourceBipedPrePhysicsServices &,
    const SourcePhysicsContactRecordObserver & = {});
std::uint8_t source_select_biped_dead_action_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object,
    std::uint32_t action_packet, std::uint32_t physical_packet,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint,
    const SourceBipedPrePhysicsServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete E6280 retained action-release timer and E63C0 request.
void source_update_biped_action_release_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object, std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint, const SourceBipedPrePhysicsServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete E7E60 iterates all sixty retained action slots and clears a bit
// only after its actual authored owner returns false.
std::uint8_t source_update_biped_active_actions_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object, std::uint32_t entry_esp,
    const SourceBipedPrePhysicsServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete DE960 pre-physics caller, including parent branches, heading,
// angular-velocity threshold, signed counters, action and physical packets,
// natural E2520 scheduling and post-phase activation. ESP is original entry
// ESP backed by a real temporary ABI region, not a captured scratch packet.
std::uint8_t source_update_biped_prephysics_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object, std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint, const SourceBipedPrePhysicsServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
