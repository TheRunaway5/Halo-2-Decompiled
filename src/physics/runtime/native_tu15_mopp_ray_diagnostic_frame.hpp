#pragma once
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"

namespace halo2::native_mopp_ray_caller_detail {
// Native invalid-program reporting through its real diagnostic owners. The
// final source-memory null write remains an actual fault boundary; this view
// neither fabricates a diagnostic return nor replaces the failure with success.
template<class State,class Before,class Dispatch>class DiagnosticFrame:public IntegerFrame<State,Before,ScopeIo<State>> {
    using Base=IntegerFrame<State,Before,ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;
    native_raw_x87_detail::Scope<State>&scope_;Dispatch&dispatch_;
    void call(std::uint32_t pc,std::uint32_t target,std::uint32_t returned){
        const auto address=s_.gp[4]-4u;Base::store(pc,address,returned,[&]{s_.gp[4]=address;},true);
        scope_.child([&]{dispatch_(s_,target,pc);});
    }
public:
    DiagnosticFrame(State&s,Before&before,native_raw_x87_detail::Scope<State>&scope,Dispatch&dispatch)
        :Base{s,before,ScopeIo<State>{&scope}},scope_(scope),dispatch_(dispatch){}
    void invoke(){
        auto&g=s_.gp;at(0x30ae16u);g[1]=word(0x48c99cu);at(0x30ae1cu);g[2]=word(g[1]);
        push(0x30ae1eu,g[0]);push(0x30ae1fu,0x41c4d4u);
        at(0x30ae24u);const auto format=word(g[2]+4u);call(0x30ae24u,format,0x30ae27u);
        push(0x30ae27u,g[0]);at(0x30ae28u);call(0x30ae28u,0x2e3080u,0x30ae2du);
        at(0x30ae2du);g[1]=word(0x48c99cu);at(0x30ae33u);g[0]=word(g[1]);g[4]=s_.flags.plus(g[4],12u,4);
        at(0x30ae38u);const auto emit=word(g[0]+4u);call(0x30ae38u,emit,0x30ae3bu);
        g[1]=g[0];at(0x30ae3du);call(0x30ae3du,0x2e30e0u,0x30ae42u);
        Base::store(0x30ae42u,0u,0u,[]{});
        g[7]=pop(0x30ae4cu);g[6]=pop(0x30ae4du);g[3]=pop(0x30ae4eu);g[4]=g[5];
        g[5]=pop(0x30ae51u);pop(0x30ae52u);g[4]+=12u;
    }
};
}
