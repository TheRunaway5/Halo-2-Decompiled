#pragma once
#include "halo2/native_tu15_solver_hull_polytope.hpp"
#include "physics/native_tu15_joint_constraint_numeric_internal.hpp"
namespace halo2::solver_hull_sample_value_reference {
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
}
