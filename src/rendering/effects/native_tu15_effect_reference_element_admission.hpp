#pragma once
#include "rendering/effects/native_tu15_effect_reference_element_frame.hpp"
#include "gameplay/weapons/native_tu15_model_marker_lookup_admission.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include <cstdlib>

namespace halo2::native_effect_reference_element_detail {
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){
    static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_EFFECT_REFERENCE_ELEMENT");return p&&*p;}();return enabled;
}
template<class State>bool admitted(const State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||
       !native_model_marker_lookup_detail::protocol(s.services,s.observe)||!native_raw_x87_detail::supported(s))return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    return s.services.instruction?s.literal_service_sites&&s.instruction_cursor==&s.services.instruction_cursor:
        !s.instruction_cursor&&!s.literal_service_sites;
#else
    (void)s;return false;
#endif
}
// Source defaults remain unbound until the exact complete source contexts,
// live child routing, reference controls and untouched refusals are qualified.
template<class State,class Dispatch,class Before=Noop>
bool bridge(State&s,std::uint32_t entry,Dispatch dispatch,Before before={}){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if((entry!=0x178070u&&entry!=0x13a6b0u&&entry!=0x22b790u&&entry!=0x22aca0u&&entry!=0x22b710u)||!admitted(s))return false;
    if((entry==0x178070u||entry==0x13a6b0u)&&
       !native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;
    run_private(s,entry,before,dispatch);return true;
#else
    (void)s;(void)entry;(void)dispatch;(void)before;return false;
#endif
}
} // namespace halo2::native_effect_reference_element_detail
