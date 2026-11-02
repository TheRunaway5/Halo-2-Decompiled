#pragma once
#include "halo2/native_tu15_phantom_overlap_remove.hpp"

namespace halo2 {
// Actual315100, whose overlap array is at+80/+84. Unlike315A70 it performs
// no collidable shape-word test. The source always sends a removal event.
void native_tu15_remove_simple_phantom_overlap(SourcePhysicsContactMemory&,
    std::uint32_t phantom,NativeTu15ContactPayload other,NativeTu15PhantomOverlapEvent,
    NativeTu15PhysicsFrameFp&,const NativeTu15PhantomOverlapRemoveServices&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
