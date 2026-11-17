#pragma once
#include "halo2/native_tu15_solver_hull_line.hpp"
#include "halo2/native_tu15_solver_hull_line_kernel.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <cstring>
#include <optional>
#include <stdexcept>
#include <utility>
#include <xmmintrin.h>

namespace halo2::native_solver_hull_line_value_detail {
// Preserve the legacy DATA facade's two FP lifetimes and single vector
// write. Its explicit scratch addresses do not stand in for the raw stack.
template<class Memory,class Environment>struct View {
    using Vector=decltype(std::declval<Memory&>().vector(std::uint32_t{}));
    using Data=NativeTu15SolverHullLine<Vector>;using P=NativeTu15SolverHullLinePhase;
    static constexpr bool first_sum_before_sign=true;
    Memory memory;SourcePhysicsContactMemory&backing;NativeTu15PhysicsFrameFp&fp;unsigned depth;
    const NativeTu15SolverHullSupportServices&services;const SourcePhysicsContactRecordObserver&observe;
    NativeTu15SolverHullLineStats&stats;std::uint32_t context;NativeTu15SolverHullLineScratch scratch;
    std::optional<Environment>phase;std::uint32_t initial_direction{},second_vertex{};

    void first_support(Data&){
        initial_direction=memory.word(context+0x50u);
        native_tu15_solver_hull_support_pair(backing,context,initial_direction,scratch.first_pair,
            scratch.transformed_direction,fp,depth,services,observe,&stats.support);
    }
    Vector vector(P p,const Data&){
        switch(p){
        case P::first_vertex:phase.emplace(fp,depth);return memory.vector(memory.word(context+0x50u));
        case P::first_transform:return memory.vector(scratch.first_pair+0x20u);
        case P::first_point:return memory.vector(scratch.first_pair);
        case P::sign:return memory.vector(0x461960u);
        case P::direction:return memory.vector(initial_direction);
        case P::second_transform:return memory.vector(scratch.second_pair+0x20u);
        case P::second_point:return memory.vector(scratch.second_pair);
        case P::second_vertex:return memory.vector(second_vertex);
        }
        throw std::logic_error("Unknown hull-line value acquisition");
    }
    Vector subtract(Vector a,Vector b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    Vector multiply(Vector a,Vector b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    Vector add_scalar(Vector a,Vector b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
    template<unsigned Mask>Vector shuffle(Vector a,Vector b){return _mm_shuffle_ps(a,b,Mask);}
    Vector xor_bits(Vector a,Vector b){return _mm_xor_ps(a,b);}
    static std::uint32_t low(Vector value){std::uint32_t bits{};std::memcpy(&bits,&value,4);return bits;}
    void prepare_second(const Data&){}
    void second_support(const Data&d){
        memory.word(0x31370au,scratch.first_distance,low(d.score));
        memory.vector(0x31370eu,scratch.negative_direction,d.negative);
        phase.reset();
        native_tu15_solver_hull_support_pair(backing,context,scratch.negative_direction,scratch.second_pair,
            scratch.transformed_direction,fp,depth,services,observe,&stats.support);
    }
    void load_first_score(const Data&){
        phase.emplace(fp,depth);const auto bits=memory.word(scratch.first_distance);
        asm volatile("flds %0"::"m"(bits):"memory","st");
    }
    void second_vertex_owner(const Data&){second_vertex=memory.word(context+0x50u);}
    bool compare_second(const Data&d){
        memory.word(0x313752u,scratch.second_distance,low(d.score));
        const auto bits=memory.word(scratch.second_distance);std::uint16_t status{};
        asm volatile("fcomps %1;fnstsw %0":"=a"(status):"m"(bits):"memory","st");
        phase.reset();return (status&0x100u)!=0u;
    }
    void append(const Data&d){
        if(d.second)++stats.selected_second;else ++stats.selected_first;
        native_tu15_append_solver_hull_vertex(backing,context,d.second?scratch.second_pair:scratch.first_pair,
            fp,depth,observe,&stats.support);
    }
    void finish(const Data&){}
};
} // namespace halo2::native_solver_hull_line_value_detail
#endif
