#pragma once
#include "halo2/native_tu15_activation_agents.hpp"
#include "halo2/native_tu15_array_growth.hpp"
namespace halo2 {
struct NativeTu15ActivationContactManagerServices {
    NativeTu15ArrayGrowthServices allocation;
    std::function<void(NativeTu15PhysicsBody,NativeTu15PhysicsVirtualOwner,
        std::uint32_t disposing,NativeTu15PhysicsFrameFp&)>destroy_body;
    std::function<void(std::uint32_t object,std::uint32_t flags,
        NativeTu15PhysicsFrameFp&)>release_base_lock; // Actual 2E12A0.
};
struct NativeTu15ActivationContactManagerStats {
    std::uint64_t factories{},constructors{},collectors{},registrations{},
        unregistrations{},deferred_references{},array_growths{},body_destructions{},
        protected_references{},storage_releases{},collector_destructions{},base_lock_releases{};
};
// Actual 3232E0 and its constructor/listener-registration dependency family.
// Allocation, release and any true body destructor retain typed ownership.
NativeTu15ActivationContactManager native_tu15_create_activation_contact_manager(
    SourcePhysicsContactMemory&,std::uint32_t factory,
    NativeTu15ContactPayload,NativeTu15ContactPayload,NativeTu15PhysicsFrameFp&,
    const NativeTu15ActivationContactManagerServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ActivationContactManagerStats* = nullptr);
} // namespace halo2
