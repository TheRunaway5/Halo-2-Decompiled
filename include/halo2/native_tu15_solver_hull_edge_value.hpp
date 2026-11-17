#pragma once
#include "halo2/native_tu15_solver_hull_edge.hpp"
#include "halo2/native_tu15_solver_hull_edge_kernel.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <cstring>
#include <optional>
#include <stdexcept>
#include <utility>
#include <xmmintrin.h>
namespace halo2::native_solver_hull_edge_value_detail {
// Private candidate for the existing depth-zero value interface. It retains
// its two Environment lifetimes and one16-byte vector event; the raw Frame's
// live GP/formals and8+8 events are separate contracts.
template<class Memory,class Environment>struct View {
    using Vector=decltype(std::declval<Memory&>().vector(std::uint32_t{}));
    using Data=NativeTu15SolverHullEdge<Vector>;using P=NativeTu15SolverHullEdgePhase;
    Memory memory;SourcePhysicsContactMemory&backing;NativeTu15PhysicsFrameFp&fp;unsigned depth;
    const NativeTu15SolverHullSupportServices&services;const SourcePhysicsContactRecordObserver&observe;
    NativeTu15SolverHullEdgeStats&stats;std::uint32_t context,direction,origin,output;
    NativeTu15SolverHullEdgeScratch scratch;std::optional<Environment>phase;
    void begin(Data&d){d.context=context;}
    std::uint32_t direction_owner(const Data&){return direction;}
    std::uint32_t output_owner(const Data&){return output;}
    std::uint32_t origin_owner(const Data&){return origin;}
    Vector vector(P p,const Data&){
        if(p==P::sign||p==P::origin)phase.emplace(fp,depth);
        switch(p){
        case P::sign:return memory.vector(0x461960u);
        case P::axis:case P::axis_again:return memory.vector(direction);
        case P::origin:return memory.vector(origin);
        case P::first_point:return memory.vector(scratch.first_pair);
        case P::first_transformed:return memory.vector(scratch.first_pair+0x20u);
        case P::second_transformed:return memory.vector(output+0x20u);
        case P::negative_axis:return memory.vector(scratch.negative_direction);
        case P::second_point:return memory.vector(output);
        case P::first_local_second:return memory.vector(scratch.first_pair+0x10u);
        }
        throw std::logic_error("Unknown hull-edge value acquisition");
    }
    void prepare_first(const Data&){}
    Vector xor_bits(Vector a,Vector b){return _mm_xor_ps(a,b);}
    template<unsigned Mask>Vector shuffle(Vector a,Vector b){return _mm_shuffle_ps(a,b,Mask);}
    Vector subtract(Vector a,Vector b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    Vector multiply(Vector a,Vector b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    Vector add_scalar(Vector a,Vector b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
    static std::uint32_t low(Vector value){std::uint32_t bits{};std::memcpy(&bits,&value,4);return bits;}
    void support_first(const Data&d){memory.vector(0x313494u,scratch.negative_direction,d.negative);phase.reset();native_tu15_solver_hull_support_pair(backing,context,direction,scratch.first_pair,scratch.transformed_direction,fp,depth,services,observe,&stats.support);}
    void support_second(const Data&){native_tu15_solver_hull_support_pair(backing,context,scratch.negative_direction,output,scratch.transformed_direction,fp,depth,services,observe,&stats.support);}
    void record_first_and_load_comparison(const Data&d){memory.word(0x3134f0u,scratch.first_projection,low(d.first_sum));const auto bits=memory.word(scratch.first_projection);asm volatile("flds %0"::"m"(bits):"memory","st");}
    void record_second(const Data&d){memory.word(0x313521u,scratch.second_projection,low(d.second_sum));}
    bool select_first(const Data&){const auto bits=memory.word(scratch.second_projection);std::uint16_t relation{};asm volatile("fcomps %1;fnstsw %0":"=a"(relation):"m"(bits):"memory","st");return !(relation&0x100u);}
    std::uint32_t first_projection_again(const Data&){return memory.word(scratch.first_projection);}
    std::uint32_t second_projection_again(const Data&){return memory.word(scratch.second_projection);}
    void promote_first(const Data&d){memory.vector(0x313539u,output,d.first);memory.vector(0x31353cu,output+0x10u,d.first_local_second);memory.vector(0x313540u,output+0x20u,d.first_transformed);memory.word(0x313544u,output+0x30u,d.selected_projection);}
    void keep_second(const Data&d){memory.word(0x313555u,output+0x30u,d.selected_projection);}
    void finish(const Data&d){if(d.selected_first)++stats.selected_first;else ++stats.selected_second;phase.reset();}
};
} // namespace halo2::native_solver_hull_edge_value_detail
#endif
