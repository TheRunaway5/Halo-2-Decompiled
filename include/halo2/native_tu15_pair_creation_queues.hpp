#pragma once
#include "halo2/native_tu15_array_reserve.hpp"

namespace halo2 {
struct NativeTu15BodyPairInputs {
    // Current typed formal DATA, read at the source's post-reservation and
    // post-first-publication phases. They supply no allocation outcome.
    std::function<std::uint32_t()> first,second;
};
// Actual317200. Append an eight-byte body-pair merge request at world+20.
void native_tu15_queue_body_pair_merge(SourcePhysicsContactMemory&,std::uint32_t world,
    const NativeTu15BodyPairInputs&,NativeTu15PhysicsFrameFp&,
    const NativeTu15ArrayGrowthServices&,const SourcePhysicsContactRecordObserver& = {});
void native_tu15_queue_body_pair_merge(SourcePhysicsContactMemory&,std::uint32_t world,
    std::uint32_t first,std::uint32_t second,NativeTu15PhysicsFrameFp&,
    const NativeTu15ArrayGrowthServices&,const SourcePhysicsContactRecordObserver& = {});
// Actual320DA0. Append the current two handle words to island+74. The pair
// input is read after genuine growth, and its second word after the first store.
void native_tu15_defer_collision_pair(SourcePhysicsContactMemory&,std::uint32_t island,
    const std::function<std::uint32_t()>& pair,NativeTu15PhysicsFrameFp&,
    const NativeTu15ArrayGrowthServices&,const SourcePhysicsContactRecordObserver& = {});
void native_tu15_defer_collision_pair(SourcePhysicsContactMemory&,std::uint32_t island,
    std::uint32_t pair,NativeTu15PhysicsFrameFp&,
    const NativeTu15ArrayGrowthServices&,const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
