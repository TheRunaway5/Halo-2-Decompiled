#pragma once
#include "halo2/native_tu15_object_table_ratio_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include <bit>

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_object_table_ratio_detail {
template<class State,class Before> class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::pop;

    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width=4) {
        at(pc);return word(address,width);
    }
    __m128 scalar(std::uint32_t pc,std::uint32_t address) {
        return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(read(pc,address))));
    }
    static std::uint32_t signed_word(std::uint32_t value) {return (value^0x8000u)-0x8000u;}
    static __m128 convert(__m128 previous,std::uint32_t value) {
        return _mm_cvtsi32_ss(previous,std::bit_cast<std::int32_t>(value));
    }
public:
    Frame(State& state,Before& before,native_raw_x87_detail::Scope<State>& fp)
        :Base{state,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}} {}

    void open_object_row() {
        auto& g=s_.gp;
        g[1]=read(0x24b430u,0x4ecbd0u);
        g[2]=read(0x24b436u,g[1]+0x44u);
        s_.xmm[0]=_mm_setzero_ps();
        g[0]=s_.flags.logic(g[0]&0xffffu,4);
        g[0]+=g[0]*2u;
        g[0]=read(0x24b444u,g[2]+g[0]*4u+8u);
        g[2]=read(0x24b448u,0x4f3218u);
        g[1]=signed_word(read(0x24b44eu,g[0]+0x17eu,2));
        s_.set(0,read(0x24b455u,g[2]+0x244u,2),2,0);
        const auto ax=s_.reg(0,2,0);
        s_.flags.logic(ax,2);
        g[0]=s_.flags.zf?1u:signed_word(ax);
    }
    bool has_index() {
        s_.flags.minus(s_.gp[1],0xffffffffu,4);
        s_.xmm[1]=convert(s_.xmm[1],s_.gp[0]);
        return !s_.flags.zf;
    }
    bool has_constant_flag() {
        auto& g=s_.gp;
        g[0]=read(0x24b474u,0x52d710u);
        s_.flags.logic(read(0x24b479u,g[0]+g[1]+0x170u,1)&8u,1);
        return !s_.flags.zf;
    }
    void load_constant() {s_.xmm[0]=scalar(0x24b483u,0x46a0d0u);}
    void scale_positive_count() {
        auto& g=s_.gp;
        s_.set(1,read(0x24b48cu,g[0]+g[1]*2u+0x1a0u,2),2,0);
        const auto cx=s_.reg(1,2,0);
        s_.flags.logic(cx,2);
        if(!s_.flags.zf&&s_.flags.sf==s_.flags.of)g[0]=signed_word(cx);
        else g[0]=s_.flags.logic(0u,4);
        g[1]=read(0x24b4a0u,0x51df54u);
        s_.xmm[0]=scalar(0x24b4a6u,g[1]+4u);
        asm volatile("divss %1,%0":"+x"(s_.xmm[0]):"x"(s_.xmm[1]));
        s_.xmm[1]=convert(s_.xmm[1],g[0]);
        asm volatile("mulss %1,%0":"+x"(s_.xmm[0]):"x"(s_.xmm[1]));
    }
    __m128 finish_constant() {pop(0x24b48bu);return s_.xmm[0];}
    __m128 finish() {pop(0x24b4b7u);return s_.xmm[0];}
};

template<class State,class Before> __m128 run_private(State& state,Before before) {
    native_raw_x87_detail::Scope<State> fp{state};
    Frame<State,Before> view{state,before,fp};
    return query(view);
}
} // namespace halo2::native_object_table_ratio_detail
#endif
