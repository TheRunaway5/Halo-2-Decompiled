#pragma once
#include "halo2/native_tu15_body_properties.hpp"
#include "halo2/native_tu15_island_activation.hpp"
#include <map>
namespace halo2 {
struct NativeTu15ComponentResetScratch {
    std::uint32_t removed_component{},removed_slot{}; // Two caller-owned4B DATA words.
};
struct NativeTu15ComponentResetServices {
    NativeTu15ArrayGrowthServices allocation;
    std::function<void(NativeTu15PhysicsBody,NativeTu15PhysicsVirtualOwner,
        std::uint32_t disposing,NativeTu15PhysicsFrameFp&,unsigned)> destroy_body;
};
struct NativeTu15ComponentResetStats {
    std::uint64_t calls{},references{},protected_resets{},body_destructions{},
        property_removals{},property_publications{},record_releases{},unbound_services{};
    std::map<std::uint32_t,std::uint64_t> unbound_owners;
};
// Actual1D8470. Reuses sole283450 reservation predicate and31C450/31C5E0
// property owners. Caller result words are ordinary DATA, never source ESP.
// Current source counts/pointers are reloaded after native lifetime services.
void native_tu15_reset_physics_component(SourcePhysicsContactMemory&,
    std::uint32_t component,NativeTu15ComponentResetScratch,NativeTu15PhysicsFrameFp&,
    unsigned actual_preserved_x87_depth,const NativeTu15ComponentResetServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ComponentResetStats* = nullptr);
} // namespace halo2
