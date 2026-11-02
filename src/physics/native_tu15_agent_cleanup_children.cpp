#include "halo2/native_tu15_agent_cleanup_children.hpp"
#include "native_tu15_contact_unwind_internal.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory& memory_;const SourcePhysicsContactRecordObserver& observer_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observer_(o){}
    std::uint32_t word(std::uint32_t p,unsigned size=4)const{std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(size));std::uint32_t v{};for(unsigned n=0;n<size;++n)v|=std::to_integer<unsigned>(b[n])<<(8u*n);return v;}
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t value){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(8u*n));SourcePhysicsContactRecordStore e{pc,p,{},{}};if(observer_){e.before.resize(4);memory_.read(p,e.before);e.written.assign(b.begin(),b.end());}memory_.write(p,b);if(observer_)observer_(e);}
};
void release(const NativeTu15ArrayGrowthServices&s,NativeTu15ArrayReleaseRequest r,NativeTu15PhysicsFrameFp&fp){if(!s.release)throw std::runtime_error("Native TU1.5 agent cleanup requires its actual release owner");s.release(r,fp);}
}
void native_tu15_remove_cached_contact_points(SourcePhysicsContactMemory&memory,
    std::uint32_t records,std::uint32_t count_field,std::uint32_t manager,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15AgentCleanupChildrenServices&s,
    const SourcePhysicsContactRecordObserver&observer){
    Memory m(memory,observer);if(!m.word(count_field))return;
    do{const auto count=m.word(count_field)-1u;m.word(0x30ca7eu,count_field,count);
        const auto table=m.word(manager);const auto point=m.word(records+count*48u+0x1cu,2);
        const NativeTu15ContactVirtualOwner owner{m.word(table+8u)};
        if(!s.remove_point)throw std::runtime_error("Native TU1.5 agent cleanup requires its current point-removal owner");
        s.remove_point(manager,owner,static_cast<std::uint16_t>(point),fp);
    }while(m.word(count_field));
}
void native_tu15_cleanup_cached_contact_agent(SourcePhysicsContactMemory&memory,
    NativeTu15ContactAgent agent,NativeTu15PhysicsFrameFp&fp,const NativeTu15AgentCleanupChildrenServices&s,
    const SourcePhysicsContactRecordObserver&observer){
    Memory m(memory,observer);const auto p=agent.identity,manager=m.word(p+8u);
    native_tu15_remove_cached_contact_points(memory,p+0x30u,p+0x18u,manager,fp,s,observer);
    native_tu15_dispose_contact_reference(memory,p,fp,s.agents);
}
void native_tu15_cleanup_wrapped_contact_agent(SourcePhysicsContactMemory&memory,
    NativeTu15ContactAgent agent,NativeTu15PhysicsFrameFp&fp,const NativeTu15AgentCleanupChildrenServices&s){
    const SourcePhysicsContactRecordObserver none;Memory m(memory,none);const auto child=m.word(agent.identity+0xcu),table=m.word(child);
    const NativeTu15ContactVirtualOwner owner{m.word(table+0x14u)};
    if(!s.agents.cleanup_child)throw std::runtime_error("Native TU1.5 wrapped cleanup requires its current child owner");
    s.agents.cleanup_child({child},owner,fp);
    native_tu15_dispose_contact_reference(memory,agent.identity,fp,s.agents);
}
void native_tu15_cleanup_bv_contact_agent(SourcePhysicsContactMemory&memory,
    NativeTu15ContactAgent agent,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15AgentCleanupChildrenServices&s,const SourcePhysicsContactRecordObserver&observer){
    Memory m(memory,observer);
    const auto cleanup=[&](std::uint32_t child){
        const auto table=m.word(child);
        const NativeTu15ContactVirtualOwner owner{m.word(table+0x14u)};
        if(!s.agents.cleanup_child)throw std::runtime_error("Native BV cleanup requires its actual child owner");
        s.agents.cleanup_child({child},owner,fp);
    };
    cleanup(m.word(agent.identity+12u));
    if(const auto secondary=m.word(agent.identity+16u)){
        cleanup(secondary);
        m.word(0x2f0db7u,agent.identity+16u,0);
    }
    native_tu15_dispose_contact_reference(memory,agent.identity,fp,s.agents);
}
void native_tu15_cleanup_single_point_contact_agent(SourcePhysicsContactMemory&memory,
    NativeTu15ContactAgent agent,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15AgentCleanupChildrenServices&s){
    const SourcePhysicsContactRecordObserver none;Memory m(memory,none);
    const auto point=m.word(agent.identity+12u,2);
    if(point!=0xffffu){
        const auto manager=m.word(agent.identity+8u),table=m.word(manager);
        const NativeTu15ContactVirtualOwner owner{m.word(table+8u)};
        if(!s.remove_point)throw std::runtime_error("Native single-point cleanup requires its actual removal owner");
        s.remove_point(manager,owner,std::uint16_t(point),fp);
    }
    native_tu15_dispose_contact_reference(memory,agent.identity,fp,s.agents);
}
void native_tu15_destroy_contact_agent_base(SourcePhysicsContactMemory&memory,
    NativeTu15ContactAgent agent,const SourcePhysicsContactRecordObserver&observer){Memory m(memory,observer);m.word(0x2f7a10u,agent.identity,0x45edf0u);}
void native_tu15_destroy_contact_agent_storage(SourcePhysicsContactMemory&memory,
    bool records,NativeTu15ContactAgent agent,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ArrayGrowthServices&s,const SourcePhysicsContactRecordObserver&observer){
    Memory m(memory,observer);const auto p=agent.identity;
    if(records)m.word(0x2f359du,p,0x41cb58u);
    const auto capacity=m.word(p+0x14u);
    native_contact_detail::UnwindGuard base([&]{native_tu15_destroy_contact_agent_base(memory,agent,observer);});base.arm();
    if(!(capacity&0x80000000u)){
        const auto allocator=m.word(0x48c938u),table=m.word(allocator),bytes=(capacity&0x7fffffffu)*(records?12u:4u),data=m.word(p+0xcu);
        const NativeTu15ContactVirtualOwner owner{m.word(table+0x14u)};
        release(s,{allocator,owner.identity,data,bytes,0x12u},fp);
    }
    m.word(records?0x2f35d3u:0x2f7edau,p,0x45edf0u);base.disarm();
}
NativeTu15ContactAgent native_tu15_destroy_contact_agent(SourcePhysicsContactMemory&memory,
    NativeTu15AgentDestructor kind,NativeTu15ContactAgent agent,std::uint32_t disposing,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15AgentCleanupChildrenServices&s,
    const SourcePhysicsContactRecordObserver&observer){
    if(kind==NativeTu15AgentDestructor::owner_2f8090)native_tu15_destroy_contact_agent_storage(memory,false,agent,fp,s.allocation,observer);
    else if(kind==NativeTu15AgentDestructor::owner_3051a0)native_tu15_destroy_contact_agent_storage(memory,true,agent,fp,s.allocation,observer);
    else if(kind==NativeTu15AgentDestructor::owner_2fc540)native_tu15_destroy_contact_agent_base(memory,agent,observer);
    else throw std::runtime_error("Unqualified native TU1.5 scalar deleting destructor");
    if(disposing&1u){
        const SourcePhysicsContactRecordObserver none;Memory m(memory,none);const auto size=m.word(agent.identity+4u,2),allocator=m.word(0x48c938u),table=m.word(allocator);
        release(s.allocation,{allocator,m.word(table+0x14u),agent.identity,size,0x1au},fp);
    }
    return agent;
}
} // namespace halo2
