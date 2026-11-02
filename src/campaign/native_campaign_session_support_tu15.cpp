#include "halo2/native_campaign_session_support_tu15.hpp"

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t at,unsigned width=4){
    std::array<std::byte,4> bytes{};memory.read(at,std::span(bytes).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);return value;
}
void write(SourcePhysicsContactMemory &memory,std::uint32_t at,std::uint32_t value,unsigned width=4){
    std::array<std::byte,4> bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(8u*n));memory.write(at,std::span(bytes).first(width));
}
}
void native_campaign_session_clear_peer_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t session,std::uint32_t peer){
    const auto selected=session+0x72dcu+peer*0x14u,phase=read(memory,session+0x741cu);
    if(phase>=5u&&phase<=8u&&read(memory,selected+1u,1)&&read(memory,selected+2u,1)){
        for(std::uint32_t n=0;n<16u;++n){
            if(n==peer)continue;
            const auto row=session+0x72dcu+n*0x14u;
            if(read(memory,row,1)&&read(memory,row+1u,1)&&read(memory,row+2u,1))break;
        }
    }
    const auto link=read(memory,selected+4u);
    if(link!=0xffffffffu){
        const auto players=read(memory,session+8u),record=players+link*0x550u+0xa8u;
        const auto shift=read(memory,session+0x10u);
        const auto bit=std::uint8_t(std::uint32_t{1}<<(shift&31u));
        const auto flags=read(memory,record+9u,1);
        write(memory,record+9u,flags&std::uint8_t(~bit),1);
        write(memory,selected+4u,0xffffffffu);
    }
    for(std::uint32_t n=0;n<0x14u;n+=4u)write(memory,selected+n,0);
    write(memory,selected+4u,0xffffffffu);
}
bool native_campaign_session_address_present_tu15(const SourcePhysicsContactMemory &memory,
    std::uint32_t descriptor){
    if(!descriptor)return false;
    const auto kind=read(memory,descriptor+0x12u,2);
    if(kind==0xffffu||kind==4u)return read(memory,descriptor)!=0;
    if(kind!=16u)return false;
    for(std::uint32_t n=0;n<8u;++n)if(read(memory,descriptor+n*2u,2))return true;
    return false;
}
std::uint8_t native_campaign_session_phase_policy_tu15(const SourcePhysicsContactMemory &memory,
    std::uint32_t session){
    switch(read(memory,session+0x741cu)){
    case 2:case 4:case 6:return 1;
    case 7:case 8:return std::uint8_t(read(memory,session+0x7420u,1));
    default:return 0;
    }
}
}
