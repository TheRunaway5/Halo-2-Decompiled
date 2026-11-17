#pragma once
#include "halo2/native_tu15_solver_hull_support.hpp"
#include "halo2/native_tu15_solver_hull_initialize_kernel.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <bit>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <utility>
#include <xmmintrin.h>
namespace halo2::native_solver_hull_initialize_value_detail {
// Private depth-zero public compatibility view. Preserve the old two
// Environment lifetimes and16-byte vector events; no raw ABI or hostile
// provider-state contract is inferred from this value adapter.
template<class Memory,class Environment>struct View {
    using Vector=decltype(std::declval<Memory&>().vector(std::uint32_t{}));
    using Data=NativeTu15SolverHullInitialize<Vector>;using P=NativeTu15SolverHullInitializePhase;
    Memory memory;SourcePhysicsContactMemory&backing;NativeTu15PhysicsFrameFp&fp;unsigned depth;
    const NativeTu15SolverHullSupportServices&services;const SourcePhysicsContactRecordObserver&observe;
    NativeTu15SolverHullSupportStats*stats;std::uint32_t context,output_flag_address,output_plane;
    NativeTu15SolverHullInitializeScratch scratch;std::optional<Environment>phase;
    void begin(Data&d){d.context=context;phase.emplace(fp,depth);}
    Vector vector(P p,const Data&d){
        switch(p){
        case P::sign:return memory.vector(0x461960u);
        case P::translation:return memory.vector(context+0x30u);
        case P::difference:phase.emplace(fp,depth);return memory.vector(d.differences);
        case P::fresh_difference:return memory.vector(d.differences);
        case P::rounded_length:return memory.vector(scratch.squared_vector);
        case P::direction:return memory.vector(scratch.direction);
        case P::point:return memory.vector(scratch.point_pair);
        }
        throw std::logic_error("Unknown hull-initialize value acquisition");
    }
    Vector xor_bits(Vector a,Vector b){return _mm_xor_ps(a,b);}
    Vector multiply(Vector a,Vector b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    Vector add_scalar(Vector a,Vector b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
    template<unsigned Mask>Vector shuffle(Vector a,Vector b){return _mm_shuffle_ps(a,b,Mask);}
    Vector square_root(Vector a){asm volatile("sqrtss %0,%0":"+x"(a)::"memory");return a;}
    static std::uint32_t low(Vector value){std::uint32_t bits{};std::memcpy(&bits,&value,4);return bits;}
    bool above_or_unordered(std::uint32_t threshold){
        const auto measured=memory.word(scratch.scalar);asm volatile("flds %0"::"m"(measured):"memory","st");
        const auto bound=memory.word(threshold);std::uint16_t status{};
        asm volatile("fcomps %1;fnstsw %0":"=a"(status):"m"(bound):"memory","st");
        return (std::popcount(unsigned((status>>8u)&0x41u))&1u)==0u;
    }
    void save_direction(const Data&d){memory.vector(0x31357au,scratch.direction,d.direction);}
    bool initial_length_above_or_unordered(const Data&d){memory.word(0x31359cu,scratch.scalar,low(d.length));return above_or_unordered(0x46a180u);}
    void default_direction(const Data&){memory.word(0x3135b1u,scratch.direction,0u);memory.word(0x3135b9u,scratch.direction+4u,0x3f800000u);memory.word(0x3135c1u,scratch.direction+8u,0u);memory.word(0x3135c9u,scratch.direction+12u,0u);}
    void support(const Data&){phase.reset();native_tu15_solver_hull_support_pair(backing,context,scratch.direction,scratch.point_pair,scratch.transformed_direction,fp,depth,services,observe,stats);}
    void append(const Data&){native_tu15_append_solver_hull_vertex(backing,context,scratch.point_pair,fp,depth,observe,stats);}
    std::uint32_t differences_owner(const Data&){return memory.word(context+0x50u);}
    bool difference_above_or_unordered(const Data&d){memory.word(0x313611u,scratch.scalar,low(d.length));return above_or_unordered(0x46a454u);}
    std::uint32_t output_owner(const Data&){return output_plane;}
    void save_square(const Data&d){memory.vector(0x313648u,scratch.squared_vector,d.square);}
    void save_root(const Data&d){memory.word(0x313651u,scratch.squared_vector,low(d.root));}
    void save_and_load_length(const Data&d){memory.word(0x313660u,scratch.scalar,low(d.rounded));const auto bits=memory.word(scratch.scalar);asm volatile("flds %0"::"m"(bits):"memory","st");}
    void output_length(const Data&){memory.pop_float(0x31366du,output_plane+0x20u);}
    void output_direction(const Data&d){memory.vector(0x313670u,output_plane,d.direction_again);}
    void output_point(const Data&d){memory.vector(0x313678u,output_plane+0x10u,d.point);}
    void output_weight(const Data&){memory.word(0x31367cu,output_plane+0x24u,0x3f000000u);}
    void output_flag(const Data&d){
        SourcePhysicsContactRecordStore event{d.produced_plane?0x313686u:0x313693u,output_flag_address,{},{}};
        if(observe){event.before.resize(1);backing.read(output_flag_address,event.before);}
        const std::array bytes{std::byte(d.produced_plane?1u:0u)};backing.write(output_flag_address,bytes);
        if(observe){event.written.assign(bytes.begin(),bytes.end());observe(event);}
    }
    void finish(const Data&){phase.reset();}
};
} // namespace halo2::native_solver_hull_initialize_value_detail
#endif
