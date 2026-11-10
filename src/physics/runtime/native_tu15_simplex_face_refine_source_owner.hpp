#pragma once
#include "physics/runtime/native_tu15_simplex_face_refine_abi.hpp"
#include <cstdlib>
#include <thread>
namespace halo2::native_simplex_face_refine_owner_detail {
inline bool reference(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_SIMPLEX_FACE_REFINE");return p&&*p;}();return enabled;}
struct Activity{std::uint64_t actual_calls{};};
inline Activity&activity(){thread_local Activity value;return value;}
template<class State>bool admitted(const State&s){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>6u)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(!s.services.instruction&&(s.instruction_cursor||s.observe))return false;
    if(s.services.instruction){
        const auto*p=s.instruction_cursor;
        if(!s.literal_service_sites||!p||p!=&s.services.instruction_cursor||!p->position||!p->callback_type||
           s.services.instruction.target_type()!=*p->callback_type||p->executor!=std::this_thread::get_id()||
           !p->literal_service_sites||p->service_sites.size()!=3u||p->service_sites[0]!=0x38846eu||
           p->service_sites[1]!=0x21f590u||p->service_sites[2]!=0x33b595u)return false;
        if((p->first_trace&&*p->first_trace)||(p->second_trace&&*p->second_trace))return false;
    }
    if(s.observe&&(!s.instruction_cursor||!s.instruction_cursor->observation_only_stores))return false;
    return true;
#endif
}
template<class State>bool bridge(State&s){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(!admitted(s))return false;
    ++activity().actual_calls;native_simplex_face_refine_detail::run_private_owner(s);return true;
#endif
}
} // namespace halo2::native_simplex_face_refine_owner_detail
