#pragma once
#include "gameplay/weapons/native_tu15_object_parent_mode.hpp"
#include "gameplay/weapons/native_tu15_model_marker_lookup_admission.hpp"
#include <cstdlib>
namespace halo2::native_object_parent_mode_detail {
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
#endif
inline bool reference(){
    static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_OBJECT_PARENT_MODE");return p&&*p;}();return enabled;
}
template<class State>bool admitted(const State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||
       !native_model_marker_lookup_detail::protocol(s.services,s.observe)||
       (!native_raw_x87_detail::supported(s)&&!native_sse_mmx_detail::supported(s)))return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    return s.services.instruction?s.literal_service_sites&&s.instruction_cursor==&s.services.instruction_cursor:
        !s.instruction_cursor&&!s.literal_service_sites;
#else
    (void)s;return false;
#endif
}
template<class State,class Before=Noop>bool bridge(State&s,Before before={}){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(!admitted(s))return false;run_private(s,std::move(before));return true;
#else
    (void)s;(void)before;return false;
#endif
}
inline bool effect(SourcePhysicsContactMemory&memory,SourcePhysicsWorldRuntimeAbi&abi,
    const SourcePhysicsWorldRuntimeServices&services){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    // The actual FC510 helper has no store observer argument or writes.
    const SourcePhysicsContactRecordObserver observe;
    if(reference()||!native_model_marker_lookup_detail::protocol(services,observe)||
       !abi.floating_point||abi.x87_depth>8||(abi.x87_status&0x8080u)||
       (abi.floating_point->x87_control_word&0x3fu)!=0x3fu||
       (abi.floating_point->mxcsr&0x1f80u)!=0x1f80u||(abi.floating_point->mxcsr&0xffff0000u)||
       (abi.active_mmx&&(abi.x87_depth||(abi.x87_status&0x3800u))))return false;
    physics_world_runtime_detail::WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    try{run_private(state,[&](auto&current,auto){abi=current.save();});}
    catch(...){abi=state.save();throw;}
    abi=state.save();return true;
#else
    (void)memory;(void)abi;(void)services;return false;
#endif
}
} // namespace halo2::native_object_parent_mode_detail
