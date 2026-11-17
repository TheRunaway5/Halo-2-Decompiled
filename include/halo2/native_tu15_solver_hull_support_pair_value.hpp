#pragma once
#include "halo2/native_tu15_solver_hull_support.hpp"
#include "halo2/native_tu15_solver_hull_support_pair_kernel.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <optional>
#include <stdexcept>
#include <utility>
#include <xmmintrin.h>
namespace halo2::native_solver_hull_support_pair_value_detail {
template<class Memory,class Environment>struct View {
    using Vector=decltype(std::declval<Memory&>().vector(std::uint32_t{}));
    using Data=NativeTu15SolverHullSupportPair<Vector>;using P=NativeTu15SolverHullSupportPairPhase;
    static constexpr bool translation_before_second_y=true;
    Memory memory;NativeTu15PhysicsFrameFp&fp;unsigned depth;
    const NativeTu15SolverHullSupportServices&services;NativeTu15SolverHullSupportStats*stats;
    std::uint32_t context,direction,point_pair,transformed_direction;
    std::optional<Environment>phase;
    void begin(Data&d){d.context=context;d.direction=direction;d.point_pair=point_pair;d.second_output=point_pair+16u;d.transformed_direction=transformed_direction;}
    std::uint32_t first_shape(const Data&d){return memory.word(d.context+0x40u);}
    std::uint32_t first_table(const Data&d){return memory.word(d.first_shape);}
    void support(std::uint32_t shape,std::uint32_t table,std::uint32_t input,std::uint32_t output){
        const auto owner=memory.word(table+0x30u);if(stats)++stats->shape_support_calls;
        if(!services.support)throw std::runtime_error("Native solver hull requires its actual shape-support service");
        services.support({shape},{owner},input,output,fp);
    }
    void support_first(const Data&d){support(d.first_shape,d.first_table,d.direction,d.point_pair);}
    Vector vector(P p,const Data&,std::uint32_t address){
        if(p==P::first_z||p==P::second_local)phase.emplace(fp,depth);
        return memory.vector(address);
    }
    std::uint32_t second_shape(const Data&d){return memory.word(d.context+0x44u);}
    Vector xor_bits(Vector a,Vector b){asm volatile("xorps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    template<unsigned Mask>Vector shuffle(Vector a,Vector b){return _mm_shuffle_ps(a,b,Mask);}
    Vector multiply(Vector a,Vector b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    Vector add(Vector a,Vector b){asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    void prepare_second(const Data&d){memory.vector(0x31341du,d.transformed_direction,d.transformed);phase.reset();}
    std::uint32_t second_table(const Data&d){return memory.word(d.second_shape);}
    void support_second(const Data&d){support(d.second_shape,d.second_table,d.transformed_direction,d.second_output);}
    void store_result(const Data&d){memory.vector(0x313461u,d.point_pair+0x20u,d.result);}
    void finish(const Data&){phase.reset();}
};
} // namespace halo2::native_solver_hull_support_pair_value_detail
#endif
