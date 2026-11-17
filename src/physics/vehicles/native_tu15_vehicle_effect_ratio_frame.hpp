#pragma once

#include "halo2/native_tu15_vehicle_effect_ratio_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <bit>

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_integer_fp_scope.hpp"
#include "physics/runtime/native_tu15_mopp_aabb_traversal.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"

namespace halo2::native_vehicle_effect_ratio_detail {

struct Noop { template<class State> void operator()(State&,std::uint32_t)const{} };
template<class Scope> struct Io {
    Scope* scope;
    template<class Callback> void operator()(Callback&& callback)const {
        scope->io(std::forward<Callback>(callback));
    }
};

template<class State,class Before> class Frame :
    public native_mopp_ray_caller_detail::Frame<State,Before,Io<native_integer_fp_detail::Scope<State>>> {
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;

    std::uint32_t read(std::uint32_t pc,std::uint32_t address) {
        at(pc);
        return word(address);
    }
    __m128 scalar(std::uint32_t pc,std::uint32_t address) {
        return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(read(pc,address))));
    }
    static __m128 divide(__m128 left,__m128 right) {
        asm volatile("divss %1,%0":"+x"(left):"x"(right):"memory");
        return left;
    }
    static __m128 subtract(__m128 left,__m128 right) {
        asm volatile("subss %1,%0":"+x"(left):"x"(right):"memory");
        return left;
    }
    bool above()const {return !s_.flags.cf&&!s_.flags.zf;}
public:
    Frame(State& s,Before& before,Scope& scope):Base{s,before,{&scope}}{}

    void acquire_body() {
        auto& g=s_.gp;
        g[1]=read(0xd3750u,0x4ecbd0u);
        g[2]=read(0xd3756u,g[1]+0x44u);
        at(0xd3759u);s_.xmm[0]=_mm_setzero_ps();
        at(0xd375cu);g[0]=s_.flags.logic(g[0]&0xffffu,4);
        at(0xd3761u);g[0]*=3u;
        g[1]=read(0xd3764u,g[2]+g[0]*4u+8u);
    }
    bool has_elapsed_time() {
        s_.xmm[1]=scalar(0xd3768u,s_.gp[1]+0x334u);
        at(0xd3770u);s_.flags.scalar_compare(s_.xmm[1],s_.xmm[0],false);
        at(0xd3773u);return above();
    }
    void acquire_effect_definition() {
        auto& g=s_.gp;
        g[0]=read(0xd3775u,g[1]);
        g[2]=read(0xd3777u,0x4f0414u);
        s_.xmm[1]=scalar(0xd377du,g[1]+0x338u);
        at(0xd3785u);g[0]=s_.flags.logic(g[0]&0xffffu,4);
        at(0xd378au);g[0]=native_mopp_aabb_abi_detail::shifted_immediate<4>(s_,g[0]);
        g[2]=read(0xd378du,g[0]+g[2]+8u);
    }
    bool starts_at_zero() {
        at(0xd3791u);s_.flags.scalar_compare(s_.xmm[1],s_.xmm[0],true);
        at(0xd3794u);s_.set(0,s_.flags.bits()|2u,1,8);
        at(0xd3795u);s_.flags.logic(s_.reg(0,1,8)&0x44u,1);
        at(0xd3798u);return !s_.flags.pf;
    }
    bool within_initial_interval() {
        s_.xmm[0]=scalar(0xd379au,s_.gp[2]+0x1d4u);
        const auto elapsed=scalar(0xd37a2u,s_.gp[1]+0x334u);
        s_.flags.scalar_compare(s_.xmm[0],elapsed,false);
        at(0xd37a9u);return above();
    }
    void initial_fraction() {
        s_.xmm[0]=scalar(0xd37abu,s_.gp[1]+0x334u);
        const auto limit=scalar(0xd37b3u,s_.gp[2]+0x1d4u);
        s_.xmm[0]=divide(s_.xmm[0],limit);
    }
    void saturate() {s_.xmm[0]=scalar(0xd37bcu,0x46a0d0u);}
    bool past_baseline() {
        s_.xmm[1]=scalar(0xd37c5u,s_.gp[1]+0x334u);
        const auto baseline=scalar(0xd37cdu,s_.gp[1]+0x338u);
        s_.flags.scalar_compare(s_.xmm[1],baseline,false);
        at(0xd37d4u);return above();
    }
    void baseline_fraction() {
        at(0xd37d6u);s_.xmm[0]=s_.xmm[1];
        const auto baseline=scalar(0xd37d9u,s_.gp[1]+0x338u);
        s_.xmm[0]=subtract(s_.xmm[0],baseline);
        const auto duration=scalar(0xd37e1u,s_.gp[2]+0x1dcu);
        s_.xmm[0]=divide(s_.xmm[0],duration);
    }
    void finish(Exit exit) {
        (void)Base::pop(exit==Exit::elapsed_fraction?0xd37bbu:exit==Exit::saturated?0xd37c4u:0xd37e9u);
    }
};

// Raw TU1.5 proof only: providers and Before do not mutate retained GP, SIMD
// or cursor policy. The logical x87/MMX file stays in State; scalar SSE flags
// and exceptions belong to this source frame. No generated admission here.
template<class State,class Before=Noop>void run_private(State& state,Before before={}) {
    native_integer_fp_detail::Scope scope{state};
    Frame view{state,before,scope};
    evaluate(view);
}

} // namespace halo2::native_vehicle_effect_ratio_detail
#endif
