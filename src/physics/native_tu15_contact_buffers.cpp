#include "halo2/native_tu15_contact_buffers.hpp"
#include "native_tu15_contact_unwind_internal.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
std::uint32_t component_bytes(std::uint32_t count){return(count*256u+15u)&0xfffffff0u;}
template<class F>void require(const F&f,const char*why){if(!f)throw std::runtime_error(why);}
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observer_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observer_(o){}
    std::uint32_t word(std::uint32_t p)const{std::array<std::byte,4>b{};memory_.read(p,b);std::uint32_t v{};for(unsigned n=0;n<4;++n)v|=std::to_integer<unsigned>(b[n])<<(n*8u);return v;}
    void word(std::uint32_t site,std::uint32_t p,std::uint32_t value,unsigned width=4){std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(value>>(n*8u));SourcePhysicsContactRecordStore event{site,p,{},{}};if(observer_){event.before.resize(width);memory_.read(p,event.before);event.written.assign(b.begin(),b.begin()+width);}memory_.write(p,std::span(b).first(width));if(observer_)observer_(event);}
};
}
NativeTu15ContactComponentBuffer native_tu15_initialize_contact_components(SourcePhysicsContactMemory&memory,NativeTu15ContactComponentBuffer buffer,std::uint32_t count,NativeTu15PhysicsFrameFp&fp,const NativeTu15ContactBufferServices&s,const SourcePhysicsContactRecordObserver&observer){
    Memory m(memory,observer);native_contact_detail::UnwindGuard cleanup([&]{native_tu15_release_owned_contact_components(memory,buffer,fp,s);});const auto p=buffer.identity;m.word(0x303c90,p,0);m.word(0x303c92,p+4u,0);m.word(0x303c95,p+8u,0x80000000u);
    const NativeTu15ContactBufferAllocator allocator{m.word(0x48c938u)};const auto remaining=m.word(allocator.identity+0xcu),bytes=component_bytes(count);std::uint32_t data;
    cleanup.arm(); // Actual303CA9, after both allocator input reads.
    if(less(remaining,bytes)){const auto table=m.word(allocator.identity);const NativeTu15ContactBufferOwner owner{m.word(table+0x24u)};require(s.allocate_stack_block,"Native TU1.5 contact component buffer requires actual stack block allocation owner");data=s.allocate_stack_block(allocator,owner,bytes,fp);}
    else{data=m.word(allocator.identity+8u);m.word(0x303cd1,allocator.identity+8u,data+bytes);m.word(0x303cd4,allocator.identity+0xcu,remaining-bytes);}
    m.word(0x303cda,p,data);m.word(0x303ce2,p+0xcu,data);m.word(0x303ce5,p+8u,count|0x80000000u);m.word(0x303cec,p+0x10u,count);cleanup.disarm();return buffer;
}
void native_tu15_destroy_contact_components(SourcePhysicsContactMemory&memory,NativeTu15ContactComponentBuffer buffer,NativeTu15PhysicsFrameFp&fp,const NativeTu15ContactBufferServices&s,const SourcePhysicsContactRecordObserver&observer){
    Memory m(memory,observer);native_contact_detail::UnwindGuard cleanup([&]{native_tu15_release_owned_contact_components(memory,buffer,fp,s);});const auto p=buffer.identity;const auto count=m.word(p+0x10u),original=m.word(p+0xcu);const NativeTu15ContactBufferAllocator allocator{m.word(0x48c938u)};const auto bytes=component_bytes(count);bool release=false;cleanup.arm(); // Actual303D33.
    if(bytes){const auto marker=m.word(allocator.identity+0x14u);release=marker==original||marker==0;}
    if(release){const auto table=m.word(allocator.identity);const NativeTu15ContactBufferOwner owner{m.word(table+0x28u)};require(s.release_stack_block,"Native TU1.5 contact component buffer requires actual stack block release owner");s.release_stack_block(allocator,owner,original,bytes,fp);}
    else{const auto cursor=m.word(allocator.identity+8u),remaining=m.word(allocator.identity+0xcu);m.word(0x303d5b,allocator.identity+8u,cursor-bytes);m.word(0x303d5e,allocator.identity+0xcu,remaining+bytes);}
    const auto capacity=m.word(p+8u);cleanup.disarm(); // Actual303D66, after capacity read and before owned release.
    if(less(capacity,0))return;
    const NativeTu15ContactBufferAllocator actual_allocator{m.word(0x48c938u)};const auto table=m.word(actual_allocator.identity);const auto owned_bytes=(capacity&0x7fffffffu)*256u,data=m.word(p);const NativeTu15ContactBufferOwner owner{m.word(table+0x14u)};
    require(s.release_owned_components,"Native TU1.5 contact component buffer requires actual owned data release owner");s.release_owned_components(actual_allocator,owner,data,owned_bytes,0x12u,fp);
}
void native_tu15_release_owned_contact_components(SourcePhysicsContactMemory&memory,NativeTu15ContactComponentBuffer buffer,NativeTu15PhysicsFrameFp&fp,const NativeTu15ContactBufferServices&s){
    const SourcePhysicsContactRecordObserver none;Memory m(memory,none);const auto capacity=m.word(buffer.identity+8u);if(less(capacity,0))return;const NativeTu15ContactBufferAllocator allocator{m.word(0x48c938u)};const auto table=m.word(allocator.identity);const auto bytes=(capacity&0x7fffffffu)*256u,data=m.word(buffer.identity);const NativeTu15ContactBufferOwner owner{m.word(table+0x14u)};require(s.release_owned_components,"Native TU1.5 contact component base requires actual owned data release owner");s.release_owned_components(allocator,owner,data,bytes,0x12u,fp);
}
void native_tu15_release_owned_contact_records(SourcePhysicsContactMemory&memory,NativeTu15ContactRecordArray array,NativeTu15PhysicsFrameFp&fp,const NativeTu15ArrayGrowthServices&s){
    const SourcePhysicsContactRecordObserver none;Memory m(memory,none);const auto capacity=m.word(array.identity+8u);if(less(capacity,0))return;const auto allocator=m.word(0x48c938u),table=m.word(allocator);const auto bytes=(capacity&0x7fffffffu)*128u,data=m.word(array.identity);const auto owner=m.word(table+0x14u);require(s.release,"Native TU1.5 contact record base requires actual owned data release owner");s.release({allocator,owner,data,bytes,0x12u},fp);
}
NativeTu15ContactRecord native_tu15_append_contact_record(SourcePhysicsContactMemory&memory,NativeTu15ContactRecordArray array,NativeTu15ContactRecord input,NativeTu15PhysicsFrameFp&fp,const NativeTu15ArrayGrowthServices&s,const SourcePhysicsContactRecordObserver&observer){
    Memory m(memory,observer);const auto count=m.word(array.identity+4u);m.word(0x303ef4,input.identity+0x7du,0,1);m.word(0x303efb,input.identity+0x7cu,0,1);const auto capacity=m.word(array.identity+8u)&0x7fffffffu;
    if(count==capacity){NativeTu15ArrayGrowthMemory growth(memory,{array.identity},observer);native_tu15_grow_array(growth,128u,fp,s);}
    const auto index=m.word(array.identity+4u),data=m.word(array.identity);const NativeTu15ContactRecord destination{data+index*128u};m.word(0x303f2e,array.identity+4u,index+1u);NativeTu15ContactRecordMemory records(memory,observer);native_tu15_copy_contact_record(records,destination,input);
    const auto final_count=m.word(array.identity+4u),final_data=m.word(array.identity);return{final_data+final_count*128u-128u};
}
} // namespace halo2
