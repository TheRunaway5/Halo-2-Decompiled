#pragma once
#include "halo2/physics_contact_runtime.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_instanced_ray_child_frame.hpp"
#endif
#include <cstdint>
#include <cstdlib>
#include <thread>

namespace halo2::native_instanced_ray_child_binding_detail {
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference_mode(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_INSTANCED_RAY_CHILD");return p&&*p;}();return value;}
template<class State>bool admitted(State&s){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    (void)s;return false;
#else
    if(reference_mode()||s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s))return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(!s.services.instruction&&(s.instruction_cursor||s.observe))return false;
    if(s.services.instruction){
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*c=s.instruction_cursor;
        if(c!=&s.services.instruction_cursor||!c->callback_type||s.services.instruction.target_type()!=*c->callback_type||
           c->executor!=std::this_thread::get_id()||!c->literal_service_sites||c->service_sites.size()!=3||
           c->service_sites[0]!=0x38846eu||c->service_sites[1]!=0x21f590u||c->service_sites[2]!=0x33b595u)return false;
        if((c->first_trace&&*c->first_trace)||(c->second_trace&&*c->second_trace))return false;
    }
    if(s.observe&&(!s.instruction_cursor||!s.instruction_cursor->observation_only_stores))return false;
    return true;
#endif
}
template<class State,class Before=Noop>bool bridge(State&s,Before before={}){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    (void)s;(void)before;return false;
#else
    if(!admitted(s))return false;
    native_instanced_ray_child_detail::resolve_retained(s,before);return true;
#endif
}
} // namespace halo2::native_instanced_ray_child_binding_detail
