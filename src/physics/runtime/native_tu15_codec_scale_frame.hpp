#pragma once
#include "halo2/native_tu15_codec_scale_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_integer_fp_scope.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"

namespace halo2::native_codec_scale_detail {
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
public:
    Frame(State&s,Before&before,Scope&scope):Base{s,before,{&scope}}{}
    NativeTu15WordCopy scale(){
        auto&g=s_.gp;
        at(0x296220u);g[0]=word(0x510f60u);
        at(0x296225u);g[1]=word(g[0]+16u);
        at(0x296228u);g[2]=word(0x510f9cu);g[1]=g[1]+g[2]*4u;
        return {g[1]+g[0],0u,1u,0u};
    }
    std::uint32_t read_word(std::uint32_t address){
        at(0x296231u);const auto value=word(address);s_.gp[2]=value;return value;
    }
    void write_word(std::uint32_t,std::uint32_t value){
        at(0x296234u);s_.gp[0]=word(0x510fa0u);
        Base::store(0x296239u,s_.gp[0]+28u,value,[]{});
    }
    void advanced(const NativeTu15WordCopy&){}
    void finish(){at(0x29623cu);(void)word(s_.gp[4]);s_.gp[4]+=4u;}
};
// Private physical candidate. GP/FP metadata and observer policy are read-only
// during providers; no generated entry admission or default is installed.
template<class State,class Before=Noop>void run_private(State&s,Before before={}){
    native_integer_fp_detail::Scope scope{s};Frame view{s,before,scope};decode(view);
}
} // namespace halo2::native_codec_scale_detail
#endif
