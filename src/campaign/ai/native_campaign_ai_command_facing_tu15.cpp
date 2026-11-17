#include "halo2/native_campaign_ai_command_facing_tu15.hpp"
#include "halo2/native_tu15_retained_store_observation.hpp"
#include <array>
#include <vector>

namespace halo2 {
namespace {
using U=std::uint32_t;
U read(SourcePhysicsContactMemory &memory,U address) {
    std::array<std::byte,4> bytes{};memory.read(address,bytes);U value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<U>(bytes[n])<<(n*8u);
    return value;
}
void write(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    U pc,U address,U value,unsigned width) {
    const bool observed=native_tu15_observes_retained_store(memory,observe,pc,address,width);
    SourcePhysicsContactRecordStore event{pc,address,{},std::vector<std::byte>(width)};
    if(observed){event.before.resize(width);memory.read(address,event.before);}
    for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(n*8u));
    memory.write(address,event.written);if(observed)observe(event);
}
}
void native_campaign_ai_command_facing_call_tu15(SourcePhysicsContactMemory &memory,U entry,
    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    if(entry!=0x280c90u && entry!=0x280d70u)throw std::invalid_argument("Unknown native command-facing entry");
    const bool object=entry==0x280d70u;
    auto &g=abi.gp;g[1]=read(memory,0x50eee8u);
    if(g[1]!=0xffffffffu) {
        g[1]=(g[1]&0xffffu)*0xd4u;const U pool=read(memory,0x50eee0u),enabled=g[2]&0xffu;
        g[0]=read(memory,pool+0x44u)+g[1];
        write(memory,observe,object?0x280d95u:0x280cb5u,g[0]+0x52u,enabled,1);
        if(enabled) {
            if(object)write(memory,observe,0x280d9au,g[0]+0x46u,0,1);
            write(memory,observe,object?0x280d9eu:0x280cbau,g[0]+0x54u,object?1u:2u,2);
            write(memory,observe,object?0x280da4u:0x280cc0u,g[0]+0x58u,g[7],4);
            write(memory,observe,object?0x280da7u:0x280cc3u,g[0]+0x51u,0,1);
        }
        g[0]=read(memory,pool+0x44u)+g[1];
        write(memory,observe,object?0x280db2u:0x280cceu,g[0]+0x46u,enabled,1);
        if(enabled) {
            write(memory,observe,object?0x280db8u:0x280cd4u,g[0]+0x48u,object?1u:2u,2);
            write(memory,observe,object?0x280dbeu:0x280cdau,g[0]+0x4cu,g[7],4);
        }
    }
    g[4]+=4u;
}
}
