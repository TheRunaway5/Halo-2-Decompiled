#include "halo2/native_tu15_solver_hull_polytope_state.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
void native_tu15_clear_solver_hull_polytope_flags(SourcePhysicsContactMemory&memory,
    std::uint32_t polytope,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15SolverHullPolytopeStateStats*stats) {
    if(stats)++stats->clear_calls;
    try {
        const auto word=[&](std::uint32_t p) {
            std::array<std::byte,4>b{};memory.read(p,b);std::uint32_t n{};
            for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);
            return n;
        };
        const auto clear=[&](std::uint32_t pc,std::uint32_t p,unsigned width) {
            std::array<std::byte,4>zero{};SourcePhysicsContactRecordStore e{pc,p,{},{}};
            if(observe){e.before.resize(width);memory.read(p,e.before);}
            memory.write(p,std::span(zero).first(width));
            if(observe){e.written.assign(width,std::byte{});observe(e);}
        };
        auto count=word(polytope+0x1b4u);
        if(std::bit_cast<std::int32_t>(count)>0) {
            std::uint32_t index{},offset{};
            do {
                const auto array=word(polytope+0x1b0u);
                clear(0x314896u,array+offset+0x44u,1u);
                count=word(polytope+0x1b4u);++index;offset+=0x50u;
                if(stats)++stats->facets_cleared;
                if(std::bit_cast<std::int32_t>(index)>=std::bit_cast<std::int32_t>(count))break;
            }while(true);
        }
        count=word(polytope+0x2104u);
        if(std::bit_cast<std::int32_t>(count)>0) {
            std::uint32_t index{},offset{};
            do {
                const auto array=word(polytope+0x2100u);
                clear(0x3148c6u,array+offset+0x10u,4u);
                count=word(polytope+0x2104u);++index;offset+=0x20u;
                if(stats)++stats->edges_cleared;
                if(std::bit_cast<std::int32_t>(index)>=std::bit_cast<std::int32_t>(count))break;
            }while(true);
        }
        if(stats)++stats->clear_completed;
    }catch(...){if(stats)++stats->refusals;throw;}
}

std::uint32_t native_tu15_clip_solver_hull_facet(SourcePhysicsContactMemory&memory,
    std::uint32_t facet,std::uint32_t point,std::uint32_t lower,std::uint32_t upper,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const SourcePhysicsContactRecordObserver&observe,
    NativeTu15SolverHullPolytopeStateStats*stats) {
    if(stats)++stats->clip_calls;
    try {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        using namespace joint_constraint_numeric;
        if(depth)throw std::runtime_error("Native facet clipping requires qualified empty x87 DATA");
        Environment phase(fp,depth);Memory m(memory,observe);
        const auto reference=m.word(facet+0x14u);
        const auto origin=m.vector(reference);auto difference=m.vector(point);
        asm volatile("subps %1,%0":"+x"(difference):"x"(origin):"memory");
        const auto normal=m.vector(facet);const auto product=multiply(normal,difference);
        const auto projection=sum_three(product);
        push(projection);const auto initial_lower=m.word(lower);
        asm volatile("fcomps %0"::"m"(initial_lower):"memory","st");
        push(projection);std::uint16_t relation;
        asm volatile("fnstsw %0":"=a"(relation));
        std::uint32_t result=1u;
        if((relation&0x4100u)==0) {
            const auto current_upper=m.word(upper);
            asm volatile("fcomps %1;fnstsw %0":"=a"(relation):"m"(current_upper):"memory","st");
            if((std::popcount(unsigned((relation>>8u)&5u))&1u)!=0)
                m.word(0x314836u,upper,projection);
        }else {
            add_scalar(m.word(0x41d7a0u));const auto current_lower=m.word(lower);
            asm volatile("fcoms %1;fnstsw %0":"=a"(relation):"m"(current_lower):"memory");
            if((relation&0x4100u)==0)m.pop_float(0x314849u,lower);
            else asm volatile("fstp %%st(0)":::"memory","st");
            result=2u;
        }
        push(m.word(lower));const auto current_upper=m.word(upper);
        asm volatile("fcomps %1;fnstsw %0":"=a"(relation):"m"(current_upper):"memory","st");
        if(!(relation&0x100u))result=4u;
        if(stats)++stats->clip_completed;return result;
#else
        (void)memory;(void)facet;(void)point;(void)lower;(void)upper;
        (void)fp;(void)depth;(void)observe;
        throw std::runtime_error("Exact native facet clipping backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2
