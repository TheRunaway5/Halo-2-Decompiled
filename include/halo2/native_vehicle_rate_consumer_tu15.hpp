#pragma once
#include "halo2/physics_contact_record.hpp"
#include <functional>
#include <thread>
namespace halo2 {
struct NativeVehicleRateActivityTu15 {
 std::uint64_t acceleration_calls{},approach_calls{},live_cursor_calls{},observation_refusals{};
};
struct NativeVehicleRateConsumerPolicyTu15 {
 std::uint32_t caller_stack_begin{},caller_stack_end{}; // Exclusive end.
 std::function<void(std::uint32_t actual_instruction)> publish_phase;
 const bool*first_trace{};const bool*second_trace{};
 bool observation_only_stores{};
 NativeVehicleRateActivityTu15*world_activity{};
};
// Synchronous native admission is tied to actual retained memory and the
// constructing executor. It does not add instruction-machine capabilities to
// old state headers. Active unsupported contexts must refuse, never fall back.
class NativeVehicleRateConsumerScopeTu15 {
 SourcePhysicsContactMemory*memory_;NativeVehicleRateConsumerPolicyTu15 policy_;
 std::thread::id executor_;NativeVehicleRateConsumerScopeTu15*previous_{};
 friend const NativeVehicleRateConsumerPolicyTu15*native_vehicle_rate_consumer_policy_tu15(const SourcePhysicsContactMemory&);
public:
 NativeVehicleRateConsumerScopeTu15(SourcePhysicsContactMemory&,NativeVehicleRateConsumerPolicyTu15);
 ~NativeVehicleRateConsumerScopeTu15();
 NativeVehicleRateConsumerScopeTu15(const NativeVehicleRateConsumerScopeTu15&)=delete;
 NativeVehicleRateConsumerScopeTu15&operator=(const NativeVehicleRateConsumerScopeTu15&)=delete;
};
const NativeVehicleRateConsumerPolicyTu15*native_vehicle_rate_consumer_policy_tu15(const SourcePhysicsContactMemory&);
NativeVehicleRateActivityTu15 native_vehicle_rate_activity_tu15(const SourcePhysicsContactMemory&);
void native_vehicle_rate_activity_reset_tu15(const SourcePhysicsContactMemory&);
void native_vehicle_rate_call_tu15(const SourcePhysicsContactMemory&,bool approach,bool published_cursor);
void native_vehicle_rate_observation_refused_tu15(const SourcePhysicsContactMemory&);
} // namespace halo2
