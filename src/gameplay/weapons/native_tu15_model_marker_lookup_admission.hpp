#pragma once
#include "gameplay/weapons/native_tu15_model_marker_lookup.hpp"
#include "halo2/native_weapon_effect_birth_color_observer.hpp"
#include <cstdlib>
#include <thread>

namespace halo2::native_model_marker_lookup_detail {
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
enum class Observation {actual,canonical};
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
#endif
inline bool reference(){
    static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_MODEL_MARKER_LOOKUP");return p&&*p;}();return enabled;
}
inline bool protocol(const SourcePhysicsWorldRuntimeServices&services,
    const SourcePhysicsContactRecordObserver&observe){
    const auto&c=services.instruction_cursor;
    if(!services.instruction)return !c.position&&!c.literal_service_sites&&
        (!observe||NativeWeaponEffectBirthColorCollector::qualified(observe));
    return c.position&&c.callback_type&&services.instruction.target_type()==*c.callback_type&&
        c.executor==std::this_thread::get_id()&&c.literal_service_sites&&
        c.service_sites.size()==3&&c.service_sites[0]==0x38846eu&&
        c.service_sites[1]==0x21f590u&&c.service_sites[2]==0x33b595u&&
        !(c.first_trace&&*c.first_trace)&&!(c.second_trace&&*c.second_trace)&&
        (!observe||c.observation_only_stores||NativeWeaponEffectBirthColorCollector::qualified(observe));
}
template<class State>bool admitted(const State&s,Observation observation){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(reference()||(observation==Observation::actual&&s.image!=SourcePhysicsContactImage::TitleUpdate15)||
       !protocol(s.services,s.observe)||
       (!native_raw_x87_detail::supported(s)&&!native_sse_mmx_detail::supported(s)))return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(s.services.instruction)return s.literal_service_sites&&s.instruction_cursor==&s.services.instruction_cursor;
    return !s.instruction_cursor&&!s.literal_service_sites;
#else
    (void)s;(void)observation;return false;
#endif
}
template<class State,class Before=Noop>bool bridge(State&s,Observation observation,Before before={}){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(!admitted(s,observation))return false;run_private(s,std::move(before),observation);return true;
#else
    (void)s;(void)observation;(void)before;return false;
#endif
}
// The Effect API owns the same complete ABI, rather than a WorldState alias.
// Publish its real referenced packet before providers and completed observers.
inline bool effect(SourcePhysicsContactMemory&memory,SourcePhysicsWorldRuntimeAbi&abi,
    const SourcePhysicsWorldRuntimeServices&services,const SourcePhysicsContactRecordObserver&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(reference()||!protocol(services,observe)||!abi.floating_point||abi.x87_depth>8||
       (abi.x87_status&0x8080u)||(abi.floating_point->x87_control_word&0x3fu)!=0x3fu||
       (abi.floating_point->mxcsr&0x1f80u)!=0x1f80u||(abi.floating_point->mxcsr&0xffff0000u)||
       (abi.active_mmx&&(abi.x87_depth||(abi.x87_status&0x3800u))))return false;
    using State=physics_world_runtime_detail::WorldState;State*live{};
    SourcePhysicsContactRecordObserver transport;
    if(observe)transport=[&](const auto&event){abi=live->save();observe(event);};
    State state{memory,transport,services,SourcePhysicsContactImage::TitleUpdate15};live=&state;state.load(abi);
    try{run_private(state,[&](auto&current,auto){abi=current.save();},Observation::actual);}
    catch(...){abi=state.save();throw;}
    abi=state.save();return true;
#else
    (void)memory;(void)abi;(void)services;(void)observe;return false;
#endif
}
} // namespace halo2::native_model_marker_lookup_detail
