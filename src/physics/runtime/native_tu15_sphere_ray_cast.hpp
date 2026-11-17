#pragma once
#include "halo2/physics_contact_runtime.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_sphere_ray_cast_frame.hpp"
#endif
#include <cstdlib>
#include <thread>

namespace halo2::native_sphere_ray_cast_binding_detail {
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_SPHERE_RAY_CAST");return p&&*p;}();return enabled;}
inline std::uint64_t&calls(){thread_local std::uint64_t value{};return value;}
template<class State>bool admitted(const State&s){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    (void)s;return false;
#else
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>5u)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(!s.services.instruction&&(s.instruction_cursor||s.observe))return false;
    if(s.services.instruction){const auto*c=s.instruction_cursor;
        if(!s.literal_service_sites||!c||c!=&s.services.instruction_cursor||!c->position||!c->callback_type||
           s.services.instruction.target_type()!=*c->callback_type||c->executor!=std::this_thread::get_id()||
           !c->literal_service_sites||c->service_sites.size()!=3u||c->service_sites[0]!=0x38846eu||
           c->service_sites[1]!=0x21f590u||c->service_sites[2]!=0x33b595u)return false;
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
    ++calls();native_sphere_ray_cast_detail::run_private(s,before);return true;
#endif
}
} // namespace halo2::native_sphere_ray_cast_binding_detail
