#include "halo2/native_tu15_phantom_pair_create.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory& memory,std::uint32_t address){
    std::array<std::byte,4> bytes{};memory.read(address,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);
    return value;
}
std::uint32_t payload(std::uint32_t handle){return handle?handle-0x10u:0u;}
void add(const SourcePhysicsContactMemory& memory,std::uint32_t self,
    std::uint32_t other,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15PhantomPairCreateServices& services){
    const auto phantom=word(memory,self+0x20u),table=word(memory,phantom);
    const NativeTu15ContactVirtualOwner owner{word(memory,table+0x1cu)};
    if(!services.add_overlap)throw std::runtime_error("Native TU1.5 phantom creation requires actual overlap-addition owner");
    services.add_overlap(phantom,owner,{other},phantom&0xffffff00u,fp);
}
}
void native_tu15_create_phantom_pair(SourcePhysicsContactMemory& memory,std::uint32_t pair,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15PhantomPairCreateServices& services){
    const auto first=word(memory,pair);
    if(word(memory,first+8u)==2u){
        const auto self=payload(first),other=payload(word(memory,pair+4u));
        add(memory,self,other,fp,services);
    }
    const auto second=word(memory,pair+4u);
    if(word(memory,second+8u)==2u){
        const auto other=payload(word(memory,pair)),self=payload(second);
        add(memory,self,other,fp,services);
    }
}
} // namespace halo2
