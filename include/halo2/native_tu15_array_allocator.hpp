#pragma once
#include "halo2/native_tu15_array_growth.hpp"
namespace halo2 {
enum class NativeTu15PoolCounter {in_use,free_blocks,total_blocks,pages,reservations};
enum class NativeTu15PoolSmallField {start,end,cursor};
class NativeTu15PoolStorage {
public:
  virtual ~NativeTu15PoolStorage()=default;
  virtual std::uint32_t identity()const=0;
  virtual std::uint32_t small_class(std::uint32_t bytes)const=0;
  virtual std::uint32_t large_class(std::uint32_t bytes)const=0;
  virtual std::uint32_t free_head(std::uint32_t index)const=0;
  virtual std::uint32_t class_size(std::uint32_t index)const=0;
  virtual std::uint32_t counter(NativeTu15PoolCounter,std::uint32_t index=0)const=0;
  virtual std::uint32_t small(NativeTu15PoolSmallField)const=0;
  virtual std::uint32_t link(std::uint32_t data)const=0;
  virtual void free_head(std::uint32_t index,std::uint32_t,std::uint32_t publication)=0;
  virtual void counter(NativeTu15PoolCounter,std::uint32_t index,std::uint32_t,std::uint32_t publication)=0;
  virtual void small(NativeTu15PoolSmallField,std::uint32_t,std::uint32_t publication)=0;
  virtual void link(std::uint32_t data,std::uint32_t,std::uint32_t publication)=0;
};
// Complete actual233250 pool allocation. Source byte counts are compared as
// signed32, address/products wrap as32bits, class lookup/metadata are retained
// inputs, and exhausted source pages return the original null result.
std::uint32_t native_tu15_pool_allocate(NativeTu15PoolStorage&,std::uint32_t bytes);
// Actual233420's <=2000 source branch. Its >2000 imported free owner remains
// explicitly required; no platform free or allocator result is guessed.
bool native_tu15_pool_release(NativeTu15PoolStorage&,std::uint32_t data,std::uint32_t bytes);
enum class NativeTu15ArrayHeap {fixed,primary,secondary};
class NativeTu15ArrayAllocatorStorage {
public:
  virtual ~NativeTu15ArrayAllocatorStorage()=default;
  virtual std::uint32_t mode()const=0;
  virtual std::uint32_t heap(NativeTu15ArrayHeap)const=0;
  virtual std::uint32_t owner(std::uint32_t receiver,bool releasing)const=0;
  virtual std::uint32_t secondary_start(std::uint32_t receiver)const=0;
  virtual std::uint32_t secondary_size(std::uint32_t receiver)const=0;
};
struct NativeTu15ArrayAllocatorServices {
  std::function<std::uint32_t(const NativeTu15ArrayAllocationRequest&,NativeTu15PhysicsFrameFp&)> allocate_owner;
  std::function<void(const NativeTu15ArrayReleaseRequest&,NativeTu15PhysicsFrameFp&)> release_owner;
  // Actual149930 remains a distinct owner, including any required lazy heap
  // construction. An initialized pointer may not be supplied as its result.
  std::function<std::uint32_t(NativeTu15PhysicsFrameFp&)> default_heap;
};
std::uint32_t native_tu15_route_array_allocation(NativeTu15ArrayAllocatorStorage&,
  std::uint32_t bytes,std::uint32_t memory_class,NativeTu15PhysicsFrameFp&,
  const NativeTu15ArrayAllocatorServices&);
void native_tu15_route_array_release(NativeTu15ArrayAllocatorStorage&,std::uint32_t data,
  std::uint32_t bytes,std::uint32_t memory_class,NativeTu15PhysicsFrameFp&,
  const NativeTu15ArrayAllocatorServices&);
class NativeTu15PoolMemory final:public NativeTu15PoolStorage {
  SourcePhysicsContactMemory&memory_;std::uint32_t pool_;SourcePhysicsContactRecordObserver observer_;
  std::uint32_t read(std::uint32_t,unsigned bytes=4)const;
  void write(std::uint32_t site,std::uint32_t,std::uint32_t);
  std::uint32_t counter_address(NativeTu15PoolCounter,std::uint32_t)const;
public:
  NativeTu15PoolMemory(SourcePhysicsContactMemory&,std::uint32_t pool,const SourcePhysicsContactRecordObserver& = {});
  std::uint32_t identity()const override;
  std::uint32_t small_class(std::uint32_t)const override;
  std::uint32_t large_class(std::uint32_t)const override;
  std::uint32_t free_head(std::uint32_t)const override;
  std::uint32_t class_size(std::uint32_t)const override;
  std::uint32_t counter(NativeTu15PoolCounter,std::uint32_t=0)const override;
  std::uint32_t small(NativeTu15PoolSmallField)const override;
  std::uint32_t link(std::uint32_t)const override;
  void free_head(std::uint32_t,std::uint32_t,std::uint32_t)override;
  void counter(NativeTu15PoolCounter,std::uint32_t,std::uint32_t,std::uint32_t)override;
  void small(NativeTu15PoolSmallField,std::uint32_t,std::uint32_t)override;
  void link(std::uint32_t,std::uint32_t,std::uint32_t)override;
};
class NativeTu15ArrayAllocatorMemory final:public NativeTu15ArrayAllocatorStorage {
  SourcePhysicsContactMemory&memory_;std::uint32_t read(std::uint32_t)const;
public:
  explicit NativeTu15ArrayAllocatorMemory(SourcePhysicsContactMemory&);
  std::uint32_t mode()const override;
  std::uint32_t heap(NativeTu15ArrayHeap)const override;
  std::uint32_t owner(std::uint32_t,bool)const override;
  std::uint32_t secondary_start(std::uint32_t)const override;
  std::uint32_t secondary_size(std::uint32_t)const override;
};
} // namespace halo2
