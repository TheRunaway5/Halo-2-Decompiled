#pragma once
#include "halo2/native_tu15_array_growth.hpp"

namespace halo2 {
// Actual2E10A0/2E1110 slow-path ownership. Each backing allocation starts
// with the previous arena's four DATA words; neither owner copies control
// stack bytes or supplies a platform allocation outcome.
std::uint32_t native_tu15_allocate_stack_arena(SourcePhysicsContactMemory&,
    std::uint32_t arena,std::uint32_t requested_bytes,NativeTu15PhysicsFrameFp&,
    const NativeTu15ArrayGrowthServices&,const SourcePhysicsContactRecordObserver& = {});
// The original first formal block argument is unused: the current arena
// boundary selects the backing allocation that is restored and released.
void native_tu15_release_stack_arena(SourcePhysicsContactMemory&,
    std::uint32_t arena,std::uint32_t block,std::uint32_t requested_bytes,
    NativeTu15PhysicsFrameFp&,const NativeTu15ArrayGrowthServices&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
