#pragma once
#include "halo2/native_tu15_node_resource_kernel.hpp"
#include <cstdlib>

namespace halo2::native_node_resource_detail {
inline bool reference_mode(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_NODE_RESOURCE");return p&&*p;}();return enabled;}
template<class State,class Tail>bool bridge(State&s,Tail tail) {
    if(reference_mode()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(s.services.instruction) {
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*cursor=s.instruction_cursor;
        if((cursor->first_trace&&*cursor->first_trace)||(cursor->second_trace&&*cursor->second_trace))return false;
        if(s.observe&&!cursor->observation_only_stores)return false;
        for(const auto site:cursor->service_sites)if(site>=0x1e5af0u&&site<=0x1e5b24u)return false;
    }
    const auto position=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    const auto progress=[&](NativeTu15NodeResourcePhase phase,const NativeTu15NodeResource&data) {
        using P=NativeTu15NodeResourcePhase;
        switch(phase) {
        case P::node_table:
            s.gp[0]=data.index;s.gp[1]=0;s.flags.minus(data.index,0xffffffffu,4);position(0x1e5af7u);break;
        case P::node_flags:
            s.gp[0]=data.value;s.gp[1]=data.escaped;
            if(data.index==0xffffffffu)s.flags.minus(data.index,0xffffffffu,4);
            else s.flags.plus(data.index*0x6cu,data.nodes,4);
            position(0x1e5b01u);break;
        case P::resource_index:s.flags.logic(data.tested,1);position(0x1e5b07u);break;
        case P::resource_table:s.gp[0]=data.resource_index;position(0x1e5b0au);break;
        case P::resource_enabled:s.gp[0]=data.value;s.gp[1]=data.resources;position(0x1e5b16u);break;
        }
    };
    const auto result=native_tu15_node_resource(s.memory,s.gp[2],s.gp[0],progress);
    s.gp[0]=result.value;s.gp[1]=result.escaped;s.flags.logic(result.tested,1);
    if(result.tail){position(0x1e5b1fu);tail(s);}
    else {position(0x1e5b24u);s.pop();}
    return true;
}
} // namespace halo2::native_node_resource_detail
