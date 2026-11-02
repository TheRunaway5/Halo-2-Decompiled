#include "halo2/native_tu15_raw_allocator.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
template<class F>void require(const F&f,NativeTu15RawAllocatorStats*stats,const char*why){if(!f){if(stats)++stats->unbound_services;throw std::runtime_error(why);}}
}
std::uint32_t native_tu15_route_raw_allocation(NativeTu15ArrayAllocatorStorage&s,std::uint32_t bytes,
 std::uint32_t memory_class,NativeTu15PhysicsFrameFp&fp,const NativeTu15RawAllocatorServices&services,
 NativeTu15RawAllocatorStats*stats){
  if(stats)++stats->allocations;
  const auto allocate=[&](std::uint32_t heap){const auto owner=s.owner(heap,false);
    require(services.allocate_owner,stats,"Native TU1.5 raw allocator requires actual allocation owner");
    return services.allocate_owner({heap,owner,bytes,memory_class},fp);
  };
  if(s.mode()-1u==0){if(stats)++stats->fixed_attempts;return allocate(s.heap(NativeTu15ArrayHeap::fixed));}
  if(stats)++stats->primary_attempts;auto data=allocate(s.heap(NativeTu15ArrayHeap::primary));if(data)return data;
  if(stats)++stats->secondary_attempts;data=allocate(s.heap(NativeTu15ArrayHeap::secondary));if(data)return data;
  require(services.default_heap,stats,"Native TU1.5 raw allocator requires actual149930 lazy default owner");
  if(stats)++stats->default_attempts;return allocate(services.default_heap(fp));
}
void native_tu15_route_raw_release(NativeTu15ArrayAllocatorStorage&s,std::uint32_t data,
 NativeTu15PhysicsFrameFp&fp,const NativeTu15RawAllocatorServices&services,NativeTu15RawAllocatorStats*stats){
  if(stats)++stats->releases;if(!data){if(stats)++stats->null_releases;return;}
  const auto release=[&](std::uint32_t heap){const auto owner=s.owner(heap,true);
    require(services.release_owner,stats,"Native TU1.5 raw allocator requires actual release owner");
    services.release_owner({heap,owner,data},fp);
  };
  auto heap=s.heap(NativeTu15ArrayHeap::primary);
  if(!less(data,heap+0x4340u)&&!less(heap+0xf0340u,data)){if(stats)++stats->primary_attempts;release(heap);return;}
  heap=s.heap(NativeTu15ArrayHeap::fixed);
  if(!less(data,heap+0x30u)&&!less(heap+0x15030u,data)){if(stats)++stats->fixed_attempts;release(heap);return;}
  heap=s.heap(NativeTu15ArrayHeap::secondary);const auto start=s.secondary_start(heap);
  if(!less(data,start)){const auto size=s.secondary_size(heap);if(!less(start+size,data)){if(stats)++stats->secondary_attempts;release(heap);return;}}
  require(services.default_heap,stats,"Native TU1.5 raw release requires actual149930 lazy default owner");
  if(stats)++stats->default_attempts;release(services.default_heap(fp));
}
namespace {
std::uint32_t raw_word(SourcePhysicsContactMemory&m,std::uint32_t p){std::array<std::byte,4>b{};m.read(p,b);std::uint32_t v{};for(unsigned n=0;n<4;++n)v|=std::to_integer<unsigned>(b[n])<<(8u*n);return v;}
void raw_store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,std::uint32_t site,std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(v>>(8u*n));SourcePhysicsContactRecordStore e{site,p,{},{}};if(o){e.before.resize(4);m.read(p,e.before);e.written.assign(b.begin(),b.end());}m.write(p,b);if(o)o(e);}
}
std::uint32_t native_tu15_allocate_tagged_raw_block(SourcePhysicsContactMemory&m,std::uint32_t allocator,
 std::uint32_t bytes,std::uint32_t memory_class,NativeTu15PhysicsFrameFp&fp,const NativeTu15ArrayGrowthServices&s,
 const SourcePhysicsContactRecordObserver&o,NativeTu15RawAllocatorStats*stats){
  if(stats)++stats->tagged_allocations;const auto table=raw_word(m,allocator);
  const NativeTu15ArrayAllocationRequest request{allocator,raw_word(m,table+0x10u),bytes+16u,memory_class};
  require(s.allocate,stats,"Native TU1.5 tagged raw allocation requires actual array owner");const auto block=s.allocate(request,fp);if(!block)return 0;
  raw_store(m,o,0x233178u,block+4u,bytes);raw_store(m,o,0x23317bu,block+8u,memory_class);raw_store(m,o,0x23317fu,block,0x02345656u);return block+16u;
}
void native_tu15_release_tagged_raw_block(SourcePhysicsContactMemory&m,std::uint32_t allocator,
 std::uint32_t data,NativeTu15PhysicsFrameFp&fp,const NativeTu15ArrayGrowthServices&s,
 const SourcePhysicsContactRecordObserver&o,NativeTu15RawAllocatorStats*stats){
  if(stats)++stats->tagged_releases;const auto memory_class=raw_word(m,data-8u),block=data-16u,bytes=raw_word(m,block+4u)+16u;
  raw_store(m,o,0x2331b3u,block,0xdeadbeefu);const auto table=raw_word(m,allocator);
  const NativeTu15ArrayReleaseRequest request{allocator,raw_word(m,table+0x14u),block,bytes,memory_class};
  require(s.release,stats,"Native TU1.5 tagged raw release requires actual array owner");s.release(request,fp);
}
NativeTu15RawAllocatorMemory::NativeTu15RawAllocatorMemory(SourcePhysicsContactMemory&m):memory_(m){}
std::uint32_t NativeTu15RawAllocatorMemory::read(std::uint32_t p)const{std::array<std::byte,4>b{};memory_.read(p,b);std::uint32_t v{};for(unsigned n=0;n<4;++n)v|=std::to_integer<unsigned>(b[n])<<(8u*n);return v;}
std::uint32_t NativeTu15RawAllocatorMemory::mode()const{return read(0x486064u);}
std::uint32_t NativeTu15RawAllocatorMemory::heap(NativeTu15ArrayHeap which)const{
  switch(which){case NativeTu15ArrayHeap::fixed:return read(0x486068u);case NativeTu15ArrayHeap::primary:return read(0x48606cu);case NativeTu15ArrayHeap::secondary:return read(0x486070u);}throw std::runtime_error("Unknown raw heap DATA selection");
}
std::uint32_t NativeTu15RawAllocatorMemory::owner(std::uint32_t heap,bool releasing)const{return read(read(heap)+(releasing?4u:0u));}
std::uint32_t NativeTu15RawAllocatorMemory::secondary_start(std::uint32_t heap)const{return read(heap+0x18u);}
std::uint32_t NativeTu15RawAllocatorMemory::secondary_size(std::uint32_t heap)const{return read(heap+0x1cu);}
} // namespace halo2
