#pragma once
#include "physics/runtime/native_tu15_simplex_boundary_frame.hpp"
#include "physics/runtime/native_tu15_simplex_triangle_source_owner.hpp"
#include "physics/runtime/native_tu15_closest_segments_source_owner.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include <cstdlib>
#include <thread>
namespace halo2::native_simplex_boundary_owner_detail {
inline bool reference(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_SIMPLEX_BOUNDARY");return p&&*p;}();return enabled;}
struct Activity{std::uint64_t calls{},direct_triangle_calls{},direct_closest_calls{},dispatch_child_calls{};};
inline Activity&activity(){thread_local Activity value;return value;}
template<class State>bool admitted(const State&s){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>6u)return false;
    if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;
    if(!s.services.instruction&&(s.instruction_cursor||s.observe))return false;
    if(s.services.instruction){const auto*p=s.instruction_cursor;
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
template<class State,class Call>bool bridge(State&s,Call call){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(!admitted(s))return false;++activity().calls;
    const auto child=[&](auto&v,std::uint32_t target,std::uint32_t site){
        // A present required-owner provider retains the original dispatcher,
        // even if it observes or overrides a known external target. With no
        // provider, the already qualified actual kernels supply the known
        // static owners. Reference/admission refusal always keeps dispatch.
        if(!v.services.required_owner){
            if(target==0x30d3f0u){
                try{if(native_simplex_triangle_detail::bridge_actual(v)){++activity().direct_triangle_calls;return;}}
                catch(...){++activity().direct_triangle_calls;throw;}
            }else if(target==0x312620u){
                try{if(native_closest_segments_owner_detail::bridge_actual(v)){++activity().direct_closest_calls;return;}}
                catch(...){++activity().direct_closest_calls;throw;}
            }
        }
        ++activity().dispatch_child_calls;call(v,target,site);
    };
    native_simplex_boundary_detail::run_private_owner(s,child);return true;
#endif
}
} // namespace halo2::native_simplex_boundary_owner_detail
