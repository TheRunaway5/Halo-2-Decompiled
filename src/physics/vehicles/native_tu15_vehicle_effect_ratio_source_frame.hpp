#pragma once

#include "halo2/native_tu15_vehicle_effect_ratio_kernel.hpp"
#include "physics/runtime/physics_step_internal.hpp"
#include "physics/vehicles/native_tu15_vehicle_effect_ratio_ordered.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_vehicle_effect_ratio_detail {

// The generated VehicleUpdate source uses canonical sites even on TU1.5.
// Deliver each authored callback through the actual live service (including
// Phantom's remap) before acquiring the register or memory operand. The
// original provider owns the host FP transport on this separate source path.
template<bool Literal,bool ObservedShift,class State> class SourceFrame {
    State& s_;

    void at(std::uint32_t pc) {
        if constexpr(Literal)s_.instruction_literal(pc);
        else if(s_.services.instruction)s_.services.instruction(pc);
    }
    std::uint32_t word(std::uint32_t address) {
        return physics_step_detail::r(s_.memory,address,4);
    }
    __m128 scalar(std::uint32_t address) {
        return _mm_set_ss(physics_step_detail::f(word(address)));
    }
    bool above() const {return !s_.flags.cf&&!s_.flags.zf;}
public:
    explicit SourceFrame(State& state):s_(state){}

    void acquire_body() {
        auto& g=s_.gp;
        at(0xd26c0u);g[1]=word(0x4ecbd0u);
        at(0xd26c6u);g[2]=word(g[1]+0x44u);
        at(0xd26c9u);s_.xmm[0]=_mm_xor_ps(s_.xmm[0],s_.xmm[0]);
        at(0xd26ccu);g[0]=s_.flags.logic(g[0]&0xffffu,4);
        at(0xd26d1u);g[0]*=3u;
        at(0xd26d4u);g[1]=word(g[2]+g[0]*4u+8u);
    }
    bool has_elapsed_time() {
        at(0xd26d8u);s_.xmm[1]=scalar(s_.gp[1]+0x334u);
        at(0xd26e0u);s_.flags.scalar_compare(s_.xmm[1],s_.xmm[0],false);
        at(0xd26e3u);return above();
    }
    void acquire_effect_definition() {
        auto& g=s_.gp;
        at(0xd26e5u);g[0]=word(g[1]);
        at(0xd26e7u);g[2]=word(0x4f0414u);
        at(0xd26edu);s_.xmm[1]=scalar(g[1]+0x338u);
        at(0xd26f5u);g[0]=s_.flags.logic(g[0]&0xffffu,4);
        at(0xd26fau);
        if constexpr(ObservedShift)g[0]=native_vehicle_effect_ratio_ordered_detail::shift_index(s_,g[0]);
        else g[0]=s_.flags.shift(g[0],4u,4);
        at(0xd26fdu);g[2]=word(g[0]+g[2]+8u);
    }
    bool starts_at_zero() {
        at(0xd2701u);s_.flags.scalar_compare(s_.xmm[1],s_.xmm[0],true);
        at(0xd2704u);s_.set(0,s_.flags.bits()|2u,1,8);
        at(0xd2705u);s_.flags.logic(s_.reg(0,1,8)&0x44u,1);
        at(0xd2708u);return !s_.flags.pf;
    }
    bool within_initial_interval() {
        at(0xd270au);s_.xmm[0]=scalar(s_.gp[2]+0x1d4u);
        at(0xd2712u);s_.flags.scalar_compare(s_.xmm[0],scalar(s_.gp[1]+0x334u),false);
        at(0xd2719u);return above();
    }
    void initial_fraction() {
        at(0xd271bu);s_.xmm[0]=scalar(s_.gp[1]+0x334u);
        at(0xd2723u);s_.xmm[0]=_mm_div_ss(s_.xmm[0],scalar(s_.gp[2]+0x1d4u));
    }
    void saturate() {
        at(0xd272cu);s_.xmm[0]=scalar(0x46a0d0u);
    }
    bool past_baseline() {
        at(0xd2735u);s_.xmm[1]=scalar(s_.gp[1]+0x334u);
        at(0xd273du);s_.flags.scalar_compare(s_.xmm[1],scalar(s_.gp[1]+0x338u),false);
        at(0xd2744u);return above();
    }
    void baseline_fraction() {
        at(0xd2746u);s_.xmm[0]=s_.xmm[1];
        at(0xd2749u);s_.xmm[0]=_mm_sub_ss(s_.xmm[0],scalar(s_.gp[1]+0x338u));
        at(0xd2751u);s_.xmm[0]=_mm_div_ss(s_.xmm[0],scalar(s_.gp[2]+0x1dcu));
    }
    void finish(Exit exit) {
        switch(exit) {
        case Exit::common:at(0xd2759u);break;
        case Exit::elapsed_fraction:at(0xd272bu);break;
        case Exit::saturated:at(0xd2734u);break;
        }
        (void)s_.pop();
    }
};

// Unbound source candidate for an authenticated TU1.5 VehicleUpdate owner.
// No generated default or mapping/image admission is provided by this view.
template<bool Literal,bool ObservedShift=false,class State> void run_source_private(State& state) {
    SourceFrame<Literal,ObservedShift,State> view{state};
    evaluate(view);
}

} // namespace halo2::native_vehicle_effect_ratio_detail
#endif
