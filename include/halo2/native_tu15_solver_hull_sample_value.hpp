#pragma once
#include "halo2/native_tu15_solver_hull_sample_kernel.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <array>
#include <bit>

namespace halo2::native_solver_hull_sample_value_detail {
// The old public DATA entry owns its empty-stack FP environment and
// receives the seed and bounds as values. It never writes a caller slot.
template<class Memory>struct View {
    Memory memory;
    std::uint32_t current{},lower{},upper{},output{};
    void begin_range(){
        asm volatile("flds %1;fsubs %0"::"m"(lower),"m"(upper):"memory","st");
    }
    void begin_seed(){current=(native_solver_hull_sample_offset-current*native_solver_hull_sample_multiplier)&native_solver_hull_sample_mask;}
    void scale_sample(unsigned){
        const auto integer=std::bit_cast<std::int32_t>(current);
        asm volatile("fildl %0"::"m"(integer):"memory","st");
        const auto scale=memory.word(0x41d780u);
        asm volatile("fmuls %0"::"m"(scale):"memory","st");
    }
    void add_lower(unsigned){asm volatile("fadds %0"::"m"(lower):"memory","st");}
    void scale_range(){asm volatile("fmul %%st(1),%%st(0)":::"memory","st");}
    void advance_seed(unsigned){begin_seed();}
    void publish_sample(unsigned axis){
        constexpr std::array sites{0x313353u,0x313375u,0x313388u};
        memory.pop_float(sites[axis],output+axis*4u);
    }
    void finish(){asm volatile("fstp %%st(0)":::"memory","st");}
};
} // namespace halo2::native_solver_hull_sample_value_detail
#endif
