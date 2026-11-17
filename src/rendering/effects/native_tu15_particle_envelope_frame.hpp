#pragma once
#include "halo2/native_tu15_particle_envelope_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_particle_envelope_detail {
template<class State,class Before> class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::pop;
    static __m128 scalar(std::uint32_t bits) {return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
    __m128 read(std::uint32_t pc,std::uint32_t address) {at(pc);return scalar(word(address));}
    void below_or_equal(__m128 left,__m128 right) {s_.flags.scalar_compare(left,right,false);}
    bool taken() const {return s_.flags.cf||s_.flags.zf;}
    void leave(std::uint32_t pc) {pop(pc);}
public:
    Frame(State& state,Before& observer,native_raw_x87_detail::Scope<State>& fp)
        :Base{state,observer,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}} {}
    bool override_set() {
        auto& g=s_.gp;
        at(0x252ec0u);g[1]=word(0x51df50u);
        s_.flags.logic(g[1]&g[1],4);
        s_.xmm[0]=_mm_setzero_ps();
        if(s_.flags.zf)return false;
        at(0x252ecdu);s_.set(2,word(g[1]+5u,1),1,0);
        s_.flags.logic(s_.reg(2,1,0),1);
        return !s_.flags.zf;
    }
    bool after_start() {
        below_or_equal(s_.xmm[1],read(0x252ed4u,s_.gp[0]+0x18u));return !taken();
    }
    bool before_end() {
        s_.xmm[2]=read(0x252edau,s_.gp[0]+0x24u);
        below_or_equal(s_.xmm[2],s_.xmm[1]);return !taken();
    }
    bool in_attack() {
        auto& x=s_.xmm;
        x[0]=read(0x252ee4u,s_.gp[0]+0x1cu);
        x[0]=add(x[0],read(0x252ee9u,s_.gp[0]+0x18u));
        below_or_equal(x[0],x[1]);return !taken();
    }
    void return_attack() {
        auto& x=s_.xmm;
        x[1]=subtract(x[1],read(0x252ef3u,s_.gp[0]+0x18u));
        x[1]=multiply(x[1],read(0x252ef8u,s_.gp[0]+0x20u));
        x[0]=x[1];leave(0x252f00u);
    }
    bool in_release() {
        auto& x=s_.xmm;
        x[0]=read(0x252f01u,s_.gp[0]+0x24u);
        x[0]=subtract(x[0],read(0x252f06u,s_.gp[0]+0x28u));
        below_or_equal(x[1],x[0]);return !taken();
    }
    void return_release() {
        auto& x=s_.xmm;
        x[0]=read(0x252f10u,s_.gp[0]+0x24u);
        x[0]=subtract(x[0],x[1]);
        x[0]=multiply(x[0],read(0x252f19u,s_.gp[0]+0x2cu));
        leave(0x252f1eu);
    }
    void return_one() {s_.xmm[0]=read(0x252f1fu,0x46a0d0u);leave(0x252f27u);}
    void return_zero() {leave(0x252f27u);}
};

template<class State,class Before> void run_private(State& state,Before before) {
    native_raw_x87_detail::Scope<State> fp{state};
    Frame<State,Before> frame{state,before,fp};envelope(frame);
}
} // namespace halo2::native_particle_envelope_detail
#endif
