#include "halo2/native_tu15_dirty_island_objects.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{
        std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(width));std::uint32_t value{};
        for(unsigned i=0;i<width;++i)value|=std::to_integer<unsigned>(b[i])<<(8u*i);return value;
    }
    void word(std::uint32_t site,std::uint32_t p,std::uint32_t value,unsigned width=4){
        std::array<std::byte,4>b{};for(unsigned i=0;i<width;++i)b[i]=std::byte(value>>(8u*i));
        SourcePhysicsContactRecordStore event{site,p,{},{}};
        if(observe_){event.before.resize(width);memory_.read(p,event.before);event.written.assign(b.begin(),b.begin()+width);}
        memory_.write(p,std::span(b).first(width));if(observe_)observe_(event);
    }
};
bool positive(std::uint32_t n){return std::bit_cast<std::int32_t>(n)>0;}
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
}
void native_tu15_process_dirty_island_objects(SourcePhysicsContactMemory&memory,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15DirtyIslandObjectServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15DirtyIslandObjectStats*stats){
    if(stats)++stats->calls;Memory m(memory,observe);
    const auto missing=[&](std::uint32_t owner)->void{
        if(stats){++stats->unbound_services;++stats->unbound_owners[owner];}
        throw std::runtime_error("Native TU1.5 dirty island objects requires native owner="+std::to_string(owner));
    };
    if(!m.word(0x56dfccu,1)){if(stats)++stats->pending_skips;return;}
    if(m.word(0x48b873u,1)){if(stats)++stats->reentrant_skips;return;}
    m.word(0x282c35u,0x48b873u,1,1);
    for(std::uint32_t cohort=0;cohort<2u;++cohort){
        const auto world=m.word(0x52be48u),header=world+(cohort?0x14u:8u);auto count=m.word(header+4u);
        if(!positive(count))continue;
        for(std::uint32_t island_index=0;;){
            const auto array=m.word(header),island=m.word(array+island_index*4u);if(stats)++stats->island_rows;
            if(m.word(island+0xaeu,1)){
                // Removal may change both the list and its current count. Source
                // resumes at this row, rather than advancing the old index.
                --island_index;const auto initial_bodies=m.word(island+0x40u);
                m.word(0x282c91u,island+0xaeu,0,1);if(stats)++stats->dirty_islands;
                std::array<std::uint32_t,128>selected{},deferred{};std::uint32_t selected_count{},deferred_count{};
                for(unsigned pass=0;pass<2u;++pass){
                    selected_count=0;deferred_count=0;
                    if(positive(initial_bodies)){
                        auto cursor=m.word(island+0x3cu),remaining=m.word(island+0x40u);
                        do{
                            const auto body=m.word(cursor);if(stats)++stats->body_rows;
                            if(!m.word(body+0x40u,1)){
                                const auto properties=m.word(body+0x34u);std::uint32_t component{};
                                if(positive(properties)){
                                    const auto values=m.word(body+0x30u);
                                    for(std::uint32_t property=0;;++property){
                                        if(m.word(values+property*8u)==0x2001u){component=m.word(values+property*8u+4u);break;}
                                        if(!less(property+1u,properties))break;
                                    }
                                }
                                const auto components=m.word(0x52be5cu),rows=m.word(components+0x44u);
                                const auto datum=m.word(rows+(component&0xffffu)*160u+8u);if(stats)++stats->component_queries;
                                const auto objects=m.word(0x4ecbd0u),headers=m.word(objects+0x44u),object_header=headers+(datum&0xffffu)*12u;
                                bool candidate=true;
                                if(pass==0){const auto type=m.word(object_header+3u,1);candidate=((1u<<(type&31u))&0x81u)==0;}
                                bool exists=false;
                                for(std::uint32_t i=0;i<selected_count;++i)if(selected[i]==datum){exists=true;break;}
                                for(std::uint32_t i=0;!exists&&i<deferred_count;++i)if(deferred[i]==datum){exists=true;break;}
                                if(!exists){
                                    auto&size=candidate?selected_count:deferred_count;auto&values=candidate?selected:deferred;
                                    if(size>=values.size()){
                                        if(stats)++stats->unsupported_local_set_sizes;
                                        throw std::runtime_error("Native TU1.5 dirty island object set exceeds its source local DATA domain");
                                    }
                                    values[size++]=datum;
                                }
                            }
                            cursor+=4u;--remaining;
                        }while(remaining);
                    }
                    if(positive(selected_count))break;
                }
                for(std::uint32_t i=0;i<selected_count;++i){
                    const auto datum=selected[i],objects=m.word(0x4ecbd0u),game=m.word(0x4f3218u),headers=m.word(objects+0x44u);
                    const auto mode=m.word(game+0xcu,1),offset=(datum&0xffffu)*12u,object=m.word(headers+offset+8u);
                    if(stats)++stats->candidate_objects;
                    if(mode==4u&&m.word(object+0xd4u)!=0xffffffffu){
                        m.word(0x282e1au,object+0xc0u,m.word(object+0xc0u,1)|0x20u,1);
                        if(!services.set_component_flags)missing(0x1cc310u);services.set_component_flags(datum,0u,fp,depth);continue;
                    }
                    const bool collect=((m.word(object+0xc0u,1)>>6u)&1u)!=0;
                    if(collect){if(!services.collect_memory)missing(0x1496f0u);services.collect_memory(fp,depth);}
                    const auto current_objects=m.word(0x4ecbd0u),current_headers=m.word(current_objects+0x44u),current_object=m.word(current_headers+offset+8u);
                    const auto component=m.word(current_object+0xb4u);auto headers_field=current_objects+0x44u;std::uint32_t active_counter{};
                    if(component!=0xffffffffu){
                        const auto components=m.word(0x52be5cu),rows=m.word(components+0x44u),row=rows+(component&0xffffu)*160u,own=m.word(row+8u);
                        if(!services.destroy_component)missing(0x1d7ca0u);if(stats)++stats->component_destructions;services.destroy_component(row,fp,depth);
                        const auto pool=m.word(0x52be5cu);
                        if(!services.free_component)missing(0x16e8a0u);if(stats)++stats->component_frees;services.free_component(pool,component,fp,depth);
                        const auto latest_objects=m.word(0x4ecbd0u),latest_headers=m.word(latest_objects+0x44u);active_counter=m.word(0x52be44u);headers_field=latest_objects+0x44u;
                        const auto own_object=m.word(latest_headers+(own&0xffffu)*12u+8u);
                        if(m.word(own_object+0xc0u,1)&1u){m.word(0x282ebfu,own_object+0xc0u,m.word(own_object+0xc0u,1)&0xfeu,1);m.word(0x282ec6u,active_counter,m.word(active_counter)-1u);}
                        m.word(0x282ed4u,current_object+0xb4u,0xffffffffu);
                    }else active_counter=m.word(0x52be44u);
                    const auto latest_headers=m.word(headers_field),latest_object=m.word(latest_headers+offset+8u);
                    if(m.word(latest_object+0xc0u,1)&1u){m.word(0x282ef5u,latest_object+0xc0u,m.word(latest_object+0xc0u,1)&0xfeu,1);m.word(0x282efcu,active_counter,m.word(active_counter)-1u);}
                    if(collect){
                        if(stats)++stats->recursive_updates;
                        native_tu15_process_dirty_island_objects(memory,fp,depth,services,observe,stats);
                        if(!services.collect_memory)missing(0x1496f0u);services.collect_memory(fp,depth);
                    }
                    if(!services.delete_object)missing(0xba570u);if(stats)++stats->object_deletions;services.delete_object(datum,fp,depth);
                }
                for(std::uint32_t i=0;i<deferred_count;++i){
                    const auto datum=deferred[i],objects=m.word(0x4ecbd0u),headers=m.word(objects+0x44u),object=m.word(headers+(datum&0xffffu)*12u+8u),component=m.word(object+0xb4u);
                    if(component!=0xffffffffu){
                        const auto pool=m.word(0x52be5cu),rows=m.word(pool+0x44u),row=rows+(component&0xffffu)*160u;
                        if(!services.remove_component)missing(0x1d9510u);services.remove_component(row,fp,depth);
                    }
                    m.word(0x282f75u,object+0xc0u,m.word(object+0xc0u,1)&0xbfu,1);
                    if(!services.defer_object)missing(0xba8c0u);if(stats)++stats->deferred_objects;services.defer_object(datum,fp,depth);
                }
            }
            count=m.word(header+4u);++island_index;if(!less(island_index,count))break;
        }
    }
    m.word(0x282fb4u,0x48b873u,0,1);m.word(0x282fbbu,0x56dfccu,0,1);
}
} // namespace halo2
