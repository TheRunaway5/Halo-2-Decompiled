#include "halo2/native_tu15_array_allocator.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
using Counter=NativeTu15PoolCounter;using Small=NativeTu15PoolSmallField;
std::int32_t signed_word(std::uint32_t n){return std::bit_cast<std::int32_t>(n);}
void change(NativeTu15PoolStorage&s,Counter kind,std::uint32_t index,std::uint32_t delta,std::uint32_t site){s.counter(kind,index,s.counter(kind,index)+delta,site);}
}
std::uint32_t native_tu15_pool_allocate(NativeTu15PoolStorage&s,std::uint32_t bytes){
  if(signed_word(bytes)>0x2000)return 0;
  const auto index=signed_word(bytes)<=0x200?s.small_class(bytes):s.large_class(bytes);
  const auto head=s.free_head(index);change(s,Counter::in_use,index,1,0x23328f);
  if(head){const auto next=s.link(head);s.free_head(index,next,0x23329a);change(s,Counter::free_blocks,index,0xffffffffu,0x2332a7);return head;}
  const auto size=s.class_size(index);std::uint32_t result,next,batch;
  if(signed_word(bytes)<=0x200){
    const auto cursor=s.small(Small::cursor),end=s.small(Small::end);
    if(cursor+size>end){
      const auto pages=s.counter(Counter::pages);if(signed_word(pages)>=0x76)return 0;
      const auto page=s.identity()+0x4340u+(pages<<13u);
      s.small(Small::start,page,0x2332ed);s.small(Small::cursor,page,0x2332f0);s.small(Small::end,page+0x1fc0u,0x2332f8);
      const auto reservations=s.counter(Counter::reservations);s.counter(Counter::pages,0,pages+1u,0x233303);s.counter(Counter::reservations,0,reservations+1u,0x233309);
    }
    result=s.small(Small::cursor);next=result+size;s.small(Small::cursor,next,0x23331b);
    const auto in_use=s.counter(Counter::in_use,index);batch=size;
    if(signed_word(size)<0x100){
      while(next+size<s.small(Small::end)){
        change(s,Counter::total_blocks,index,1,0x233338);change(s,Counter::free_blocks,index,1,0x23333f);change(s,Counter::in_use,index,0xffffffffu,0x233346);
        const auto previous=s.free_head(index);s.link(next,previous,0x233351);s.free_head(index,next,0x233353);
        next=s.small(Small::cursor)+size;batch+=size;s.small(Small::cursor,next,0x233364);
        if(signed_word(batch)>=0x100)break;
      }
    }
    s.counter(Counter::in_use,index,in_use,0x23336d);
  }else{
    const auto pages=s.counter(Counter::pages);if(signed_word(pages)>=0x76)return 0;
    const auto reservations=s.counter(Counter::reservations);const auto page_offset=pages<<13u;
    s.counter(Counter::pages,0,pages+1u,0x2333aa);s.counter(Counter::reservations,0,reservations+1u,0x2333b0);
    const auto in_use=s.counter(Counter::in_use,index);result=s.identity()+0x4340u+page_offset;next=result+size;
    if(signed_word(size)<0x2000){
      batch=next-result;
      do{
        change(s,Counter::total_blocks,index,1,0x2333d1);change(s,Counter::free_blocks,index,1,0x2333d8);change(s,Counter::in_use,index,0xffffffffu,0x2333df);
        const auto previous=s.free_head(index);s.link(next,previous,0x2333ea);s.free_head(index,next,0x2333ec);
        batch+=size;next+=size;
      }while(signed_word(batch)<0x2000);
    }
    s.counter(Counter::in_use,index,in_use,0x23336d);
  }
  change(s,Counter::total_blocks,index,1,0x23337e);return result;
}
bool native_tu15_pool_release(NativeTu15PoolStorage&s,std::uint32_t data,std::uint32_t bytes){
  if(signed_word(bytes)>0x2000)return false;
  const auto index=signed_word(bytes)<=0x200?s.small_class(bytes):s.large_class(bytes);
  const auto in_use=s.counter(Counter::in_use,index);const auto available=s.counter(Counter::free_blocks,index);
  s.counter(Counter::free_blocks,index,available+1u,0x233458);const auto head=s.free_head(index);
  s.counter(Counter::in_use,index,in_use-1u,0x233463);s.link(data,head,0x23346e);s.free_head(index,data,0x233470);return true;
}
std::uint32_t native_tu15_route_array_allocation(NativeTu15ArrayAllocatorStorage&s,std::uint32_t bytes,std::uint32_t memory_class,NativeTu15PhysicsFrameFp&fp,const NativeTu15ArrayAllocatorServices&services){
  const auto call=[&](std::uint32_t receiver){const auto owner=s.owner(receiver,false);if(!services.allocate_owner)throw std::runtime_error("Native TU1.5 allocator requires its current allocation owner");return services.allocate_owner({receiver,owner,bytes,memory_class},fp);};
  if(s.mode()==1u)return call(s.heap(NativeTu15ArrayHeap::fixed));
  if(const auto result=call(s.heap(NativeTu15ArrayHeap::primary)))return result;
  if(const auto result=call(s.heap(NativeTu15ArrayHeap::secondary)))return result;
  if(!services.default_heap)throw std::runtime_error("Native TU1.5 allocator requires actual default-heap owner149930");
  return call(services.default_heap(fp));
}
void native_tu15_route_array_release(NativeTu15ArrayAllocatorStorage&s,std::uint32_t data,std::uint32_t bytes,std::uint32_t memory_class,NativeTu15PhysicsFrameFp&fp,const NativeTu15ArrayAllocatorServices&services){
  if(!data)return;
  const auto call=[&](std::uint32_t receiver){const auto owner=s.owner(receiver,true);if(!services.release_owner)throw std::runtime_error("Native TU1.5 allocator requires its current release owner");services.release_owner({receiver,owner,data,bytes,memory_class},fp);};
  const auto primary=s.heap(NativeTu15ArrayHeap::primary);
  if(signed_word(primary+0x4340u)<=signed_word(data)&&signed_word(primary+0xf0340u)>=signed_word(data)){call(primary);return;}
  const auto fixed=s.heap(NativeTu15ArrayHeap::fixed);
  if(signed_word(fixed+0x30u)<=signed_word(data)&&signed_word(fixed+0x15030u)>=signed_word(data)){call(fixed);return;}
  const auto secondary=s.heap(NativeTu15ArrayHeap::secondary);const auto first=s.secondary_start(secondary);
  if(signed_word(first)<=signed_word(data)&&signed_word(first+s.secondary_size(secondary))>=signed_word(data)){call(secondary);return;}
  if(!services.default_heap)throw std::runtime_error("Native TU1.5 allocator requires actual default-heap owner149930");call(services.default_heap(fp));
}
NativeTu15PoolMemory::NativeTu15PoolMemory(SourcePhysicsContactMemory&m,std::uint32_t pool,const SourcePhysicsContactRecordObserver&o):memory_(m),pool_(pool),observer_(o){}
std::uint32_t NativeTu15PoolMemory::read(std::uint32_t p,unsigned bytes)const{std::array<std::byte,4>raw{};memory_.read(p,std::span(raw).first(bytes));std::uint32_t n{};for(unsigned i=0;i<bytes;++i)n|=std::to_integer<unsigned>(raw[i])<<(i*8u);return n;}
void NativeTu15PoolMemory::write(std::uint32_t site,std::uint32_t p,std::uint32_t n){std::array<std::byte,4>raw{};for(unsigned i=0;i<4;++i)raw[i]=std::byte(n>>(8*i));SourcePhysicsContactRecordStore event{site,p,{},{}};if(observer_){event.before.resize(4);memory_.read(p,event.before);event.written.assign(raw.begin(),raw.end());}memory_.write(p,raw);if(observer_)observer_(event);}
std::uint32_t NativeTu15PoolMemory::counter_address(Counter kind,std::uint32_t index)const{switch(kind){case Counter::in_use:return pool_+0x2e8u+index*4u;case Counter::free_blocks:return pool_+0xf0388u+index*4u;case Counter::total_blocks:return pool_+0xf0344u+index*4u;case Counter::pages:return pool_+0xf0340u;case Counter::reservations:return pool_+0x2d0u;}throw std::runtime_error("Unknown native pool counter");}
std::uint32_t NativeTu15PoolMemory::identity()const{return pool_;}
std::uint32_t NativeTu15PoolMemory::small_class(std::uint32_t bytes)const{return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(read(pool_+0xacu+bytes,1)))));}
std::uint32_t NativeTu15PoolMemory::large_class(std::uint32_t bytes)const{const auto before=bytes-1u;const auto index=(before>>10u)|((before&0x80000000u)?0xffc00000u:0u);return read(pool_+0x2b0u+index*4u);}
std::uint32_t NativeTu15PoolMemory::free_head(std::uint32_t index)const{return read(pool_+0x24u+index*4u);}
std::uint32_t NativeTu15PoolMemory::class_size(std::uint32_t index)const{return read(pool_+0x68u+index*4u);}
std::uint32_t NativeTu15PoolMemory::counter(Counter kind,std::uint32_t index)const{return read(counter_address(kind,index));}
std::uint32_t NativeTu15PoolMemory::small(Small kind)const{return read(pool_+0x18u+unsigned(kind)*4u);}
std::uint32_t NativeTu15PoolMemory::link(std::uint32_t data)const{return read(data);}
void NativeTu15PoolMemory::free_head(std::uint32_t index,std::uint32_t n,std::uint32_t site){write(site,pool_+0x24u+index*4u,n);}
void NativeTu15PoolMemory::counter(Counter kind,std::uint32_t index,std::uint32_t n,std::uint32_t site){write(site,counter_address(kind,index),n);}
void NativeTu15PoolMemory::small(Small kind,std::uint32_t n,std::uint32_t site){write(site,pool_+0x18u+unsigned(kind)*4u,n);}
void NativeTu15PoolMemory::link(std::uint32_t data,std::uint32_t n,std::uint32_t site){write(site,data,n);}
NativeTu15ArrayAllocatorMemory::NativeTu15ArrayAllocatorMemory(SourcePhysicsContactMemory&m):memory_(m){}
std::uint32_t NativeTu15ArrayAllocatorMemory::read(std::uint32_t p)const{std::array<std::byte,4>raw{};memory_.read(p,raw);std::uint32_t n{};for(unsigned i=0;i<4;++i)n|=std::to_integer<unsigned>(raw[i])<<(8*i);return n;}
std::uint32_t NativeTu15ArrayAllocatorMemory::mode()const{return read(0x486064);}
std::uint32_t NativeTu15ArrayAllocatorMemory::heap(NativeTu15ArrayHeap kind)const{return read(0x486068u+unsigned(kind)*4u);}
std::uint32_t NativeTu15ArrayAllocatorMemory::owner(std::uint32_t receiver,bool releasing)const{return read(read(receiver)+(releasing?0x14u:0x10u));}
std::uint32_t NativeTu15ArrayAllocatorMemory::secondary_start(std::uint32_t receiver)const{return read(receiver+0x18u);}
std::uint32_t NativeTu15ArrayAllocatorMemory::secondary_size(std::uint32_t receiver)const{return read(receiver+0x1cu);}
} // namespace halo2
