#pragma once
#include "halo2/native_tu15_kernel_yield_kernel.hpp"
#include "physics/runtime/physics_step_internal.hpp"
#include <cstdlib>
namespace halo2::native_kernel_yield_detail {
inline bool reference_mode(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_KERNEL_YIELD");return p&&*p;}();return enabled;}
template<class State,class Call>bool bridge(State&s,Call call) {
    if(reference_mode()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(s.services.instruction){
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*c=s.instruction_cursor;
        if((c->first_trace&&*c->first_trace)||(c->second_trace&&*c->second_trace))return false;
        if(s.observe&&!c->observation_only_stores)return false;
        for(const auto site:c->service_sites)if(site>=0x2dbf74u&&site<=0x2dbf86u)return false;
    }
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    at(0x2dbf74u);
    const auto target=physics_step_detail::r(s.memory,0x41b69cu,4);
    s.push(0x2dbf74u,0x2dbf7au);call(s,target,0x2dbf74u,false);
    s.gp[1]=0;s.flags.logic(0,4);s.flags.minus(s.gp[0],0x40000024u,4);
    s.gp[1]=native_tu15_kernel_yield_result(s.gp[0]);s.gp[0]=s.gp[1];
    at(0x2dbf86u);s.pop();return true;
}
} // namespace halo2::native_kernel_yield_detail
