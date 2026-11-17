#pragma once
#include "halo2/native_tu15_codec_row_copy_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_codec_rotation_interpolation_frame.hpp"
#include "physics/runtime/native_tu15_mopp_aabb_traversal.hpp"

namespace halo2::native_codec_row_copy_detail {
using Noop=native_codec_rotation_interpolation_detail::Noop;
template<Row Profile,class State,class Before>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,
        native_codec_rotation_interpolation_detail::Io<native_integer_fp_detail::Scope<State>>>{
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,
        native_codec_rotation_interpolation_detail::Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;
    static constexpr bool quaternion=Profile==Row::quaternion;
    static constexpr unsigned count=quaternion?4u:3u;
    static constexpr auto reads=quaternion?std::array{0x2963f5u,0x2963f9u,0x2963ffu,0x296405u}:std::array{0x29642eu,0x29643eu,0x296444u,0u};
    static constexpr auto writes=quaternion?std::array{0x2963f7u,0x2963fcu,0x296402u,0x296408u}:std::array{0x29643cu,0x296441u,0x296447u,0u};
    unsigned ordinal_{};
public:
    Frame(State&s,Before&before,Scope&scope):Base{s,before,{&scope}}{}
    NativeTu15WordCopy acquire(){
        auto&g=s_.gp;
        at(quaternion?0x2963d0u:0x296410u);g[1]=word(0x510f60u);
        at(quaternion?0x2963d6u:0x296416u);g[2]=word(g[1]+(quaternion?20u:24u));
        at(quaternion?0x2963d9u:0x296419u);const auto index=word(quaternion?0x510f94u:0x510f98u);
        g[2]=native_codec_rotation_interpolation_detail::product(s_,g[2],index);
        at(quaternion?0x2963e0u:0x296420u);g[0]=word(0x510f44u);
        if constexpr(quaternion){
            g[2]=s_.flags.plus(g[2],g[1],4);
            at(0x2963e7u);g[1]=word(0x510fa0u);
            g[0]=s_.flags.plus(g[0],2u,4);
            g[0]=native_mopp_aabb_abi_detail::shifted_immediate<4>(s_,g[0]);
            g[0]=s_.flags.plus(g[0],g[2],4);
        }else{
            g[0]=g[2]+(g[0]+g[0]*2u)*4u;
            at(0x29642bu);const auto offset=word(g[1]+12u);g[0]=s_.flags.plus(g[0],offset,4);
            at(0x29642eu);g[2]=word(g[0]+g[1]);
            g[0]=s_.flags.plus(g[0],g[1],4);
            at(0x296433u);g[1]=word(0x510fa0u);
            g[1]=s_.flags.plus(g[1],16u,4);
        }
        return{g[0],g[1],count,0u};
    }
    std::uint32_t read_word(std::uint32_t address){
        // Translation's first input was loaded before the output pointer.
        if constexpr(!quaternion)if(!ordinal_)return s_.gp[2];
        at(reads[ordinal_]);return s_.gp[ordinal_+1u==count?0u:2u]=word(address);
    }
    void write_word(std::uint32_t address,std::uint32_t value){Base::store(writes[ordinal_],address,value,[]{});}
    void advanced(const NativeTu15WordCopy&){++ordinal_;}
    void finish(){(void)Base::pop(quaternion?0x29640bu:0x29644au);}
};
// Private physical frame: GP/XMM/Before providers are readonly; no source
// continuation, production binding or older-image admission is implied.
template<Row Profile,class State,class Before=Noop>void run_private(State&s,Before before={}){
    native_integer_fp_detail::Scope scope{s};Frame<Profile,State,Before>view{s,before,scope};native_codec_row_copy_detail::decode(view);
}
} // namespace halo2::native_codec_row_copy_detail
#endif
