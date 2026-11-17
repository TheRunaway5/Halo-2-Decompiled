#pragma once
#include "gameplay/weapons/native_tu15_marker_owner.hpp"
#include "gameplay/weapons/native_tu15_object_parent_mode_admission.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/native_tu15_required_abi_fp.hpp"
#include <cstdlib>

namespace halo2::native_marker_owner_detail{
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
enum class Observation{actual,canonical};
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
#endif
inline bool reference(){
    static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_MARKER_OWNER");return p&&*p;}();return enabled;
}
template<class State>bool admitted(const State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(reference()||native_object_parent_mode_detail::reference()||
       !native_required_owner_policy_detail::preserves_instruction_policy(s.services)||
       s.image!=SourcePhysicsContactImage::TitleUpdate15||
       !native_model_marker_lookup_detail::protocol(s.services,s.observe)||
       !native_raw_x87_detail::supported(s))return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    return s.services.instruction?s.literal_service_sites&&s.instruction_cursor==&s.services.instruction_cursor:
        !s.instruction_cursor&&!s.literal_service_sites;
#else
    (void)s;return false;
#endif
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class State>void actual_child(State&state,std::uint32_t target,std::uint32_t site){
    if(target!=0xfc510u||(site!=0xbad39u&&site!=0xb9bf9u))throw std::runtime_error("Unexpected native marker-owner child");
    auto abi=state.save();
    // The real Effect parent uses this required-owner boundary even when a
    // local generated FC510 body exists. Preserve a replaced provider and its
    // complete actual entry ABI instead of bypassing it with a direct facade.
    if(!state.services.required_owner)throw std::runtime_error("Native marker owner requires its actual callee owner");
    native_required_abi_fp_detail::invoke(state,abi,[&]{
        state.services.required_owner(state.memory,{site,target,false},abi,state.observe);
    });
}
#endif
template<class State,class Before=Noop>bool bridge(State&s,Observation observation=Observation::actual,Before before={}){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(!admitted(s))return false;run_private(s,[](auto&state,auto target,auto site){actual_child(state,target,site);},std::move(before),observation);return true;
#else
    (void)s;(void)observation;(void)before;return false;
#endif
}
inline bool effect(SourcePhysicsContactMemory&memory,SourcePhysicsWorldRuntimeAbi&abi,
    const SourcePhysicsWorldRuntimeServices&services,const SourcePhysicsContactRecordObserver&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(reference()||native_object_parent_mode_detail::reference()||
       !native_required_owner_policy_detail::preserves_instruction_policy(services)||
       !native_model_marker_lookup_detail::protocol(services,observe)||!abi.floating_point||
       abi.active_mmx||abi.x87_depth>8u||(abi.x87_status&0x8080u)||
       (abi.floating_point->x87_control_word&0x3fu)!=0x3fu||
       (abi.floating_point->mxcsr&0x1f80u)!=0x1f80u||(abi.floating_point->mxcsr&0xffff0000u))return false;
    using State=physics_world_runtime_detail::WorldState;State*live{};
    SourcePhysicsContactRecordObserver transport;
    if(observe)transport=[&](const auto&event){abi=live->save();observe(event);};
    State state{memory,transport,services,SourcePhysicsContactImage::TitleUpdate15};live=&state;state.load(abi);
    // The Effect owner hands the real caller's referenced ABI and observer
    // to its child. Keep that identity while publishing the separate State.
    const auto child=[&](auto&current,auto target,auto site){
        abi=current.save();
        if(!services.required_owner)throw std::runtime_error("Native marker owner requires its actual callee owner");
        native_required_abi_fp_detail::invoke(current,abi,[&]{
            services.required_owner(memory,{site,target,false},abi,observe);
        });
    };
    try{run_private(state,child,[&](auto&current,auto){abi=current.save();});}
    catch(...){abi=state.save();throw;}
    abi=state.save();return true;
#else
    (void)memory;(void)abi;(void)services;(void)observe;return false;
#endif
}
} // namespace halo2::native_marker_owner_detail
