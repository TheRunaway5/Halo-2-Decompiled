#include "halo2/native_tu15_solver_hull_polytope.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
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
std::uint32_t native_tu15_sample_solver_hull_direction(SourcePhysicsContactMemory&memory,
    std::uint32_t seed,std::uint32_t lower,std::uint32_t upper,std::uint32_t output,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15SolverHullPolytopeStats*stats){
    if(stats)++stats->sample_calls;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        using namespace joint_constraint_numeric;
        if(depth)throw std::runtime_error("Native hull sampling requires qualified empty x87 DATA");
        Environment phase(fp,depth);Memory m(memory,observe);push(upper);
        asm volatile("fsubs %0"::"m"(lower):"memory","st");
        std::uint32_t current=(0x3039u-seed*0x3e39b193u)&0x7fffffffu;
        constexpr std::array<std::uint32_t,3>sites{0x313353u,0x313375u,0x313388u};
        for(unsigned axis=0;axis<3;++axis){
            const auto signed_value=std::bit_cast<std::int32_t>(current);
            asm volatile("fildl %0"::"m"(signed_value):"memory","st");
            const auto scale=m.word(0x41d780u);
            asm volatile("fmuls %0;fadds %1;fmul %%st(1),%%st(0)"::"m"(scale),"m"(lower):"memory","st");
            if(axis<2)current=(0x3039u-current*0x3e39b193u)&0x7fffffffu;
            m.pop_float(sites[axis],output+axis*4u);
        }
        asm volatile("fstp %%st(0)":::"memory","st");if(stats)++stats->sample_completed;return current;
#else
        (void)memory;(void)seed;(void)lower;(void)upper;(void)output;(void)fp;(void)depth;(void)observe;
        throw std::runtime_error("Exact native hull sampling backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2
