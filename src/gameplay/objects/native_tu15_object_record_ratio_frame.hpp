#pragma once
#include "halo2/native_tu15_object_record_ratio_kernel.hpp"
#include "gameplay/objects/native_tu15_object_record_ratio_ordered.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include <bit>

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_object_record_ratio_detail {
template<class State,class Before> class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::pop;using Base::push;

    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width=4) {
        at(pc);return word(address,width);
    }
    template<unsigned Count> std::uint32_t shift_eax(std::uint32_t value) {
        return observed_shift_eax<Count>(s_,value);
    }
    __m128 scalar(std::uint32_t pc,std::uint32_t address) {
        return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(read(pc,address))));
    }
    __m128 result(std::uint32_t pop_pc,std::uint32_t ret_pc) {
        s_.gp[6]=pop(pop_pc);pop(ret_pc);s_.gp[4]+=4u;
        return s_.xmm[0];
    }
public:
    Frame(State& state,Before& before,native_raw_x87_detail::Scope<State>& fp)
        :Base{state,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}} {}

    void open_record_table() {
        auto& g=s_.gp;
        g[1]=read(0x107140u,0x4ecbd0u);
        g[2]=read(0x107146u,g[1]+0x44u);
        s_.xmm[0]=_mm_setzero_ps();
        g[0]=s_.flags.logic(g[0]&0xffffu,4);
        g[0]+=g[0]*2u;
        g[1]=read(0x107154u,g[2]+g[0]*4u+8u);
        g[0]=read(0x107158u,g[1]);
        g[2]=read(0x10715au,0x4f0414u);
        g[0]=shift_eax<4>(s_.flags.logic(g[0]&0xffffu,4));
        g[2]=read(0x107168u,g[2]+g[0]+8u);
        g[0]=sign_extend(read(0x10716cu,g[4]+4u,2),16u);
        push(0x107171u,g[6]);
    }
    bool index_below_count() {
        auto& g=s_.gp;
        s_.flags.minus(read(0x107172u,g[2]+0x2c8u),g[0],4);
        return !s_.flags.zf&&s_.flags.sf==s_.flags.of;
    }
    Record record_kind() {
        auto& g=s_.gp;
        g[6]=g[0]+g[0]*2u;
        g[1]+=g[6]*4u+0x20cu;
        g[6]=sign_extend(read(0x107184u,g[1],1),8u);
        g[6]=s_.flags.increment(g[6],4,true);
        if(s_.flags.zf)return Record::ratio;
        g[6]=s_.flags.increment(g[6],4,true);
        return s_.flags.zf?Record::constant:Record::other;
    }
    void load_constant() {s_.xmm[0]=scalar(0x10718du,0x46a0d0u);}
    void subtract_scaled_ratio() {
        auto& g=s_.gp;
        g[1]=sign_extend(read(0x107199u,g[1]+2u,2),16u);
        g[2]=read(0x10719du,g[2]+0x2ccu);
        s_.xmm[0]=scalar(0x1071a3u,0x46a0d0u);
        g[0]=shift_eax<6>(g[0]);
        s_.xmm[1]=_mm_cvtsi32_ss(s_.xmm[1],std::bit_cast<std::int32_t>(g[1]));
        const auto denominator=scalar(0x1071b2u,g[0]+g[2]+0x1cu);
        asm volatile("divss %1,%0":"+x"(s_.xmm[1]):"x"(denominator));
        g[0]=read(0x1071b8u,0x51df54u);
        const auto scale=scalar(0x1071bdu,g[0]+4u);
        asm volatile("mulss %1,%0":"+x"(s_.xmm[1]):"x"(scale));
        asm volatile("subss %1,%0":"+x"(s_.xmm[0]):"x"(s_.xmm[1]));
    }
    __m128 finish_constant() {return result(0x107195u,0x107196u);}
    __m128 finish() {return result(0x1071c6u,0x1071c7u);}
private:
    static std::uint32_t sign_extend(std::uint32_t value,unsigned bits) {
        const auto sign=1u<<(bits-1u);
        return (value^sign)-sign;
    }
};

template<class State,class Before> __m128 run_private(State& state,Before before) {
    native_raw_x87_detail::Scope<State> fp{state};
    Frame<State,Before> view{state,before,fp};
    return query(view);
}
} // namespace halo2::native_object_record_ratio_detail
#endif
