#pragma once
#include "halo2/native_tu15_codec_translation_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_integer_fp_scope.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"

namespace halo2::native_codec_translation_detail {
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
template<class Scope>struct Io{
    Scope*scope;
    template<class Callback>void operator()(Callback&&callback)const{scope->io(std::forward<Callback>(callback));}
};
template<class State,class Before>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,Io<native_integer_fp_detail::Scope<State>>>{
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;
    unsigned component_{};
public:
    Frame(State&s,Before&before,Scope&scope):Base{s,before,{&scope}}{}
    NativeTu15WordCopy translation(){
        auto&g=s_.gp;
        at(0x2961f0u);g[0]=word(0x510f98u);g[1]=g[0]+g[0]*2u;
        at(0x2961f8u);g[0]=word(0x510f60u);
        at(0x2961fdu);g[2]=word(g[0]+12u);g[1]=g[2]+g[1]*4u;
        at(0x296203u);g[2]=word(0x510fa0u);g[1]=s_.flags.plus(g[1],g[0],4);
        return {g[1],g[2]+16u,3u,0u};
    }
    std::uint32_t read_word(std::uint32_t address){
        constexpr std::array sites{0x29620bu,0x296212u,0x296218u};
        at(sites[component_]);const auto value=word(address);
        s_.gp[component_==2u?1u:0u]=value;return value;
    }
    void write_word(std::uint32_t address,std::uint32_t value){
        constexpr std::array sites{0x296210u,0x296215u,0x29621bu};
        // The output ADD follows the first source load, so its flags/EDX are
        // not published if that real load fails.
        if(!component_)s_.gp[2]=s_.flags.plus(s_.gp[2],16u,4);
        Base::store(sites[component_],address,value,[]{});
    }
    void advanced(const NativeTu15WordCopy&){++component_;}
    void finish(){at(0x29621eu);(void)word(s_.gp[4]);s_.gp[4]+=4u;}
};
// Private candidate only; no production admission or default installation.
template<class State,class Before=Noop>void run_private(State&s,Before before={}){
    native_integer_fp_detail::Scope scope{s};Frame view{s,before,scope};decode(view);
}
} // namespace halo2::native_codec_translation_detail
#endif
