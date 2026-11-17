#pragma once
#include "physics/runtime/native_tu15_arena_world_publication.hpp"
#include <cstdlib>
#include <utility>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_arena_initialize_source_frame.hpp"
#endif

namespace halo2::native_arena_world_initialize_owner_detail {
inline bool reference(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_ARENA_INITIALIZE");return p&&*p;}();return value;}
inline std::uint64_t&activity(){thread_local std::uint64_t value{};return value;}
template<bool Prepared,class State>bool admitted(const State&s){
    // Raw callbacks remain the complete retained route. Prepared bodies still
    // retain the direct trailing REP callback; the World construction witness
    // excludes instruction census and tracing before eliding that delivery.
    if constexpr(!Prepared)return false;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||s.observe||
       s.active_mmx||s.fp.count>8u||s.fp.top>7u||
       !native_arena_world_publication_detail::Borrow::qualified(s.memory,s.services))return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    std::uint16_t cw;asm volatile("fnstcw %0":"=m"(cw));
    if((cw&0x3fu)!=0x3fu||(_mm_getcsr()&0x1f80u)!=0x1f80u)return false;
    const auto&cursor=s.services.instruction_cursor;
    return s.services.instruction&&s.instruction_cursor==&cursor&&s.literal_service_sites&&
        cursor.position&&cursor.callback_type&&s.services.instruction.target_type()==*cursor.callback_type&&
        cursor.executor==std::this_thread::get_id()&&cursor.literal_service_sites&&
        cursor.service_sites.size()==3&&cursor.service_sites[0]==0x38846eu&&
        cursor.service_sites[1]==0x21f590u&&cursor.service_sites[2]==0x33b595u&&
        !(cursor.first_trace&&*cursor.first_trace)&&!(cursor.second_trace&&*cursor.second_trace);
#else
    (void)s;return false;
#endif
}
template<bool Prepared,class State,class Call>std::uint32_t bridge(State&s,Call&&child){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(!admitted<Prepared>(s))return 0u;
    ++activity();
    const auto next=native_arena_initialize_source_detail::run_source(s,
        [](const auto&current){return admitted<Prepared>(current);},std::forward<Call>(child));
    return next?next:0xffffffffu;
#else
    (void)s;(void)child;return 0u;
#endif
}
} // namespace halo2::native_arena_world_initialize_owner_detail
