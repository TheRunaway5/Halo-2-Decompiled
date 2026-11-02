#include "halo2/native_tu15_contact_collector_info.hpp"
#include <bit>

namespace halo2 {
void native_tu15_get_contact_collector_info(SourcePhysicsContactMemory& memory,
    std::uint32_t collector,NativeTu15IslandConstraintInfo info,
    const SourcePhysicsContactRecordObserver& observe){
    const auto store=[&](std::uint32_t site,std::uint32_t address,std::uint32_t value){
        std::array<std::byte,4> bytes{};for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(n*8u));
        SourcePhysicsContactRecordStore event{site,address,{},{}};
        if(observe){event.before.resize(4);memory.read(address,event.before);event.written.assign(bytes.begin(),bytes.end());}
        memory.write(address,bytes);if(observe)observe(event);
    };
    const auto p=info.identity;
    store(0x325e96u,p,0);store(0x325e98u,p+4u,0);store(0x325e9bu,p+12u,0);store(0x325e9eu,p+8u,0x18u);
    std::array<std::byte,4> bytes{};memory.read(collector+0x94u,bytes);std::uint32_t count{};
    for(unsigned n=0;n<4;++n)count|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    store(0x325eb8u,p,count*48u+0x80u);
    auto entries=count+2u,rows=entries*48u,atoms=count*4u+0x2cu;
    store(0x325eceu,p+4u,rows);store(0x325ed1u,p+8u,atoms);store(0x325ed4u,p+12u,entries);
    if(std::bit_cast<std::int32_t>(count)>=2){
        rows+=0x20u;atoms+=4u;++entries;
        store(0x325ee0u,p+4u,rows);store(0x325ee3u,p+8u,atoms);store(0x325ee6u,p+12u,entries);
    }
}
} // namespace halo2
