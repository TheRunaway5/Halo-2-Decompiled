#pragma once
#include "halo2/native_tu15_contact_creation_listeners.hpp"
#include "halo2/native_tu15_array_growth.hpp"

namespace halo2 {
struct NativeTu15ContactConstraintRegistrationServices {
    NativeTu15ArrayGrowthServices allocation;
    std::function<void(std::uint32_t constraint,NativeTu15ContactVirtualOwner,
        std::uint32_t info,NativeTu15PhysicsFrameFp&)> get_info;
    std::function<void(std::uint32_t island,NativeTu15ContactVirtualOwner,
        std::uint32_t constraint,std::uint32_t info,NativeTu15PhysicsFrameFp&)> add_info;
    std::function<void(std::uint32_t world,std::uint32_t island,
        NativeTu15PhysicsFrameFp&)> activate_island;
    NativeTu15ContactCreationListenerServices listeners;
};
struct NativeTu15ContactConstraintRegistrationStats {
    std::uint64_t attempts{},completed{},refusals{},array_growths{},info_queries{},
        metadata_updates{},merge_requests{},activation_requests{};
    NativeTu15ContactCreationListenerStats notifications;
};
// Actual320E40. info16 is independent caller-owned DATA, fully initialized by
// the selected constraint virtual28 before the island virtual8 consumes it.
void native_tu15_register_island_contact_constraint(SourcePhysicsContactMemory&,
    std::uint32_t island,std::uint32_t constraint,std::uint32_t info,
    NativeTu15PhysicsFrameFp&,const NativeTu15ContactConstraintRegistrationServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ContactConstraintRegistrationStats* = nullptr);
// Actual317350. Body/island identities are current source DATA cached at their
// true phases. Queue growth and callbacks retain their independent native owners.
std::uint32_t native_tu15_register_world_contact_constraint(SourcePhysicsContactMemory&,
    std::uint32_t world,std::uint32_t constraint,std::uint32_t info,
    NativeTu15PhysicsFrameFp&,const NativeTu15ContactConstraintRegistrationServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ContactConstraintRegistrationStats* = nullptr);
} // namespace halo2
