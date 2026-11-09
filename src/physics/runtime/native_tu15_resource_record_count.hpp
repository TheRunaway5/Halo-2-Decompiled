#pragma once
#include "halo2/physics_contact_runtime.hpp"
#include "halo2/native_tu15_resource_record_count_kernel.hpp"
#include <cstdlib>

namespace halo2::native_resource_record_count_detail {
inline bool reference_mode(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_RESOURCE_RECORD_COUNT");return p&&*p;}();return enabled;}
template<class State>bool bridge(State&s) {
    if(reference_mode()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(s.services.instruction) {
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*cursor=s.instruction_cursor;
        if((cursor->first_trace&&*cursor->first_trace)||(cursor->second_trace&&*cursor->second_trace))return false;
        if(s.observe&&!cursor->observation_only_stores)return false;
        for(const auto site:cursor->service_sites)if(site>=0x126480u&&site<=0x1264d9u)return false;
    }
    s.push(0x126480u,s.gp[3]);const auto pool=native_resource_count_value(s.memory,0x4f0418u);
    s.push(0x126487u,s.gp[5]);s.push(0x126488u,s.gp[6]);s.push(0x12648bu,s.gp[7]);
    const auto result=native_tu15_count_inactive_resource_records(s.memory,pool,s.gp[1]);
    s.gp[0]=result.count;s.gp[1]=result.escaped;s.gp[2]=result.index;
    if(result.exit==NativeTu15ResourceCountExit::null_record)s.flags.plus(result.offset,result.records,4);
    else if(result.exit==NativeTu15ResourceCountExit::negative_index){s.flags.cf=false;s.flags.increment(result.index-1u,4,false);}
    else s.flags.minus(result.index,result.limit,4);
    s.gp[7]=s.pop();s.gp[6]=s.pop();s.gp[5]=s.pop();s.gp[3]=s.pop();
    if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=0x1264bfu;
    s.pop();return true;
}
} // namespace halo2::native_resource_record_count_detail
