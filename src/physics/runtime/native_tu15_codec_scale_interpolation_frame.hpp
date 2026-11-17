#pragma once
#include "halo2/native_tu15_codec_scale_interpolation_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_codec_rotation_interpolation_frame.hpp"

namespace halo2::native_codec_scale_interpolation_detail {
using Noop=native_codec_rotation_interpolation_detail::Noop;
template<class State,class Before>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,
        native_codec_rotation_interpolation_detail::Io<native_integer_fp_detail::Scope<State>>>{
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,
        native_codec_rotation_interpolation_detail::Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;
    static __m128 scalar(std::uint32_t value){return std::bit_cast<__m128>(std::array<std::uint32_t,4>{value,0,0,0});}
    static std::uint32_t bits(__m128 value){return std::bit_cast<std::array<std::uint32_t,4>>(value)[0];}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value){Base::store(pc,address,value,[]{});}
public:
    struct Input{__m128 first,second;};
    Frame(State&s,Before&before,Scope&scope):Base{s,before,{&scope}}{}
    Input input(){
        auto&g=s_.gp;g[4]=s_.flags.minus(g[4],8u,4);
        at(0x296623u);g[1]=word(0x510f60u);
        at(0x296629u);g[0]=word(g[1]+28u);
        at(0x29662cu);const auto index=word(0x510f9cu);
        g[0]=native_codec_rotation_interpolation_detail::product(s_,g[0],index);
        at(0x296633u);const auto offset=word(g[1]+16u);g[0]=s_.flags.plus(g[0],offset,4);
        g[0]=s_.flags.plus(g[0],g[1],4);
        at(0x296638u);g[1]=word(0x510f44u);
        at(0x29663eu);g[2]=word(g[0]+g[1]*4u);
        at(0x296641u);g[1]=word(0x510f48u);
        store(0x296647u,g[4]+4u,g[2]);
        at(0x29664bu);g[2]=word(g[0]+g[1]*4u);
        at(0x29664eu);s_.xmm[0]=scalar(word(g[4]+4u));
        at(0x296654u);g[0]=word(0x510fa0u);
        store(0x296659u,g[4],g[2]);
        at(0x29665cu);s_.xmm[1]=scalar(word(g[4]));
        return{s_.xmm[0],s_.xmm[1]};
    }
    __m128 subtract(__m128 second,__m128 first)const{
        asm volatile("subss %1,%0":"+x"(second):"x"(first):"memory");return second;
    }
    __m128 scale_difference(__m128 difference){
        s_.xmm[1]=difference;at(0x296665u);const auto factor=scalar(word(0x510f4cu));
        asm volatile("mulss %1,%0":"+x"(difference):"x"(factor):"memory");
        return difference;
    }
    __m128 add_first(__m128 product,__m128 first)const{
        asm volatile("addss %1,%0":"+x"(product):"x"(first):"memory");return product;
    }
    void publish(__m128 value){s_.xmm[1]=value;store(0x296671u,s_.gp[0]+28u,bits(value));}
    void finish(){s_.gp[4]=s_.flags.plus(s_.gp[4],8u,4);at(0x296679u);(void)word(s_.gp[4]);s_.gp[4]+=4u;}
};
// Private physical-publication candidate; no generated admission or binding.
template<class State,class Before=Noop>void run_private(State&s,Before before={}){
    native_integer_fp_detail::Scope scope{s};Frame view{s,before,scope};native_codec_scale_interpolation_detail::decode(view);
}
} // namespace halo2::native_codec_scale_interpolation_detail
#endif
