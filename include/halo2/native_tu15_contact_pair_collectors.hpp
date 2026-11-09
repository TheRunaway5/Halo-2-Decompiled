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
// Actual1CDFA0, the4615B8 collector reached by genuine object-body overlap.
// Only the second descriptor is read. Preserve its root,2001 component,
// current shape-user-data and excluded-component predicates before writing.
// No FP arithmetic or callback result is supplied.
void native_tu15_collect_object_overlap_pair(SourcePhysicsContactMemory&,
    NativeTu15ContactQueryCollector,NativeTu15PhysicsBody second,
    const SourcePhysicsContactRecordObserver& = {});
}
