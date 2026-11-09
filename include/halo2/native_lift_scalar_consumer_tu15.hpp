#pragma once
#include "halo2/physics_contact_record.hpp"
#include "halo2/source_lift_scalar.hpp"
#include <array>
#include <functional>
#include <thread>
namespace halo2 {
struct NativeTu15PhysicsFrameFp;
class NativeTu15RetainedStoreObserver;
enum class NativeTu15LiftVelocityPhase { add_linear, set_angular };
struct NativeTu15LiftVelocityDelivery {
  NativeTu15LiftVelocityPhase phase;
  std::uint32_t component, slot, caller_vector;
  unsigned incoming_x87_depth;
};
using NativeTu15LiftVelocityService = std::function<void(SourcePhysicsContactMemory &,
    const NativeTu15LiftVelocityDelivery &, NativeTu15PhysicsFrameFp &,
    const NativeTu15RetainedStoreObserver &)>;
struct NativeLiftScalarConsumerTu15 {
  std::uint32_t call_site{},entry_stack{},stack_pointer{},scratch_word{},fs_base{},continuation{};
  bool allow_reverse{};
  bool scratch_popped{};
  std::thread::id executor;
  SourceLiftScalarEnvironment floating_point;
  std::uint16_t numeric_status{};
};
struct NativeLiftScalarActivityTu15 {
  std::array<std::uint64_t,4> caller_calls{};
  std::uint64_t power_calls{},errno_writes{},observation_refusals{};
  std::array<std::uint64_t,2> velocity_attempts{}, velocity_completed{}, velocity_observed_stores{};
  std::uint64_t velocity_refusals{};
};
struct NativeLiftScalarConsumerPolicyTu15 {
  std::uint32_t caller_stack_begin{},caller_stack_end{}; // Exclusive end.
  std::function<void(std::uint32_t)> publish_phase;
  const bool* first_trace{};
  const bool* second_trace{};
  bool observation_only_stores{};
  NativeLiftScalarActivityTu15* world_activity{};
  // Typed DATA-only owner of the two direct1DA750 velocity calls. Live scope
  // never falls through to the old executor when this owner is unavailable.
  NativeTu15LiftVelocityService deliver_velocity;
  std::function<std::uint16_t(SourcePhysicsContactMemory &, std::uint32_t object,
      bool direct, std::uint32_t marker, std::int16_t capacity, std::uint32_t destination,
      NativeTu15PhysicsFrameFp &)> request_marker;
};
// An owning native world can admit its real retained caller stack and readonly
// collector without importing legacy machine-dispatch capabilities. The scope
// is synchronous, memory-bound and bound to its constructing native thread.
class NativeLiftScalarConsumerScopeTu15 {
  SourcePhysicsContactMemory* memory_;
  NativeLiftScalarConsumerPolicyTu15 policy_;
  std::thread::id executor_;
  NativeLiftScalarConsumerScopeTu15* previous_{};
  friend const NativeLiftScalarConsumerPolicyTu15* native_lift_scalar_consumer_policy_tu15(const SourcePhysicsContactMemory&);
public:
  NativeLiftScalarConsumerScopeTu15(SourcePhysicsContactMemory&,NativeLiftScalarConsumerPolicyTu15);
  ~NativeLiftScalarConsumerScopeTu15();
  NativeLiftScalarConsumerScopeTu15(const NativeLiftScalarConsumerScopeTu15&)=delete;
  NativeLiftScalarConsumerScopeTu15&operator=(const NativeLiftScalarConsumerScopeTu15&)=delete;
};
const NativeLiftScalarConsumerPolicyTu15* native_lift_scalar_consumer_policy_tu15(const SourcePhysicsContactMemory&);
NativeLiftScalarActivityTu15 native_lift_scalar_activity_tu15();
NativeLiftScalarActivityTu15 native_lift_scalar_activity_tu15(const SourcePhysicsContactMemory&);
void native_lift_scalar_activity_reset_tu15(const SourcePhysicsContactMemory&);
void native_lift_scalar_observation_refused_tu15();
void native_lift_scalar_observation_refused_tu15(const SourcePhysicsContactMemory&);
// Transitional binding at the four declared1DA750 consumer seams. It returns
// the live scalar/ratio and real stack protocol; it does not fabricate unused
// GP/XMM postimages or advertise a full original-instruction observer.
SourceLiftScalarOutcome native_lift_scalar_consumer_tu15(SourcePhysicsContactMemory&,
    NativeLiftScalarConsumerTu15&,const std::function<void(std::uint32_t)>& publish_phase,
    const SourcePhysicsContactRecordObserver& = {});
}
