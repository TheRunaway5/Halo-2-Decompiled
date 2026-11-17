#pragma once
#include "halo2/native_tu15_codec_transform_packed_kernel.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_codec_transform_composition_frame.hpp"
#include "physics/runtime/native_tu15_mopp_ray_integer_frame.hpp"

namespace halo2::native_codec_transform_packed_detail {
using native_codec_quaternion_product_detail::Io;
using native_codec_quaternion_product_detail::Noop;

template<class State,class Before>class Frame:
    public native_mopp_ray_caller_detail::IntegerFrame<State,Before,Io<native_integer_fp_detail::Scope<State>>> {
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;using Base::before_;using Base::io;
    Scope& scope_;
    std::uint32_t read(std::uint32_t pc,std::uint32_t address) {
        at(pc);return word(address);
    }
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
        Base::store(pc,address,value,[]{});
    }
    static __m128 scalar(std::uint32_t value) {
        return std::bit_cast<__m128>(std::array<std::uint32_t,4>{value,0,0,0});
    }
public:
    Frame(State& state,Before& before,Scope& scope):Base{state,before,{&scope}},scope_(scope){}
    __m128 packed_quaternion() {
        auto& g=s_.gp;auto& x=s_.xmm;
        g[2]=read(0x283ee9u,g[4]+28u);g[2]=s_.flags.plus(g[2],4u,4u);g[0]=g[4]+48u;
        store(0x283ef4u,g[4]+44u,g[2]);store(0x283ef8u,g[4]+40u,g[0]);
        g[1]=read(0x283efcu,g[4]+44u);g[0]=read(0x283f00u,g[4]+40u);
        at(0x283f04u);std::array<std::int16_t,4> packed{};
        io([&]{s_.memory.read(g[1],std::as_writable_bytes(std::span(packed)));});
        // PUNPCK/PSRAD leaves the four signed source words; conversion is
        // exact for all int16 values. Keep the two old upper XMM pairs
        // through the actual EMMS retirement, before MOVLHPS replaces XMM1.
        const auto converted=_mm_cvtepi32_ps(_mm_set_epi32(packed[3],packed[2],packed[1],packed[0]));
        x[1]=_mm_movelh_ps(converted,_mm_movehl_ps(x[1],x[1]));
        x[2]=_mm_movelh_ps(_mm_movehl_ps(converted,converted),_mm_movehl_ps(x[2],x[2]));
        s_.emms();s_.fp.top=0;
        at(0x283f1du);
        return x[1]=_mm_movelh_ps(x[1],x[2]);
    }
    static __m128 multiply(__m128 left,__m128 right) {
        asm volatile("mulps %1,%0":"+x"(left):"x"(right):"memory");return left;
    }
    static __m128 add(__m128 left,__m128 right) {
        asm volatile("addps %1,%0":"+x"(left):"x"(right):"memory");return left;
    }
    static __m128 opposite(__m128 value){return _mm_shuffle_ps(value,value,78);}
    static __m128 alternate(__m128 value){return _mm_shuffle_ps(value,value,17);}
    __m128 estimate(__m128 quaternion,const native_packed_rotation_detail::Norm<__m128>& norm) {
        auto& x=s_.xmm;x[0]=norm.value;x[1]=quaternion;x[3]=norm.opposite;x[4]=norm.alternate;
        auto value=x[0];
        asm volatile("rsqrtps %0,%0":"+x"(value)::"memory");
        return x[0]=value;
    }
    void store_quaternion(__m128 value) {
        s_.xmm[1]=value;Base::vector_store(0x283f40u,s_.gp[0],value);
    }
    void compose_transform() {
        auto& g=s_.gp;auto& x=s_.xmm;
        x[0]=scalar(read(0x283f43u,g[2]+20u));g[1]=g[2]+8u;
        g[0]=read(0x283f4bu,g[1]);store(0x283f4du,g[4]+64u,g[0]);
        g[0]=read(0x283f51u,g[1]+4u);g[1]=read(0x283f54u,g[1]+8u);
        g[2]=g[4]+48u;store(0x283f5bu,g[4]+68u,g[0]);Base::push(0x283f5fu,g[2]);
        g[0]=g[4]+116u;Base::push(0x283f64u,g[0]);g[2]=g[4]+152u;
        store(0x283f6cu,g[4]+80u,g[1]);
        store(0x283f70u,g[4]+84u,std::bit_cast<std::array<std::uint32_t,4>>(x[0])[0]);
        Base::push(0x283f76u,0x283f7bu);
        scope_.child([&]{native_codec_transform_composition_detail::run_private(s_,std::ref(before_));});
    }
};

// Enters from a genuine phase-one state at 283EE9 and returns before the
// first output ancestry read (283F7B). No result/bitplane store is performed.
template<class State,class Before=Noop>void run_private_suffix(State& state,Before before={}) {
    native_integer_fp_detail::Scope scope{state};Frame view{state,before,scope};compose_packed(view);
}
} // namespace halo2::native_codec_transform_packed_detail
#endif
