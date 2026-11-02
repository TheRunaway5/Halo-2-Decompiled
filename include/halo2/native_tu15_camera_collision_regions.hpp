#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
// Actual1F7E90. Iterator is ordinary caller-owned28B DATA; +18 is initialized
// by the first advance before its use, exactly as in the source lifetime.
void native_tu15_begin_camera_collision_regions(SourcePhysicsContactMemory&,
    std::uint32_t collision,std::uint32_t region_state,std::uint32_t iterator,
    std::uint32_t optional_model,const SourcePhysicsContactRecordObserver& = {});
// Actual1F7F60. Publishes the encoded region/permutation/BSP key and its DATA
// descriptor pointer; exhaustion preserves the source post-store reread.
bool native_tu15_next_camera_collision_region(SourcePhysicsContactMemory&,
    std::uint32_t iterator,const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
