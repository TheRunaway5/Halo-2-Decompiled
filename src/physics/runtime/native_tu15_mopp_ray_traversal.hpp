#pragma once
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include <cstdlib>
#include <thread>
#include <utility>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_ray_controller.hpp"
#include "physics/runtime/native_tu15_mopp_ray_slab_frame.hpp"
#include "physics/runtime/native_tu15_mopp_ray_diagnostic_frame.hpp"
#endif

namespace halo2::native_mopp_ray_traversal_detail {
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){
    static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_MOPP_RAY_TRAVERSAL");return p&&*p;}();return enabled;
}
// Private candidate: source defaults and complete source-route/refusal proof
// are not installed. Actual dynamic child owners remain genuine handoffs.
template<class State>bool initial_state_admitted(const State&s){
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
    // The complete original CFG reaches three temporary x87 values. Every
    // recursive/external call is balanced at zero temporaries. Capacity for
    // all authored branches therefore requires at most five incoming values.
    return s.fp.count<=5u&&native_raw_x87_detail::supported(s);
#else
    (void)s;return false;
#endif
}
template<class State>bool admitted(const State&s){
    if(!initial_state_admitted(s))return false;
    // A real child can rebind policy while this continuation is parked. This
    // explicit owned callback/capture contract is independent of the cursor
    // and store-observer capability; arbitrary providers remain refused.
    return native_required_owner_policy_detail::preserves_instruction_policy(s.services);
}
template<class State,class Dispatch,class Before=Noop>
bool bridge(State&s,Dispatch dispatch,Before before={}){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(!admitted(s))return false;
    auto slab=[](auto&state,auto&observe,auto&scope,auto opcode,auto&program){return native_mopp_ray_caller_detail::slab(state,observe,scope,opcode,program);};
    auto diagnostic=[](auto&state,auto&observe,auto&scope,auto&child){native_mopp_ray_caller_detail::DiagnosticFrame view{state,observe,scope,child};view.invoke();};
    native_mopp_ray_caller_detail::traverse(s,before,dispatch,slab,diagnostic);return true;
#else
    (void)s;(void)dispatch;(void)before;return false;
#endif
}
}
