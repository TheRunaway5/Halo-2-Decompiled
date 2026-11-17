#pragma once
#include "halo2/native_tu15_solver_hull_support.hpp"
#include "physics/native_tu15_joint_constraint_numeric_internal.hpp"
namespace halo2::solver_hull_initialize_value_reference {
void hull_byte(SourcePhysicsContactMemory&memory,std::uint32_t pc,std::uint32_t p,
    std::uint8_t value,const SourcePhysicsContactRecordObserver&observe){
    SourcePhysicsContactRecordStore event{pc,p,{},{}};
    if(observe){event.before.resize(1);memory.read(p,event.before);}
    const std::array b{std::byte(value)};memory.write(p,b);
    if(observe){event.written.assign(b.begin(),b.end());observe(event);}
}
std::uint32_t native_tu15_initialize_solver_hull(SourcePhysicsContactMemory&memory,
    std::uint32_t context,std::uint32_t output_flag,std::uint32_t output_plane,
    NativeTu15SolverHullInitializeScratch scratch,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15SolverHullSupportServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15SolverHullSupportStats*stats){
    if(stats)++stats->initialize_calls;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        using namespace joint_constraint_numeric;
        if(depth)throw std::runtime_error("Native solver hull initialization requires qualified empty x87 DATA");
        Memory m(memory,observe);
        const auto above_or_unordered=[&](std::uint32_t measured,std::uint32_t threshold){
            push(measured);const auto bound=m.word(threshold);std::uint16_t status;
            asm volatile("fcomps %1;fnstsw %0":"=a"(status):"m"(bound):"memory","st");
            return (std::popcount(unsigned((status>>8u)&0x41u))&1u)==0u;
        };
        {
            Environment phase(fp,depth);
            const auto sign=m.vector(0x461960u),translation=m.vector(context+0x30u);
            const auto direction=xor_bits(translation,sign);m.vector(0x31357au,scratch.direction,direction);
            const auto product=multiply(direction,direction),sum=add_scalar(lane<0xaau>(product),add_scalar(lane<0x55u>(product),product));
            m.word(0x31359cu,scratch.scalar,scalar_bits(sum));
            if(!above_or_unordered(m.word(scratch.scalar),0x46a180u)){
                m.word(0x3135b1u,scratch.direction,0);m.word(0x3135b9u,scratch.direction+4u,0x3f800000u);
                m.word(0x3135c1u,scratch.direction+8u,0);m.word(0x3135c9u,scratch.direction+12u,0);
            }
        }
        native_tu15_solver_hull_support_pair(memory,context,scratch.direction,scratch.point_pair,
            scratch.transformed_direction,fp,depth,services,observe,stats);
        native_tu15_append_solver_hull_vertex(memory,context,scratch.point_pair,fp,depth,observe,stats);
        const auto differences=m.word(context+0x50u);
        {
            Environment phase(fp,depth);
            const auto first=m.vector(differences),product=multiply(first,first);
            const auto sum=add_scalar(lane<0xaau>(product),add_scalar(lane<0x55u>(product),product));
            m.word(0x313611u,scratch.scalar,scalar_bits(sum));
            if(!above_or_unordered(m.word(scratch.scalar),0x46a454u)){
                const auto current=m.vector(differences),squared=multiply(current,current);
                auto square=add_scalar(lane<0xaau>(squared),add_scalar(lane<0x55u>(squared),squared));
                m.vector(0x313648u,scratch.squared_vector,square);
                asm volatile("sqrtss %0,%0":"+x"(square)::"memory");
                m.word(0x313651u,scratch.squared_vector,scalar_bits(square));
                const auto rounded=m.vector(scratch.squared_vector);m.word(0x313660u,scratch.scalar,scalar_bits(rounded));
                push(m.word(scratch.scalar));const auto direction=m.vector(scratch.direction);
                m.pop_float(0x31366du,output_plane+0x20u);m.vector(0x313670u,output_plane,direction);
                const auto first_point=m.vector(scratch.point_pair);m.vector(0x313678u,output_plane+0x10u,first_point);
                m.word(0x31367cu,output_plane+0x24u,0x3f000000u);hull_byte(memory,0x313686u,output_flag,1,observe);
            }else hull_byte(memory,0x313693u,output_flag,0,observe);
        }
        if(stats)++stats->initialize_completed;return output_flag;
#else
        (void)memory;(void)context;(void)output_flag;(void)output_plane;(void)scratch;(void)fp;(void)depth;(void)services;(void)observe;
        throw std::runtime_error("Exact native solver hull backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
}
