#pragma once
#include "physics/runtime/native_tu15_solver_hull_minimum_source_frame.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#endif
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <thread>

namespace halo2::native_solver_hull_minimum_owner_detail {
inline bool reference(){
    static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_SOLVER_HULL_MINIMUM");return p&&*p;}();
    return enabled;
}
struct Activity{std::uint64_t calls{},completed{},continuations{};};
inline Activity&activity(){thread_local Activity value;return value;}
struct Continuation{std::uint32_t pc;const void*owner;};

template<bool Literal,class State>bool admitted(const State&s,unsigned depth_limit=6u){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if constexpr(!Literal)if(s.services.instruction)return false;
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||
       !native_raw_x87_detail::supported(s)||s.fp.count>depth_limit)return false;
    if(!s.services.instruction&&(s.instruction_cursor||s.observe))return false;
    if(s.services.instruction){
        const auto*p=s.live_instruction_cursor();
        if(!s.literal_service_sites||!p||!p->literal_service_sites||p->service_sites.size()!=3u||
           p->service_sites[0]!=0x38846eu||p->service_sites[1]!=0x21f590u||p->service_sites[2]!=0x33b595u||
           (p->first_trace&&*p->first_trace)||(p->second_trace&&*p->second_trace))return false;
    }
    return !s.observe||(s.instruction_cursor&&s.instruction_cursor->observation_only_stores);
#endif
}
inline unsigned depth_limit_at(std::uint32_t next){
    switch(next){
    case 0x3131b1u:case 0x3131b7u:case 0x3131bdu:case 0x313288u:case 0x313289u:return 6u;
    case 0x3131e3u:case 0x3131e7u:case 0x3131ffu:case 0x313203u:
    case 0x31321eu:case 0x313222u:case 0x31323du:case 0x313241u:
    case 0x313268u:case 0x31326fu:return 8u;
    default:return 7u;
    }
}
template<bool Literal,bool ClearAuthoredC1,class State,class Spill>std::uint32_t bridge(State&s,Spill spill){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return 0u;
#else
    if(!admitted<Literal>(s))return 0u;
    ++activity().calls;const std::byte token{};
    const auto checkpoint=[&](auto&v,std::uint32_t next){
        if(!admitted<Literal>(v,depth_limit_at(next)))throw Continuation{next,&token};
    };
    try{
        native_solver_hull_minimum_source_detail::run_private_source<ClearAuthoredC1>(
            s,native_solver_hull_minimum_source_detail::Noop{},checkpoint,spill);
        ++activity().completed;return 1u;
    }catch(const Continuation&resume){
        if(resume.owner!=&token)throw;
        ++activity().continuations;return resume.pc;
    }
#endif
}
} // namespace halo2::native_solver_hull_minimum_owner_detail
