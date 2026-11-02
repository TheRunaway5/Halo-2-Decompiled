#pragma once
#include "halo2/native_tu15_activation_payload_filter.hpp"
#include "halo2/native_tu15_array_growth.hpp"
#include "halo2/native_tu15_island_transition.hpp"
namespace halo2 {
struct NativeTu15BodyDeactivationListenerServices {
    NativeTu15ArrayGrowthServices allocation;
    // Actual1D7260: component is its stack argument, index its public EAX input.
    // The native callback receives those DATA values without a virtual CPU ABI.
    std::function<void(std::uint32_t component,std::uint32_t index,
        NativeTu15PhysicsFrameFp&)>publish_component_body_motion;
};
struct NativeTu15BodyDeactivationListenerStats {
    std::uint64_t calls{},deferred_bodies{},array_growths{},property_queries{},
        missing_datums{},component_motion_publications{},unbound_services{};
};
// Actual2815F0, including deferred reference/queue order and fresh component
// count reads after each real publication. Unknown publication remains typed.
void native_tu15_body_deactivation_listener(SourcePhysicsContactMemory&,
    NativeTu15PhysicsListener,NativeTu15PhysicsBody,NativeTu15PhysicsFrameFp&,
    const NativeTu15BodyDeactivationListenerServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15BodyDeactivationListenerStats* = nullptr);
}
