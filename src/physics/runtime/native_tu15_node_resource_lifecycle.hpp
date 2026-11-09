#pragma once
#include "halo2/native_tu15_node_resource_lifecycle_kernel.hpp"
#include <cstdlib>

namespace halo2::native_node_resource_lifecycle_detail {
inline bool reference_mode(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_NODE_RESOURCE_LIFECYCLE");return p&&*p;}();return enabled;}
template<class State,class Call>bool bridge(State&s,Call call){
    if(reference_mode()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(s.services.instruction){
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*cursor=s.instruction_cursor;
        if((cursor->first_trace&&*cursor->first_trace)||(cursor->second_trace&&*cursor->second_trace))return false;
        if(s.observe&&!cursor->observation_only_stores)return false;
        for(const auto site:cursor->service_sites)if(site>=0x1e5c80u&&site<=0x1e5d5du)return false;
    }
    using P=NativeTu15NodeResourceLifecyclePhase;using E=NativeTu15NodeResourceLifecycleExit;
    const auto position=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    const auto low=[&](unsigned n,std::uint32_t value,unsigned mask){s.gp[n]=(s.gp[n]&~mask)|(value&mask);};
    const auto root=[&](const auto&d){
        s.gp[0]=native_tu15_signed_node_index(d.index);s.gp[1]=0;s.flags.minus(s.gp[0],0xffffffffu,4);
    };
    const auto node=[&](const auto&d){
        const auto index=native_tu15_signed_node_index(d.next_index);s.gp[0]=index;s.gp[1]=0;
        if(index==0xffffffffu)s.flags.minus(index,0xffffffffu,4);
        else{s.gp[0]=d.next_node;s.gp[1]=d.next_node;s.flags.plus(index*0x6cu,d.nodes,4);}
    };
    const auto resource=[&](const auto&d){
        if(!d.node_flags){node(d);s.flags.logic(d.node_flags,1);}
        else{s.gp[0]=d.resource;s.gp[1]=(d.resources&0xffffff00u)|d.resource_enabled;s.flags.logic(d.resource_enabled,1);}
    };
    const auto progress=[&](P phase,const NativeTu15NodeResourceLifecycle&d){
        switch(phase){
        case P::enabled_first:position(0x1e5c80u);break;
        case P::enabled_second:low(0,d.enabled_first,255u);s.flags.logic(d.enabled_first,1);position(0x1e5c8du);break;
        case P::save_cursor:low(0,d.enabled_second,255u);s.flags.logic(d.enabled_second,1);position(0x1e5c9au);s.push(0x1e5c9au,s.gp[6]);break;
        case P::handle:position(0x1e5c9bu);break;
        case P::descriptor_handle:s.gp[6]=d.index;s.flags.minus(d.index,0xffffu,2);position(0x1e5cacu);break;
        case P::save_descriptor:
            s.gp[0]=d.descriptor_handle;s.flags.minus(d.descriptor_handle&0xffffu,0xffffu,2);
            position(0x1e5cb4u);s.push(0x1e5cb4u,s.gp[7]);s.gp[7]=d.descriptor;break;
        case P::descriptor_owner:s.gp[1]=native_tu15_signed_node_index(d.descriptor_handle);break;
        case P::node_table:root(d);position(0x1e5ccdu);break;
        case P::head_index:
            s.gp[0]=native_tu15_signed_node_index(d.index);s.gp[2]=d.nodes;s.gp[1]=d.node;s.gp[6]=d.node;
            s.flags.plus(s.gp[0]*0x6cu,d.nodes,4);position(0x1e5cd9u);break;
        case P::head_table:
            s.gp[1]=native_tu15_signed_node_index(d.head_index);s.gp[6]=0;s.flags.minus(s.gp[1],0xffffffffu,4);
            position(0x1e5cedu);break;
        case P::initial_owner:
            if(d.head_index==0xffffu){low(1,d.head_index,0xffffu);s.flags.minus(d.head_index,0xffffu,2);}
            else{s.gp[1]=d.cursor;s.gp[6]=d.cursor;s.flags.plus(native_tu15_signed_node_index(d.head_index)*0x6cu,d.nodes,4);}
            s.gp[2]=d.descriptor;break;
        case P::initial_next:position(0x1e5cfeu);break;
        case P::loop_next:s.flags.minus(d.last_next,0xffffu,2);position(0x1e5d05u);break;
        case P::resource_node_table:
            s.gp[0]=native_tu15_signed_node_index(d.next_index);s.gp[1]=0;s.flags.minus(s.gp[0],0xffffffffu,4);
            position(0x1e5d10u);break;
        case P::node_flags:node(d);position(0x1e5d1au);break;
        case P::resource_index:s.flags.logic(d.node_flags,1);position(0x1e5d20u);break;
        case P::resource_table:s.gp[0]=d.resource_index;position(0x1e5d23u);break;
        case P::resource_enabled:s.gp[0]=d.resource;s.gp[1]=d.resources;position(0x1e5d2fu);break;
        case P::resource_owner:resource(d);low(1,1u,255u);break;
        case P::advance_next:
            if(!d.node_flags||!d.resource_enabled)resource(d);
            position(0x1e5d3du);break;
        case P::advance_table:
            s.gp[0]=native_tu15_signed_node_index(d.next_index);s.gp[1]=0;s.flags.minus(s.gp[0],0xffffffffu,4);
            position(0x1e5d48u);break;
        case P::advance_test:node(d);position(0x1e5d52u);break;
        case P::advance_done:s.flags.minus(d.last_next,0xffffu,2);s.gp[6]=d.cursor;break;
        case P::finished:
            if(d.exit==E::first_disabled){low(0,d.enabled_first,255u);s.flags.logic(d.enabled_first,1);}
            else if(d.exit==E::second_disabled){low(0,d.enabled_second,255u);s.flags.logic(d.enabled_second,1);}
            else if(d.exit==E::invalid_handle){s.gp[6]=d.index;s.flags.minus(d.index,0xffffu,2);}
            else{s.flags.minus(d.last_next,0xffffu,2);position(0x1e5d5bu);s.gp[7]=s.pop();}
            if(d.exit==E::invalid_handle||d.exit==E::complete){position(0x1e5d5cu);s.gp[6]=s.pop();}
            position(0x1e5d5du);s.pop();s.gp[4]+=4u;break;
        }
    };
    struct Owners{
        State&s;Call&call;decltype(position)&at;
        void invoke(std::uint32_t target,std::uint32_t site,std::uint32_t continuation){
            at(site);s.push(site,continuation);call(s,target,site);
        }
        std::uint32_t descriptor(NativeTu15NodeResourceLifecycle&){
            invoke(0x1e3270u,0x1e5cbcu,0x1e5cc1u);s.gp[7]=s.gp[0];return s.gp[7];
        }
        void initial(NativeTu15NodeResourceLifecycle&d){
            invoke(0x1e5af0u,0x1e5cf9u,0x1e5cfeu);d.descriptor=s.gp[7];d.cursor=s.gp[6];
        }
        void resource(NativeTu15NodeResourceLifecycle&d){
            invoke(0x126200u,0x1e5d38u,0x1e5d3du);d.descriptor=s.gp[7];d.cursor=s.gp[6];
        }
    }owners{s,call,position};
    native_tu15_node_resource_lifecycle(s.memory,s.gp[4]+4u,s.gp[2],owners,progress);return true;
}
} // namespace halo2::native_node_resource_lifecycle_detail
