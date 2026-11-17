#pragma once
#include "halo2/native_tu15_particle_type_filter_kernel.hpp"
#include "gameplay/objects/native_tu15_object_body_frame.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_particle_type_filter_detail {
template<class State,class Before> class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;
    Before& before;
    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width=4) {
        at(pc);return word(address,width);
    }
    void leave(std::uint32_t saved,std::uint32_t exit) {
        s_.gp[3]=pop(saved);pop(exit);s_.gp[4]+=4u;
    }
public:
    Frame(State& state,Before& observer,native_raw_x87_detail::Scope<State>& fp)
        :Base{state,observer,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},before(observer) {}
    void lookup_body() {
        auto& g=s_.gp;
        g[1]=read(0x17de60u,0x4f720cu);
        g[2]=read(0x17de66u,g[1]+0x44u);
        g[0]=s_.flags.logic(g[0]&0xffffu,4);
        g[0]=native_object_body_detail::arithmetic<0>(s_,g[0],0x190u);
        g[0]=read(0x17de74u,g[0]+g[2]+0x48u);
        push(0x17de78u,g[3]);push(0x17de79u,3u);
        s_.set(3,s_.flags.logic(0u,1),1,0);
        push(0x17de7du,0x17de82u);
        native_object_body_detail::run_private(s_,before);
    }
    bool has_body() {s_.flags.logic(s_.gp[0],4);return !s_.flags.zf;}
    bool has_definition() {
        auto& g=s_.gp;
        g[0]=read(0x17de86u,g[0]+0x13cu);
        s_.flags.minus(g[0],0xffffffffu,4);return !s_.flags.zf;
    }
    void compare_definition_type() {
        auto& g=s_.gp;
        g[1]=read(0x17de91u,0x4f54f4u);
        g[2]=read(0x17de97u,g[1]+0x44u);
        g[1]=read(0x17de9au,g[4]+8u);
        g[0]=s_.flags.logic(g[0]&0xffffu,4);
        g[0]=native_object_body_detail::arithmetic<0>(s_,g[0],0x21cu);
        g[0]=(read(0x17dea9u,g[0]+g[2]+0x28u,2)^0x8000u)-0x8000u;
        s_.flags.minus(g[0],g[1],4);
    }
    void return_comparison() {s_.set(0,s_.flags.zf,1,0);leave(0x17deb3u,0x17deb4u);}
    void return_missing() {s_.set(0,s_.reg(3,1,0),1,0);leave(0x17deb9u,0x17debau);}
};

template<class State,class Before> void run_private(State& state,Before before) {
    native_raw_x87_detail::Scope<State> fp{state};
    Frame<State,Before> frame{state,before,fp};filter(frame);
}
} // namespace halo2::native_particle_type_filter_detail
#endif
