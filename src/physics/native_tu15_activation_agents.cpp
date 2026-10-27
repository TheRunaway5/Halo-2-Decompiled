#include "halo2/native_tu15_activation_agents.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observer_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observer_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(width));std::uint32_t value{};for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(n*8u);return value;}
    std::uint32_t signed_byte(std::uint32_t p)const{return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(word(p,1)))));}
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t value){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(n*8u));SourcePhysicsContactRecordStore event{pc,p,{},{}};if(observer_){event.before.resize(4);memory_.read(p,event.before);event.written.assign(b.begin(),b.end());}memory_.write(p,b);if(observer_)observer_(event);}
};
template<class F>void require(const F&f,const char*message){if(!f)throw std::runtime_error(message);}
}
void native_tu15_recreate_island_agents(SourcePhysicsContactMemory&memory,NativeTu15PhysicsIsland island,
 NativeTu15ActivationAgentScratch scratch,NativeTu15PhysicsFrameFp&fp,
 const NativeTu15ActivationAgentServices&s,const SourcePhysicsContactRecordObserver&observe,
 NativeTu15ActivationAgentStats*stats){
    Memory m(memory,observe);if(stats)++stats->calls;auto count=m.word(island.identity+0x6cu);std::uint32_t index{},offset{};
    if(less(0,count))do{
        const auto entries=m.word(island.identity+0x68u),entry=entries+offset,existing=m.word(entry+8u);if(stats)++stats->pairs;
        if(existing){if(stats)++stats->existing_agents;}
        else{
            const auto first_entry=m.word(entry),second_entry=m.word(entry+4u),first_type=m.word(first_entry+8u);
            const NativeTu15ContactPayload first{first_entry-0x10u},second{second_entry-0x10u};
            const auto first_entity=first_type==1u?m.word(first.identity+0x20u):0u;
            const auto second_entity=m.word(second.identity+0x18u)==1u?m.word(second.identity+0x20u):0u;
            const auto world=m.word(island.identity+0x20u),filter=m.word(world+0xd0u);bool filtered=false;
            if(filter){const auto receiver=filter+8u,table=m.word(receiver);const NativeTu15ContactVirtualOwner owner{m.word(table)};
                require(s.filter_pair,"Native TU1.5 activation agents require actual pair filter");const auto result=s.filter_pair(receiver,owner,scratch.filter_result,first,second,fp);
                if(!m.word(result,1)){const auto current_entries=m.word(island.identity+0x68u);m.word(0x2828fdu,current_entries+offset+8u,0);filtered=true;if(stats)++stats->filtered_pairs;}}
            if(!filtered){
                const auto current_world=m.word(island.identity+0x20u);const NativeTu15ContactContext manager_context{m.word(current_world+0xccu)};
                const auto first_kind=m.signed_byte(first_entity+0x48u),second_kind=m.signed_byte(second_entity+0x48u),manager_index=second_kind+first_kind*8u;
                const auto manager_table=m.word(manager_context.identity),manager_factory=m.word(manager_table+manager_index*4u+12u),manager_virtual=m.word(manager_factory);
                const NativeTu15ContactVirtualOwner manager_owner{m.word(manager_virtual+4u)};
                require(s.create_manager,"Native TU1.5 activation agents require actual contact manager factory");const auto manager=s.create_manager(manager_factory,manager_owner,first,second,manager_context,fp);if(stats)++stats->managers_created;
                const auto dispatch_world=m.word(island.identity+0x20u);const NativeTu15ContactContext agent_context{m.word(dispatch_world+0xccu)};const auto dispatch=m.word(agent_context.identity);
                const NativeTu15ContactChild first_shape{m.word(first.identity)};const auto first_table=m.word(first_shape.identity);const NativeTu15ContactVirtualOwner first_owner{m.word(first_table+20u)};
                require(s.classify_shape,"Native TU1.5 activation agents require actual shape class");const auto first_class=s.classify_shape(first_shape,first_owner,fp);if(stats)++stats->class_calls;
                const NativeTu15ContactChild second_shape{m.word(second.identity)};const auto second_table=m.word(second_shape.identity);const NativeTu15ContactVirtualOwner second_owner{m.word(second_table+20u)};
                const auto second_class=s.classify_shape(second_shape,second_owner,fp);if(stats)++stats->class_calls;
                const NativeTu15ContactVirtualOwner factory{m.word(dispatch+((first_class<<5u)+second_class)*4u+0x18cu)};
                const auto factory_entries=m.word(island.identity+0x68u),destination=factory_entries+offset;
                require(s.create_agent,"Native TU1.5 activation agents require actual selected agent factory");const auto created=s.create_agent(factory,first,second,agent_context,manager,fp);if(stats)++stats->factory_calls;
                m.word(0x28297fu,destination+8u,created.identity);
                const auto current_entries=m.word(island.identity+0x68u),agent_field=current_entries+offset+8u,agent=m.word(agent_field);
                if(agent&&m.word(agent+8u)){
                    const auto current_first_type=m.word(first.identity+0x18u);
                    const auto current_first=current_first_type==1u?m.word(first.identity+0x20u):0u;
                    const auto current_second=m.word(second.identity+0x18u)==1u?m.word(second.identity+0x20u):0u;
                    const auto first_motion=m.word(current_first+0x3cu),first_motion_table=m.word(first_motion);const NativeTu15ContactVirtualOwner first_motion_owner{m.word(first_motion_table+24u)};
                    require(s.motion_kind,"Native TU1.5 activation agents require actual motion type owner");const auto first_motion_kind=s.motion_kind(first_motion,first_motion_owner,fp);if(stats)++stats->motion_queries;
                    bool disabled=false;
                    if(first_motion_kind==6u||m.word(current_first+0x40u,1)){
                        const auto second_motion=m.word(current_second+0x3cu),second_motion_table=m.word(second_motion);const NativeTu15ContactVirtualOwner second_motion_owner{m.word(second_motion_table+24u)};
                        const auto second_motion_kind=s.motion_kind(second_motion,second_motion_owner,fp);if(stats)++stats->motion_queries;
                        disabled=second_motion_kind==6u||m.word(current_second+0x40u,1);
                    }
                    if(disabled){const auto rank_entries=m.word(island.identity+0x68u);m.word(0x282a3cu,rank_entries+offset+12u,0xffffffffu);if(stats)++stats->disabled_ranks;}
                    else{
                        const NativeTu15PhysicsWorld query_world{m.word(island.identity+0x20u)};require(s.discrete_dispatcher,"Native TU1.5 activation agents require actual316200 owner");const auto dispatcher=s.discrete_dispatcher(query_world,fp);const auto table=m.word(dispatcher);const NativeTu15ContactVirtualOwner owner{m.word(table+4u)};
                        require(s.test_discrete_pair,"Native TU1.5 activation agents require actual discrete pair query");const auto result=s.test_discrete_pair(dispatcher,owner,scratch.discrete_result,first,second,fp);if(stats)++stats->discrete_queries;
                        const bool accepted=m.word(result,1)!=0;const auto rank_entries=m.word(island.identity+0x68u);m.word(accepted?0x282a22u:0x282a2fu,rank_entries+offset+12u,accepted?0u:0xffffffffu);if(stats){if(accepted)++stats->zero_ranks;else ++stats->disabled_ranks;}
                    }
                    if(stats)++stats->retained_agents;
                }else{
                    m.word(0x282a4au,agent_field,0);const auto table=m.word(manager.identity);const NativeTu15ContactVirtualOwner owner{m.word(table+16u)};require(s.release_manager,"Native TU1.5 activation agents require actual contact manager release");s.release_manager(manager,owner,fp);if(stats)++stats->released_managers;
                }
            }
        }
        count=m.word(island.identity+0x6cu);++index;offset+=16u;
    }while(less(index,count));
    count=m.word(island.identity+0x40u);index=0;
    if(less(0,count))do{
        const auto bodies=m.word(island.identity+0x3cu),body=m.word(bodies+index*4u),policy=m.word(body+0x54u),table=m.word(policy);const NativeTu15ContactVirtualOwner owner{m.word(table+28u)};
        require(s.body_policy_kind,"Native TU1.5 activation agents require actual body policy type");const auto kind=s.body_policy_kind(policy,owner,fp);if(stats)++stats->body_policy_queries;
        if(kind==1u){const auto current_bodies=m.word(island.identity+0x3cu);const NativeTu15PhysicsBody current_body{m.word(current_bodies+index*4u)};require(s.mark_body,"Native TU1.5 activation agents require actual31B730 owner");s.mark_body(current_body,2u,fp);if(stats)++stats->bodies_marked;}
        count=m.word(island.identity+0x40u);++index;
    }while(less(index,count));
}
} // namespace halo2
