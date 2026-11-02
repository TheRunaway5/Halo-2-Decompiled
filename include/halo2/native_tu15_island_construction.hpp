#pragma once
#include "halo2/native_tu15_island_integration.hpp"
namespace halo2 {
// Entire3212C0. Initializes source island fields and inline array DATA while
// preserving the allocation size/reference header and other unwritten bytes.
NativeTu15PhysicsIsland native_tu15_construct_physics_island(SourcePhysicsContactMemory&,
  NativeTu15PhysicsIsland,NativeTu15PhysicsWorld,const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
