#pragma once
#include "halo2/native_tu15_conditional_contact_process.hpp"

namespace halo2 {
struct NativeTu15ContactPairCollectorServices {
    // Current collector virtual+4, thiscall2/RET8. Both arguments are actual
    // current body descriptors; a callback result is never supplied.
    std::function<void(NativeTu15ContactQueryCollector,NativeTu15PhysicsVirtualOwner,
        NativeTu15PhysicsBody first,NativeTu15PhysicsBody second,
        NativeTu15PhysicsFrameFp&)>emit_pair;
};
// Actual30BA80 ignores both ordinary descriptor inputs and sets byte+4.
void native_tu15_mark_contact_pair(SourcePhysicsContactMemory&,
    NativeTu15ContactQueryCollector,const SourcePhysicsContactRecordObserver& = {});
// Actual30B960 rereads receiver+8, its current table/virtual+4, then forwards
// second,first. It transforms no normal/vector and writes no result itself.
void native_tu15_forward_reversed_contact_pair(SourcePhysicsContactMemory&,
    NativeTu15ContactQueryCollector,NativeTu15PhysicsBody first,
    NativeTu15PhysicsBody second,NativeTu15PhysicsFrameFp&,
    const NativeTu15ContactPairCollectorServices&);
}
