#pragma once
#include "halo2/native_tu15_agent_cleanup.hpp"
#include "halo2/native_tu15_array_growth.hpp"
namespace halo2 {
enum class NativeTu15AgentDestructor {owner_2f8090,owner_3051a0,owner_2fc540};
struct NativeTu15AgentCleanupChildrenServices {
    NativeTu15AgentCleanupServices agents;
    NativeTu15ArrayGrowthServices allocation;
    // Actual current contact-manager virtual8, ordinary RET4(point16).
    std::function<void(std::uint32_t manager,NativeTu15ContactVirtualOwner,
        std::uint16_t point,NativeTu15PhysicsFrameFp&)> remove_point;
};
// Actual30CA60: decrement the current count before removing each48B record's
// point ID. Both the count and manager dispatch are reread after callbacks.
void native_tu15_remove_cached_contact_points(SourcePhysicsContactMemory&,
    std::uint32_t records,std::uint32_t count_field,std::uint32_t manager,
    NativeTu15PhysicsFrameFp&,const NativeTu15AgentCleanupChildrenServices&,
    const SourcePhysicsContactRecordObserver& = {});
// Actual2F82D0/2F6C80; use the same typed virtual cleanup/destroy boundaries
// as the recursive parents. No source stack or register context is retained.
void native_tu15_cleanup_cached_contact_agent(SourcePhysicsContactMemory&,
    NativeTu15ContactAgent,NativeTu15PhysicsFrameFp&,
    const NativeTu15AgentCleanupChildrenServices&,
    const SourcePhysicsContactRecordObserver& = {});
void native_tu15_cleanup_wrapped_contact_agent(SourcePhysicsContactMemory&,
    NativeTu15ContactAgent,NativeTu15PhysicsFrameFp&,
    const NativeTu15AgentCleanupChildrenServices&);
// Actual2F0DA0: clean the current primary child, then reread the optional
// secondary child. Clear that reference only after its cleanup returns, and
// dispatch the current scalar destructor with flag1 after both children.
void native_tu15_cleanup_bv_contact_agent(SourcePhysicsContactMemory&,
    NativeTu15ContactAgent,NativeTu15PhysicsFrameFp&,
    const NativeTu15AgentCleanupChildrenServices&,
    const SourcePhysicsContactRecordObserver& = {});
// Actual2FBB10. The point is zero-extended16-bit DATA; FFFF skips every
// manager read. Dispatch virtual+8 for any other point, then current virtual0
// with flag1. All referenced source tables use the existing2FC540 destructor.
void native_tu15_cleanup_single_point_contact_agent(SourcePhysicsContactMemory&,
    NativeTu15ContactAgent,NativeTu15PhysicsFrameFp&,
    const NativeTu15AgentCleanupChildrenServices&);
// Actual2F7A10; and2F7E90/2F3580 (2F5B30 is its genuine jump thunk).
// Native C++ unwind ownership releases the base at the source arm phase.
void native_tu15_destroy_contact_agent_base(SourcePhysicsContactMemory&,
    NativeTu15ContactAgent,const SourcePhysicsContactRecordObserver& = {});
void native_tu15_destroy_contact_agent_storage(SourcePhysicsContactMemory&,
    bool records_12_bytes,NativeTu15ContactAgent,NativeTu15PhysicsFrameFp&,
    const NativeTu15ArrayGrowthServices&,
    const SourcePhysicsContactRecordObserver& = {});
// Actual scalar deleting destructors. The normal result is the same agent;
// the low-bit flag is read after base destruction. Unknown release owners
// must remain explicit in allocation.release, rather than supplying results.
NativeTu15ContactAgent native_tu15_destroy_contact_agent(SourcePhysicsContactMemory&,
    NativeTu15AgentDestructor,NativeTu15ContactAgent,std::uint32_t disposing,
    NativeTu15PhysicsFrameFp&,const NativeTu15AgentCleanupChildrenServices&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
