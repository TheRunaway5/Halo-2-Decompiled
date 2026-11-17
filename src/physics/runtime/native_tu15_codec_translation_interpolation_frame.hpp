#pragma once
#include "halo2/native_tu15_codec_translation_interpolation_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_codec_rotation_interpolation_frame.hpp"

namespace halo2::native_codec_translation_interpolation_detail {
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
    struct Input{__m128 first,second,factor;};
    Frame(State&s,Before&before,Scope&scope):Base{s,before,{&scope}}{}
    Input input(){
        auto&g=s_.gp;
        at(0x2962f0u);g[1]=word(0x510f60u);
        at(0x2962f6u);g[0]=word(g[1]+24u);
        at(0x2962f9u);g[2]=word(g[1]+12u);
        at(0x2962fcu);const auto index=word(0x510f98u);
        g[0]=native_codec_rotation_interpolation_detail::product(s_,g[0],index);
        at(0x296303u);s_.xmm[0]=scalar(word(0x510f4cu));
        g[0]=s_.flags.plus(g[0],g[2],4);g[0]=s_.flags.plus(g[0],g[1],4);
        at(0x29630fu);g[1]=word(0x510f44u);
        g[1]=g[1]+g[1]*2u;g[2]=g[0]+g[1]*4u;
        at(0x29631bu);g[1]=word(g[2]);store(0x29631du,0x56e6acu,g[1]);
        at(0x296323u);g[1]=word(g[2]+4u);
        at(0x296326u);s_.xmm[1]=scalar(word(0x56e6acu));
        store(0x29632eu,0x56e6b0u,g[1]);
        at(0x296334u);g[2]=word(g[2]+8u);
        at(0x296337u);g[1]=word(0x510f48u);
        store(0x29633du,0x56e6b4u,g[2]);
        g[1]=g[1]+g[1]*2u;g[2]=g[0]+g[1]*4u;
        at(0x296349u);g[0]=word(g[2]);store(0x29634bu,0x56e6a0u,g[0]);
        at(0x296350u);g[1]=word(g[2]+4u);
        at(0x296353u);s_.xmm[2]=scalar(word(0x56e6a0u));
        at(0x29635bu);g[0]=word(0x510fa0u);
        return {s_.xmm[1],s_.xmm[2],s_.xmm[0]};
    }
    __m128 interpolate(__m128 second,__m128 first,__m128 factor)const{
        asm volatile("subss %1,%0":"+x"(second):"x"(first):"memory");
        asm volatile("mulss %1,%0":"+x"(second):"x"(factor):"memory");
        asm volatile("addss %1,%0":"+x"(second):"x"(first):"memory");
        return second;
    }
    void publish_x(__m128 value){
        s_.xmm[2]=value;store(0x29636cu,0x56e6a4u,s_.gp[1]);
        at(0x296372u);s_.gp[2]=word(s_.gp[2]+8u);
        store(0x296375u,0x56e6a8u,s_.gp[2]);
        store(0x29637bu,s_.gp[0]+16u,bits(s_.xmm[2]));
    }
    __m128 first_y(){at(0x296380u);s_.xmm[1]=scalar(word(0x56e6b0u));return s_.xmm[1];}
    __m128 second_y(){at(0x296388u);s_.xmm[2]=scalar(word(0x56e6a4u));return s_.xmm[2];}
    void publish_y(__m128 value){s_.xmm[2]=value;store(0x29639cu,s_.gp[0]+20u,bits(value));}
    __m128 first_z(){at(0x2963a1u);s_.xmm[1]=scalar(word(0x56e6b4u));return s_.xmm[1];}
    __m128 second_z(){at(0x2963a9u);s_.xmm[2]=scalar(word(0x56e6a8u));return s_.xmm[2];}
    void publish_z(__m128 value){s_.xmm[2]=value;store(0x2963bdu,s_.gp[0]+24u,bits(value));}
    void finish(){at(0x2963c2u);(void)word(s_.gp[4]);s_.gp[4]+=4u;}
};
// Private physical-publication draft. No admission, source continuation,
// production binding or numerical qualification is supplied by this file.
template<class State,class Before=Noop>void run_private(State&s,Before before={}){
    native_integer_fp_detail::Scope scope{s};Frame view{s,before,scope};native_codec_translation_interpolation_detail::decode(view);
}
} // namespace halo2::native_codec_translation_interpolation_detail
#endif
