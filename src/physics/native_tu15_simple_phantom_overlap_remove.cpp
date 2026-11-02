#include "halo2/native_tu15_simple_phantom_overlap_remove.hpp"
#include <bit>

namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory& memory,std::uint32_t address){
    std::array<std::byte,4> bytes{};memory.read(address,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);
    return value;
}
void write(SourcePhysicsContactMemory& memory,std::uint32_t site,std::uint32_t address,
    std::uint32_t value,const SourcePhysicsContactRecordObserver& observe){
    std::array<std::byte,4> bytes{};for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8u*n));
    SourcePhysicsContactRecordStore event{site,address,{},{}};
    if(observe){event.before.resize(4);memory.read(address,event.before);event.written.assign(bytes.begin(),bytes.end());}
    memory.write(address,bytes);if(observe)observe(event);
}
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
}
void native_tu15_remove_simple_phantom_overlap(SourcePhysicsContactMemory& memory,
    std::uint32_t phantom,NativeTu15ContactPayload other,NativeTu15PhantomOverlapEvent event,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15PhantomOverlapRemoveServices& services,
    const SourcePhysicsContactRecordObserver& observe){
    const auto count=word(memory,phantom+0x84u);std::uint32_t index{};bool found=false;
    if(less(0u,count)){
        auto cursor=word(memory,phantom+0x80u);
        do{if(word(memory,cursor)==other.identity){found=true;break;}++index;cursor+=4u;}while(less(index,count));
    }
    native_tu15_notify_phantom_overlap_removed(memory,phantom,other,found?1u:0u,event,fp,services,observe);
    if(!found)return;
    const auto last=word(memory,phantom+0x84u)-1u;
    write(memory,0x31514cu,phantom+0x84u,last,observe);
    const auto current_data=word(memory,phantom+0x80u),replacement=word(memory,current_data+last*4u);
    write(memory,0x31515bu,current_data+index*4u,replacement,observe);
}
} // namespace halo2
