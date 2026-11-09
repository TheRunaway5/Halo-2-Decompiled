#include "halo2/native_tu15_data_array_allocate.hpp"
#include "halo2/native_tu15_datum_allocate_kernel.hpp"
#include "halo2/physics_contact_runtime.hpp"
#include <array>
#include <bit>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t at,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(at,std::span(bytes).first(width));
    std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
void write(SourcePhysicsContactMemory &memory,std::uint32_t at,std::uint32_t value,unsigned width=4) {
    std::array<std::byte,4> bytes{};
    for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));
    memory.write(at,std::span(bytes).first(width));
}
std::int32_t signed32(std::uint32_t value){return std::bit_cast<std::int32_t>(value);}
struct ScalarAllocationStorage {
    SourcePhysicsContactMemory&memory;
    std::uint32_t read(NativeTu15DatumAllocatePhase,const NativeTu15DatumAllocation&,std::uint32_t at,unsigned width) {
        std::array<std::byte,4>bytes{};memory.read(at,std::span(bytes).first(width));std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
    }
    void write(NativeTu15DatumAllocatePhase,const NativeTu15DatumAllocation&,std::uint32_t at,
        std::uint32_t value,unsigned width,std::uint32_t,bool) {
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));memory.write(at,std::span(bytes).first(width));
    }
    void complete(NativeTu15DatumAllocatePhase,const NativeTu15DatumAllocation&){}
};
}
std::uint32_t native_tu15_data_array_allocate(SourcePhysicsContactMemory &memory,std::uint32_t pool) {
    NativeTu15DatumAllocation data;data.pool=pool;
    data.cursor=read(memory,pool+0x34u);data.extent=read(memory,pool+0x38u);
    ScalarAllocationStorage storage{memory};native_tu15_allocate_datum(storage,data);
    return data.success?data.handle:0xffffffffu;
}

void native_tu15_data_array_allocate_abi(SourcePhysicsContactMemory& m,
        SourcePhysicsContactRuntimeAbi& a,const SourcePhysicsContactRecordObserver& o){
    auto& g=a.gp;
    const auto w=[&](std::uint32_t pc,std::uint32_t at,std::uint32_t value,unsigned bytes=4){
        SourcePhysicsContactRecordStore store{pc,at,std::vector<std::byte>(bytes),std::vector<std::byte>(bytes)};m.read(at,store.before);
        for(unsigned n=0;n<bytes;++n)store.written[n]=std::byte(value>>(n*8u));m.write(at,store.written);if(o)o(store);
    };
    const auto push=[&](std::uint32_t value){g[4]-=4u;write(m,g[4],value);};
    const auto pop=[&](){const auto value=read(m,g[4]);g[4]+=4u;return value;};
    push(g[3]);push(g[5]);push(g[6]);g[6]=read(m,g[2]+0x34u);push(g[7]);g[7]=read(m,g[2]+0x38u);g[0]=0xffffffffu;
    bool chosen=false;
    if(signed32(g[6])<signed32(g[7])){
        g[3]=read(m,g[2]+0x48u);
        for(;;){
            g[1]=g[6]&31u;g[5]=1u<<(g[1]&31u);g[1]=std::uint32_t(signed32(g[6])>>5u);
            if(!(read(m,g[3]+g[1]*4u)&g[5])){g[3]=g[6];chosen=g[6]!=0xffffffffu;break;}
            g[1]=read(m,g[2]+0x38u);++g[6];if(signed32(g[6])>=signed32(g[1]))break;
        }
    }
    if(!chosen){
        if(signed32(g[7])>=signed32(read(m,g[2]+0x20u))||g[7]==0xffffffffu){
            g[7]=pop();g[6]=pop();g[5]=pop();g[3]=pop();(void)pop();return;
        }
        g[3]=g[7];
    }
    g[6]=read(m,g[2]+0x24u);g[0]=read(m,g[2]+0x44u);g[6]*=g[3];g[1]=read(m,g[2]+0x48u);g[6]+=g[0];
    g[0]=std::uint32_t(signed32(g[3])>>5u);g[5]=read(m,g[1]+g[0]*4u);g[0]=g[1]+g[0]*4u;
    g[1]=g[3]&31u;g[7]=1u<<(g[1]&31u);g[5]|=g[7];w(0x16e70eu,g[0],g[5]);
    g[7]=read(m,g[2]+0x3cu);g[1]=read(m,g[2]+0x38u);++g[7];const bool grow=signed32(g[1])<=signed32(g[3]);g[0]=g[3]+1u;
    w(0x16e71cu,g[2]+0x3cu,g[7]);w(0x16e71fu,g[2]+0x34u,g[0]);if(grow)w(0x16e724u,g[2]+0x38u,g[0]);
    g[1]=read(m,g[2]+0x24u);g[5]=g[1];g[1]>>=2u;g[0]=0;g[7]=g[6];const bool backwards=(a.eflags&0x400u)!=0;
    while(g[1]){w(0x16e733u,g[7],g[0]);g[7]+=backwards?0xfffffffcu:4u;--g[1];}
    g[1]=g[5]&3u;while(g[1]){w(0x16e73au,g[7],g[0],1);g[7]+=backwards?0xffffffffu:1u;--g[1];}
    g[0]=(g[0]&0xffff0000u)|read(m,g[2]+0x40u,2);w(0x16e740u,g[6],g[0],2);w(0x16e743u,g[2]+0x40u,read(m,g[2]+0x40u,2)+1u,2);
    if(read(m,g[2]+0x40u,2)==0xffffu)w(0x16e74eu,g[2]+0x40u,0x8000u,2);
    g[0]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(m,g[6],2)))));g[0]<<=16u;g[0]|=g[3];
    g[7]=pop();g[6]=pop();g[5]=pop();g[3]=pop();(void)pop();
}
}
