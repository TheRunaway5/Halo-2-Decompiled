#include "halo2/native_tu15_contact_pair_collectors.hpp"
#include <stdexcept>

namespace halo2 {
void native_tu15_mark_contact_pair(SourcePhysicsContactMemory&memory,
    NativeTu15ContactQueryCollector collector,const SourcePhysicsContactRecordObserver&observe){
    const std::array written{std::byte{1}};
    SourcePhysicsContactRecordStore event{0x30ba80u,collector.identity+4u,{},{}};
    if(observe){event.before.resize(1);memory.read(event.address,event.before);event.written.assign(written.begin(),written.end());}
    memory.write(event.address,written);if(observe)observe(event);
}
void native_tu15_forward_reversed_contact_pair(SourcePhysicsContactMemory&memory,
    NativeTu15ContactQueryCollector collector,NativeTu15PhysicsBody first,
    NativeTu15PhysicsBody second,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ContactPairCollectorServices&services){
    const auto word=[&](std::uint32_t p){std::array<std::byte,4>b{};memory.read(p,b);
        std::uint32_t n{};for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;};
    const auto destination=word(collector.identity+8u),table=word(destination);
    const NativeTu15PhysicsVirtualOwner owner{word(table+4u)};
    if(!services.emit_pair)throw std::runtime_error("Native reversed pair collector requires its current callback owner");
    services.emit_pair({destination},owner,second,first,fp);
}
}
