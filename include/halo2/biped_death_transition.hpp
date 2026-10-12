#pragma once
#include "halo2/biped_ragdoll_admission.hpp"

namespace halo2 {
// Complete1EFF60 dead physical packet initialization. Unassigned bytes are
// retained, including+1C..4F; authored source vector pointers remain required.
void source_initialize_biped_dead_state_1_1(
    SourcePhysicsContactMemory &, std::uint32_t packet,
    const SourcePhysicsContactRecordObserver & = {});
// Complete1F3DA0 ordinary physical packet constructor, including its authored
// identity matrix and source484DE0 support value; preserves unassigned bytes.
void source_initialize_biped_ground_state_1_1(
    SourcePhysicsContactMemory &, std::uint32_t packet,
    const SourcePhysicsContactRecordObserver & = {});
struct SourceBipedPhysicalStateServices {
  // Actual1FFD10 owner when leaving state4/5.
  std::function<void(std::uint32_t object, std::uint32_t packet)> leave;
  // Other actual state constructors1FFCB0/1EF030. This callback must
  // initialize their real packet; no state-byte or success default is used.
  std::function<void(std::uint8_t state, std::uint32_t physical,
                     std::uint32_t anchor)>
      initialize;
  SourceObjectPhysicsRebuildServices rebuild;
};
// Complete1E8E60 state0..6 dispatch and rebuild ordering. State3 owns the
// actual ground/dead packets here. Original0/7+ destination dispatch is undefined.
void source_biped_physical_state_1_1(
    SourcePhysicsContactMemory &, std::uint32_t physical,
    std::uint8_t destination, std::uint32_t anchor_scratch,
    SourcePhysicsFactoryFloatingPoint, const SourceBipedPhysicalStateServices &,
    const SourcePhysicsContactRecordObserver & = {});
// CompleteE6FB0 default selector: the actual object10Abit2 and authored
// bipd264bit4 choose dead3, alternate2 or ordinary1 physical state.
std::uint8_t source_biped_default_physical_state_1_1(
    const SourcePhysicsContactMemory &, std::uint32_t object);
// CompleteE6F60/E6D80 routing through the native physical-state transition.
void source_reset_biped_physical_state_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object,
    std::uint32_t anchor_scratch, SourcePhysicsFactoryFloatingPoint,
    const SourceBipedPhysicalStateServices &,
    const SourcePhysicsContactRecordObserver & = {});
// CompleteDFAF0 cold biped field construction. Generic object/unit allocation,
// model nodes and animation managers must already be constructed by their
// owning source routines. This function never fills a whole host packet.
void source_construct_biped_physical_state_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object,
    std::uint32_t anchor_scratch, SourcePhysicsFactoryFloatingPoint,
    const SourceBipedPhysicalStateServices &,
    const SourcePhysicsContactRecordObserver & = {});
// CompleteE6DC0 death-state transition and airborne contact-byte clearing.
void source_transition_biped_dead_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object,
    std::uint32_t anchor_scratch, SourcePhysicsFactoryFloatingPoint,
    const SourceBipedPhysicalStateServices &,
    const SourcePhysicsContactRecordObserver & = {});
struct SourceBipedDeathTransitionServices {
  SourceBipedPhysicalStateServices physical;
  // ActualBB0C0 orientation vector andB8490 root relocation. Called only
  // by the source airborne clearance branch, on its own current state.
  std::function<void(std::uint32_t object, std::uint32_t output3)> orientation;
  std::function<void(std::uint32_t object, std::uint32_t position3)> relocate;
  // Actual110BF0 with EAX210 and seven source arguments, then1D1940.
  // Only an explicitly nonzero forced-animation input reaches these owners.
  std::function<void(std::uint32_t object)> forced_animation, update_animation;
};
struct SourceBipedDeathTransitionRequest {
  std::uint32_t object, source_object, direction3;
  std::uint8_t forced_animation;
};
// CompleteE6050. Ordinary zero-flag deaths do not select landing_dead. Fatal
// attribution/direction are copied only on the original first transition.
void source_biped_death_transition_1_1(
    SourcePhysicsContactMemory &, const SourceBipedDeathTransitionRequest &,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint,
    const SourceBipedDeathTransitionServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
