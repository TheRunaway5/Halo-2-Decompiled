#include "halo2/native_tu15_compound_process.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;
    const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o)
        :memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p)const{
        std::array<std::byte,4>b{};memory_.read(p,b);std::uint32_t n{};
        for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);
        return n;
    }
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t n){
        SourcePhysicsContactRecordStore event{pc,p,{},{}};
        if(observe_){event.before.resize(4);memory_.read(p,event.before);}
        std::array<std::byte,4>b{};
        for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(k*8u));
        memory_.write(p,b);
        if(observe_){event.written.assign(b.begin(),b.end());observe_(event);}
    }
};
}
void native_tu15_process_compound_agent(SourcePhysicsContactMemory&memory,
    NativeTu15IslandCollisionAgent agent,NativeTu15PhysicsBody first,
    NativeTu15PhysicsBody second,NativeTu15PhysicsStepConfiguration configuration,
    NativeTu15IslandContactOutput output,NativeTu15CompoundBodyProxy proxy,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15CompoundProcessServices&services,
    const SourcePhysicsContactRecordObserver&observe,
    NativeTu15CompoundProcessStats*stats){
    Memory m(memory,observe);if(stats)++stats->calls;
    const auto count=m.word(agent.identity+0x10u);
    const auto compound_shape=m.word(first.identity);
    m.word(0x2f7a5fu,proxy.identity+12u,first.identity);
    const auto inherited_motion=m.word(first.identity+8u);
    auto child_cursor=m.word(agent.identity+12u);
    m.word(0x2f7a6eu,proxy.identity+8u,inherited_motion);
    if(std::bit_cast<std::int32_t>(count)<=0)return;
    for(std::uint32_t index=0;index<count;++index,child_cursor+=4u){
        const auto shape_children=m.word(compound_shape+12u);
        const auto child_shape=m.word(shape_children+index*8u);
        m.word(0x2f7a7au,proxy.identity,child_shape);
        m.word(0x2f7a8cu,proxy.identity+4u,index);
        const NativeTu15IslandCollisionAgent child{m.word(child_cursor)};
        const auto table=m.word(child.identity);
        const NativeTu15PhysicsVirtualOwner owner{m.word(table+0x10u)};
        if(!services.process_child)
            throw std::runtime_error("Native TU1.5 compound process requires its actual child vtable+10 owner");
        if(stats)++stats->child_calls;
        services.process_child({child,owner,proxy,second,configuration,output},fp);
        if(stats)++stats->child_returns;
    }
}
} // namespace halo2
