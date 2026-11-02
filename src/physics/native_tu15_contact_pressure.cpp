#include "halo2/native_tu15_contact_pressure.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory& memory_;const SourcePhysicsContactRecordObserver& observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{
        std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(width));std::uint32_t n{};
        for(unsigned k=0;k<width;++k)n|=std::to_integer<unsigned>(b[k])<<(8u*k);return n;
    }
    void word(std::uint32_t site,std::uint32_t p,std::uint32_t n,unsigned width=4){
        std::array<std::byte,4>b{};for(unsigned k=0;k<width;++k)b[k]=std::byte(n>>(8u*k));
        SourcePhysicsContactRecordStore e{site,p,{},{}};
        if(observe_){e.before.resize(width);memory_.read(p,e.before);e.written.assign(b.begin(),b.begin()+width);}
        memory_.write(p,std::span(b).first(width));if(observe_)observe_(e);
    }
};
std::int32_t signed32(std::uint32_t n){return std::bit_cast<std::int32_t>(n);}
[[noreturn]] void missing(NativeTu15ContactPressureStats*s,const char* owner){
    if(s)++s->unbound_services;throw std::runtime_error(owner);
}
}
void native_tu15_prune_contact_pressure(SourcePhysicsContactMemory& memory,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15ContactPressureServices& services,
    const SourcePhysicsContactRecordObserver& observe,NativeTu15ContactPressureStats* stats){
    if(stats)++stats->calls;Memory m(memory,observe);auto world=m.word(0x52be48u);
    if(!world){if(stats)++stats->null_worlds;return;}
    if(m.word(0x48b873u,1)){if(stats)++stats->reentrant_skips;return;}
    m.word(0x1cce1eu,0x48b873u,1,1);auto count=m.word(world+0xcu);
    if(signed32(count)>0){
        std::uint32_t index{};
        do{
            const auto entries=m.word(world+8u),island=m.word(entries+index*4u);
            auto effective=m.word(island+0x6cu);if(stats)++stats->islands;
            if(signed32(effective)>24){
                if(stats)++stats->dense_islands;
                std::uint32_t row{},offset{};
                if(signed32(effective)>0)do{
                    const auto contacts=m.word(island+0x68u);
                    auto handle=m.word(contacts+offset);const auto first_type=m.word(handle+8u);
                    const auto first=first_type==1?m.word(handle+0x10u):0u;
                    handle=m.word(contacts+offset+4u);const auto second_type=m.word(handle+8u);
                    const auto second=second_type==1?m.word(handle+0x10u):0u;
                    const auto kind=[&](std::uint32_t body){
                        const auto shape=m.word(body+0x3cu),table=m.word(shape),owner=m.word(table+0x18u);
                        if(!services.classify_shape)missing(stats,"Native contact pressure requires its actual shape type owner");
                        if(stats)++stats->shape_queries;return services.classify_shape(shape,owner,fp);
                    };
                    if(kind(first)==7||kind(first)==6||kind(second)==7||kind(second)==6)--effective;
                    ++row;offset+=16u;count=m.word(island+0x6cu);
                }while(signed32(row)<signed32(count));
                if(signed32(effective)>24){
                    std::uint32_t selected=0xffffffffu;
                    for(std::uint32_t mode=0;mode<3;++mode){
                        std::uint32_t body_index{},best=0x80000000u;count=m.word(island+0x40u);
                        if(signed32(count)>0)do{
                            const auto bodies=m.word(island+0x3cu),body=m.word(bodies+body_index*4u);
                            const auto priority=m.word(body+0x8cu);
                            if(signed32(priority)>signed32(best)){
                                if(!services.select_candidate)missing(stats,"Native contact pressure requires actual1CCAD0 candidate owner");
                                if(stats)++stats->candidate_queries;
                                const auto candidate=services.select_candidate({body,mode,0xffffffffu,true,false},fp);
                                if(candidate!=0xffffffffu){best=priority;selected=candidate;}
                            }
                            count=m.word(island+0x40u);++body_index;
                        }while(signed32(body_index)<signed32(count));
                        if(selected!=0xffffffffu)break;
                    }
                    if(selected!=0xffffffffu){
                        auto effect_state=m.word(0x4ecc1cu);effect_state=m.word(effect_state+0xb4u);
                        auto pool=m.word(0x4ecbd0u);const auto object_offset=(selected&0xffffu)*12u;
                        auto rows=m.word(pool+0x44u);const auto effect=m.word(effect_state+4u);
                        auto object=m.word(rows+object_offset+8u);
                        if(effect!=0xffffffffu){
                            const auto system=m.word(0x474d10u);
                            if(!services.emit_effect)missing(stats,"Native contact pressure requires actual179470 effect owner");
                            if(stats)++stats->effect_calls;services.emit_effect(system,effect,object+0x70u,object+0x30u,fp);
                        }
                        pool=m.word(0x4ecbd0u);rows=m.word(pool+0x44u);object=m.word(rows+object_offset+8u);
                        auto component=m.word(object+0xb4u);
                        if(component!=0xffffffffu){
                            const auto components=m.word(0x52be5cu),component_rows=m.word(components+0x44u);
                            if(!services.remove_component)missing(stats,"Native contact pressure requires actual1D9510 removal owner");
                            if(stats)++stats->component_removals;services.remove_component(component_rows+(component&0xffffu)*160u,fp);
                        }
                        m.word(0x1ccfe5u,object+0xc0u,m.word(object+0xc0u,1)&0xbfu,1);
                        pool=m.word(0x4ecbd0u);rows=m.word(pool+0x44u);auto current=m.word(rows+object_offset+8u);
                        const bool collect=(m.word(current+0xc0u,1)&0x40u)!=0;
                        if(collect){if(!services.collect_memory)missing(stats,"Native contact pressure requires actual1496F0 memory owner");services.collect_memory(fp);}
                        pool=m.word(0x4ecbd0u);rows=m.word(pool+0x44u);object=m.word(rows+object_offset+8u);
                        component=m.word(object+0xb4u);const auto rows_field=pool+0x44u;std::uint32_t active_counter{};
                        if(component!=0xffffffffu){
                            const auto components=m.word(0x52be5cu),component_rows=m.word(components+0x44u);
                            const auto component_data=component_rows+(component&0xffffu)*160u,other=m.word(component_data+8u);
                            if(!services.destroy_component)missing(stats,"Native contact pressure requires actual1D7CA0 destruction owner");
                            if(stats)++stats->component_destructions;services.destroy_component(component_data,fp);
                            const auto component_pool=m.word(0x52be5cu);
                            if(!services.free_component)missing(stats,"Native contact pressure requires actual16E8A0 data-array owner");
                            if(stats)++stats->component_frees;services.free_component(component_pool,component,fp);
                            const auto object_pool=m.word(0x4ecbd0u),object_rows=m.word(object_pool+0x44u);
                            current=m.word(object_rows+(other&0xffffu)*12u+8u);
                            const bool active=(m.word(current+0xc0u,1)&1u)!=0;active_counter=m.word(0x52be44u);
                            if(active){m.word(0x1cd080u,current+0xc0u,m.word(current+0xc0u,1)&0xfeu,1);m.word(0x1cd087u,active_counter,m.word(active_counter)-1u);}
                            m.word(0x1cd08du,object+0xb4u,0xffffffffu);
                        }else active_counter=m.word(0x52be44u);
                        rows=m.word(rows_field);current=m.word(rows+object_offset+8u);
                        if(m.word(current+0xc0u,1)&1u){m.word(0x1cd0aeu,current+0xc0u,m.word(current+0xc0u,1)&0xfeu,1);m.word(0x1cd0b5u,active_counter,m.word(active_counter)-1u);}
                        if(collect){
                            if(!services.update_contacts)missing(stats,"Native contact pressure requires actual282C10 contact owner");services.update_contacts(fp);
                            if(!services.collect_memory)missing(stats,"Native contact pressure requires actual1496F0 memory owner");services.collect_memory(fp);
                        }
                        if(!services.delete_object)missing(stats,"Native contact pressure requires actualBA570 object owner");
                        if(stats)++stats->object_deletions;services.delete_object(selected,fp);
                    }
                }
            }
            world=m.word(0x52be48u);count=m.word(world+0xcu);++index;
        }while(signed32(index)<signed32(count));
    }
    m.word(0x1cd0f1u,0x48b873u,0,1);
}
}
