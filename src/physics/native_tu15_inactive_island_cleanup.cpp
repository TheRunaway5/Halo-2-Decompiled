#include "halo2/native_tu15_inactive_island_cleanup.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned n=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v{};
    for(unsigned i=0;i<n;++i)v|=std::to_integer<unsigned>(b[i])<<(i*8u);return v;
}
void clear(SourcePhysicsContactMemory&m,std::uint32_t p,const SourcePhysicsContactRecordObserver&o){
    std::array<std::byte,4>b{};SourcePhysicsContactRecordStore e{0x282b1bu,p,{},{}};
    if(o){e.before.resize(4);m.read(p,e.before);e.written.assign(b.begin(),b.end());}
    m.write(p,b);if(o)o(e);
}
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
template<class F>void require(const F&f,const char*s){if(!f)throw std::runtime_error(s);}
std::uint32_t body(const SourcePhysicsContactMemory&m,std::uint32_t handle){
    const auto kind=word(m,handle+8u);return kind==1u?word(m,handle+0x10u):0u;
}
}
void native_tu15_cleanup_inactive_island(SourcePhysicsContactMemory&m,NativeTu15PhysicsIsland island,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15InactiveIslandCleanupServices&s,
    const SourcePhysicsContactRecordObserver&o,NativeTu15InactiveIslandCleanupStats*stats){
    if(stats)++stats->calls;
    auto count=word(m,island.identity+0x6cu);std::uint32_t index{},offset{};
    if(less(0u,count))do{
        const auto row=word(m,island.identity+0x68u)+offset;
        bool invalid=word(m,body(m,word(m,row))+0x40u,1)!=0;
        if(!invalid)invalid=word(m,body(m,word(m,row+4u))+0x40u,1)!=0;
        if(invalid){
            const auto agent=word(m,row+8u);
            if(agent){
                const auto table=word(m,agent),manager=word(m,agent+8u),owner=word(m,table+0x14u);
                require(s.invalidate_agent,"Native TU1.5 inactive island requires actual agent invalidation");
                if(stats)++stats->invalidated_agents;s.invalidate_agent(agent,{owner},fp);
                const auto manager_owner=word(m,word(m,manager)+0x10u);
                require(s.invalidate_manager,"Native TU1.5 inactive island requires actual manager invalidation");
                if(stats)++stats->invalidated_managers;s.invalidate_manager(manager,{manager_owner},fp);
            }
            clear(m,word(m,island.identity+0x68u)+offset+8u,o);if(stats)++stats->cleared_pairs;
        }
        count=word(m,island.identity+0x6cu);++index;offset+=0x10u;
    }while(less(index,count));
    count=word(m,island.identity+0x40u);index=0;
    if(less(0u,count))do{
        const auto current=word(m,word(m,island.identity+0x3cu)+index*4u),policy=word(m,current+0x54u);
        const auto owner=word(m,word(m,policy)+0x1cu);
        require(s.policy_kind,"Native TU1.5 inactive island requires actual policy kind");
        if(stats)++stats->policy_queries;
        if(s.policy_kind(policy,{owner},fp)==2u){
            const NativeTu15PhysicsBody selected{word(m,word(m,island.identity+0x3cu)+index*4u)};
            require(s.select_policy,"Native TU1.5 inactive island requires actual policy selection");
            if(stats)++stats->policy_selections;s.select_policy(selected,1u,fp);
        }
        count=word(m,island.identity+0x40u);++index;
    }while(less(index,count));
}
} // namespace halo2
