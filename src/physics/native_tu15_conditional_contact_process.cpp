#include "halo2/native_tu15_conditional_contact_process.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory& memory_;
    const SourcePhysicsContactRecordObserver& observe_;
public:
    Memory(SourcePhysicsContactMemory& m,const SourcePhysicsContactRecordObserver& o)
        :memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p) const {
        std::array<std::byte,4> b{};memory_.read(p,b);std::uint32_t result{};
        for(unsigned k=0;k<4;++k)result|=std::to_integer<unsigned>(b[k])<<(k*8u);
        return result;
    }
    std::uint8_t byte(std::uint32_t p) const {
        std::array<std::byte,1> b{};memory_.read(p,b);return std::to_integer<std::uint8_t>(b[0]);
    }
    void store(std::uint32_t pc,std::uint32_t p,std::span<const std::byte> b) {
        SourcePhysicsContactRecordStore event{pc,p,{},{}};
        if(observe_){event.before.resize(b.size());memory_.read(p,event.before);}
        memory_.write(p,b);
        if(observe_){event.written.assign(b.begin(),b.end());observe_(event);}
    }
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t value) {
        std::array<std::byte,4> b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(value>>(k*8u));store(pc,p,b);
    }
    void byte(std::uint32_t pc,std::uint32_t p,std::uint8_t value) {
        const std::array b{std::byte(value)};store(pc,p,b);
    }
};
}
void native_tu15_process_conditional_contact_agent(SourcePhysicsContactMemory& memory,
    NativeTu15IslandCollisionAgent agent,NativeTu15PhysicsBody first,
    NativeTu15PhysicsBody second,NativeTu15PhysicsStepConfiguration configuration,
    NativeTu15IslandContactOutput output,NativeTu15ConditionalContactScratch scratch,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15ConditionalContactServices& services,
    const SourcePhysicsContactRecordObserver& observe,
    NativeTu15ConditionalContactStats* stats) {
    if(stats)++stats->calls;
    try {
        Memory m(memory,observe);
        const auto profile=[&](std::uint32_t cursor,std::uint32_t limit,
            std::uint32_t marker,std::uint32_t marker_pc,std::uint32_t time_pc,
            std::uint32_t cursor_pc) {
            if(cursor>=limit)return;
            m.word(marker_pc,cursor,marker);
            if(!services.timestamp_counter)
                throw std::runtime_error("Native conditional contact requires its actual profiling clock");
            const auto timestamp=std::uint32_t(services.timestamp_counter());
            m.word(time_pc,cursor+4u,timestamp);
            m.word(cursor_pc,0x48c974u,cursor+12u);
            if(stats)++stats->profile_entries;
        };
        const auto initial_limit=m.word(0x48c978u);
        const auto initial_cursor=m.word(0x48c974u);
        profile(initial_cursor,initial_limit,0x41cac8u,0x2f0f43u,0x2f0f53u,0x2f0f59u);
        const auto transform=m.word(first.identity+8u);
        const auto shape=m.word(first.identity);
        m.word(0x2f0f68u,scratch.body.identity+12u,first.identity);
        m.word(0x2f0f6cu,scratch.body.identity+8u,transform);
        const auto query_shape=m.word(shape+12u);
        const auto inherited=m.word(first.identity+4u);
        m.word(0x2f0f76u,scratch.body.identity+4u,inherited);
        m.word(0x2f0f7au,scratch.body.identity,query_shape);
        m.word(0x2f0f7eu,scratch.collector.identity,0x41ca8cu);
        m.byte(0x2f0f86u,scratch.collector.identity+4u,0);
        const auto query=m.word(agent.identity+12u);
        const auto query_table=m.word(query);
        const NativeTu15PhysicsVirtualOwner query_owner{m.word(query_table+4u)};
        if(!services.query)
            throw std::runtime_error("Native conditional contact requires actual query virtual+4");
        if(stats)++stats->query_calls;
        services.query({query},query_owner,scratch.body,second,configuration,
            scratch.collector,fp);
        if(m.byte(scratch.collector.identity+4u)) {
            const auto cursor=m.word(0x48c974u);
            const auto limit=m.word(0x48c978u);
            profile(cursor,limit,0x41cac0u,0x2f0fc9u,0x2f0fd9u,0x2f0fdfu);
            const auto current_inherited=m.word(first.identity+4u);
            const auto process_shape=m.word(shape+16u);
            m.word(0x2f0fefu,scratch.body.identity+4u,current_inherited);
            auto child=m.word(agent.identity+16u);
            m.word(0x2f0ff8u,scratch.body.identity,process_shape);
            if(!child) {
                const auto manager=m.word(agent.identity+8u);
                const auto dispatcher=m.word(configuration.identity);
                const auto first_table=m.word(process_shape);
                const NativeTu15PhysicsVirtualOwner first_class_owner{m.word(first_table+20u)};
                if(!services.shape_class)
                    throw std::runtime_error("Native conditional contact requires actual shape classes");
                if(stats)++stats->class_calls;
                const auto first_class=services.shape_class(process_shape,first_class_owner,fp);
                const auto second_shape=m.word(second.identity);
                const auto second_table=m.word(second_shape);
                const NativeTu15PhysicsVirtualOwner second_class_owner{m.word(second_table+20u)};
                if(stats)++stats->class_calls;
                const auto second_class=services.shape_class(second_shape,second_class_owner,fp);
                const NativeTu15PhysicsVirtualOwner constructor{
                    m.word(dispatcher+0x18cu+((first_class<<5u)+second_class)*4u)};
                if(!services.create_agent)
                    throw std::runtime_error("Native conditional contact requires actual dispatched constructor");
                if(stats)++stats->factory_calls;
                child=services.create_agent(constructor,scratch.body,second,
                    configuration,manager,fp).identity;
                m.word(0x2f103bu,agent.identity+16u,child);
            }
            child=m.word(agent.identity+16u);
            const auto table=m.word(child);
            const NativeTu15PhysicsVirtualOwner owner{m.word(table+16u)};
            if(!services.processing.process_child)
                throw std::runtime_error("Native conditional contact requires actual child processing");
            if(stats)++stats->process_calls;
            services.processing.process_child({{child},owner,scratch.body,second,
                configuration,output},fp);
        } else {
            const auto child=m.word(agent.identity+16u);
            if(child) {
                const auto table=m.word(child);
                const NativeTu15PhysicsVirtualOwner owner{m.word(table+20u)};
                if(!services.clear_agent)
                    throw std::runtime_error("Native conditional contact requires actual cached-agent clear");
                if(stats)++stats->clear_calls;
                services.clear_agent({child},owner,fp);
                m.word(0x2f1060u,agent.identity+16u,0);
            }
        }
        const auto final_cursor=m.word(0x48c974u);
        const auto final_limit=m.word(0x48c978u);
        profile(final_cursor,final_limit,0x41cabcu,0x2f1079u,0x2f1089u,0x2f108fu);
        if(stats)++stats->completed;
    } catch(...) {
        if(stats)++stats->refusals;
        throw;
    }
}
} // namespace halo2
