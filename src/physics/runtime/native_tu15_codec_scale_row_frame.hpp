#pragma once
#include "halo2/native_tu15_codec_scale_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_codec_rotation_interpolation_frame.hpp"

namespace halo2::native_codec_scale_row_detail {
using Noop=native_codec_rotation_interpolation_detail::Noop;
template<class State,class Before>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,
        native_codec_rotation_interpolation_detail::Io<native_integer_fp_detail::Scope<State>>>{
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,
        native_codec_rotation_interpolation_detail::Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;
public:
    Frame(State&s,Before&before,Scope&scope):Base{s,before,{&scope}}{}
    NativeTu15WordCopy scale(){
        auto&g=s_.gp;at(0x2cec60u);g[0]=word(0x510f60u);
        at(0x2cec65u);g[1]=word(g[0]+28u);
        at(0x2cec68u);const auto index=word(0x510f9cu);
        g[1]=native_codec_rotation_interpolation_detail::product(s_,g[1],index);
        at(0x2cec6fu);g[2]=word(0x510f44u);g[1]+=g[2]*4u;
        at(0x2cec78u);const auto offset=word(g[0]+16u);g[1]=s_.flags.plus(g[1],offset,4);
        return{g[1]+g[0],0u,1u,0u};
    }
    std::uint32_t read_word(std::uint32_t address){at(0x2cec7bu);return s_.gp[2]=word(address);}
    void write_word(std::uint32_t,std::uint32_t value){
        at(0x2cec7eu);s_.gp[0]=word(0x510fa0u);
        Base::store(0x2cec83u,s_.gp[0]+28u,value,[]{});
    }
    void advanced(const NativeTu15WordCopy&){}
    void finish(){(void)Base::pop(0x2cec86u);}
};
// Private physical view of actual2CEC60. The integer copy keeps FP/MMX opaque.
template<class State,class Before=Noop>void run_private(State&s,Before before={}){
    native_integer_fp_detail::Scope scope{s};Frame view{s,before,scope};native_codec_scale_detail::decode(view);
}
} // namespace halo2::native_codec_scale_row_detail
#endif
