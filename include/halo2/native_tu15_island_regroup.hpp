#pragma once
#include "halo2/native_tu15_island_partition_edges.hpp"
#include "halo2/native_tu15_island_partition_temporary.hpp"
#include "halo2/native_tu15_island_construction.hpp"
#include "halo2/native_tu15_island_partition_metadata.hpp"
namespace halo2 {
struct NativeTu15IslandRegroupScratch {
  NativeTu15ArrayHeader edges,parents,edge_roots,groups;
  NativeTu15IslandPartitionGraph graph;
  NativeTu15IslandPartitionEdgeScratch edge_actions;
  NativeTu15ArrayHeader action_bodies;std::uint32_t inline_bodies{};
  NativeTu15IslandConstraintInfo info;
};
struct NativeTu15IslandRegroupServices {
  NativeTu15IslandLifetimeServices lifetime;
  NativeTu15IslandPartitionBuildServices partition;
  NativeTu15IslandPartitionEdgeServices edges;
  std::function<std::uint32_t(std::uint32_t motion,NativeTu15PhysicsVirtualOwner,
    NativeTu15PhysicsFrameFp&)>motion_weight;
  std::function<void(std::uint32_t constraint,NativeTu15PhysicsVirtualOwner,
    NativeTu15IslandConstraintInfo,NativeTu15PhysicsFrameFp&)>constraint_info;
  std::function<void(NativeTu15PhysicsIsland,NativeTu15PhysicsVirtualOwner,std::uint32_t constraint,
    NativeTu15IslandConstraintInfo,NativeTu15PhysicsFrameFp&)>add_info,remove_info;
  std::function<void(NativeTu15PhysicsIsland,NativeTu15PhysicsVirtualOwner,
    std::uint32_t disposing,NativeTu15PhysicsFrameFp&)>destroy_island;
};
struct NativeTu15IslandRegroupStats {
  std::uint64_t calls{},active_visits{},empty_islands{},flag_skips{},fully_connected{},created_islands{},
    moved_bodies{},compacted_bodies{},moved_actions{},moved_constraints{},moved_contacts{},retained_contacts{},
    motion_queries{},native_motion_queries{},info_queries{},native_info_queries{},array_growths{},unbound_services{};
  NativeTu15IslandLifetimeStats lifetime;
  NativeTu15IslandPartitionBuildStats partition;
  NativeTu15IslandPartitionEdgeStats edges;
};
// Complete normal-completion3187C0 connectivity repartition algorithm.
// Every scratch field names caller-owned ordinary DATA. Unknown real owners
// remain counted typed requirements; this parent never replays through a CPU
// backend. Callback fault/unwind prefixes require separate qualification.
void native_tu15_regroup_physics_islands(SourcePhysicsContactMemory&,NativeTu15PhysicsWorld,
  NativeTu15IslandRegroupScratch,NativeTu15PhysicsFrameFp&,const NativeTu15IslandRegroupServices&,
  const SourcePhysicsContactRecordObserver& = {},NativeTu15IslandRegroupStats* = nullptr);
} // namespace halo2
