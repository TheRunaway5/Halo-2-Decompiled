#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/physics_contact_runtime.hpp"
#include "physics/runtime/native_tu15_instanced_ray_callback_frame.hpp"
#endif
#include <cstdlib>
#include <thread>

namespace halo2::native_instanced_ray_callback_binding_detail {
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){
    static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_INSTANCED_RAY_CALLBACKS");return p&&*p;}();
    return value;
}
template<class State>bool admitted(const State&s){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    (void)s;return false;
#else
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||
       s.fp.count>6u||(s.other_flags&0x400u)||!native_raw_x87_detail::supported(s))return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(!s.services.instruction)return !s.instruction_cursor&&!s.literal_service_sites&&!s.observe;
    const auto*c=s.instruction_cursor;
    if(!s.literal_service_sites||!c||c!=&s.services.instruction_cursor||!c->position||!c->callback_type||
       s.services.instruction.target_type()!=*c->callback_type||c->executor!=std::this_thread::get_id()||
       !c->literal_service_sites||c->service_sites.size()!=3||c->service_sites[0]!=0x38846eu||
       c->service_sites[1]!=0x21f590u||c->service_sites[2]!=0x33b595u||
       (c->first_trace&&*c->first_trace)||(c->second_trace&&*c->second_trace)||
       (s.observe&&!c->observation_only_stores))return false;
    return true;
#endif
}
template<class State,class Dispatch,class Before=Noop>bool forward(State&s,Dispatch dispatch,Before before={}){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    (void)s;(void)dispatch;(void)before;return false;
#else
    if(!admitted(s))return false;
    native_instanced_ray_callback_detail::forward_retained(s,before,dispatch);return true;
#endif
}
template<class State,class Dispatch,class Before=Noop>bool collect(State&s,Dispatch dispatch,Before before={}){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    (void)s;(void)dispatch;(void)before;return false;
#else
    if(!admitted(s))return false;
    native_instanced_ray_callback_detail::collect_retained(s,before,dispatch);return true;
#endif
}
} // namespace halo2::native_instanced_ray_callback_binding_detail
