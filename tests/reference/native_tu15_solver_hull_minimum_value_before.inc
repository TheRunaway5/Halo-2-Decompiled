#pragma once
#include "halo2/native_tu15_solver_hull_polytope.hpp"
#include "physics/native_tu15_joint_constraint_numeric_internal.hpp"
namespace halo2::solver_hull_minimum_value_reference {
std::uint32_t native_tu15_minimum_solver_hull_facet(SourcePhysicsContactMemory&memory,
    std::uint32_t polytope,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    NativeTu15SolverHullPolytopeStats*stats){
    if(stats)++stats->minimum_calls;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        using namespace joint_constraint_numeric;
        if(depth)throw std::runtime_error("Native facet minimum requires qualified empty x87 DATA");
        Environment phase(fp,depth);const SourcePhysicsContactRecordObserver no_observer;
        Memory m(memory,no_observer);
        const auto first=m.word(polytope+0x1b0u),count=m.word(polytope+0x1b4u);
        push(m.word(first+0x10u));auto minimum=first,cursor=first;
        const auto candidate=[&](std::uint32_t record){
            const auto value=m.word(record+0x10u);std::uint32_t rounded;std::uint16_t relation;
            asm volatile("flds %2;fsts %0;fcomp %%st(1);fnstsw %1":"=m"(rounded),"=a"(relation):"m"(value):"memory","st");
            if(stats)++stats->facet_candidates;
            if((std::popcount(unsigned((relation>>8u)&5u))&1u)!=0){
                asm volatile("fstp %%st(0)":::"memory","st");minimum=record;push(rounded);
            }
        };
        auto remaining=count-2u;const auto count_minus_one=remaining+1u;
        if(std::bit_cast<std::int32_t>(count_minus_one)>=4){
            auto batches=count_minus_one>>2u;remaining-=batches*4u;
            do{candidate(cursor+0x50u);candidate(cursor+0xa0u);candidate(cursor+0xf0u);candidate(cursor+0x140u);cursor+=0x140u;}while(--batches);
        }
        if(std::bit_cast<std::int32_t>(remaining)>=0){
            auto tail=remaining+1u;do{cursor+=0x50u;candidate(cursor);}while(--tail);
        }
        asm volatile("fstp %%st(0)":::"memory","st");if(stats)++stats->minimum_completed;return minimum;
#else
        (void)memory;(void)polytope;(void)fp;(void)depth;
        throw std::runtime_error("Exact native facet minimum backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
}
