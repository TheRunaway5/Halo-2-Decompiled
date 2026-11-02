#pragma once
#include "halo2/native_tu15_gameplay_phantom_membership.hpp"

namespace halo2 {
struct NativeTu15PhantomMembershipPolicy {
    std::uint32_t component_datum{},other_component_datum{},phantom_index{};
};
// Actual1D9FB0, semantic ECX/EDX/EAX ordinary inputs. Return the actual AL
// predicate from authored shape flags, signed group limits and current other
// object type/attachment/flight fields. No admission outcome is supplied.
bool native_tu15_admit_gameplay_phantom_member(SourcePhysicsContactMemory&,
    const NativeTu15PhantomMembershipPolicy&);
struct NativeTu15PhantomMembershipEnter {
    std::uint32_t other_body{},component{},component_datum{},body_slot{},phantom_index{};
};
// Actual1DA0F0: other_body/own datum are semantic EAX/ECX; component, slot,
// phantom are RET12 inputs. Allocate/grow the actual list, publish source
// fields in order and increment the byte reference after the policy stores.
void native_tu15_add_gameplay_phantom_membership(SourcePhysicsContactMemory&,
    const NativeTu15PhantomMembershipEnter&,NativeTu15PhysicsFrameFp&,
    const NativeTu15ArrayGrowthServices&,const SourcePhysicsContactRecordObserver& = {});
// Actual1F34D0, shape virtual+28/RET12. The third ordinary context argument
// is unused by this owner. Both roots/property lookups execute at source phase.
void native_tu15_enter_gameplay_phantom(SourcePhysicsContactMemory&,
    std::uint32_t shape,std::uint32_t self_root,std::uint32_t other_root,
    std::uint32_t context,NativeTu15PhysicsFrameFp&,
    const NativeTu15ArrayGrowthServices&,const SourcePhysicsContactRecordObserver& = {});
}
