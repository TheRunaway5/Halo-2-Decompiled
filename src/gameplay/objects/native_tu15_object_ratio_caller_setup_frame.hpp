#pragma once
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include <bit>

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_object_ratio_caller_detail {
// Original 1046C0 through the first function-key comparison. The continuation
// owns that comparison and all subsequent branches; this frame does not run it.
template<class State,class Before> class Setup:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;

    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width=4) {
        at(pc);return word(address,width);
    }
    std::uint32_t definition_index(std::uint32_t value) {
        // Original 1046FD encodes SHL ECX,4 and exposes the observed flags
        // at the following comparison, before any branch can replace them.
        std::uint32_t result;
        asm volatile("shll $4,%%ecx; movl %%ecx,%%edx; seto %%cl; lahf; movb %%ah,%%ch"
            :"+c"(value),"=d"(result)::"eax","cc");
        s_.flags.load(((value>>8u)&0xd5u)|((value&1u)<<11u));
        return result;
    }
public:
    Setup(State& state,Before& before,native_raw_x87_detail::Scope<State>& fp)
        :Base{state,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}} {}

    void open_object() {
        auto& g=s_.gp;
        push(0x1046c0u,g[1]);
        g[1]=read(0x1046c1u,0x4ecbd0u);
        g[2]=read(0x1046c7u,g[1]+0x44u);
        s_.xmm[3]=_mm_setzero_ps();
        s_.xmm[4]=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(read(0x1046cdu,0x46a0d0u))));
        push(0x1046d5u,g[3]);push(0x1046d6u,g[5]);push(0x1046d7u,g[6]);
        g[6]=read(0x1046d8u,g[4]+0x18u);
        push(0x1046dcu,g[7]);
        g[7]=read(0x1046ddu,g[4]+0x18u);
        g[0]=s_.flags.logic(g[7]&0xffffu,4);
        g[0]+=g[0]*2u;
        g[5]=read(0x1046ebu,g[2]+g[0]*4u+8u);
        g[1]=read(0x1046efu,g[5]);
        g[0]=read(0x1046f2u,0x4f0414u);
        g[1]=definition_index(s_.flags.logic(g[1]&0xffffu,4));
        at(0x104700u);
    }
};

template<class State,class Before> void run_setup(State& state,Before before) {
    native_raw_x87_detail::Scope<State> fp{state};
    Setup<State,Before> view{state,before,fp};view.open_object();
}
} // namespace halo2::native_object_ratio_caller_detail
#endif
