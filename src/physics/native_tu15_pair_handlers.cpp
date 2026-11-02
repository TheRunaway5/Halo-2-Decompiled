#include "halo2/native_tu15_pair_handlers.hpp"
#include <bit>
#include <stdexcept>
#include <optional>

namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory& memory_;
    const SourcePhysicsContactRecordObserver& observe_;
public:
    Memory(SourcePhysicsContactMemory& memory,const SourcePhysicsContactRecordObserver& observe)
        :memory_(memory),observe_(observe){}
    std::uint32_t word(std::uint32_t address,unsigned width=4)const{
        std::array<std::byte,4> bytes{};memory_.read(address,std::span(bytes).first(width));
        std::uint32_t result{};for(unsigned n=0;n<width;++n)result|=std::to_integer<unsigned>(bytes[n])<<(8u*n);
        return result;
    }
    std::uint32_t signed_byte(std::uint32_t address)const{
        return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(word(address,1)))));
    }
    void word(std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned width=4){
        std::array<std::byte,4> bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(8u*n));
        SourcePhysicsContactRecordStore event{site,address,{},{}};
        if(observe_){event.before.resize(width);memory_.read(address,event.before);event.written.assign(bytes.begin(),bytes.begin()+width);}
        memory_.write(address,std::span(bytes).first(width));if(observe_)observe_(event);
    }
};
template<class F>void require(const F& fn,const char* reason){if(!fn)throw std::runtime_error(reason);}
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
std::uint32_t payload(std::uint32_t handle){return handle?handle-0x10u:0u;}
void destroy(Memory& m,NativeTu15ContactAgent agent,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15PairHandlerServices& services){
    const auto table=m.word(agent.identity),manager=m.word(agent.identity+8u);
    const NativeTu15ContactVirtualOwner owner{m.word(table+0x14u)};
    require(services.destroy_agent,"Native TU1.5 pair deletion requires actual agent destruction");
    services.destroy_agent(agent,owner,fp);
    const auto manager_table=m.word(manager);
    const NativeTu15ContactVirtualOwner release{m.word(manager_table+0x10u)};
    require(services.agents.release_manager,"Native TU1.5 pair deletion requires actual manager release");
    services.agents.release_manager({manager},release,fp);
}
}
void native_tu15_create_pair(SourcePhysicsContactMemory& memory,std::uint32_t handler,
    std::uint32_t pair,NativeTu15PairHandlerScratch scratch,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15PairHandlerServices& services,const SourcePhysicsContactRecordObserver& observe){
    Memory m(memory,observe);auto cursor=m.word(handler+0x80u);
    if(cursor<m.word(handler+0x84u))for(;;){
        cursor=m.word(handler+0x80u);
        const auto wanted=m.word(pair),old=m.word(cursor),wanted_id=m.word(wanted+4u),old_id=m.word(old+4u);
        bool earlier=old_id<wanted_id;
        if(!earlier&&old_id==wanted_id){const auto second=m.word(cursor+4u),wanted_second=m.word(pair+4u);const auto second_id=m.word(second+4u),wanted_second_id=m.word(wanted_second+4u);earlier=second_id<wanted_second_id;}
        if(!earlier)break;
        const auto first=m.word(cursor),out=m.word(handler+0x88u);
        m.word(0x320f45u,out,first);m.word(0x320f4au,out+4u,m.word(cursor+4u));
        m.word(0x320f50u,out+8u,m.word(cursor+8u));m.word(0x320f56u,out+0xcu,m.word(cursor+0xcu));
        const auto destination=m.word(handler+0x88u),source=m.word(handler+0x80u);
        m.word(0x320f6eu,handler+0x88u,destination+16u);m.word(0x320f74u,handler+0x80u,source+16u);
        if(source+16u>=m.word(handler+0x84u))break;
    }
    if(m.word(handler+0x50u)){
        cursor=m.word(handler+0x80u);
        if(cursor!=m.word(handler+0x84u)){
            const auto first=m.word(pair),old=m.word(cursor),old_id=m.word(old+4u);
            if(old_id==m.word(first+4u)){
                const auto old_second=m.word(cursor+4u),second=m.word(pair+4u),old_second_id=m.word(old_second+4u);
                if(old_second_id==m.word(second+4u))return;
            }
        }
    }
    const auto out=m.word(handler+0x88u),first_handle=m.word(pair),first=payload(first_handle),second=payload(m.word(pair+4u));
    m.word(0x320feeu,out,first?first+0x10u:0u);m.word(0x320ff9u,out+4u,second?second+0x10u:0u);
    auto second_body=m.word(second+0x20u);const auto first_body=m.word(first+0x20u),first_island=m.word(first_body+0x44u);
    if(first_island!=m.word(second_body+0x44u)){
        const auto other=first_island==handler-0x1cu?m.word(second_body+0x44u):first_island;
        if(m.word(other+0x24u)!=0xffffffffu&&!m.word(other+0x29u,1)){
            require(services.activate_island,"Native TU1.5 pair creation requires actual island activation");
            services.activate_island({m.word(handler+4u)},{other},fp);
        }
        if(!m.word(other+0x30u,1)){
            require(services.defer_pair,"Native TU1.5 pair creation requires actual deferred-pair publication");
            services.defer_pair({other},pair,fp);return;
        }
        if(!m.word(first_body+0x40u,1)&&!m.word(second_body+0x40u,1)){
            require(services.merge_bodies,"Native TU1.5 pair creation requires actual body-island merge request");
            services.merge_bodies({m.word(handler+4u)},{first_body},{second_body},fp);
        }
    }
    const auto second_kind=m.signed_byte(second_body+0x48u),first_kind=m.signed_byte(first_body+0x48u),context=m.word(handler+0x7cu),manager_table=m.word(context);
    const auto manager_factory=m.word(manager_table+(second_kind+first_kind*8u)*4u+0xcu),factory_table=m.word(manager_factory);
    const NativeTu15ContactVirtualOwner manager_owner{m.word(factory_table+4u)};
    require(services.agents.create_manager,"Native TU1.5 pair creation requires actual contact-manager factory");
    const auto manager=services.agents.create_manager(manager_factory,manager_owner,{first},{second},{context},fp);
    const auto first_shape=m.word(first),first_shape_table=m.word(first_shape),context_after=m.word(handler+0x7cu),dispatch=m.word(context_after);
    m.word(0x3210a7u,scratch.formal_pair_data,dispatch);
    const NativeTu15ContactVirtualOwner first_class_owner{m.word(first_shape_table+0x14u)};
    require(services.agents.classify_shape,"Native TU1.5 pair creation requires actual shape class");
    const auto first_class=services.agents.classify_shape({first_shape},first_class_owner,fp);
    const auto second_shape=m.word(second),second_shape_table=m.word(second_shape);
    const auto second_class=services.agents.classify_shape({second_shape},{m.word(second_shape_table+0x14u)},fp);
    const NativeTu15ContactVirtualOwner agent_owner{m.word(dispatch+((first_class<<5u)+second_class)*4u+0x18cu)};
    require(services.agents.create_agent,"Native TU1.5 pair creation requires actual selected agent factory");
    const auto agent=services.agents.create_agent(agent_owner,{first},{second},{context_after},manager,fp);
    m.word(0x3210deu,out+8u,agent.identity);
    if(agent.identity&&m.word(agent.identity+8u)){
        const auto retained_first=m.word(first+0x20u),first_motion=m.word(retained_first+0x3cu),retained_second=m.word(second+0x20u),motion_table=m.word(first_motion);
        m.word(0x3210f5u,scratch.formal_pair_data,retained_second);
        require(services.agents.motion_kind,"Native TU1.5 pair creation requires actual motion kind");
        const auto first_kind_now=services.agents.motion_kind(first_motion,{m.word(motion_table+0x18u)},fp);
        bool disabled=false;
        if(first_kind_now==6u||m.word(retained_first+0x40u,1)){
            const auto second_motion=m.word(retained_second+0x3cu),second_motion_table=m.word(second_motion);
            const auto second_kind_now=services.agents.motion_kind(second_motion,{m.word(second_motion_table+0x18u)},fp);
            disabled=second_kind_now==6u||m.word(retained_second+0x40u,1);
        }
        if(disabled)m.word(0x32114cu,out+0xcu,0xffffffffu);
        else{
            const auto world=m.word(handler+4u),dispatcher=m.word(world+0x250u),table=m.word(dispatcher);
            require(services.agents.test_discrete_pair,"Native TU1.5 pair creation requires actual discrete pair query");
            const auto result=services.agents.test_discrete_pair(dispatcher,{m.word(table+4u)},scratch.formal_pair_data,{first},{second},fp);
            const auto accepted=m.word(result,1);
            m.word(0x321143u,out+0xcu,accepted?0u:0xffffffffu);
        }
    }else{
        m.word(0x321155u,out+8u,0);const auto table=m.word(manager.identity);
        require(services.agents.release_manager,"Native TU1.5 pair creation requires actual rejected-manager release");
        services.agents.release_manager(manager,{m.word(table+0x10u)},fp);
    }
    const auto destination=m.word(handler+0x88u);m.word(0x321163u,handler+0x88u,destination+16u);
}
static void delete_pair_data(SourcePhysicsContactMemory& memory,std::uint32_t handler,
    std::uint32_t pair,std::optional<NativeTu15PairDeletionScratch> scratch,NativeTu15PhysicsFrameFp& fp,const NativeTu15PairHandlerServices& services,
    const SourcePhysicsContactRecordObserver& observe){
    Memory m(memory,observe);auto cursor=m.word(handler+0x80u);const auto end=m.word(handler+0x84u);
    m.word(0x32027bu,handler+0xcu,1,1);
    if(cursor<end){
        const auto first=m.word(pair),first_id=m.word(first+4u);
        for(;;){const auto old=m.word(cursor),old_id=m.word(old+4u);bool earlier=old_id<first_id;
            if(!earlier&&old_id==first_id){const auto second=m.word(cursor+4u),second_id=m.word(second+4u),wanted=m.word(pair+4u);earlier=second_id<m.word(wanted+4u);}
            if(!earlier)break;
            const auto current_end=m.word(handler+0x84u);cursor+=16u;if(cursor>=current_end)break;
        }
    }
    bool found=false;
    if(cursor!=end){const auto old=m.word(cursor),first=m.word(pair),old_id=m.word(old+4u);
        if(old_id==m.word(first+4u)){const auto old_second=m.word(cursor+4u),second=m.word(pair+4u),old_second_id=m.word(old_second+4u);found=old_second_id==m.word(second+4u);}}
    if(found){
        if(m.word(handler+0x80u)<cursor)do{
            const auto source=m.word(handler+0x80u),first=m.word(source),destination=m.word(handler+0x88u);
            m.word(0x3202feu,destination,first);m.word(0x320303u,destination+4u,m.word(source+4u));
            m.word(0x320309u,destination+8u,m.word(source+8u));m.word(0x32030fu,destination+0xcu,m.word(source+0xcu));
            const auto from=m.word(handler+0x80u),to=m.word(handler+0x88u);
            m.word(0x32032bu,handler+0x88u,to+16u);m.word(0x320331u,handler+0x80u,from+16u);
            if(from+16u>=cursor)break;
        }while(true);
        const auto current=m.word(handler+0x80u),agent=m.word(current+8u);
        if(agent)destroy(m,{agent},fp,services);
        const auto source=m.word(handler+0x80u);m.word(0x320355u,handler+0x80u,source+16u);return;
    }
    auto first_handle=m.word(pair);const auto first_body=m.word(payload(first_handle)+0x20u),first_island=m.word(first_body+0x44u);
    auto pending=first_island+0x74u,count=m.word(first_island+0x78u),index=0u;std::uint32_t data{},second_handle{};
    const auto search=[&](std::uint32_t header,std::uint32_t initial_count,bool second_list)->bool{
        if(!less(0u,initial_count))return false;
        const auto first_id=m.word(first_handle+4u);data=m.word(header);auto slot=data;index=0;
        do{const auto first=m.word(slot);if(m.word(first+4u)==first_id){const auto second=m.word(slot+4u),wanted=second_list?second_handle:m.word(pair+4u),second_id=m.word(second+4u);if(second_id==m.word(wanted+4u))return true;}
            const auto current_count=m.word(header+4u);++index;slot+=8u;if(!less(index,current_count))break;
        }while(true);return false;
    };
    bool deferred=search(pending,count,false);
    if(!deferred){second_handle=m.word(pair+4u);if(scratch)m.word(0x3203c9u,scratch->formal_pair_data,second_handle);const auto second_body=m.word(payload(second_handle)+0x20u),second_island=m.word(second_body+0x44u);pending=second_island+0x74u;count=m.word(second_island+0x78u);deferred=search(pending,count,true);}
    if(deferred){const auto current_count=m.word(pending+4u)-1u;m.word(0x3203d8u,pending+4u,current_count);const auto first=m.word(data+current_count*8u);m.word(0x3203dfu,data+index*8u,first);m.word(0x3203e8u,data+index*8u+4u,m.word(data+current_count*8u+4u));return;}
    // The first failed search retains the original first handle; the second
    // handle is read from the current pair before selecting the other island.
    const auto current_first=m.word(payload(first_handle)+0x20u);auto other=m.word(current_first+0x44u);
    if(other==handler-0x1cu){const auto current_second=m.word(payload(second_handle)+0x20u);other=m.word(current_second+0x44u);}
    count=m.word(other+0x6cu);if(!less(0u,count))return;
    data=m.word(other+0x68u);const auto first_id=m.word(first_handle+4u);auto slot=data;index=0;
    for(;;){const auto first=m.word(slot);if(m.word(first+4u)==first_id){const auto second=m.word(slot+4u),second_id=m.word(second+4u);if(second_id==m.word(second_handle+4u))break;}
        const auto current_count=m.word(other+0x6cu);++index;slot+=16u;if(!less(index,current_count))return;
    }
    const auto offset=index*16u,agent=m.word(data+offset+8u);if(agent)destroy(m,{agent},fp,services);
    const auto last=m.word(other+0x6cu)-1u;m.word(0x3204ccu,other+0x6cu,last);
    if(!less(index,last))return;
    auto offset_now=offset;
    do{const auto destination=m.word(other+0x68u)+offset_now,source=destination+16u;
        m.word(0x3204dfu,destination,m.word(source));m.word(0x3204e4u,destination+4u,m.word(source+4u));
        m.word(0x3204eau,destination+8u,m.word(source+8u));m.word(0x3204f0u,destination+0xcu,m.word(source+0xcu));
        const auto current_count=m.word(other+0x6cu);++index;offset_now+=16u;if(!less(index,current_count))break;
    }while(true);
}
void native_tu15_delete_pair(SourcePhysicsContactMemory&memory,std::uint32_t handler,
    std::uint32_t pair,NativeTu15PairDeletionScratch scratch,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15PairHandlerServices&services,const SourcePhysicsContactRecordObserver&observe){
    delete_pair_data(memory,handler,pair,scratch,fp,services,observe);
}
void native_tu15_delete_pair(SourcePhysicsContactMemory&memory,std::uint32_t handler,
    std::uint32_t pair,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15PairHandlerServices&services,const SourcePhysicsContactRecordObserver&observe){
    delete_pair_data(memory,handler,pair,std::nullopt,fp,services,observe);
}
} // namespace halo2
