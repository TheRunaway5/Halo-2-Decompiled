#include "halo2/native_tu15_contact_candidate_buffers.hpp"
#include "native_tu15_contact_unwind_internal.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
std::uint32_t entry_bytes(std::uint32_t count){return(count*12u+15u)&0xfffffff0u;}
template<class F>void require(const F&f,const char*why){if(!f)throw std::runtime_error(why);}
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observer_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observer_(o){}
    std::uint32_t word(std::uint32_t p)const{std::array<std::byte,4>b{};memory_.read(p,b);std::uint32_t v{};for(unsigned n=0;n<4;++n)v|=std::to_integer<unsigned>(b[n])<<(n*8u);return v;}
    void word(std::uint32_t site,std::uint32_t p,std::uint32_t value){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(n*8u));SourcePhysicsContactRecordStore event{site,p,{},{}};if(observer_){event.before.resize(4);memory_.read(p,event.before);event.written.assign(b.begin(),b.end());}memory_.write(p,b);if(observer_)observer_(event);}
};
void sort(Memory&m,std::uint32_t keys,std::uint32_t first,std::uint32_t last,
 std::uint32_t forwarded){
 for(;;){
  const auto middle=std::uint32_t(std::bit_cast<std::int32_t>(first+last)>>1);
  const auto pivot=m.word(keys+middle*4u);auto high=last,low=first;
  for(;;){
   if(m.word(keys+low*4u)<pivot)do{++low;}while(m.word(keys+low*4u)<pivot);
   if(pivot<m.word(keys+high*4u))do{--high;}while(pivot<m.word(keys+high*4u));
   if(less(high,low))break;
   if(high!=low){const auto low_value=m.word(keys+low*4u),high_value=m.word(keys+high*4u);m.word(0x2f33f2u,keys+high*4u,low_value);m.word(0x2f33f8u,keys+low*4u,high_value);}
   --high;++low;if(less(high,low))break;
  }
  if(less(first,high))sort(m,keys,first,high,forwarded);
  if(!less(low,last))return;
  first=low;
 }
}
}
void native_tu15_sort_contact_keys(SourcePhysicsContactMemory&memory,std::uint32_t keys,
 std::uint32_t first,std::uint32_t last,std::uint32_t forwarded,
 const SourcePhysicsContactRecordObserver&observer){Memory m(memory,observer);sort(m,keys,first,last,forwarded);}
NativeTu15ContactEntryBuffer native_tu15_initialize_contact_entries(SourcePhysicsContactMemory&memory,
 NativeTu15ContactEntryBuffer buffer,std::uint32_t count,NativeTu15PhysicsFrameFp&fp,
 const NativeTu15ContactBufferServices&s,const SourcePhysicsContactRecordObserver&observer){
 Memory m(memory,observer);native_contact_detail::UnwindGuard cleanup([&]{native_tu15_release_owned_contact_entries(memory,buffer,fp,s);});
 const auto p=buffer.identity;m.word(0x2f3450u,p,0);m.word(0x2f3452u,p+4u,0);m.word(0x2f3455u,p+8u,0x80000000u);
 const NativeTu15ContactBufferAllocator allocator{m.word(0x48c938u)};const auto remaining=m.word(allocator.identity+0xcu);
 cleanup.arm(); // Actual2F3469, after both allocator input reads.
 const auto bytes=entry_bytes(count);std::uint32_t data;
 if(less(remaining,bytes)){const auto table=m.word(allocator.identity);const NativeTu15ContactBufferOwner owner{m.word(table+0x24u)};require(s.allocate_stack_block,"Native TU1.5 entry buffer requires actual stack-block allocation owner");data=s.allocate_stack_block(allocator,owner,bytes,fp);}
 else{data=m.word(allocator.identity+8u);m.word(0x2f348eu,allocator.identity+8u,data+bytes);m.word(0x2f3496u,allocator.identity+0xcu,remaining-bytes);}
 m.word(0x2f34a2u,p+8u,count|0x80000000u);m.word(0x2f34a9u,p+0x10u,count);m.word(0x2f34acu,p,data);m.word(0x2f34aeu,p+0xcu,data);
 cleanup.disarm();return buffer;
}
void native_tu15_destroy_contact_entries(SourcePhysicsContactMemory&memory,NativeTu15ContactEntryBuffer buffer,
 NativeTu15PhysicsFrameFp&fp,const NativeTu15ContactBufferServices&s,const SourcePhysicsContactRecordObserver&observer){
 Memory m(memory,observer);native_contact_detail::UnwindGuard cleanup([&]{native_tu15_release_owned_contact_entries(memory,buffer,fp,s);});
 const auto p=buffer.identity,count=m.word(p+0x10u),original=m.word(p+0xcu);const NativeTu15ContactBufferAllocator allocator{m.word(0x48c938u)};
 const auto bytes=entry_bytes(count);cleanup.arm(); // Actual2F3507.
 bool release=false;if(bytes){const auto marker=m.word(allocator.identity+0x14u);release=original==marker||marker==0;}
 if(release){const auto table=m.word(allocator.identity);const NativeTu15ContactBufferOwner owner{m.word(table+0x28u)};require(s.release_stack_block,"Native TU1.5 entry buffer requires actual stack-block release owner");s.release_stack_block(allocator,owner,original,bytes,fp);}
 else{const auto cursor=m.word(allocator.identity+8u),remaining=m.word(allocator.identity+0xcu);m.word(0x2f352fu,allocator.identity+8u,cursor-bytes);m.word(0x2f3532u,allocator.identity+0xcu,remaining+bytes);}
 const auto capacity=m.word(p+8u);cleanup.disarm(); // Actual2F353A, after capacity read.
 if(less(capacity,0))return;
 const NativeTu15ContactBufferAllocator actual{m.word(0x48c938u)};const auto table=m.word(actual.identity);const auto bytes_owned=(capacity&0x7fffffffu)*12u,data=m.word(p);const NativeTu15ContactBufferOwner owner{m.word(table+0x14u)};
 require(s.release_owned_components,"Native TU1.5 entry buffer requires actual owned-data release owner");s.release_owned_components(actual,owner,data,bytes_owned,0x12u,fp);
}
void native_tu15_release_owned_contact_entries(SourcePhysicsContactMemory&memory,NativeTu15ContactEntryBuffer buffer,
 NativeTu15PhysicsFrameFp&fp,const NativeTu15ContactBufferServices&s){
 const SourcePhysicsContactRecordObserver none;Memory m(memory,none);const auto capacity=m.word(buffer.identity+8u);if(less(capacity,0))return;
 const NativeTu15ContactBufferAllocator allocator{m.word(0x48c938u)};const auto data=m.word(buffer.identity),table=m.word(allocator.identity);const auto bytes=(capacity&0x7fffffffu)*12u;const NativeTu15ContactBufferOwner owner{m.word(table+0x14u)};
 require(s.release_owned_components,"Native TU1.5 entry base requires actual owned-data release owner");s.release_owned_components(allocator,owner,data,bytes,0x12u,fp);
}
} // namespace halo2
