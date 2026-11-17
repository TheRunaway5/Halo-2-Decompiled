#pragma once
#include "physics/runtime/native_tu15_solver_hull_line_source_frame.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#endif
#include <cstdlib>
#include <thread>

namespace halo2::native_solver_hull_line_owner_detail {
// Private candidate. No generated owner or public entry includes this header.
inline bool reference(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_SOLVER_HULL_LINE");return p&&*p;}();return enabled;}
struct Activity{std::uint64_t calls{},completed{},continuations{},children{};};
struct Continuation{std::uint32_t pc;const void*owner;};
inline Activity&activity(){thread_local Activity value;return value;}
template<bool Literal,class State>bool admitted(const State&s,unsigned limit=7u){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if constexpr(!Literal)if(s.services.instruction)return false;
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>limit)return false;
    if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;
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
template<bool Literal,class State>bool continuation_admitted(const State&s,std::uint32_t next){
    // A successful owned FLD remains live until the FCOMP retires.
    const bool extra=next==0x31371cu||next==0x313721u||next==0x313724u||next==0x313729u||next==0x31372cu||next==0x313756u;
    return admitted<Literal>(s,extra?8u:7u);
}
template<bool Literal,bool ClearLoadC1,class State,class Call>std::uint32_t bridge(State&s,Call call){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return 0u;
#else
    if(!admitted<Literal>(s))return 0u;++activity().calls;const std::byte token{};
    const auto child=[&](auto&v,std::uint32_t target,std::uint32_t pc){++activity().children;call(v,target,pc);};
    const auto checkpoint=[&](auto&v,std::uint32_t next){if(!continuation_admitted<Literal>(v,next))throw Continuation{next,&token};};
    try{
        native_solver_hull_line_source_detail::run_private_source<ClearLoadC1>(s,child,native_solver_hull_line_source_detail::Noop{},checkpoint);
        ++activity().completed;return 1u;
    }catch(const Continuation&resume){if(resume.owner!=&token)throw;++activity().continuations;return resume.pc;}
#endif
}
} // namespace halo2::native_solver_hull_line_owner_detail
