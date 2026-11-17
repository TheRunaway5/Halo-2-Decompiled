#pragma once
#include "halo2/native_tu15_solver_hull_minimum_kernel.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_polytope.hpp"
#include <bit>

namespace halo2::native_solver_hull_minimum_value_detail {
// The public DATA function keeps rounded scratch in local storage. Its
// original provider reads and selected facet identity are unchanged.
template<class Memory>struct View {
    Memory memory;std::uint32_t polytope{};
    NativeTu15SolverHullPolytopeStats*stats{};
    std::uint32_t minimum{},cursor{},remaining{},batches{},tail{};
    static void load(std::uint32_t bits){asm volatile("flds %0"::"m"(bits):"memory","st");}
    static void drop(){asm volatile("fstp %%st(0)":::"memory","st");}
    void begin(){
        const auto first=memory.word(polytope+0x1b0u),count=memory.word(polytope+0x1b4u);
        load(memory.word(first+0x10u));minimum=cursor=first;remaining=count-2u;
    }
    bool begin_batches(){
        const auto count_minus_one=remaining+1u;
        if(std::bit_cast<std::int32_t>(count_minus_one)<4)return false;
        batches=count_minus_one>>2u;remaining-=batches*4u;return true;
    }
    void candidate(std::uint32_t record){
        const auto value=memory.word(record+0x10u);
        std::uint32_t rounded;std::uint16_t relation;
        asm volatile("flds %2;fsts %0;fcomp %%st(1);fnstsw %1":"=m"(rounded),"=a"(relation):"m"(value):"memory","st");
        if(stats)++stats->facet_candidates;
        if((std::popcount(unsigned((relation>>8u)&5u))&1u)!=0){
            drop();minimum=record;load(rounded);
        }
    }
    void batch_candidate(unsigned slot){candidate(cursor+(slot+1u)*0x50u);}
    bool next_batch(){cursor+=0x140u;return --batches!=0u;}
    void end_batches(){}
    bool begin_tail(){
        if(std::bit_cast<std::int32_t>(remaining)<0)return false;
        tail=remaining+1u;return true;
    }
    void tail_candidate(){cursor+=0x50u;candidate(cursor);}
    bool next_tail(){return --tail!=0u;}
    void finish(){drop();}
};
} // namespace halo2::native_solver_hull_minimum_value_detail
#endif
