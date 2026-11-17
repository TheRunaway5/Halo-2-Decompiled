#pragma once
#include "halo2/native_tu15_map_world_property_caller_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_map_world_property_caller_detail {
template<bool Literal,class State,class Child> class SourceFrame {
    State& s_;
    Child& child_;
    void step(std::uint32_t pc) {
        if constexpr(Literal)s_.instruction_literal(pc);
        else if(s_.services.instruction)s_.services.instruction(pc);
    }
    std::uint32_t read(std::uint32_t address) {
        return physics_world_runtime_detail::r(s_.memory,address,4);
    }
    std::uint32_t finish() {
        step(0x11e4e8u);s_.gp[3]=s_.pop();
        step(0x11e4e9u);s_.pop();s_.gp[4]+=16u;return s_.gp[0];
    }
public:
    SourceFrame(State& state,Child& child):s_(state),child_(child) {}
    bool find_attachment() {
        auto& g=s_.gp;
        step(0x11e490u);g[0]=read(g[4]+4u);
        step(0x11e494u);g[1]=read(0x4ecbd0u);
        step(0x11e49au);g[2]=read(g[1]+0x44u);
        step(0x11e49du);g[0]=s_.flags.logic(g[0]&0xffffu,4);
        step(0x11e4a2u);g[0]+=g[0]*2u;
        step(0x11e4a5u);g[0]=read(g[2]+g[0]*4u+8u);
        step(0x11e4a9u);g[0]=read(g[0]+0xb4u);
        step(0x11e4afu);s_.push(0x11e4afu,s_.reg(3,4,0));
        step(0x11e4b0u);s_.set(3,s_.flags.logic(0u,1),1,0);
        step(0x11e4b2u);s_.flags.minus(g[0],0xffffffffu,4);
        step(0x11e4b5u);return !s_.flags.zf;
    }
    void query_attachment() {
        auto& g=s_.gp;
        step(0x11e4b7u);g[2]=read(g[4]+16u);
        step(0x11e4bbu);s_.push(0x11e4bbu,s_.reg(6,4,0));
        step(0x11e4bcu);g[0]=s_.flags.logic(g[0]&0xffffu,4);
        step(0x11e4c1u);g[6]=g[0]+g[0]*4u;
        step(0x11e4c4u);g[0]=read(0x52be5cu);
        step(0x11e4c9u);g[1]=read(g[0]+0x44u);
        step(0x11e4ccu);g[6]=s_.flags.shift(g[6],5u,4);
        step(0x11e4cfu);g[6]=s_.flags.plus(g[6],g[1],4);
        step(0x11e4d1u);g[1]=read(g[4]+24u);
        step(0x11e4d5u);s_.push(0x11e4d5u,s_.reg(1,4,0));
        step(0x11e4d6u);g[1]=read(g[4]+20u);
        step(0x11e4dau);s_.push(0x11e4dau,0x11e4dfu);child_(s_);
    }
    std::uint32_t finish_absent() {
        step(0x11e4e6u);s_.set(0,s_.reg(3,1,0),1,0);return finish();
    }
    std::uint32_t finish_child() {
        step(0x11e4dfu);s_.flags.logic(s_.reg(0,1,0),1);
        step(0x11e4e1u);s_.gp[6]=s_.pop();
        step(0x11e4e2u);s_.set(0,1u,1,0);
        step(0x11e4e4u);
        return s_.flags.zf?finish_absent():finish();
    }
};

template<bool Literal,class State,class Child> std::uint32_t run_source_frame(State& state,Child child) {
    SourceFrame<Literal,State,Child> view{state,child};return query(view);
}
} // namespace halo2::native_map_world_property_caller_detail
