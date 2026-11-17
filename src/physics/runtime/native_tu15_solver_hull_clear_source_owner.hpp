#pragma once
#include "physics/runtime/native_tu15_solver_hull_clear_frame.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace halo2::native_solver_hull_clear_owner_detail {
inline bool reference(){
    static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_SOLVER_HULL_CLEAR");return p&&*p;}();
    return enabled;
}
struct Activity{std::uint64_t calls{},completed{},continuations{};};
inline Activity&activity(){thread_local Activity value;return value;}
struct Continuation{std::uint32_t next;const void*owner;};

// Source uses only integer operations, including the source State's own
// PUSH/POP and observer publication. Keep the initial admission domain
// explicit rather than extending the separate physical FP qualification.
template<bool Literal,class State>bool admitted(const State&s){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    (void)s;return false;
#else
    if constexpr(!Literal)if(s.services.instruction)return false;
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||
       !native_raw_x87_detail::supported(s))return false;
    if(!s.services.instruction)return !s.instruction_cursor&&!s.observe;
    const auto*p=s.live_instruction_cursor();
    return s.literal_service_sites&&p&&p->literal_service_sites&&p->service_sites.size()==3u&&
        p->service_sites[0]==0x38846eu&&p->service_sites[1]==0x21f590u&&p->service_sites[2]==0x33b595u&&
        (!p->first_trace||!*p->first_trace)&&(!p->second_trace||!*p->second_trace)&&
        (!s.observe||p->observation_only_stores);
#endif
}
inline std::uint32_t successor(std::uint32_t pc){
    switch(pc){
    case 0x314870u:return 0x314876u;
    case 0x314876u:return 0x314877u;
    case 0x31487du:return 0x31487eu;
    case 0x314890u:return 0x314896u;
    case 0x314896u:return 0x31489au;
    case 0x31489au:return 0x3148a0u;
    case 0x3148a8u:return 0x3148aeu;
    case 0x3148c0u:return 0x3148c6u;
    case 0x3148c6u:return 0x3148cau;
    case 0x3148cau:return 0x3148d0u;
    case 0x3148d8u:return 0x3148d9u;
    case 0x3148d9u:return 0x3148dau;
    default:return 0u;
    }
}
template<bool Literal,class State>std::uint32_t bridge(State&s){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    (void)s;return 0u;
#else
    if(!admitted<Literal>(s))return 0u;
    ++activity().calls;const std::byte token{};
    const auto retired=[&](auto&state,std::uint32_t pc){
        if(admitted<Literal>(state))return;
        const auto next=successor(pc);
        if(!next)throw std::logic_error("Unsupported native hull-clear retired instruction");
        throw Continuation{next,&token};
    };
    try{
        native_solver_hull_clear_detail::run_private_source(s,retired);
        ++activity().completed;return 1u;
    }catch(const Continuation&resume){
        if(resume.owner!=&token)throw;
        ++activity().continuations;return resume.next;
    }
#endif
}
} // namespace halo2::native_solver_hull_clear_owner_detail
