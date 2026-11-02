#pragma once
#include "halo2/native_tu15_activation_agents.hpp"
#include <optional>
namespace halo2 {
// The authenticated constant shape/motion/policy type getters used by the
// activation service cohort. Unknown owners remain explicit dependencies.
std::optional<std::uint32_t> native_tu15_activation_type(std::uint32_t owner) noexcept;
struct NativeTu15ActivationDiscreteServices {
    std::function<std::uint32_t(std::uint32_t filter,NativeTu15ContactVirtualOwner,
        NativeTu15ContactPayload,NativeTu15ContactPayload,NativeTu15PhysicsFrameFp&)>test_filter;
};
struct NativeTu15ActivationBodyServices {
    std::function<void(std::uint32_t body_datum,NativeTu15PhysicsFrameFp&)>publish_body_update; //22D460
};
struct NativeTu15ActivationServiceStats {
    std::uint64_t filter_calls{},same_group_rejections{},mask_queries{},discrete_calls{},
        body_listener_calls{},object_wake_calls{},object_headers_marked{},body_updates{};
};
// Actual2E9A00. Preserve the second/first group read order, same authored
// nonzero group rejection, x86 shift masking, matrix lookup and byte result.
std::uint32_t native_tu15_activation_group_filter(SourcePhysicsContactMemory&,
    std::uint32_t filter,std::uint32_t result,NativeTu15ContactPayload,
    NativeTu15ContactPayload,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ActivationServiceStats* = nullptr);
// Actual281530: the wrapper delegates its two payloads to the actual current
// filter and publishes only AL. The returned identity is the result byte.
std::uint32_t native_tu15_activation_discrete_pair(SourcePhysicsContactMemory&,
    std::uint32_t dispatcher,std::uint32_t result,NativeTu15ContactPayload,
    NativeTu15ContactPayload,NativeTu15PhysicsFrameFp&,
    const NativeTu15ActivationDiscreteServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ActivationServiceStats* = nullptr);
// ActualB9390: mark the object's original header and each ancestor through
// original datum storage. It reads the first flag before testing datum -1.
void native_tu15_wake_object_ancestors(SourcePhysicsContactMemory&,std::uint32_t object_datum,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ActivationServiceStats* = nullptr);
// Actual2816A0: source property search/body flag publication, native object
// wake, and the genuine body-update owner. A missing property yields datum0
// in the source; do not replace it with an invented no-op admission.
void native_tu15_body_activation_listener(SourcePhysicsContactMemory&,NativeTu15PhysicsBody,
    NativeTu15PhysicsFrameFp&,const NativeTu15ActivationBodyServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ActivationServiceStats* = nullptr);
} // namespace halo2
