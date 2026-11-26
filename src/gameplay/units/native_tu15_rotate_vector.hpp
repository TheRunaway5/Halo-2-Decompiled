#pragma once
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_rotate_vector_detail {
using namespace native_object_tick_detail::boundary;
using native_object_tick_detail::Trace;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;
using native_small_queries_detail::VerifySummary;

enum Branch : unsigned {separate, in_place, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{};return value;}
inline VerifySummary &summary(){static VerifySummary value{"Rotate vector"};return value;}

struct Term {
    std::uint32_t coefficient_offset, vector_offset;
    std::uint32_t coefficient_pc, component_pc;
};
struct Axis {
    std::array<Term,3> terms;
    std::array<std::uint32_t,2> sum_pcs;
    std::uint32_t output_pc;
};
// Each row reads its coefficients and components in this order, including
// fresh component reads after storing an earlier row to overlapping memory.
inline constexpr Axis axes[]{
    {{{{0x18u,8u,0x145b91u,0x145b96u},
       {0x0cu,4u,0x145b9bu,0x145ba0u},
       {0x00u,0u,0x145ba9u,0x145badu}}},{0x145ba5u,0x145bb1u},0x145bb5u},
    {{{{0x1cu,8u,0x145bb9u,0x145bbeu},
       {0x04u,0u,0x145bc3u,0x145bc8u},
       {0x10u,4u,0x145bd0u,0x145bd5u}}},{0x145bccu,0x145bdau},0x145bdeu},
    {{{{0x20u,8u,0x145be3u,0x145be8u},
       {0x08u,0u,0x145bedu,0x145bf2u},
       {0x14u,4u,0x145bfau,0x145bffu}}},{0x145bf6u,0x145c04u},0x145c08u},
};

template<class State,bool Observed> struct Rotation : Port<State> {
    static constexpr std::uint32_t frame_bytes=12u;
    Rotation(State &s,std::uint32_t *cursor,Trace<State> *trace):Port<State>(s,cursor,trace,{}){}

    void step(std::uint32_t pc) {
        if constexpr(Observed)this->s.instruction_literal(pc);
    }
    void copy_aliased_input() {
        auto &gp=this->s.gp;
        step(0x145b77u);
        gp[esp]-=4u;
        this->store(0x145b77u,gp[esp],4,gp[esi]);
        step(0x145b78u);
        gp[esi]=this->read(0x145b78u,gp[ecx],4);
        step(0x145b7au);
        this->store(0x145b7au,gp[esp]+4u,4,gp[esi]);
        step(0x145b7eu);
        gp[esi]=this->read(0x145b7eu,gp[ecx]+4u,4);
        step(0x145b81u);
        gp[ecx]=this->read(0x145b81u,gp[ecx]+8u,4);
        step(0x145b84u);
        this->store(0x145b84u,gp[esp]+8u,4,gp[esi]);
        step(0x145b88u);
        this->store(0x145b88u,gp[esp]+12u,4,gp[ecx]);
        step(0x145b8cu);
        gp[ecx]=gp[esp]+4u;
        step(0x145b90u);
        gp[esi]=this->read(0x145b90u,gp[esp],4);
        gp[esp]+=4u;
    }
    void product(__m128 &target,const Term &term) {
        const auto &gp=this->s.gp;
        step(term.coefficient_pc);
        target=_mm_set_ss(native_tu15_leaves::as_float(this->read(term.coefficient_pc,gp[edx]+term.coefficient_offset,4)));
        step(term.component_pc);
        const auto component=this->read(term.component_pc,gp[ecx]+term.vector_offset,4);
        target=ordered_mul_ss(target,_mm_set_ss(native_tu15_leaves::as_float(component)));
    }
    void run() {
        auto &gp=this->s.gp;
        auto &flags=this->s.flags;
        auto &xmm=this->s.xmm;
        step(0x145b70u);
        gp[esp]=flags.minus(gp[esp],frame_bytes,4);
        step(0x145b73u);
        flags.minus(gp[ecx],gp[eax],4);
        step(0x145b75u);
        this->reached(flags.zf?in_place:separate);
        if(flags.zf)copy_aliased_input();

        std::uint32_t output_offset{};
        for(const auto &axis:axes) {
            product(xmm[0],axis.terms[0]);
            for(unsigned i=1;i<axis.terms.size();++i) {
                product(xmm[1],axis.terms[i]);
                step(axis.sum_pcs[i-1]);
                xmm[0]=ordered_add_ss(xmm[0],xmm[1]);
            }
            step(axis.output_pc);
            this->store(axis.output_pc,gp[eax]+output_offset,4,native_tu15_leaves::low_bits(xmm[0]));
            output_offset+=4u;
        }
        step(0x145c0du);
        gp[esp]=flags.plus(gp[esp],frame_bytes,4);
        step(0x145c10u);
        (void)this->read(0x145c10u,gp[esp],4);
        gp[esp]+=4u;
    }
};

template<bool Observed,class State,class Reference>
void rotate(State &s,std::uint32_t *cursor,bool verify,Reference reference) {
    auto &tally=summary();
    native_object_tick_detail::execute(s,cursor,verify,coverage(),tally.calls,tally,reference,
        [](State &v,std::uint32_t *pc,Trace<State> *trace){return Rotation<State,Observed>{v,pc,trace};});
    if(verify)tally.hit=coverage().load(std::memory_order_relaxed);
}
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    using native_small_queries_detail::flag;
    static const bool reference_mode=flag("HALO2_REFERENCE_ROTATE_VECTOR");
    static const bool verify_mode=flag("HALO2_VERIFY_ROTATE_VECTOR");
    if(reference_mode || s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    // Both routes use live GP and XMM registers. When the caller observes
    // every instruction, keep delivery before each operation so that a
    // private particle bank sees the exact PC at its access boundary.
    std::uint32_t *cursor;
    if(native_object_tick_detail::admitted(s,0x145b70u,0x145c10u,cursor,false))
        rotate<false>(s,cursor,verify_mode,reference);
    else
        rotate<true>(s,nullptr,false,reference);
    return true;
}
}
