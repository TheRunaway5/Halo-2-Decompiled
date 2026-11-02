#include "halo2/native_tu15_data_array_allocate_specific.hpp"
#include <array>
#include <bit>

namespace halo2 {
namespace {
using M=SourcePhysicsContactMemory;using A=SourcePhysicsContactRuntimeAbi;
using O=SourcePhysicsContactRecordObserver;
std::uint32_t read(const M& m,std::uint32_t at,unsigned bytes=4){
    std::array<std::byte,4> data{};m.read(at,std::span(data).first(bytes));std::uint32_t value{};
    for(unsigned n=0;n<bytes;++n)value|=std::to_integer<unsigned>(data[n])<<(n*8u);return value;
}
void write(M& m,const O& o,std::uint32_t pc,std::uint32_t at,std::uint32_t value,unsigned bytes=4){
    SourcePhysicsContactRecordStore store{pc,at,std::vector<std::byte>(bytes),std::vector<std::byte>(bytes)};m.read(at,store.before);
    for(unsigned n=0;n<bytes;++n)store.written[n]=std::byte(value>>(n*8u));m.write(at,store.written);if(o)o(store);
}
void push(M& m,A& a,std::uint32_t pc,std::uint32_t value){a.gp[4]-=4u;write(m,{},pc,a.gp[4],value);}
std::uint32_t pop(const M& m,A& a){const auto value=read(m,a.gp[4]);a.gp[4]+=4u;return value;}
std::int32_t signed32(std::uint32_t value){return std::bit_cast<std::int32_t>(value);}
}

void native_tu15_data_array_initialize_record_abi(M& m,A& a,const O& o){
    auto& g=a.gp;g[1]=read(m,g[2]+0x24u);push(m,a,0x16e863u,g[3]);g[3]=g[1];push(m,a,0x16e866u,g[7]);
    g[1]>>=2u;g[0]=0;g[7]=g[6];const bool backwards=(a.eflags&0x400u)!=0;
    while(g[1]){write(m,o,0x16e86eu,g[7],g[0]);g[7]+=backwards?0xfffffffcu:4u;--g[1];}
    g[1]=g[3]&3u;
    while(g[1]){write(m,o,0x16e875u,g[7],g[0],1);g[7]+=backwards?0xffffffffu:1u;--g[1];}
    g[0]=(g[0]&0xffff0000u)|read(m,g[2]+0x40u,2);write(m,o,0x16e87bu,g[6],g[0],2);
    write(m,o,0x16e87eu,g[2]+0x40u,read(m,g[2]+0x40u,2)+1u,2);
    const bool sentinel=read(m,g[2]+0x40u,2)==0xffffu;g[7]=pop(m,a);g[3]=pop(m,a);
    if(sentinel)write(m,o,0x16e88bu,g[2]+0x40u,0x8000u,2);
    (void)pop(m,a);
}

void native_tu15_data_array_allocate_specific_abi(M& m,A& a,const O& o){
    auto& g=a.gp;push(m,a,0x16e770u,g[3]);push(m,a,0x16e771u,g[7]);g[7]=g[0]&0xffffu;
    g[0]=std::uint32_t(signed32(g[0])>>16u);g[3]=g[0];
    const auto refuse=[&](){g[7]=pop(m,a);g[0]=0xffffffffu;g[3]=pop(m,a);(void)pop(m,a);};
    if(signed32(g[7])<0||signed32(g[7])>=signed32(read(m,g[2]+0x20u))){refuse();return;}
    g[1]=read(m,g[2]+0x44u);push(m,a,0x16e78bu,g[6]);g[6]=read(m,g[2]+0x24u)*g[7]+g[1];
    if(read(m,g[6],2)){g[6]=pop(m,a);refuse();return;}
    g[1]=read(m,g[2]+0x48u);g[0]=std::uint32_t(signed32(g[7])>>5u);g[0]=g[1]+g[0]*4u;
    push(m,a,0x16e7a5u,g[5]);g[1]=g[7]&31u;g[5]=1u<<(g[1]&31u);
    write(m,o,0x16e7b2u,g[0],read(m,g[0])|g[5]);g[1]=read(m,g[2]+0x3cu);g[0]=read(m,g[2]+0x38u);++g[1];
    const bool grow=signed32(g[7])>=signed32(g[0]);write(m,o,0x16e7bdu,g[2]+0x3cu,g[1]);g[5]=pop(m,a);
    if(grow){g[0]=g[7]+1u;write(m,o,0x16e7c6u,g[2]+0x38u,g[0]);}
    push(m,a,0x16e7c9u,0x16e7ceu);native_tu15_data_array_initialize_record_abi(m,a,o);
    g[0]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(g[3]))));
    write(m,o,0x16e7d1u,g[6],g[3],2);g[0]<<=16u;g[6]=pop(m,a);g[0]|=g[7];
    g[7]=pop(m,a);g[3]=pop(m,a);(void)pop(m,a);
}
}
