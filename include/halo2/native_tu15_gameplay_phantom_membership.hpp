#pragma once
#include "halo2/native_tu15_array_growth.hpp"
#include "halo2/native_tu15_agent_cleanup.hpp"

namespace halo2 {
// Actual1D7530/1D75B0: signed-count8B property lookup. Missing keys return
// zero, not NONE or the address of an entry. No FP or allocation is involved.
std::uint32_t native_tu15_phantom_body_property_2001(SourcePhysicsContactMemory&,std::uint32_t body);
std::uint32_t native_tu15_phantom_body_property_2002(SourcePhysicsContactMemory&,std::uint32_t body);

struct NativeTu15PhantomMembershipLeave {
    std::uint32_t other_body{},component{},component_datum{},body_slot{},phantom_index{};
};
// Actual1DA320: other_body is semantic EAX; the remaining four fields are
// ordinary RET16 inputs. Retain the current signed-byte keys/refcount, swap
// deletion, authored shape policy, optional biped-owner clear, and real12B
// header/backing release. No membership/removal result is supplied.
void native_tu15_remove_gameplay_phantom_membership(SourcePhysicsContactMemory&,
    const NativeTu15PhantomMembershipLeave&,NativeTu15PhysicsFrameFp&,
    const NativeTu15ArrayGrowthServices&,const SourcePhysicsContactRecordObserver& = {});
// Actual1F3550 / shape virtual+2C, RET8(self_root,other_root). Both roots
// must designate actual rigid-body payloads before property owners run.
void native_tu15_leave_gameplay_phantom(SourcePhysicsContactMemory&,
    std::uint32_t shape,std::uint32_t self_root,std::uint32_t other_root,
    NativeTu15PhysicsFrameFp&,const NativeTu15ArrayGrowthServices&,
    const SourcePhysicsContactRecordObserver& = {});
struct NativeTu15PhantomCallbackCleanupServices {
    NativeTu15AgentCleanupServices agents;
    // Current shape virtual+2C, thiscall2/RET8. These are current ordinary
    // roots, not a removal result. Execute the genuine selected owner.
    std::function<void(NativeTu15ContactChild,NativeTu15ContactVirtualOwner,
        NativeTu15ContactPayload self_root,NativeTu15ContactPayload other_root,
        NativeTu15PhysicsFrameFp&)> leave;
};
// Actual2F7790. Re-read the second class and both current roots after the
// first leave callback. Dispose through current virtual0 only after both.
void native_tu15_cleanup_phantom_callback_agent(SourcePhysicsContactMemory&,
    NativeTu15ContactAgent,NativeTu15PhysicsFrameFp&,
    const NativeTu15PhantomCallbackCleanupServices&);
}
