#include "halo2/native_tu15_island_partition_allocator_binding.hpp"
#include "halo2/native_tu15_array_reserve.hpp"
#include "halo2/native_tu15_stack_arena.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
NativeTu15IslandPartitionAllocatorBinding::NativeTu15IslandPartitionAllocatorBinding(
 SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,NativeTu15IslandPartitionAllocatorActivity*a)
 :memory_(m),observer_(o),activity_(a){}
[[noreturn]]void NativeTu15IslandPartitionAllocatorBinding::unbound(const char*why){if(activity_)++activity_->unbound_services;throw std::runtime_error(why);}
std::uint32_t NativeTu15IslandPartitionAllocatorBinding::pool_allocate(const NativeTu15ArrayAllocationRequest&r,NativeTu15PhysicsFrameFp&){
 if(r.owner!=0x233250u)unbound("Native TU1.5 partition requires a qualified pool allocation owner");
 NativeTu15PoolMemory pool(memory_,r.receiver,observer_);const auto data=native_tu15_pool_allocate(pool,r.bytes);if(activity_)++activity_->pool_allocations;return data;
}
void NativeTu15IslandPartitionAllocatorBinding::pool_release(const NativeTu15ArrayReleaseRequest&r,NativeTu15PhysicsFrameFp&){
 if(r.owner!=0x233420u||std::bit_cast<std::int32_t>(r.bytes)>0x2000)unbound("Native TU1.5 partition requires qualified pool release owner/size");
 NativeTu15PoolMemory pool(memory_,r.receiver,observer_);if(!native_tu15_pool_release(pool,r.data,r.bytes))unbound("Native TU1.5 partition pool release domain is unsupported");if(activity_)++activity_->pool_releases;
}
NativeTu15ArrayGrowthServices NativeTu15IslandPartitionAllocatorBinding::array_services(){
 NativeTu15ArrayGrowthServices s;
 s.allocate=[this](const auto&r,auto&fp){if(r.owner!=0x149da0u)unbound("Native TU1.5 partition requires actual149DA0 array allocation");NativeTu15ArrayAllocatorMemory allocator(memory_);NativeTu15ArrayAllocatorServices heap;
   heap.allocate_owner=[this](const auto&r,auto&f){return pool_allocate(r,f);};heap.default_heap=[this](auto&)->std::uint32_t{unbound("Native TU1.5 partition lazy default heap requires its native owner");};
   const auto data=native_tu15_route_array_allocation(allocator,r.bytes,r.memory_class,fp,heap);if(activity_)++activity_->array_allocations;return data;};
 s.release=[this](const auto&r,auto&fp){if(r.owner!=0x149e00u)unbound("Native TU1.5 partition requires actual149E00 array release");NativeTu15ArrayAllocatorMemory allocator(memory_);NativeTu15ArrayAllocatorServices heap;
   heap.release_owner=[this](const auto&r,auto&f){pool_release(r,f);};heap.default_heap=[this](auto&)->std::uint32_t{unbound("Native TU1.5 partition lazy default heap requires its native owner");};
   native_tu15_route_array_release(allocator,r.data,r.bytes,r.memory_class,fp,heap);if(activity_)++activity_->array_releases;};return s;
}
NativeTu15RawAllocatorServices NativeTu15IslandPartitionAllocatorBinding::raw_services(){
 NativeTu15RawAllocatorServices raw;
 raw.allocate_owner=[this](const auto&r,auto&fp){if(r.owner!=0x233160u)unbound("Native TU1.5 partition requires actual233160 tagged raw allocation");NativeTu15ArrayGrowthServices heap;heap.allocate=[this](const auto&r,auto&f){return pool_allocate(r,f);};return native_tu15_allocate_tagged_raw_block(memory_,r.receiver,r.bytes,r.memory_class,fp,heap,observer_,activity_?&activity_->raw:nullptr);};
 raw.release_owner=[this](const auto&r,auto&fp){if(r.owner!=0x2331a0u)unbound("Native TU1.5 partition requires actual2331A0 tagged raw release");NativeTu15ArrayGrowthServices heap;heap.release=[this](const auto&r,auto&f){pool_release(r,f);};native_tu15_release_tagged_raw_block(memory_,r.receiver,r.data,fp,heap,observer_,activity_?&activity_->raw:nullptr);};
 raw.default_heap=[this](auto&)->std::uint32_t{unbound("Native TU1.5 partition raw lazy default heap requires its native owner");};return raw;
}
NativeTu15IslandLifetimeServices NativeTu15IslandPartitionAllocatorBinding::lifetime_services(){
 NativeTu15IslandLifetimeServices s;s.allocation=array_services();
 s.allocate_arena=[this](auto arena,auto owner,auto bytes,auto&fp){if(owner.identity!=0x2e10a0u)unbound("Native TU1.5 partition requires actual2E10A0 stack arena allocation");const auto data=native_tu15_allocate_stack_arena(memory_,arena,bytes,fp,array_services(),observer_);if(activity_)++activity_->arena_allocations;return data;};
 s.release_arena=[this](auto arena,auto owner,auto data,auto bytes,auto&fp){if(owner.identity!=0x2e1110u)unbound("Native TU1.5 partition requires actual2E1110 stack arena release");native_tu15_release_stack_arena(memory_,arena,data,bytes,fp,array_services(),observer_);if(activity_)++activity_->arena_releases;};return s;
}
NativeTu15IslandPartitionBuildServices NativeTu15IslandPartitionAllocatorBinding::graph_services(){
 NativeTu15IslandPartitionBuildServices s;
 s.reserve_array=[this](auto header,auto capacity,auto stride,auto&fp){native_tu15_reserve_array(memory_,header,capacity,stride,fp,array_services(),observer_);};
 s.allocate_group=[this](auto,auto owner,auto bytes,auto memory_class,auto&fp){if(owner.identity!=0x149bf0u)unbound("Native TU1.5 partition requires actual149BF0 raw route");NativeTu15RawAllocatorMemory allocator(memory_);return native_tu15_route_raw_allocation(allocator,bytes,memory_class,fp,raw_services(),activity_?&activity_->raw:nullptr);};
 s.release_group=[this](auto,auto owner,auto data,auto&fp){if(owner.identity!=0x149c40u)unbound("Native TU1.5 partition requires actual149C40 raw release route");NativeTu15RawAllocatorMemory allocator(memory_);native_tu15_route_raw_release(allocator,data,fp,raw_services(),activity_?&activity_->raw:nullptr);};
 s.release_array=[this](const auto&r,auto&fp){array_services().release(r,fp);};return s;
}
} // namespace halo2
