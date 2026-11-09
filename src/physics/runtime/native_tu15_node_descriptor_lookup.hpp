#pragma once
#include "halo2/native_tu15_node_descriptor_lookup_kernel.hpp"
#include "physics/runtime/native_tu15_record_lookup.hpp"
#include "physics/runtime/native_tu15_node_resource_lifecycle.hpp"

namespace halo2::native_node_descriptor_lookup_detail {
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference_mode(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_NODE_DESCRIPTOR_LOOKUP");return p&&*p;}();return enabled;}
template<class State,class Call,class Before>struct Adapter {
    State&s;Call&call;Before&before;std::uint32_t entry_sp;
    void at(std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;before(s,pc);}
    std::uint32_t descriptor()const{return s.gp[2];}
    std::uint32_t handle()const{return native_node_resource_value(s.memory,entry_sp+4u);}
    void push(std::uint32_t pc,std::uint32_t value){
        at(pc);const auto address=s.gp[4]-4u;SourcePhysicsContactRecordStore event{pc,address,{},std::vector<std::byte>(4)};
        if(s.observe){event.before.resize(4);s.memory.read(address,event.before);}
        for(unsigned n=0;n<4;++n)event.written[n]=std::byte(value>>(8u*n));
        s.memory.write(address,event.written);s.gp[4]=address;if(s.observe)s.observe(event);
    }
    std::uint32_t pop(std::uint32_t pc){at(pc);const auto value=native_node_resource_value(s.memory,s.gp[4]);s.gp[4]+=4u;return value;}
    std::uint32_t touch(const NativeTu15NodeDescriptorLookup&){
        push(0x1e31c9u,s.gp[3]);push(0x1e31cau,0x1e31cfu);at(0x1e5c80u);
        call(s,0x1e5c80u,0x1e31cau,false);return s.gp[6];
    }
    void progress(NativeTu15NodeDescriptorLookupPhase phase,const NativeTu15NodeDescriptorLookup&d){
        using P=NativeTu15NodeDescriptorLookupPhase;auto&g=s.gp;
        switch(phase){
        case P::handle:at(0x1e3150u);break;
        case P::decoded:g[1]=d.index;g[0]=s.flags.logic(0,4);at(0x1e3159u);s.flags.minus(d.index,0xffffu,2);break;
        case P::save_descriptor:push(0x1e315fu,g[3]);break;
        case P::descriptor_handle:at(0x1e3160u);break;
        case P::save_result:g[3]=d.handle;s.flags.minus(d.descriptor_index,0xffffu,2);push(0x1e3168u,g[6]);break;
        case P::direct_node:g[0]=d.signed_index;g[6]=s.flags.logic(0,4);at(0x1e3170u);s.flags.minus(g[0],0xffffffffu,4);break;
        case P::foreign_lookup:
            switch(d.lookup_phase){
            case NativeTu15RecordLookupPhase::count:at(0x1e3181u);break;
            case NativeTu15RecordLookupPhase::count_ready:
                g[6]=d.lookup.count;g[0]=d.lookup.index;push(0x1e3187u,g[7]);
                g[7]=s.flags.logic(0,4);s.flags.minus(d.lookup.index,d.lookup.count,4);break;
            case NativeTu15RecordLookupPhase::slots:at(0x1e318eu);break;
            case NativeTu15RecordLookupPhase::datum:
                g[6]=d.lookup.slots;g[0]=s.flags.plus(d.lookup.index*32u,d.lookup.slots,4);at(0x1e3196u);break;
            case NativeTu15RecordLookupPhase::objects:g[0]=d.lookup.datum;s.flags.minus(g[0],0xffffffffu,4);at(0x1e319eu);break;
            case NativeTu15RecordLookupPhase::object:
                g[6]=d.lookup.objects;g[0]=d.lookup.escaped_index;s.flags.load(native_record_lookup_detail::found_flags(d.lookup.datum));at(0x1e31acu);break;
            }
            break;
        case P::foreign_completed:
            if(d.lookup.exit==NativeTu15RecordLookupExit::absent){g[0]=d.lookup.datum;s.flags.minus(g[0],0xffffffffu,4);}
            g[7]=d.foreign_descriptor;break;
        case P::indirect_node:g[0]=d.signed_index;g[6]=s.flags.logic(0,4);at(0x1e31b5u);s.flags.minus(g[0],0xffffffffu,4);break;
        case P::node_table:at(d.foreign?0x1e31bau:0x1e3175u);break;
        case P::node_ready:g[1]=d.nodes;g[0]=s.flags.plus(d.signed_index*0x6cu,d.nodes,4);g[6]=d.value;break;
        case P::restore_foreign:g[7]=pop(0x1e31c4u);break;
        case P::consider_touch:at(0x1e31c5u);s.flags.logic(g[6],4);break;
        case P::touch:break;
        case P::finished:
            if(d.valid){at(0x1e31cfu);g[0]=g[6];g[6]=pop(0x1e31d1u);g[3]=pop(0x1e31d2u);}
            pop(0x1e31d3u);g[4]+=4u;break;
        }
    }
};
template<class State,class Call,class Before=Noop>bool bridge(State&s,Call call,Before before={}){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(reference_mode()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if(s.observe&&!s.services.instruction)return false;
    if(s.services.instruction){
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*c=s.instruction_cursor;
        if((c->first_trace&&*c->first_trace)||(c->second_trace&&*c->second_trace)||(s.observe&&!c->observation_only_stores))return false;
        for(const auto pc:c->service_sites)if(pc>=0x1e3150u&&pc<=0x1e31d3u)return false;
    }
    Adapter<State,Call,Before>adapter{s,call,before,s.gp[4]};
    native_tu15_node_descriptor_lookup(s.memory,adapter,[&](auto phase,const auto&d){adapter.progress(phase,d);});return true;
#endif
}
} // namespace halo2::native_node_descriptor_lookup_detail
