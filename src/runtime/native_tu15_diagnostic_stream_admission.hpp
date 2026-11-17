#pragma once
#include "runtime/native_tu15_diagnostic_stream.hpp"
#include <cstdlib>
#include <thread>

namespace halo2::native_diagnostic_stream_detail {
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
#endif
inline bool reference(){
    static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_DIAGNOSTIC_STREAM");return p&&*p;}();return enabled;
}
template<class State>bool admitted(const State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    const auto&c=s.services.instruction_cursor;
    if(s.services.instruction){
        if(s.instruction_cursor!=&c||!s.literal_service_sites||!c.position||!c.callback_type||
           s.services.instruction.target_type()!=*c.callback_type||c.executor!=std::this_thread::get_id()||
           !c.literal_service_sites||c.service_sites.size()!=3||c.service_sites[0]!=0x38846eu||
           c.service_sites[1]!=0x21f590u||c.service_sites[2]!=0x33b595u||
           (c.first_trace&&*c.first_trace)||(c.second_trace&&*c.second_trace)||
           (s.observe&&!c.observation_only_stores))return false;
    }else if(s.instruction_cursor||s.literal_service_sites||c.position||c.literal_service_sites||s.observe)return false;
    return native_raw_x87_detail::supported(s);
#else
    (void)s;return false;
#endif
}
// Exact MapWorld source prefixes preserve the retained route on refusal.
template<class State,class Call,class Before=Noop>
bool bridge(State&s,std::uint32_t entry,Call call,Before before={}){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if((entry!=0x2e3080u&&entry!=0x2e30e0u&&entry!=0x2e2e00u&&entry!=0x2e2d10u)||!admitted(s))return false;
    if(entry==0x2e3080u)print(s,call,before);
    else if(entry==0x2e30e0u)flush(s,call,before);
    else if(entry==0x2e2d10u)format_tail(s,call,before);
    else measure(s,before);
    return true;
#else
    (void)s;(void)entry;(void)call;(void)before;return false;
#endif
}
} // namespace halo2::native_diagnostic_stream_detail
