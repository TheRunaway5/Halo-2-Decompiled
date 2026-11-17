#pragma once
#include "physics/runtime/native_tu15_solver_hull_clip_source_frame.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#endif
#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace halo2::native_solver_hull_clip_owner_detail {
inline bool reference(){
    static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_SOLVER_HULL_CLIP");return p&&*p;}();
    return enabled;
}
struct Activity{std::uint64_t calls{},completed{},continuations{};};
inline Activity&activity(){thread_local Activity value;return value;}
struct Continuation{std::uint32_t next;const void*owner;};

template<bool Literal,class State>bool admitted(const State&s,unsigned limit=7u){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    (void)s;(void)limit;return false;
#else
    if constexpr(!Literal)if(s.services.instruction)return false;
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||
       !native_raw_x87_detail::supported(s)||s.fp.count>limit)return false;
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
    case 0x3147d0u:return 0x3147d1u;
    case 0x3147d9u:return 0x3147dcu;
    case 0x3147dcu:return 0x3147dfu;
    case 0x3147dfu:return 0x3147e2u;
    case 0x3147e2u:return 0x3147e5u;
    case 0x3147e8u:return 0x3147ebu;
    case 0x3147ebu:return 0x3147eeu;
    case 0x31480bu:return 0x31480fu;
    case 0x31480fu:return 0x314813u;
    case 0x314813u:return 0x314815u;
    case 0x314815u:return 0x314816u;
    case 0x314816u:return 0x314819u;
    case 0x314819u:return 0x31481du;
    case 0x314829u:return 0x31482bu;
    case 0x314832u:return 0x314836u;
    case 0x314836u:return 0x314838u;
    case 0x31483au:return 0x314840u;
    case 0x314840u:return 0x314842u;
    case 0x314849u:return 0x31484bu;
    case 0x314854u:return 0x314856u;
    case 0x314856u:return 0x314858u;
    case 0x314866u:return 0x314867u;
    case 0x314869u:return 0x31486au;
    default:return 0u;
    }
}
inline unsigned depth_at(std::uint32_t next){
    switch(next){
    case 0x314813u:case 0x31481du:case 0x314840u:case 0x314842u:case 0x314856u:return 8u;
    default:return 7u;
    }
}
template<bool Literal,bool Fresh,class State,class Spill>std::uint32_t bridge(State&s,Spill spill){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    (void)s;(void)spill;return 0u;
#else
    if(!admitted<Literal>(s))return 0u;
    ++activity().calls;const std::byte token{};
    const auto retired=[&](auto&state,std::uint32_t pc){
        const auto next=successor(pc);
        if(!next)throw std::logic_error("Unsupported native hull-clip retired instruction");
        if(!admitted<Literal>(state,depth_at(next)))throw Continuation{next,&token};
    };
    try{
        native_solver_hull_clip_source_detail::run_private_source<Fresh>(s,retired,spill);
        ++activity().completed;return 1u;
    }catch(const Continuation&resume){
        if(resume.owner!=&token)throw;
        ++activity().continuations;return resume.next;
    }
#endif
}
} // namespace halo2::native_solver_hull_clip_owner_detail
