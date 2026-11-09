#pragma once
#include "halo2/native_tu15_array_allocator.hpp"
namespace halo2 {
struct NativeTu15RawAllocationRequest {std::uint32_t receiver{},owner{},bytes{},memory_class{};};
struct NativeTu15RawReleaseRequest {std::uint32_t receiver{},owner{},data{};};
struct NativeTu15RawAllocatorServices {
  std::function<std::uint32_t(const NativeTu15RawAllocationRequest&,NativeTu15PhysicsFrameFp&)>allocate_owner;
  std::function<void(const NativeTu15RawReleaseRequest&,NativeTu15PhysicsFrameFp&)>release_owner;
  std::function<std::uint32_t(NativeTu15PhysicsFrameFp&)>default_heap;
  std::function<std::uint32_t(std::uint32_t receiver,std::uint32_t owner,
    NativeTu15PhysicsFrameFp&)>allocate_tail_owner;
};
struct NativeTu15RawAllocatorStats {std::uint64_t allocations{},releases{},null_releases{},
  tagged_allocations{},tagged_releases{},primary_attempts{},secondary_attempts{},default_attempts{},fixed_attempts{},unbound_services{};};
// Entire149BF0/149C40 routing. Heap methods are the actual raw virtual0/4
// owners, distinct from array149DA0/149E00's typed size/class methods.
std::uint32_t native_tu15_route_raw_allocation(NativeTu15ArrayAllocatorStorage&,std::uint32_t bytes,
  std::uint32_t memory_class,NativeTu15PhysicsFrameFp&,const NativeTu15RawAllocatorServices&,
  NativeTu15RawAllocatorStats* = nullptr);
std::uint32_t native_tu15_route_raw_allocation_delayed(NativeTu15ArrayAllocatorStorage&,
  const NativeTu15ArrayAllocatorArguments&,NativeTu15PhysicsFrameFp&,const NativeTu15RawAllocatorServices&,
  NativeTu15RawAllocatorStats* = nullptr);
void native_tu15_route_raw_release(NativeTu15ArrayAllocatorStorage&,std::uint32_t data,
  NativeTu15PhysicsFrameFp&,const NativeTu15RawAllocatorServices&,NativeTu15RawAllocatorStats* = nullptr);
// Actual233160/2331A0 raw-block header wrappers. Preserve the unwritten
// fourth header word, real allocation result, class and size, and release tag.
enum class NativeTu15RawTaggedPhase {allocation_table,allocation_owner_inputs,
  allocation_result,allocation_magic,release_class_inputs,release_class,
  release_size_inputs,release_bytes,release_table};
struct NativeTu15RawTaggedPhaseData {
  std::uint32_t table{},data{},block{},bytes{},memory_class{},size_word{};
};
struct NativeTu15RawTaggedPhases {
  std::function<void(NativeTu15RawTaggedPhase,const NativeTu15RawTaggedPhaseData&)>observe;
};
std::uint32_t native_tu15_allocate_tagged_raw_block_with_phases(SourcePhysicsContactMemory&,
  std::uint32_t allocator,const NativeTu15ArrayAllocatorArguments&,NativeTu15PhysicsFrameFp&,
  const NativeTu15ArrayGrowthServices&,const NativeTu15RawTaggedPhases&,
  const SourcePhysicsContactRecordObserver& = {},NativeTu15RawAllocatorStats* = nullptr);
void native_tu15_release_tagged_raw_block_with_phases(SourcePhysicsContactMemory&,
  std::uint32_t allocator,const std::function<std::uint32_t()>&acquire_data,NativeTu15PhysicsFrameFp&,
  const NativeTu15ArrayGrowthServices&,const NativeTu15RawTaggedPhases&,
  const SourcePhysicsContactRecordObserver& = {},NativeTu15RawAllocatorStats* = nullptr);
std::uint32_t native_tu15_allocate_tagged_raw_block(SourcePhysicsContactMemory&,std::uint32_t allocator,
  std::uint32_t bytes,std::uint32_t memory_class,NativeTu15PhysicsFrameFp&,
  const NativeTu15ArrayGrowthServices&,const SourcePhysicsContactRecordObserver& = {},
  NativeTu15RawAllocatorStats* = nullptr);
void native_tu15_release_tagged_raw_block(SourcePhysicsContactMemory&,std::uint32_t allocator,
  std::uint32_t data,NativeTu15PhysicsFrameFp&,const NativeTu15ArrayGrowthServices&,
  const SourcePhysicsContactRecordObserver& = {},NativeTu15RawAllocatorStats* = nullptr);
class NativeTu15RawAllocatorMemory final:public NativeTu15ArrayAllocatorStorage {
  SourcePhysicsContactMemory&memory_;std::uint32_t read(std::uint32_t)const;
public:
  explicit NativeTu15RawAllocatorMemory(SourcePhysicsContactMemory&);
  std::uint32_t mode()const override;
  std::uint32_t heap(NativeTu15ArrayHeap)const override;
  std::uint32_t owner(std::uint32_t,bool)const override;
  std::uint32_t secondary_start(std::uint32_t)const override;
  std::uint32_t secondary_size(std::uint32_t)const override;
};
} // namespace halo2
