#pragma once
// Defined only in a fully authenticated source owner. The World scalar handoff
// may then defer to the genuine whole-owner entry without executing early.
#define HALO2_NATIVE_CLOSEST_SEGMENTS_SOURCE_OWNER 1
#include "physics/runtime/native_tu15_closest_segments_frame.hpp"
#include <cstdint>
#include <cstdlib>
#include <thread>
namespace halo2::native_closest_segments_owner_detail {
inline bool reference(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_CLOSEST_SEGMENTS");return p&&*p;}();return enabled;}
struct Activity{std::uint64_t actual_calls{},canonical_calls{},world_owner_deferrals{},world_reference_deferrals{},legacy_handoff_fallbacks{};};
inline Activity&activity(){thread_local Activity value;return value;}
template<class State>bool admitted(const State&s){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>6u)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
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
template<bool Actual,class State>bool bridge_mode(State&s){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(!admitted(s))return false;
    if constexpr(Actual)++activity().actual_calls;else ++activity().canonical_calls;
    native_closest_segments_detail::run_private_owner<Actual>(s);return true;
#endif
}
template<class State>bool bridge(State&s){if constexpr(requires{s.selected_observation_pcs;})return bridge_mode<true>(s);else return bridge_mode<false>(s);}
template<class State>bool bridge_actual(State&s){return bridge_mode<true>(s);}
template<class State>bool defer_world_owner(const State&s,std::uint32_t target,std::uint32_t site){
    if(target!=0x312620u||site!=0x30eaccu)return false;
    if(reference()){++activity().world_reference_deferrals;return true;}
    if(admitted(s)){++activity().world_owner_deferrals;return true;}
    ++activity().legacy_handoff_fallbacks;return false;
}
} // namespace halo2::native_closest_segments_owner_detail
