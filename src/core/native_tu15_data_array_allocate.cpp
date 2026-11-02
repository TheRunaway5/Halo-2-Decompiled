#include "halo2/native_tu15_data_array_allocate.hpp"
#include "halo2/physics_contact_runtime.hpp"
#include <array>
#include <bit>
#include <optional>
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
std::uint32_t occupancy_at(std::uint32_t bitmap,std::uint32_t index) {
    return bitmap+std::uint32_t(signed32(index)>>5u)*4u;
}
}
std::uint32_t native_tu15_data_array_allocate(SourcePhysicsContactMemory &memory,std::uint32_t pool) {
    auto cursor=read(memory,pool+0x34u);
    const auto extent=read(memory,pool+0x38u);
    std::optional<std::uint32_t> available;
    if(signed32(cursor)<signed32(extent)) {
        const auto bitmap=read(memory,pool+0x48u);
        while(true) {
            if(!(read(memory,occupancy_at(bitmap,cursor))&(1u<<(cursor&31u)))) {
                if(cursor!=0xffffffffu)available=cursor;
                break;
            }
            const auto current_extent=read(memory,pool+0x38u);
            ++cursor;
            if(signed32(cursor)>=signed32(current_extent))break;
        }
    }
    if(!available) {
        if(signed32(extent)>=signed32(read(memory,pool+0x20u)) || extent==0xffffffffu)
            return 0xffffffffu;
        available=extent;
    }
    const auto index=*available;
    const auto stride=read(memory,pool+0x24u);
    const auto rows=read(memory,pool+0x44u);
    const auto record=rows+index*stride;
    const auto bitmap=read(memory,pool+0x48u);
    const auto occupied=occupancy_at(bitmap,index);
    write(memory,occupied,read(memory,occupied)|(1u<<(index&31u)));
    const auto used=read(memory,pool+0x3cu);
    const auto current_extent=read(memory,pool+0x38u);
    write(memory,pool+0x3cu,used+1u);
    write(memory,pool+0x34u,index+1u);
    if(signed32(current_extent)<=signed32(index))write(memory,pool+0x38u,index+1u);
    // Occupancy/header stores can alias the stride or record. Preserve both
    // the earlier record address and this fresh clear-length lookup.
    const auto clear_bytes=read(memory,pool+0x24u);
    for(std::uint32_t n=0;n<(clear_bytes>>2u);++n)write(memory,record+n*4u,0);
    for(std::uint32_t n=0;n<(clear_bytes&3u);++n)write(memory,record+(clear_bytes&~3u)+n,0,1);
    write(memory,record,read(memory,pool+0x40u,2),2);
    write(memory,pool+0x40u,std::uint16_t(read(memory,pool+0x40u,2)+1u),2);
    if(read(memory,pool+0x40u,2)==0xffffu)write(memory,pool+0x40u,0x8000u,2);
    return (read(memory,record,2)<<16u)|index;
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
