#pragma once
#include "halo2/physics_contact_runtime.hpp"
#include <cstdlib>
#include <initializer_list>
#include <thread>
#include <utility>

namespace halo2::native_object_node_admission_detail {
template<class State>
bool admitted(const State&s,std::initializer_list<std::pair<std::uint32_t,std::uint32_t>> ranges){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if(!s.services.instruction)return !s.instruction_cursor&&!s.literal_service_sites&&!s.observe;
    if(s.services.instruction){
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*c=s.instruction_cursor;
        if(c!=&s.services.instruction_cursor||!c->callback_type||s.services.instruction.target_type()!=*c->callback_type||
           c->executor!=std::this_thread::get_id()||!c->literal_service_sites||c->service_sites.size()!=3||
           c->service_sites[0]!=0x38846eu||c->service_sites[1]!=0x21f590u||c->service_sites[2]!=0x33b595u)return false;
        if((c->first_trace&&*c->first_trace)||(c->second_trace&&*c->second_trace)||(s.observe&&!c->observation_only_stores))return false;
        for(const auto site:c->service_sites)for(const auto&[begin,end]:ranges)if(site>=begin&&site<=end)return false;
    }
    return true;
#endif
}
} // namespace halo2::native_object_node_admission_detail
