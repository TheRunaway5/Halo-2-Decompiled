#pragma once
#include "physics/runtime/native_tu15_solver_hull_edge_frame.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include <cstdlib>
#include <thread>
namespace halo2::native_solver_hull_edge_owner_detail {
inline bool reference(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_SOLVER_HULL_EDGE");return p&&*p;}();return enabled;}
struct Activity{std::uint64_t calls{},completed{},continuations{},dispatch_children{};};
struct Continuation{std::uint32_t pc;const void*owner;};
inline Activity&activity(){thread_local Activity value;return value;}
template<bool Literal=true,class State>bool admitted(const State&s,unsigned count_limit=7u){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if constexpr(!Literal)if(s.services.instruction)return false;
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>count_limit)return false;
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
template<bool Literal=true,class State>bool continuation_admitted(const State&s,std::uint32_t next){
    const unsigned limit=next>=0x3134f8u&&next<=0x313525u?8u:7u;
    if(!admitted<Literal>(s,limit))return false;
    // Retained vector providers need not implement a physical MOVAPS fault.
    // If live provider feedback creates an unaligned next operand, resume
    // the exact source suffix before executing that original vector access.
    std::uint32_t address{};
    switch(next){
    case 0x313484u:case 0x3134beu:address=s.gp[3];break;
    case 0x313492u:address=s.gp[4]+0x28u;break;
    case 0x3134b1u:address=s.gp[2];break;
    case 0x3134b4u:address=s.gp[4]+0x30u;break;
    case 0x3134b9u:address=s.gp[4]+0x50u;break;
    case 0x3134c1u:address=s.gp[6]+0x20u;break;
    case 0x3134e3u:address=s.gp[4]+0x20u;break;
    case 0x3134f8u:case 0x313539u:address=s.gp[6];break;
    case 0x313529u:if(!(s.fp.status&0x100u))address=s.gp[4]+0x40u;break;
    case 0x31353cu:address=s.gp[6]+0x10u;break;
    case 0x313540u:address=s.gp[6]+0x20u;break;
    default:break;
    }
    return !(address&15u);
}
template<bool Literal=true,class State,class Call>std::uint32_t bridge(State&s,Call call){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return 0u;
#else
    if(!admitted<Literal>(s))return 0u;++activity().calls;const std::byte token{};
    const auto child=[&](auto&v,std::uint32_t target,std::uint32_t site){++activity().dispatch_children;call(v,target,site);};
    // The one extra live value between FLD3134F4 and FCOMP313525 belongs
    // to this owner; a caller may enter with seven live logical values.
    const auto checkpoint=[&](auto&v,std::uint32_t next){if(!continuation_admitted<Literal>(v,next))throw Continuation{next,&token};};
    try{
        native_solver_hull_edge_detail::run_private_owner<native_solver_hull_edge_detail::Publication::retained>(s,child,native_solver_hull_edge_detail::Noop{},checkpoint);
        ++activity().completed;return 1u;
    }catch(const Continuation&resume){if(resume.owner!=&token)throw;++activity().continuations;return resume.pc;}
#endif
}
} // namespace halo2::native_solver_hull_edge_owner_detail
