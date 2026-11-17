#pragma once
#include "halo2/native_tu15_map_world_property_caller_kernel.hpp"
#include "gameplay/objects/native_tu15_map_world_property_query_frame.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_map_world_property_caller_detail {
template<class State,class Before> class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_map_world_property_query_detail::HostIo> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_map_world_property_query_detail::HostIo>;
    using Base::s_;using Base::at;using Base::word;using Base::pop;using Base::push;using Base::before_;
    std::uint32_t read(std::uint32_t pc,std::uint32_t address) {at(pc);return word(address);}
    std::uint32_t finish() {
        s_.gp[3]=pop(0x11e4e8u);pop(0x11e4e9u);s_.gp[4]+=16u;return s_.gp[0];
    }
public:
    Frame(State& state,Before& before):Base{state,before,native_map_world_property_query_detail::HostIo{}} {}
    bool find_attachment() {
        auto& g=s_.gp;
        g[0]=read(0x11e490u,g[4]+4u);g[1]=read(0x11e494u,0x4ecbd0u);
        g[2]=read(0x11e49au,g[1]+0x44u);
        g[0]=s_.flags.logic(g[0]&0xffffu,4);g[0]+=g[0]*2u;
        g[0]=read(0x11e4a5u,g[2]+g[0]*4u+8u);g[0]=read(0x11e4a9u,g[0]+0xb4u);
        push(0x11e4afu,g[3]);s_.set(3,s_.flags.logic(0u,1),1,0);
        s_.flags.minus(g[0],0xffffffffu,4);return !s_.flags.zf;
    }
    void query_attachment() {
        auto& g=s_.gp;
        g[2]=read(0x11e4b7u,g[4]+16u);push(0x11e4bbu,g[6]);
        g[0]=s_.flags.logic(g[0]&0xffffu,4);g[6]=g[0]+g[0]*4u;
        g[0]=read(0x11e4c4u,0x52be5cu);g[1]=read(0x11e4c9u,g[0]+0x44u);
        g[6]=s_.flags.shift(g[6],5u,4);g[6]=s_.flags.plus(g[6],g[1],4);
        g[1]=read(0x11e4d1u,g[4]+24u);push(0x11e4d5u,g[1]);
        g[1]=read(0x11e4d6u,g[4]+20u);push(0x11e4dau,0x11e4dfu);
        at(0x1db800u);native_map_world_property_query_detail::run_private(s_,before_);
    }
    std::uint32_t finish_absent() {s_.set(0,s_.reg(3,1,0),1,0);return finish();}
    std::uint32_t finish_child() {
        s_.flags.logic(s_.reg(0,1,0),1);s_.gp[6]=pop(0x11e4e1u);s_.set(0,1u,1,0);
        if(s_.flags.zf)s_.set(0,s_.reg(3,1,0),1,0);
        return finish();
    }
};

template<class State,class Before> std::uint32_t run_private(State& state,Before before) {
    Frame view{state,before};return query(view);
}
} // namespace halo2::native_map_world_property_caller_detail
#endif
