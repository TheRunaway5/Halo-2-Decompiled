#pragma once
#include "physics/runtime/native_tu15_solver_hull_basis_source_frame.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#endif
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace halo2::native_solver_hull_basis_owner_detail {
inline bool reference(){
    static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_SOLVER_HULL_BASIS");return p&&*p;}();
    return enabled;
}
struct Activity{std::uint64_t calls{},completed{},continuations{};};
inline Activity&activity(){thread_local Activity value;return value;}
struct Continuation{std::uint32_t pc;const void*owner;};

// The original entry has 56 nonterminal provider/child boundaries. The
// reciprocal-estimate callback is a source instruction boundary as well.
inline constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,56> frontiers{{
    {0x313790u,0x313791u}, {0x31379cu,0x31379du}, {0x31379fu,0x3137a2u}, {0x3137a2u,0x3137a6u},
    {0x3137a6u,0x3137a9u}, {0x3137acu,0x3137b1u}, {0x3137b1u,0x3137b5u}, {0x3137b9u,0x3137bdu},
    {0x3137bfu,0x3137c0u}, {0x3137c0u,0x3137c4u}, {0x3137c4u,0x3137c8u}, {0x3137d4u,0x3137d8u},
    {0x3137e5u,0x3137e9u}, {0x3137eeu,0x3137f2u}, {0x313804u,0x313808u}, {0x313808u,0x31380cu},
    {0x31381cu,0x313824u}, {0x313824u,0x313828u}, {0x313828u,0x31382cu}, {0x31382cu,0x313831u},
    {0x313866u,0x313869u}, {0x313870u,0x313875u}, {0x313875u,0x313879u}, {0x313879u,0x31387fu},
    {0x31387fu,0x313884u}, {0x31388cu,0x313894u}, {0x313894u,0x31389au}, {0x3138a1u,0x3138a9u},
    {0x3138a9u,0x3138afu}, {0x3138d5u,0x3138dau}, {0x3138e8u,0x3138edu}, {0x3138edu,0x3138f1u},
    {0x3138f1u,0x3138f7u}, {0x3138f7u,0x3138fcu}, {0x31390cu,0x31390du}, {0x313918u,0x313919u},
    {0x313924u,0x313925u}, {0x313927u,0x31392cu}, {0x31392cu,0x313931u}, {0x313931u,0x313934u},
    {0x31393bu,0x31393cu}, {0x31393cu,0x31393du}, {0x313941u,0x313942u}, {0x313944u,0x313949u},
    {0x313949u,0x31394du}, {0x31394du,0x313954u}, {0x31395fu,0x313960u}, {0x313962u,0x313967u},
    {0x313967u,0x313968u}, {0x313968u,0x313969u}, {0x31396bu,0x31396cu}, {0x313974u,0x313975u},
    {0x313977u,0x31397cu}, {0x31397cu,0x31397du}, {0x31397du,0x31397eu}, {0x313980u,0x313981u}
}};
inline std::uint32_t successor(std::uint32_t pc){
    for(const auto&[before,after]:frontiers)if(before==pc)return after;
    throw std::logic_error("Unsupported hull-basis source frontier");
}
inline unsigned depth_limit_after(std::uint32_t next){
    switch(next){
    case 0x3137c8u:return 8u; // three local ranking values
    case 0x3137bdu:case 0x3137c0u:case 0x3137c4u:case 0x3137d8u:case 0x3137f2u:return 7u;
    case 0x3137b5u:case 0x3137e9u:
    case 0x313808u:case 0x31380cu:case 0x313824u:case 0x313828u:
    case 0x31394du:return 6u;
    default:return 5u;
    }
}
template<bool Literal,class State>bool admitted(const State&s,unsigned depth_limit=5u){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if constexpr(!Literal)if(s.services.instruction)return false;
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||
       !native_raw_x87_detail::supported(s)||s.fp.count>depth_limit)return false;
    if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;
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
template<bool Literal,bool ClearC1,class State,class Spill,class Estimate,class Call>
std::uint32_t bridge(State&s,Spill spill,Estimate estimate,Call call){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return 0u;
#else
    if(!admitted<Literal>(s))return 0u;
    ++activity().calls;const std::byte token{};
    const auto checkpoint=[&](auto&v,std::uint32_t pc){
        const auto next=successor(pc);
        if(!admitted<Literal>(v,depth_limit_after(next)))throw Continuation{next,&token};
    };
    try{
        native_solver_hull_basis_source_detail::run_private_source<ClearC1>(
            s,native_solver_hull_basis_source_detail::Noop{},checkpoint,spill,estimate,call);
        ++activity().completed;return 1u;
    }catch(const Continuation&resume){
        if(resume.owner!=&token)throw;
        ++activity().continuations;return resume.pc;
    }
#endif
}
} // namespace halo2::native_solver_hull_basis_owner_detail
