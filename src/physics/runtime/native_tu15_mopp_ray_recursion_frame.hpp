#pragma once
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"

namespace halo2::native_mopp_ray_caller_detail {
// The caller stores precede the native child handoff. The child actually owns
// its retained State and return; no captured result or entry-register restore
// participates in resuming the parent.
template<class State,class Before>class RecursionFrame:public IntegerFrame<State,Before,ScopeIo<State>> {
    using Base=IntegerFrame<State,Before,ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;
    native_raw_x87_detail::Scope<State>&scope_;
public:
    RecursionFrame(State&s,Before&before,native_raw_x87_detail::Scope<State>&scope)
        :Base{s,before,ScopeIo<State>{&scope}},scope_(scope){}
    template<class Child>void invoke(bool increasing,Child&&child){
        auto&g=s_.gp;
        if(increasing){
            g[1]=s_.flags.minus(g[1],g[2],4);g[0]=g[4]+0x80u;
            push(0x30af1fu,g[0]);g[1]=s_.flags.plus(g[1],g[6],4);
            push(0x30af22u,g[1]);at(0x30af23u);g[1]=word(g[4]+0x30u);
            push(0x30af27u,g[3]);push(0x30af28u,0x30af2du);
        }else{
            at(0x30b06fu);g[1]=word(g[4]+0x28u);g[2]=g[4]+0x80u;
            push(0x30b07au,g[2]);push(0x30b07bu,g[6]);
            push(0x30b07cu,g[3]);push(0x30b07du,0x30b082u);
        }
        scope_.child(std::forward<Child>(child));
    }
    void after_child(bool increasing){at(increasing?0x30af2du:0x30b082u);}
};
}
