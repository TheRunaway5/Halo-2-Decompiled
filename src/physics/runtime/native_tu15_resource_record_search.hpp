#pragma once
#include "halo2/physics_contact_runtime.hpp"
#include "halo2/native_tu15_resource_record_search_kernel.hpp"
#include <cstdlib>

namespace halo2::native_resource_record_search_detail {
inline bool reference_mode(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_RESOURCE_RECORD_SEARCH");return p&&*p;}();return enabled;}
template<class State>bool bridge(State&s) {
    if(reference_mode()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(s.services.instruction) {
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*cursor=s.instruction_cursor;
        if((cursor->first_trace&&*cursor->first_trace)||(cursor->second_trace&&*cursor->second_trace))return false;
        if(s.observe&&!cursor->observation_only_stores)return false;
        for(const auto site:cursor->service_sites)if(site>=0x126190u&&site<=0x1261f7u)return false;
    }
    const auto entry=s.gp[3],handle=native_resource_search_value(s.memory,entry+0xcu);
    s.gp[0]=s.gp[2]=0xffffffffu;s.gp[1]=handle;s.flags.minus(handle,0xffffffffu,4);
    std::uint32_t return_site=0x1261f7u;
    if(handle==0xffffffffu) {
        s.push(0x12619cu,s.gp[7]);
        const auto pool=native_resource_search_value(s.memory,0x4f041cu);
        s.push(0x1261a3u,s.gp[5]);s.push(0x1261a4u,s.gp[6]);
        const auto result=native_tu15_find_resource_record(s.memory,pool,entry);
        s.gp[0]=result.datum;s.gp[1]=result.escaped;s.gp[2]=result.index;
        if(result.exit==NativeTu15ResourceSearchExit::found)s.flags.minus(entry,result.last_owner,4);
        else if(result.exit==NativeTu15ResourceSearchExit::negative_index) {
            s.flags.minus(entry,result.last_owner,4);s.flags.increment(result.index-1u,4,false);
        }else s.flags.minus(result.index,result.limit,4);
        s.gp[6]=s.pop();s.gp[5]=s.pop();s.gp[7]=s.pop();
        if(result.exit==NativeTu15ResourceSearchExit::inner_bound)return_site=0x1261d1u;
    }
    if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=return_site;
    s.pop();return true;
}
} // namespace halo2::native_resource_record_search_detail
