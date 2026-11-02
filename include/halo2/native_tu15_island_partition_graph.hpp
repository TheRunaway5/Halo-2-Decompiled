#pragma once
#include "halo2/physics_contact_runtime.hpp"
namespace halo2 {
struct NativeTu15IslandPartitionGraph {std::uint32_t identity{};};
struct NativeTu15IslandPartitionGraphStats {
  std::uint64_t constructors{},unions{},compressions{},parent_publications{};
};
// Actual37C170 points at ordinary source array headers. It deliberately
// preserves the third workspace word, which this constructor does not write.
NativeTu15IslandPartitionGraph native_tu15_construct_island_partition_graph(
  SourcePhysicsContactMemory&,NativeTu15IslandPartitionGraph,std::uint32_t parents_header,
  std::uint32_t edge_roots_header,const SourcePhysicsContactRecordObserver& = {},
  NativeTu15IslandPartitionGraphStats* = nullptr);
// Actual37C190 joins by source root index, retaining negative component sizes.
// Input forests/indices are source DATA; cycles and invalid extents are not
// repaired or normalized. Return the ordinary minimum surviving root index.
std::uint32_t native_tu15_union_island_partition_vertices(SourcePhysicsContactMemory&,
  NativeTu15IslandPartitionGraph,std::uint32_t first,std::uint32_t second,
  const SourcePhysicsContactRecordObserver& = {},NativeTu15IslandPartitionGraphStats* = nullptr);
// Actual37C250 compresses both the parent forest and per-edge root indices.
void native_tu15_compress_island_partition_graph(SourcePhysicsContactMemory&,
  NativeTu15IslandPartitionGraph,const SourcePhysicsContactRecordObserver& = {},
  NativeTu15IslandPartitionGraphStats* = nullptr);
} // namespace halo2
